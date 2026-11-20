/**
 * @file ml_common.h
 * @brief Common definitions and utilities for the embedded multiplatform library.
 *        Provides common macros, types, and utility functions used across different
 *        components of the library.
 *
 * @author dcox69 (David GOMEZ) <dcox69@proton.me>
 *
 * @version 1.1
 * @date 2026-03-15
 * @copyright Copyright (c) 2026 dcox69 (David GOMEZ).
 *
 */
#pragma once

#include "ml_errors.h"
#include "ml_platform.h"
#include "ml_variant.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>

#if defined(ML_PLATFORM_PICO)
#include "hardware/timer.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#elif defined(ML_PLATFORM_ESP)
#include "esp_rom_sys.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#if CONFIG_FREERTOS_HZ != 1000
#error                                                                                                                 \
    "ERROR : CONFIG_FREERTOS_HZ MUST be set to 1000 Hz for this library to work properly. Change that in menuconfig !"
#endif
#if CONFIG_ESP_TASK_WDT_TIMEOUT_S < 3
#warning "[ML_COMMON] Warning : The Watchdog is set low (< 3s). Long time calibrations & setup could fire the watchdog."
#endif
#else
#error "Platform undefined !"
#endif

__attribute__((weak)) int my_printf(const char *__restrict__ str, ...);

#define GPIO_BIT(pin, gpio) (((gpio) >> (pin)) & 1)

#ifndef ML_DEBUG_MODE
#define DEBUG_PRINTF (void)
// #error NOTE : ML_DEBUG_MODE is not defined, debug prints are disabled. Define ML_DEBUG_MODE to
// enable them.
#else
#define DEBUG_PRINTF my_printf
#endif

#define UNUSED_PARAM(param) (void)param
#define _UNUSED_(param)     (void)param

/*
 * Abstract lock object.
 *
 * This is a fully abstract synchronization mechanism:
 *  - On Pico, it can wrap a critical_section, spinlock, or any custom code.
 *  - On ESP, it can wrap a FreeRTOS mutex or semaphore.
 *
 * The component only knows about these callbacks and does not depend
 * on any specific RTOS or bare-metal API.
 */
typedef struct ml_lock_s
{
    void *user_data;
    void  (*lock_fn)(void *user_data);
    void  (*unlock_fn)(void *user_data);
} ml_lock_t;

typedef void *(*ml_alloc_cb_t)(size_t size);
typedef void  (*ml_free_cb_t)(void *p);

#ifdef __DEPRECATED__
typedef struct ml_mem_s
{
    ml_alloc_cb_t ml_alloc;
    ml_free_cb_t  ml_free;
} ml_mem_t;
#endif

#if defined(ML_PLATFORM_PICO)
// --- TYPES ---
typedef absolute_time_t ml_time_t;

// --- FUNCTIONS (Direct mapping to Pico SDK functions) ---
// Time
#define ml_get_time_us()         time_us_64()
#define ml_time_diff_us(a, b)    absolute_time_diff_us(a, b)
#define ml_to_ms_since_boot(t)   to_ms_since_boot(t)

// Delay
#define ml_delay_ms(ms)          sleep_ms(ms)
#define ml_delay_us(us)          sleep_us(us)
#define ml_delay_us_blocking(us) busy_wait_us(us)
#define ml_delay_ms_blocking(ms) busy_wait_ms(ms)
#elif defined(ML_PLATFORM_ESP)
// --- ESP32 (ALL) ---
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// --- TYPES ---
typedef uint64_t ml_time_t;

// --- FUNCTIONS (Inline wrappers) ---

// Timing

/**
 * @brief Get the current monotonic time in microseconds.
 *
 * @return Current time in microseconds.
 */
static inline uint64_t ml_get_time_us(void)
{
    return (uint64_t)esp_timer_get_time();
}

/**
 * @brief Calculate the difference between two monotonic timestamps in microseconds.
 *
 * @param a The earlier timestamp.
 * @param b The later timestamp.
 * @return The difference in microseconds (b - a).
 */
static inline int64_t ml_time_diff_us(ml_time_t a, ml_time_t b)
{
    return (int64_t)(b - a);
}

/**
 * @brief Convert a monotonic timestamp to milliseconds since boot.
 *
 * @param t The monotonic timestamp in microseconds.
 * @return The time in milliseconds since boot.
 */
static inline uint32_t ml_to_ms_since_boot(ml_time_t t)
{
    return (uint32_t)(t / 1000ULL);
}

/**
 * @brief Delay execution for a specified number of microseconds (blocking).
 *
 * @param us The number of microseconds to delay.
 */
static inline void ml_delay_us_blocking(uint32_t us)
{
    esp_rom_delay_us(us);
}

/**
 * @brief Delay execution for a specified number of milliseconds (blocking).
 *
 * @param ms The number of milliseconds to delay.
 */
static inline void ml_delay_ms_blocking(uint32_t ms)
{
    esp_rom_delay_us(ms * 1000);
}

/**
 * @brief Delay execution for a specified number of milliseconds (non-blocking).
 *
 * @param ms The number of milliseconds to delay.
 * @note Requires CONFIG_FREERTOS_HZ = 1000 !
 */
static inline void ml_delay_ms(uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}

/**
 * @brief Delay execution for a specified number of microseconds (non-blocking).
 *
 * @param us The number of microseconds to delay.
 */
static inline void ml_delay_us(uint64_t us)
{
    if (us >= 1000)
    {
        ml_delay_ms((uint32_t)(us / 1000));
    }
    else
    {
        ml_delay_us_blocking((uint32_t)us);
    }
}

#else
#error "Platform undefined !"
#endif

/**
 * @brief Macros for getting the current timestamp
 *
 */
#define GET_MS_TIMESTAMP() ml_to_ms_since_boot(ml_get_time_us())
#define GET_US_TIMESTAMP() ml_get_time_us()

/**
 * @brief Macros for time checking.
 */
#ifdef NDEBUG
#define TIME_CHECK_BEGIN()
#define TIME_CHECK_END(label)
#else
#define TIME_CHECK_BEGIN()                                                                                             \
    static uint64_t __time_check_start = 0;                                                                            \
    do                                                                                                                 \
    {                                                                                                                  \
        __time_check_start = ml_get_time_us();                                                                         \
    } while (0)
#define TIME_CHECK_END(label)                                                                                          \
    do                                                                                                                 \
    {                                                                                                                  \
        uint64_t __elapsed = ml_get_time_us() - __time_check_start;                                                    \
        DEBUG_PRINTF("Duration [%s]: %llu µs\n", label, __elapsed);                                                    \
    } while (0)
#endif

/**
 * @brief Macros for bit manipulation.
 */
#define BIT_SET(value, bit)           value |= (1 << (bit))
#define BIT_SET_TO(value, bit, state) value = (((value) & ~(1 << (bit))) | ((state) << (bit)))
#define BIT_CLEAR(value, bit)         value &= ~(1 << (bit))
#define BIT_TOGGLE(value, bit)        value ^= (1 << (bit))
#define BIT_IS_SET(value, bit)        ((value) >> (bit) & 1)
