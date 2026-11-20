#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    ML_PID_HEATER_DRIVER_RELAY,
    ML_PID_HEATER_DRIVER_SSR
} ml_pid_heater_driver_type_e;

/* opaque type for internal struct */
typedef struct
{
    uint8_t rsvd[140];
} ml_pid_heater_t;

typedef enum
{
    ML_PID_EVT_NONE = 0,
    ML_EPI_EVT_STEP_DONE,         /* data = event -> step_done */
    ML_EPI_EVT_PREDICTIVE_CUTOFF, /* data = event -> predictive_cutoff */
    ML_EPI_EVT_SAFETY_CUTOFF,     /* data = event -> safety_cutoff */
    ML_EPI_EVT_INVALID_TEMP,      /* data = event -> invalid_temp */
} ml_pid_heater_event_type_e;

/**
 * @brief Event structure for the PID heater instance.
 *
 */
typedef struct
{
    ml_pid_heater_event_type_e type;
    void                      *user_ctx;
    uint32_t                   epoch_ms;
    uint32_t                   elapsed_ms;

    union
    {
        float invalid_temp;

        struct
        {
            float                       setpoint_c;
            ml_pid_heater_driver_type_e driver_type;
            float                       temp_c;
            float                       duty_percent;
            uint32_t                    on_ms;
            uint32_t                    period_ms;
        } step_done;

        struct
        {
            float setpoint_c;
            float temp_c;
            float rate_c_per_s;
            float predicted_temp;
        } predictive_cutoff;

        struct
        {
            float setpoint_c;
            float max_c;
            float temp_c;
        } safety_cutoff;
    };
} ml_pid_heater_event_t;

typedef float (*ml_pid_heater_get_temperature_cb_t)(ml_pid_heater_t *heater, void *user_ctx);
typedef void  (*ml_pid_heater_set_heater_state_cb_t)(ml_pid_heater_t *heater, bool heater_state, void *user_ctx);
typedef void  (*ml_pid_heater_event_cb_t)(ml_pid_heater_t *heater, const ml_pid_heater_event_t *evt);

typedef struct
{
    ml_pid_heater_get_temperature_cb_t  get_temperature;
    ml_pid_heater_set_heater_state_cb_t set_heater_state;
    ml_pid_heater_event_cb_t            on_event;
    void                               *user_ctx;
} ml_pid_heater_io_t;

_Static_assert(sizeof(ml_pid_heater_io_t) == 16, "ml_pid_heater_io_t struct size mismatch !");

/* public API */

/**
 * @brief Initialize the PID heater instance.
 *
 * @param heater Pointer to the PID heater instance.
 * @param type Driver type for the heater.
 * @param io Pointer to the I/O configuration.
 * @return true if initialization was successful, false otherwise.
 */
bool ml_pid_heater_init(ml_pid_heater_t *heater, ml_pid_heater_driver_type_e type, const ml_pid_heater_io_t *io);

/**
 * @brief Deinitialize the PID heater instance.
 *
 * @param heater Pointer to the PID heater instance.
 */
void ml_pid_heater_deinit(ml_pid_heater_t *heater);

/**
 * @brief Poll the PID heater instance.
 * @details This function should be called periodically to update the
 * PID heater's state based on the current temperature and control logic.
 *
 * @param heater Pointer to the PID heater instance.
 * @param epoch_ms Current time in milliseconds.
 */
void ml_pid_heater_poll(ml_pid_heater_t *heater, uint32_t epoch_ms);

/**
 * @brief Reset the state of the PID heater instance.
 *
 * @param heater Pointer to the PID heater instance.
 */
void ml_pid_heater_reset_state(ml_pid_heater_t *heater);

/**
 * @brief Set the setpoint temperature for the PID heater instance.
 *
 * @param heater Pointer to the PID heater instance.
 * @param setpoint_c Desired setpoint temperature in Celsius.
 * @param max_c Maximum allowable temperature in Celsius.
 */
void ml_pid_heater_set_setpoint_temp(ml_pid_heater_t *heater, float setpoint_c, float max_c);

/**
 * @brief Set the I/O configuration for the PID heater instance.
 *
 * @param heater Pointer to the PID heater instance.
 * @param io Pointer to the I/O configuration structure.
 */
void ml_pid_heater_set_io(ml_pid_heater_t *heater, const ml_pid_heater_io_t *io);

/** @brief Getters / Setters for the PID heater instance. */
void ml_pid_heater_set_kp(ml_pid_heater_t *heater, float kp);
float ml_pid_heater_get_kp(ml_pid_heater_t *heater);
void ml_pid_heater_set_ki(ml_pid_heater_t *heater, float ki);
float ml_pid_heater_get_ki(ml_pid_heater_t *heater);
void ml_pid_heater_set_kd(ml_pid_heater_t *heater, float kd);
float ml_pid_heater_get_kd(ml_pid_heater_t *heater);
void ml_pid_heater_set_limits(ml_pid_heater_t *heater, float out_min, float out_max);
float ml_pid_heater_get_out_min(ml_pid_heater_t *heater);
float ml_pid_heater_get_out_max(ml_pid_heater_t *heater);
void ml_pid_heater_set_lookahead(ml_pid_heater_t *heater, float lookahead_s);
float ml_pid_heater_get_lookahead(ml_pid_heater_t *heater);
void ml_pid_heater_set_max_step(ml_pid_heater_t *heater, float max_step);
float ml_pid_heater_get_max_step(ml_pid_heater_t *heater);
void ml_pid_heater_set_control_period(ml_pid_heater_t *heater, uint32_t control_period_ms);
uint32_t ml_pid_heater_get_control_period(ml_pid_heater_t *heater);
void ml_pid_heater_set_loop_period(ml_pid_heater_t *heater, uint32_t loop_period_ms);
uint32_t ml_pid_heater_get_loop_period(ml_pid_heater_t *heater);
void ml_pid_heater_set_min_on(ml_pid_heater_t *heater, uint32_t min_on_ms);
uint32_t ml_pid_heater_get_min_on(ml_pid_heater_t *heater);
void ml_pid_heater_set_min_off(ml_pid_heater_t *heater, uint32_t min_off_ms);
uint32_t ml_pid_heater_get_min_off(ml_pid_heater_t *heater);
void ml_pid_heater_set_safety_margin(ml_pid_heater_t *heater, float safety_margin_c);
float ml_pid_heater_get_safety_margin(ml_pid_heater_t *heater);
