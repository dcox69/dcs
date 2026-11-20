#pragma once

/* Hardware config */

#include "driver/gpio.h"

#define LOGIC_LOW  0
#define LOGIC_HIGH 1

/* Optocouplers module logic */ 
#define OPTO_LOGIC LOGIC_LOW

#if (OPTO_LOGIC == LOGIC_LOW)
#define OPTO_OFF true
#define OPTO_ON  false
#else
#define OPTO_OFF false
#define OPTO_ON  true
#endif

/* EC11 rotary encoder & touch switch */ 
#define EC11_KEY_GPIO                     GPIO_NUM_3
#define EC11_S2_GPIO                      GPIO_NUM_4
#define EC11_S1_GPIO                      GPIO_NUM_5
#define EC11_MEASURE_INTERVAL_US          500
#define EC11_LONG_CLICK_DELAY_MS          3000 // 3s

/* I2C bus */
#define I2C0_SPEED                        400 * 1000
#define I2C0_SDA_GPIO                     GPIO_NUM_6
#define I2C0_SCL_GPIO                     GPIO_NUM_7

/* Relays */
#define RELAY_HEATER_GPIO                 GPIO_NUM_8
#define RELAY_FAN_GPIO                    GPIO_NUM_21
#define HEATER_FEEDBACK_GPIO              GPIO_NUM_10
#define RELAY_LIGHT_GPIO                  GPIO_NUM_20

/* I2C Peripherals */
#define SSD1306_I2C_ADDR                  0x3C

#define SHT31_I2C_ADDR                    0x44
#define SHT31_MEASURE_INTERVAL_MS         500

#define SSD1306_WIDTH                     128
#define SSD1306_HEIGHT                    64

/* timings */
#define SSD1306_SLEEP_US                  2 * 60ULL * 1000ULL * 1000ULL     // 2min
#define CYCLE_START_HEATER_START_DELAY_US 5ULL * 1000ULL * 1000ULL          // 5s
#define CYCLE_END_FAN_STOP_DELAY_US       10ULL * 60ULL * 1000ULL * 1000ULL // 10min
#define LIGHT_ON_DELAY_US                 1ULL * 60ULL * 1000ULL * 1000ULL  // 1min

/* PID heater */
#define PID_KP                            3.0f
#define PID_KI                            0.03f
#define PID_KD                            15.0f
#define PID_LOOKAHEAD_S                   220.0f
#define PID_OUT_MIN                       10.0f
#define PID_OUT_MAX                       56.0f
#define PID_SAFETY_MARGIN_C               3.0f
#define PID_MIN_ON_MS                     2000 // <= 2000 Matches the 10.0% duty cycle for RELAY driver (20000 ms)
