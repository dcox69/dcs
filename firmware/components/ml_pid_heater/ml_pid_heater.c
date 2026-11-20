/**
 * @file ml_pid_heater.c
 * @author dcox69 (David GOMEZ) <dcox69@proton.me>
 * @brief Oven PID controller, with temperature prediction and duty cycle control, for embedded systems
 *
 * @version 0.1
 * @date 2026-07-09
 *
 * @note The PID controller has been initially designed & fine-tuned with the help of an AI agent (Claude Sonnet 5.0)
 *       Then the code has been hand-tuned, reorganized and commented by me.
 *
 * @remark This code has no dependencies except the standard C library.
 *
 * @warning This code is provided as-is, without any guarantee of correctness or functionality.
 *
 * @copyright Copyright (c) 2026 dcox69 (David GOMEZ)
 *
 */
#include "ml_pid_heater.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

/* control structs */

/* PID parameters */

typedef struct
{
    /* Proportional gain applied to the instantaneous temperature error. */
    float kp;
    /* Integral gain applied to the accumulated temperature error over time. */
    float ki;
    /* Derivative gain applied to the measured temperature slope. */
    float kd;
    /* Minimum PID output value, typically expressed as a duty cycle percentage. */
    float out_min;
    /* Maximum PID output value, typically expressed as a duty cycle percentage. */
    float out_max;
} pid_config_t;

_Static_assert(sizeof(pid_config_t) == 20, "pid_config_t struct size mismatch !");

typedef struct
{
    /* Accumulated integral term used to compensate long-term steady-state error. */
    float   integral;
    /* Previous control error used for internal PID state tracking. */
    float   prev_error;
    /* Previous measured temperature used to compute the derivative term on measurement. */
    float   prev_measured;
    /* Indicates whether the PID is running its first computation cycle. */
    bool    first;
    /* Reserved padding bytes used to keep structure alignment stable. */
    uint8_t rsvd[3];
} pid_state_t;

_Static_assert(sizeof(pid_state_t) == 16, "pid_state_t struct size mismatch !");

typedef struct
{
    /* Static PID tuning and output limit parameters. */
    pid_config_t cfg;
    /* Runtime PID internal state updated at each control cycle. */
    pid_state_t  state;
} pid_t;

_Static_assert(sizeof(pid_t) == 36, "pid_t struct size mismatch !");

/* predictive cutoff parameters */

typedef struct
{
    /* Prediction horizon used to estimate the future temperature trend. */
    float lookahead_s;
    /* Safety margin below the setpoint where heating can be stopped early. */
    float safety_margin_c;
} predict_config_t;

typedef struct
{
    /* Last measured temperature used as the base for rate estimation. */
    float    last_temp_c;
    /* Filtered temperature rise rate in degrees Celsius per second. */
    float    temp_rate_c_per_s;
    /* Timestamp accumulator associated with the last predictive temperature sample. */
    uint32_t last_temp_time_ms;
    /* Previous filtered temperature rate used to estimate acceleration. */
    float    prev_temp_rate_c_per_s;
    /* Filtered temperature acceleration in degrees Celsius per second squared. */
    float    temp_accel_c_per_s2;
} predict_state_t;

typedef struct
{
    /* Static predictive cutoff configuration parameters. */
    predict_config_t cfg;
    /* Runtime predictive estimator state updated at each control cycle. */
    predict_state_t  state;
} predict_t;

_Static_assert(sizeof(predict_t) == 28, "predict_t struct size mismatch !");

/* Time window / output modulation parameters */

typedef struct
{
    /* Duration of one PWM-like control window used to distribute heater ON time. */
    uint32_t control_period_ms;
    /* Main control loop execution period. */
    uint32_t loop_period_ms;
    /* Minimum heater ON duration allowed inside a control window. */
    uint32_t min_on_ms;
    /* Minimum heater OFF duration allowed inside a control window. */
    uint32_t min_off_ms;
    /* Maximum allowed duty cycle change between two successive control windows. */
    float    max_step;
} timing_config_t;

_Static_assert(sizeof(timing_config_t) == 20, "timing_config_t struct size mismatch !");

typedef struct
{
    /* Duty cycle applied during the previous control window. */
    float    prev_duty_percent;
    /* Elapsed time inside the current control window. */
    uint32_t window_elapsed_ms;
    /* Effective heater ON duration selected for the current control window. */
    uint32_t current_on_time_ms;
    /* */
    uint32_t last_epoch_time_ms;
    /* */
    uint32_t real_loop_period_ms;
} timing_state_t;

_Static_assert(sizeof(timing_state_t) == 20, "timing_state_t struct size mismatch !");

typedef struct
{
    /* Static time window configuration parameters. */
    timing_config_t cfg;
    /* Runtime time window state updated at each control cycle. */
    timing_state_t  state;
} timing_t;

_Static_assert(sizeof(timing_t) == 40, "timing_t struct size mismatch !");

/* main heater context */

typedef struct
{
    /* Indicates whether the struct has been initialized by a call to ml_pid_heater_init(). */
    bool                        initialized;
    /* Selected output driver type, for example relay or SSR. */
    ml_pid_heater_driver_type_e driver_type;
    /* Current physical heater output state. */
    bool                        heater_on;
    /* Reserved padding byte used to keep structure alignment stable. */
    bool                        rsvd;

    /* Requested target temperature in degrees Celsius. */
    float                       temp_setpoint_c;
    /* Maximum allowed temperature before safety shutdown. */
    float                       temp_max_c;

    /* Time window and output modulation configuration and rutime state. */
    timing_t                    timing;
    /* Predictive cutoff configuration and runtime state. */
    predict_t                   predict;
    /* PID configuration and runtime state. */
    pid_t                       pid;

    /* Hardware abstraction callbacks and user context. */
    ml_pid_heater_io_t          io;
} ml_pid_heater_internal_t;

_Static_assert(sizeof(ml_pid_heater_internal_t) == sizeof(ml_pid_heater_t),
               "ml_pid_heater_internal_t struct size mismatch !");

static float clampf(float x, float lo, float hi)
{
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}

static void pid_init(pid_t *pid, float kp, float ki, float kd, float out_min, float out_max)
{
    pid->cfg.kp           = kp;
    pid->cfg.ki           = ki;
    pid->cfg.kd           = kd;
    pid->cfg.out_min      = out_min;
    pid->cfg.out_max      = out_max;
    pid->state.integral   = 0.0f;
    pid->state.prev_error = 0.0f;
    pid->state.first      = true;
}

static float pid_compute(pid_t *pid, float setpoint, float measured, float dt_s)
{
    float error = setpoint - measured;
    float p     = pid->cfg.kp * error;

    /* derivative based on the measured value, not on the error (prevents a sudden surge when the setpoint changes) */
    float d     = 0.0f;
    if (!pid->state.first && dt_s > 0.0f)
    {
        d = -pid->cfg.kd * (measured - pid->state.prev_measured) / dt_s;
    }

    /* preliminary calculation without the integral term to test for saturation */
    float unclamped_without_i = p + d + pid->cfg.ki * pid->state.integral;

    bool  saturated_high      = (unclamped_without_i > pid->cfg.out_max) && (error > 0);
    bool  saturated_low       = (unclamped_without_i < pid->cfg.out_min) && (error < 0);

    if (!saturated_high && !saturated_low)
    {
        pid->state.integral += error * dt_s;
        pid->state.integral = clampf(pid->state.integral, -50.0f, 50.0f);
    }

    float i                  = pid->cfg.ki * pid->state.integral;

    pid->state.prev_measured = measured;
    pid->state.prev_error    = error;
    pid->state.first         = false;

    return clampf(p + i + d, pid->cfg.out_min, pid->cfg.out_max);
}

static void heater_force_off(ml_pid_heater_internal_t *heat)
{
    if (heat->heater_on)
    {
        heat->io.set_heater_state((ml_pid_heater_t *)heat, false, heat->io.user_ctx);
        heat->heater_on = false;
    }
}

static uint32_t sanitize_on_time_ms(const ml_pid_heater_internal_t *heat, uint32_t requested_on_ms)
{
    uint32_t period = heat->timing.cfg.control_period_ms;

    if (requested_on_ms == 0) return 0;
    if (requested_on_ms >= period) return period;

    if (requested_on_ms < heat->timing.cfg.min_on_ms) return 0;
    if ((period - requested_on_ms) < heat->timing.cfg.min_off_ms) return period;

    return requested_on_ms;
}

static void heater_control_step(ml_pid_heater_internal_t *heat)
{
    float temp_c = heat->io.get_temperature((ml_pid_heater_t *)heat, heat->io.user_ctx);

    if (!isfinite(temp_c) || temp_c < -20.0f || temp_c > 150.0f)
    {
        if (heat->io.on_event)
        {
            ml_pid_heater_event_t evt = {0};
            evt.type                  = ML_EPI_EVT_INVALID_TEMP;
            evt.user_ctx              = heat->io.user_ctx;
            evt.epoch_ms              = heat->timing.state.last_epoch_time_ms;
            evt.elapsed_ms            = heat->timing.state.real_loop_period_ms;
            evt.invalid_temp          = temp_c;
            heat->io.on_event((ml_pid_heater_t *)heat, &evt);
        }
        heater_force_off(heat);
        return;
    }

    if (temp_c >= heat->temp_max_c)
    {
        if (heat->io.on_event)
        {
            ml_pid_heater_event_t evt    = {0};
            evt.type                     = ML_EPI_EVT_SAFETY_CUTOFF;
            evt.user_ctx                 = heat->io.user_ctx;
            evt.epoch_ms                 = heat->timing.state.last_epoch_time_ms;
            evt.elapsed_ms               = heat->timing.state.real_loop_period_ms;
            evt.safety_cutoff.setpoint_c = heat->temp_setpoint_c;
            evt.safety_cutoff.max_c      = heat->temp_max_c;
            evt.safety_cutoff.temp_c     = temp_c;
            heat->io.on_event((ml_pid_heater_t *)heat, &evt);
        }
        heater_force_off(heat);
        return;
    }

    if (heat->timing.state.window_elapsed_ms == 0)
    {
        float dt_s = heat->timing.cfg.control_period_ms / 1000.0f;

        if (heat->predict.state.last_temp_time_ms != 0)
        {
            float instant_rate  = (temp_c - heat->predict.state.last_temp_c) / dt_s;
            float new_rate      = 0.7f * heat->predict.state.temp_rate_c_per_s + 0.3f * instant_rate;

            float instant_accel = (new_rate - heat->predict.state.prev_temp_rate_c_per_s) / dt_s;
            heat->predict.state.temp_accel_c_per_s2 =
                0.6f * heat->predict.state.temp_accel_c_per_s2 + 0.4f * instant_accel;

            heat->predict.state.prev_temp_rate_c_per_s = heat->predict.state.temp_rate_c_per_s;
            heat->predict.state.temp_rate_c_per_s      = new_rate;
        }
        heat->predict.state.last_temp_c = temp_c;
        heat->predict.state.last_temp_time_ms += heat->timing.cfg.control_period_ms;

        /* quadratic extrapolation : position = x0 + v*t + 0.5*a*t^2 */
        float accel_term        = heat->predict.state.temp_accel_c_per_s2 > 0.0f
                                      ? heat->predict.state.temp_accel_c_per_s2
                                      : 0.0f; /* ignore décélération pour rester prudent */
        float predicted_temp    = temp_c + heat->predict.state.temp_rate_c_per_s * heat->predict.cfg.lookahead_s +
                                  0.5f * accel_term * heat->predict.cfg.lookahead_s * heat->predict.cfg.lookahead_s;

        /* cutoff if we are close to the setpoint, wathever the slope */
        bool  margin_cutoff     = (temp_c >= heat->temp_setpoint_c - heat->predict.cfg.safety_margin_c) &&
                                  (heat->predict.state.temp_rate_c_per_s > 0.02f);

        bool  predictive_cutoff = margin_cutoff || ((heat->predict.state.temp_rate_c_per_s > 0.0f) &&
                                                    (predicted_temp >= heat->temp_setpoint_c));

        float duty_percent;
        if (predictive_cutoff)
        {
            duty_percent = 0.0f;
            if (heat->io.on_event)
            {
                ml_pid_heater_event_t evt            = {0};
                evt.type                             = ML_EPI_EVT_PREDICTIVE_CUTOFF;
                evt.user_ctx                         = heat->io.user_ctx;
                evt.epoch_ms                         = heat->timing.state.last_epoch_time_ms;
                evt.elapsed_ms                       = heat->timing.state.real_loop_period_ms;
                evt.predictive_cutoff.setpoint_c     = heat->temp_setpoint_c;
                evt.predictive_cutoff.temp_c         = temp_c;
                evt.predictive_cutoff.rate_c_per_s   = heat->predict.state.temp_rate_c_per_s;
                evt.predictive_cutoff.predicted_temp = predicted_temp;
                heat->io.on_event((ml_pid_heater_t *)heat, &evt);
            }
        }
        else
        {
            duty_percent = pid_compute(&heat->pid, heat->temp_setpoint_c, temp_c, dt_s);
        }

        duty_percent                         = clampf(duty_percent,
                                                      heat->timing.state.prev_duty_percent - heat->timing.cfg.max_step,
                                                      heat->timing.state.prev_duty_percent + heat->timing.cfg.max_step);
        heat->timing.state.prev_duty_percent = duty_percent;

        uint32_t requested_on_ms             = (uint32_t)((duty_percent / 100.0f) * heat->timing.cfg.control_period_ms);

        heat->timing.state.current_on_time_ms = sanitize_on_time_ms(heat, requested_on_ms);

        if (heat->io.on_event)
        {
            ml_pid_heater_event_t evt  = {0};
            evt.type                   = ML_EPI_EVT_STEP_DONE;
            evt.user_ctx               = heat->io.user_ctx;
            evt.epoch_ms               = heat->timing.state.last_epoch_time_ms;
            evt.elapsed_ms             = heat->timing.state.real_loop_period_ms;
            evt.step_done.setpoint_c   = heat->temp_setpoint_c;
            evt.step_done.driver_type  = heat->driver_type;
            evt.step_done.duty_percent = duty_percent;
            evt.step_done.temp_c       = temp_c;
            evt.step_done.on_ms        = heat->timing.state.current_on_time_ms;
            evt.step_done.period_ms    = heat->timing.cfg.control_period_ms;
            heat->io.on_event((ml_pid_heater_t *)heat, &evt);
        }
    }

    bool should_be_on = (heat->timing.state.window_elapsed_ms < heat->timing.state.current_on_time_ms);

    if (should_be_on != heat->heater_on)
    {
        heat->io.set_heater_state((ml_pid_heater_t *)heat, should_be_on, heat->io.user_ctx);
        heat->heater_on = should_be_on;
    }

    heat->timing.state.window_elapsed_ms += heat->timing.state.real_loop_period_ms;
    if (heat->timing.state.window_elapsed_ms >= heat->timing.cfg.control_period_ms)
    {
        heat->timing.state.window_elapsed_ms = 0;
    }
}

void ml_pid_heater_set_setpoint_temp(ml_pid_heater_t *heater, float setpoint_c, float max_c)
{
    ml_pid_heater_internal_t *heat = (ml_pid_heater_internal_t *)heater;

    if (setpoint_c != heat->temp_setpoint_c || max_c != heat->temp_max_c)
    {
        heat->temp_setpoint_c = setpoint_c;
        heat->temp_max_c      = max_c;

        /* reset to default state */
        ml_pid_heater_reset_state(heater);
    }
}

void ml_pid_heater_poll(ml_pid_heater_t *heater, uint32_t epoch_ms)
{
    ml_pid_heater_internal_t *heat = (ml_pid_heater_internal_t *)heater;
    if (!heat || !heat->initialized) return;
    if (heat->timing.state.last_epoch_time_ms == 0)
    {
        heat->timing.state.last_epoch_time_ms = epoch_ms;
        return;
    }
    if (epoch_ms - heat->timing.state.last_epoch_time_ms >= heat->timing.cfg.loop_period_ms)
    {
        heat->timing.state.real_loop_period_ms = epoch_ms - heat->timing.state.last_epoch_time_ms;
        heat->timing.state.last_epoch_time_ms  = epoch_ms;
        heater_control_step(heat);
    }
}

bool ml_pid_heater_init(ml_pid_heater_t *heater, ml_pid_heater_driver_type_e type, const ml_pid_heater_io_t *io)
{
    ml_pid_heater_internal_t *heat = (ml_pid_heater_internal_t *)heater;

    if (!heater || !io || !io->get_temperature || !io->set_heater_state)
    {
        return false;
    }

    memset(heat, 0, sizeof(*heater));

    heat->driver_type                     = type;
    heat->temp_setpoint_c                 = 50.0f;
    heat->temp_max_c                      = 60.0f;

    heat->timing.cfg.max_step             = 15.0f;

    heat->timing.state.prev_duty_percent  = 0.0f;
    heat->timing.state.window_elapsed_ms  = 0;
    heat->timing.state.current_on_time_ms = 0;

    heat->predict.cfg.lookahead_s         = 150.0f;
    heat->predict.cfg.safety_margin_c     = 3.0f;
    heat->predict.state.last_temp_c       = 0.0f;
    heat->predict.state.temp_rate_c_per_s = 0.0f;
    heat->predict.state.last_temp_time_ms = 0;

    heat->heater_on                       = false;
    heat->io                              = *io;

    if (type == ML_PID_HEATER_DRIVER_RELAY)
    {
        heat->timing.cfg.control_period_ms = 20000;
        heat->timing.cfg.loop_period_ms    = 500;
        heat->timing.cfg.min_on_ms         = 3000;
        heat->timing.cfg.min_off_ms        = 3000;
        pid_init(&heat->pid, 3.0f, 0.03f, 15.0f, 0.0f, 70.0f);
    }
    else /* SSR */
    {
        heat->timing.cfg.control_period_ms = 2000;
        heat->timing.cfg.loop_period_ms    = 100;
        heat->timing.cfg.min_on_ms         = 100;
        heat->timing.cfg.min_off_ms        = 100;
        pid_init(&heat->pid, 10.0f, 0.05f, 8.0f, 0.0f, 100.0f);
    }

    heat->initialized = true;

    return true;
}

void ml_pid_heater_set_kp(ml_pid_heater_t *heater, float kp)
{
    ml_pid_heater_internal_t *heat = (ml_pid_heater_internal_t *)heater;
    heat->pid.cfg.kp               = kp;
}

float ml_pid_heater_get_kp(ml_pid_heater_t *heater)
{
    ml_pid_heater_internal_t *heat = (ml_pid_heater_internal_t *)heater;
    return heat->pid.cfg.kp;
}

void ml_pid_heater_set_ki(ml_pid_heater_t *heater, float ki)
{
    ml_pid_heater_internal_t *heat = (ml_pid_heater_internal_t *)heater;
    heat->pid.cfg.ki               = ki;
}

float ml_pid_heater_get_ki(ml_pid_heater_t *heater)
{
    ml_pid_heater_internal_t *heat = (ml_pid_heater_internal_t *)heater;
    return heat->pid.cfg.ki;
}

void ml_pid_heater_set_kd(ml_pid_heater_t *heater, float kd)
{
    ml_pid_heater_internal_t *heat = (ml_pid_heater_internal_t *)heater;
    heat->pid.cfg.kd               = kd;
}

float ml_pid_heater_get_kd(ml_pid_heater_t *heater)
{
    ml_pid_heater_internal_t *heat = (ml_pid_heater_internal_t *)heater;
    return heat->pid.cfg.kd;
}

void ml_pid_heater_set_limits(ml_pid_heater_t *heater, float out_min, float out_max)
{
    ml_pid_heater_internal_t *heat = (ml_pid_heater_internal_t *)heater;
    heat->pid.cfg.out_min          = out_min;
    heat->pid.cfg.out_max          = out_max;
}

float ml_pid_heater_get_out_min(ml_pid_heater_t *heater)
{
    ml_pid_heater_internal_t *heat = (ml_pid_heater_internal_t *)heater;
    return heat->pid.cfg.out_min;
}

float ml_pid_heater_get_out_max(ml_pid_heater_t *heater)
{
    ml_pid_heater_internal_t *heat = (ml_pid_heater_internal_t *)heater;
    return heat->pid.cfg.out_max;
}

void ml_pid_heater_set_lookahead(ml_pid_heater_t *heater, float lookahead_s)
{
    ml_pid_heater_internal_t *heat = (ml_pid_heater_internal_t *)heater;
    heat->predict.cfg.lookahead_s  = lookahead_s;
}

float ml_pid_heater_get_lookahead(ml_pid_heater_t *heater)
{
    ml_pid_heater_internal_t *heat = (ml_pid_heater_internal_t *)heater;
    return heat->predict.cfg.lookahead_s;
}

void ml_pid_heater_set_max_step(ml_pid_heater_t *heater, float max_step)
{
    ml_pid_heater_internal_t *heat = (ml_pid_heater_internal_t *)heater;
    heat->timing.cfg.max_step      = max_step;
}

float ml_pid_heater_get_max_step(ml_pid_heater_t *heater)
{
    ml_pid_heater_internal_t *heat = (ml_pid_heater_internal_t *)heater;
    return heat->timing.cfg.max_step;
}

void ml_pid_heater_set_control_period(ml_pid_heater_t *heater, uint32_t control_period_ms)
{
    ml_pid_heater_internal_t *heat     = (ml_pid_heater_internal_t *)heater;
    heat->timing.cfg.control_period_ms = control_period_ms;
}

uint32_t ml_pid_heater_get_control_period(ml_pid_heater_t *heater)
{
    ml_pid_heater_internal_t *heat = (ml_pid_heater_internal_t *)heater;
    return heat->timing.cfg.control_period_ms;
}

void ml_pid_heater_set_loop_period(ml_pid_heater_t *heater, uint32_t loop_period_ms)
{
    ml_pid_heater_internal_t *heat  = (ml_pid_heater_internal_t *)heater;
    heat->timing.cfg.loop_period_ms = loop_period_ms;
}

uint32_t ml_pid_heater_get_loop_period(ml_pid_heater_t *heater)
{
    ml_pid_heater_internal_t *heat = (ml_pid_heater_internal_t *)heater;
    return heat->timing.cfg.loop_period_ms;
}

void ml_pid_heater_set_min_on(ml_pid_heater_t *heater, uint32_t min_on_ms)
{
    ml_pid_heater_internal_t *heat = (ml_pid_heater_internal_t *)heater;
    heat->timing.cfg.min_on_ms     = min_on_ms;
}

uint32_t ml_pid_heater_get_min_on(ml_pid_heater_t *heater)
{
    ml_pid_heater_internal_t *heat = (ml_pid_heater_internal_t *)heater;
    return heat->timing.cfg.min_on_ms;
}

void ml_pid_heater_set_min_off(ml_pid_heater_t *heater, uint32_t min_off_ms)
{
    ml_pid_heater_internal_t *heat = (ml_pid_heater_internal_t *)heater;
    heat->timing.cfg.min_off_ms    = min_off_ms;
}

uint32_t ml_pid_heater_get_min_off(ml_pid_heater_t *heater)
{
    ml_pid_heater_internal_t *heat = (ml_pid_heater_internal_t *)heater;
    return heat->timing.cfg.min_off_ms;
}

void ml_pid_heater_set_safety_margin(ml_pid_heater_t *heater, float safety_margin_c)
{
    ml_pid_heater_internal_t *heat    = (ml_pid_heater_internal_t *)heater;
    heat->predict.cfg.safety_margin_c = safety_margin_c;
}

float ml_pid_heater_get_safety_margin(ml_pid_heater_t *heater)
{
    ml_pid_heater_internal_t *heat = (ml_pid_heater_internal_t *)heater;
    return heat->predict.cfg.safety_margin_c;
}

void ml_pid_heater_set_io(ml_pid_heater_t *heater, const ml_pid_heater_io_t *io)
{
    ml_pid_heater_internal_t *heat = (ml_pid_heater_internal_t *)heater;
    heat->io                       = *io;
}

void ml_pid_heater_deinit(ml_pid_heater_t *heater)
{
    ml_pid_heater_internal_t *heat = (ml_pid_heater_internal_t *)heater;
    memset(heat, 0, sizeof(*heat));
}

void ml_pid_heater_reset_state(ml_pid_heater_t *heater)
{
    ml_pid_heater_internal_t *heat             = (ml_pid_heater_internal_t *)heater;

    /* reset default parameters */
    heat->predict.state.temp_accel_c_per_s2    = 0.0f;
    heat->predict.state.prev_temp_rate_c_per_s = 0.0f;
    heat->predict.state.last_temp_c            = 0.0f;
    heat->predict.state.temp_rate_c_per_s      = 0.0f;
    heat->predict.state.last_temp_time_ms      = 0;

    heat->timing.state.prev_duty_percent       = 0.0f;
    heat->timing.state.window_elapsed_ms       = 0;
    heat->timing.state.current_on_time_ms      = 0;
    heat->timing.state.last_epoch_time_ms      = 0;
    heat->timing.state.real_loop_period_ms     = 0;

    heat->pid.state.integral                   = 0.0f;
    heat->pid.state.prev_error                 = 0.0f;
    heat->pid.state.prev_measured              = 0.0f;
    heat->pid.state.first                      = true;

    heater_force_off(heat);
}
