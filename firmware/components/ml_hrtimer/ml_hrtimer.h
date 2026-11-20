#pragma once

#include <ml_common.h>

#if defined(ML_PLATFORM_PICO)
#include "pico/time.h"
#elif defined(ML_PLATFORM_ESP)
#include "esp_timer.h"
#endif

/* common callback */
/* return false to stop the timer, true to continue (for periodic timer) */
typedef bool (*ml_hrtimer_cb_t)(void *user_data);

typedef struct ml_hrtimer_s
{
#if defined(PLATFORM_RP2040) || defined(PLATFORM_RP2350)
    struct repeating_timer rt;
#elif defined(PLATFORM_ESP32) || defined(ESP_PLATFORM)
    esp_timer_handle_t h;
    bool               periodic;
#endif
    ml_hrtimer_cb_t cb;
    void           *user_data;
    bool            running;
} ml_hrtimer_t;

/**
 * @brief Initialize a high-resolution timer.
 *
 * @param t Pointer to the timer structure.
 * @param name Name of the timer (used for identification purposes).
 * @param cb Callback function to be called when the timer expires.
 * @param user_data User data to be passed to the callback function.
 * @return true if the timer was successfully initialized, false otherwise.
 */
bool ml_hrtimer_init(ml_hrtimer_t *t, const char *name, ml_hrtimer_cb_t cb, void *user_data);

/**
 * @brief Start or restart the timer to run once after the specified period.
 *
 * @param t Pointer to the timer structure.
 * @param period_us Timer period in microseconds.
 * @return true if the timer was successfully started, false otherwise.
 */
bool ml_hrtimer_start_once(ml_hrtimer_t *t, uint64_t period_us);

/**
 * @brief Start or restart the timer to run periodically with the specified period.
 *
 * @param t Pointer to the timer structure.
 * @param period_us Timer period in microseconds.
 * @return true if the timer was successfully started, false otherwise.
 */
bool ml_hrtimer_start_periodic(ml_hrtimer_t *t, uint64_t period_us);

/**
 * @brief Change the user data associated with the timer.
 *
 * @param t Pointer to the timer structure.
 * @param user_data New user data to be associated with the timer.
 */
static inline void ml_hrtimer_set_user_data(ml_hrtimer_t *t, void *user_data)
{
    t->user_data = user_data;
}

/**
 * @brief Change the callback function associated with the timer.
 *
 * @param t Pointer to the timer structure.
 * @param cb New callback function to be associated with the timer.
 */
static inline void ml_hrtimer_set_cb(ml_hrtimer_t *t, ml_hrtimer_cb_t cb)
{
    t->cb = cb;
}

/**
 * @brief Get the user data associated with the timer.
 *
 * @param t Pointer to the timer structure.
 * @return Pointer to the user data associated with the timer.
 */
static inline void *ml_hrtimer_get_user_data(ml_hrtimer_t *t)
{
    return t->user_data;
}

/**
 * @brief Get the callback function associated with the timer.
 *
 * @param t Pointer to the timer structure.
 * @return Callback function associated with the timer.
 */
static inline ml_hrtimer_cb_t ml_hrtimer_get_cb(ml_hrtimer_t *t)
{
    return t->cb;
}

/**
 * @brief Check if the timer is currently running.
 *
 * @param t Pointer to the timer structure.
 * @return true if the timer is running, false otherwise.
 */
bool ml_hrtimer_is_running(ml_hrtimer_t *t);

/**
 * @brief Stop the timer if it is running.
 *
 * @param t Pointer to the timer structure.
 * @return true if the timer was successfully stopped, false otherwise.
 */
bool ml_hrtimer_stop(ml_hrtimer_t *t);

/**
 * @brief Deinitialize the timer and release any associated resources.
 *        The timer should be stopped before calling this function.
 *
 * @param t Pointer to the timer structure.
 */
void ml_hrtimer_deinit(ml_hrtimer_t *t);
