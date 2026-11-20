/**
 * @file main.c
 * @author dcox69 (David GOMEZ) <dcox69@proton.me>
 * @brief Oven PID controlled firmware, with temperature prediction and duty cycle control, for embedded systems (ESP32)
 * @version 0.1
 * @date 2026-07-09
 *
 * @note This project is part of DCS (Dryer Control System) project.
 *       A 3D filament dryer made from an old, salvaged built-in oven. The project includes the hardware (circuit
 * boards), the software (this firmware), including the graphics, and the 3D print files for the enclosures created in
 * OpenSCAD.
 *
 * @copyright Copyright (c) 2026 dcox69 (David GOMEZ)
 *
 * @remarks The PID controller needs to be tuned on each oven, by modifiying the PID parameters into the hw_conf.h file.
 *          We must first address the factors that contribute to the high thermal inertia of the oven’s heating elements
 * :
 *
 *          - PID_LOOKAHEAD_S               Increase to decrease overshoot, decrease to increase settling time
 *          - PID_OUT_MIN                   Minimum duty cycle to start the heater.
 *          - PID_OUT_MAX                   Maximum duty cycle.
 *          - PID_SAFETY_MARGIN_C           Safety margin around the setpoint to avoid overheating.
 *
 * @warning *** WARNING NOTICE ***
 *
 *          On the ESP32-C3, GPIOs 2, 8 et 9 are strapping pins. We use GPIO 8, which is LOW at boot on the
 *          XIAO ESP32C3 board we use. GPIO 9 is starting HIGH at boot (can be pulled LOW by the BOOT switch).
 *
 *          The GPIOs 20 & 21 are used by UART, and they are pulled HIGH at boot, so the FAN & LIGHT relays
 *          are pulled ON at boot for a short time. To avoid this, we should have used GPIO 8, 9, 20 or 21
 *          for I2C (SDA/SCL), and pins 3, 4, 5, 6, 7, 10 for RELAYS & EC11, which is not enough
 *          (EC11 needs 3 pins, and relays needs 4).
 *
 *          So the XIAO ESP32C3 board is not suitable for this project, a V2 logical board should be based
 *          on another board with more available free / non strapping GPIOs.
 *
 *          *** END OF WARNING NOTICE ***
 *
 * 
 *
 */

/* std includes */
#include <math.h>
#include <stdio.h>

/* David's multiplatform components */
#include <ml_button.h>
#include <ml_common.h>
#include <ml_ec11.h>
#include <ml_gpio.h>
#include <ml_hrtimer.h>
#include <ml_i2c.h>
#include <ml_pid_heater.h>
#include <ml_sht3x.h>

/* ESP-IDF specific */
#include "nvs_flash.h"

/* local includes */
#include "display.h"
#include "hw_conf.h"
#include "relays.h"

/* global variables */
static ml_sht3x_t        sht;
static ml_ec11_t         ec11;
static ml_button_t       button;
static ml_hrtimer_t      sht_timer;
static ml_hrtimer_t      delayed_start_timer;
static ml_hrtimer_t      delayed_stop_timer;
static ml_hrtimer_t      cycle_timer;
static ml_hrtimer_t      light_timer;
static ml_hrtimer_t      ec11_timer;
static ml_pid_heater_t   heater;
static SemaphoreHandle_t mutex;
static dcs_runtime_t     runtime;

#define TAG          "DCS"

/* basic locking macros */

#define ENTER_LOCK() xSemaphoreTake(mutex, portMAX_DELAY)
#define EXIT_LOCK()  xSemaphoreGive(mutex)

/* helper functions */

static inline uint16_t f_to_u16_x10(float f)
{
    return (uint16_t)roundf(f * 10.0f);
}

static inline float u16_x10_to_f(uint16_t i)
{
    return (float)i / 10.0f;
}

/* utility functions */

static void toggle_light(void)
{
    bool state = relay_get_state(RELAY_LIGHT);
    if (state)
    {
        ml_hrtimer_stop(&light_timer);
    }
    else
    {
        ml_hrtimer_start_once(&light_timer, LIGHT_ON_DELAY_US);
    }
    relay_set_state(RELAY_LIGHT, !state);
}

static void probe_sht31(void)
{
    float temp, hum;
    if (!ml_sht3x_read(&sht, &temp, &hum))
    {
        ESP_LOGE(TAG, "Failed to read from SHT31!");
    }
    runtime.temp_x10 = f_to_u16_x10(temp);
    runtime.hum      = lroundf(hum);
}

/* timers callbacks */

static bool ec11_timer_cb(void *user_data)
{
    bool a   = ml_gpio_read(EC11_S1_GPIO) == GPIO_STATE_HIGH;
    bool b   = ml_gpio_read(EC11_S2_GPIO) == GPIO_STATE_HIGH;
    bool btn = ml_gpio_read(EC11_KEY_GPIO) == GPIO_STATE_HIGH;

    ENTER_LOCK();
    ml_ec11_event_e   event        = ml_ec11_process(&ec11, a, b, GET_US_TIMESTAMP());
    ml_button_event_e button_event = ml_button_process(&button, btn, GET_US_TIMESTAMP());
    dcs_runtime_t     rt           = runtime;
    EXIT_LOCK();

    switch (button_event)
    {
        case BTN_EVENT_NONE:
            break;
        case BTN_EVENT_DOUBLE:
            toggle_light();
            display_button_event(&rt, button_event);
            break;
        default:
            display_button_event(&rt, button_event);
            break;
    }

    switch (event)
    {
        case EC11_EVENT_NONE:
            break;
        default:
            display_ec11_event(&rt, event, ml_ec11_get_value(&ec11));
            break;
    }

    /* continue periodic timer */
    return true;
}

/* sht 31 timer callback */

static bool sht_timer_cb(void *user_data)
{
    probe_sht31();
    /* keep the timer running (for periodic timers) */
    return true;
}

/* cycle timer callback - runs every second - keeps the cycle running for the specified duration */

static bool cycle_timer_cb(void *user_data)
{
    ENTER_LOCK();
    if (runtime.cycle_running)
    {
        runtime.cycle_remaining_s--;
        if (runtime.cycle_remaining_s == 0)
        {
            EXIT_LOCK();
            stop_cycle();
            /* false = stop the periodic timer on return */
            return false;
        }
    }
    EXIT_LOCK();
    /* true = keep the periodic timer running */
    return true;
}

/* delayed start of the pid heater */

static bool delayed_start_time_cb(void *user_data)
{
    ml_pid_heater_set_setpoint_temp(&heater, (float)runtime.setpoint_temp, (float)runtime.setpoint_temp + 5.0f);
    runtime.pid_running = true;
    /* one shot timer - stop it on return (semantic - has no effect on one shot timers) */
    return false;
}

/* delayed stop of the fan */

static bool delayed_stop_timer_cb(void *user_data)
{
    relay_set_state(RELAY_FAN, false);
    /* one shot timer - stop it on return */
    return false;
}

/* light off timer callback */

static bool light_timer_cb(void *user_data)
{
    relay_set_state(RELAY_LIGHT, false);
    /* one shot timer - stop it on return */
    return false;
}

/* cycle control functions */

void start_cycle(uint8_t setpoint_temp_c, uint16_t duration_s)
{
    if (is_cycle_running())
    {
        stop_cycle();
        return;
    }

    /* ensure the delayed stop timer is stopped */
    ml_hrtimer_stop(&delayed_stop_timer);

    /* start the fan */
    relay_set_state(RELAY_FAN, true);

    /* start the timer for the delayed start of the heater */
    ml_hrtimer_start_once(&delayed_start_timer, CYCLE_START_HEATER_START_DELAY_US);

    /* start the cycle timer immediately */
    runtime.cycle_duration_s = runtime.cycle_remaining_s = duration_s;
    runtime.cycle_running                                = true;
    runtime.setpoint_temp                                = setpoint_temp_c;

    ESP_LOGI(TAG, "Starting cycle for %d seconds at %d°C.", duration_s, setpoint_temp_c);

    /* 1 second tick */
    ml_hrtimer_start_periodic(&cycle_timer, 1000 * 1000);
}

void stop_cycle(void)
{
    if (!is_cycle_running())
    {
        return;
    }
    ml_hrtimer_stop(&delayed_start_timer);
    ml_pid_heater_reset_state(&heater);
    ml_hrtimer_stop(&cycle_timer);

    runtime.cycle_running     = false;
    runtime.pid_running       = false;
    runtime.cycle_remaining_s = runtime.cycle_duration_s = 0;

    relay_set_state(RELAY_HEATER, false);
    ml_hrtimer_start_once(&delayed_stop_timer, CYCLE_END_FAN_STOP_DELAY_US);

    ESP_LOGI(TAG, "Cycle stopped.");
}

bool is_cycle_running(void)
{
    ENTER_LOCK();
    bool result = runtime.cycle_running;
    EXIT_LOCK();
    return result;
}

/* PID heater callbacks */

static float heater_get_temperature_cb(ml_pid_heater_t *heater, void *user_ctx)
{
    ENTER_LOCK();
    float temp = sht.last_temperature;
    EXIT_LOCK();
    return temp;
}

static void heater_set_heater_state_cb(ml_pid_heater_t *heater, bool heater_state, void *user_ctx)
{
    relay_set_state(RELAY_HEATER, heater_state);
}

static void heater_event_cb(ml_pid_heater_t *heater, const ml_pid_heater_event_t *evt)
{
    switch (evt->type)
    {
        case ML_EPI_EVT_STEP_DONE:
            ESP_LOGI(TAG,
                     "#STEP DONE# ts=[%lu] loop=[%lu] driver=%s setpoint=%.2f T=%.2f°C duty=%.1f%% on=%lu/%lu ms",
                     evt->epoch_ms,
                     evt->elapsed_ms,
                     evt->step_done.driver_type == ML_PID_HEATER_DRIVER_RELAY ? "RELAY" : "SSR",
                     evt->step_done.setpoint_c,
                     evt->step_done.temp_c,
                     evt->step_done.duty_percent,
                     evt->step_done.on_ms,
                     evt->step_done.period_ms);
            break;
        case ML_EPI_EVT_PREDICTIVE_CUTOFF:
            ESP_LOGI(TAG,
                     "#PREDICTIVE CUTOFF# ts=[%lu] loop=[%lu] setpoint=%.2f°C T=%.2f°C rate=%.4fC/s predicted=%.2f°C",
                     evt->epoch_ms,
                     evt->elapsed_ms,
                     evt->predictive_cutoff.setpoint_c,
                     evt->predictive_cutoff.temp_c,
                     evt->predictive_cutoff.rate_c_per_s,
                     evt->predictive_cutoff.predicted_temp);
            break;
        case ML_EPI_EVT_SAFETY_CUTOFF:
            ESP_LOGE(TAG,
                     "#SAFETY CUTOFF# ts=[%lu] loop=[%lu] setpoint=%.2f°C T=%.2f°C max=%.2f°C",
                     evt->epoch_ms,
                     evt->elapsed_ms,
                     evt->safety_cutoff.setpoint_c,
                     evt->safety_cutoff.temp_c,
                     evt->safety_cutoff.max_c);
            break;
        case ML_EPI_EVT_INVALID_TEMP:
            ESP_LOGE(TAG, "#INVALID TEMP# ts=[%lu] T=%.2f°C", evt->epoch_ms, evt->invalid_temp);
            break;
        default:
            break;
    }
}

/* notification functions from the display */

void display_off(void)
{
    ENTER_LOCK();
    runtime.display_on = false;
    EXIT_LOCK();
}

void display_on(void)
{
    ENTER_LOCK();
    runtime.display_on = true;
    EXIT_LOCK();
}

/* NVS functions to save and restore parameters */

void write_indexes_to_nvs(uint8_t duration_index, uint8_t temp_index)
{
    nvs_handle_t nvs_handle;
    if (nvs_open("heater", NVS_READWRITE, &nvs_handle) == ESP_OK)
    {
        nvs_set_u8(nvs_handle, "duration_index", duration_index);
        nvs_set_u8(nvs_handle, "temp_index", temp_index);
        nvs_commit(nvs_handle);
        nvs_close(nvs_handle);
    }
}

bool read_indexes_from_nvs(uint8_t *duration_index, uint8_t *temp_index)
{
    nvs_handle_t nvs_handle;
    if (nvs_open("heater", NVS_READWRITE, &nvs_handle) == ESP_OK)
    {
        nvs_get_u8(nvs_handle, "duration_index", duration_index);
        nvs_get_u8(nvs_handle, "temp_index", temp_index);
        nvs_close(nvs_handle);
        return true;
    }
    return false;
}

/* Entry point */

void app_main(void)
{
    /* init runtime */
    memset(&runtime, 0, sizeof(runtime));

    runtime.cycle_running = false;
    runtime.fan_state     = false;
    runtime.heater_state  = false;
    runtime.light_state   = false;
    runtime.display_on    = true;

    /* init (but don't start) all timers */
    ml_hrtimer_init(&delayed_start_timer, "dLay_start_timer", delayed_start_time_cb, NULL);
    ml_hrtimer_init(&delayed_stop_timer, "dLay_stop_timer", delayed_stop_timer_cb, NULL);
    ml_hrtimer_init(&light_timer, "light_timer", light_timer_cb, NULL);
    ml_hrtimer_init(&sht_timer, "sht31_timer", sht_timer_cb, NULL);
    ml_hrtimer_init(&ec11_timer, "ec11_timer", ec11_timer_cb, NULL);
    ml_hrtimer_init(&cycle_timer, "cycle_timer", cycle_timer_cb, NULL);

    /* init NVS */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        if (nvs_flash_erase() != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to erase NVS flash !");
            return;
        }
        ret = nvs_flash_init();
    }
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize NVS flash !");
        return;
    }

    /* init relays GPIOs */
    if (!relays_init())
    {
        ESP_LOGE(TAG, "Failed to initialize relays!");
        return;
    }

    /* arbitrary delay to allow relays to settle */
    ml_delay_ms(1000);

    /* create mutual exclusion mutex */
    mutex = xSemaphoreCreateMutex();

    /* init I2C bus */
    ml_i2c_init(I2C_NUM_0, I2C0_SPEED, I2C0_SDA_GPIO, I2C0_SCL_GPIO, false);

    /* init display */
    if (!display_init())
    {
        return;
    }

    /* init SHT31 sensor */
    if (!ml_sht3x_init(&sht, I2C_NUM_0, SHT31_I2C_ADDR, ML_SHT3X_REPEAT_HIGH, false))
    {
        ESP_LOGE(TAG, "Failed to initialize SHT31!");
        return;
    }
    else
    {
        ESP_LOGI(TAG, "SHT31 initialized successfully.");
    }

    /* init EC11 rotary encoder + button */
    ml_ec11_init(&ec11, 0, 100, 1, 0, 4);
    ml_ec11_set_acceleration(&ec11, false, 0, 0, 0, 0);

    ml_button_init(&button);
    ml_button_set_logic(&button, BTN_LOGIC_ACTIVE_LOW);
    ml_button_set_longclick_ms(&button, EC11_LONG_CLICK_DELAY_MS);

    ml_gpio_init(EC11_S1_GPIO, GPIO_DIR_INPUT, GPIO_PULL_UP);
    ml_gpio_init(EC11_S2_GPIO, GPIO_DIR_INPUT, GPIO_PULL_UP);
    ml_gpio_init(EC11_KEY_GPIO, GPIO_DIR_INPUT, GPIO_PULL_UP);

    /* starts the EC11 timer */
    ml_hrtimer_start_periodic(&ec11_timer, EC11_MEASURE_INTERVAL_US);

    /* starts the SHT31 timer */
    ml_hrtimer_start_periodic(&sht_timer, SHT31_MEASURE_INTERVAL_MS * 1000ULL);

    /* init PID controller */
    static ml_pid_heater_io_t heater_io = {.get_temperature  = heater_get_temperature_cb,
                                           .set_heater_state = heater_set_heater_state_cb,
                                           .on_event         = heater_event_cb,
                                           .user_ctx         = NULL};

    if (!ml_pid_heater_init(&heater, ML_PID_HEATER_DRIVER_RELAY, &heater_io))
    {
        ESP_LOGE(TAG, "Failed to initialize heater!");
        return;
    }

    /* finetune parameters to match the target oven */
    ml_pid_heater_set_kp(&heater, PID_KP);
    ml_pid_heater_set_ki(&heater, PID_KI);
    ml_pid_heater_set_kd(&heater, PID_KD);
    ml_pid_heater_set_lookahead(&heater, PID_LOOKAHEAD_S);
    ml_pid_heater_set_limits(&heater, PID_OUT_MIN, PID_OUT_MAX);
    ml_pid_heater_set_safety_margin(&heater, PID_SAFETY_MARGIN_C);
    ml_pid_heater_set_min_on(&heater, PID_MIN_ON_MS);

    /* main task loop */
    for (;;)
    {
        /* update display on a snapshot of the runtime data */
        dcs_runtime_t rt;

        ENTER_LOCK();
        runtime.heater_state = relay_get_state(RELAY_HEATER);
        runtime.fan_state    = relay_get_state(RELAY_FAN);
        runtime.light_state  = relay_get_state(RELAY_LIGHT);
        rt                   = runtime;
        EXIT_LOCK();

        if (rt.display_on)
        {
            display_update(&rt);
        }

        /* poll PID controller if a cycle is running */
        if (runtime.pid_running)
        {
            ml_pid_heater_poll(&heater, GET_MS_TIMESTAMP());
        }

        /* small delay to breath CPU */
        ml_delay_ms(100);
    }
}
