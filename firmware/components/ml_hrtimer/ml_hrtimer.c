#include "ml_hrtimer.h"

#if defined(ML_PLATFORM_ESP)
static void esp_timer_trampoline(void *arg)
{
    ml_hrtimer_t *t = (ml_hrtimer_t *)arg;
    if (!t || !t->cb)
    {
        return;
    }
    /* stop the timer if the callback returns false */
    bool keep_running = t->cb(t->user_data);
    if (t->periodic && !keep_running)
    {
        ml_hrtimer_stop(t);
    }
}
#elif defined(ML_PLATFORM_PICO)
static bool pico_timer_trampoline_periodic(struct repeating_timer *rt)
{
    ml_hrtimer_t *t = (ml_hrtimer_t *)rt->user_data;
    if (!t || !t->cb)
    {
        return false;
    }
    /* callback should return true to continue the timer, false to stop it */
    return t->cb(t->user_data);
}

static bool pico_timer_trampoline_once(struct repeating_timer *rt)
{
    ml_hrtimer_t *t = (ml_hrtimer_t *)rt->user_data;
    if (!t || !t->cb)
    {
        return false;
    }
    (void)t->cb(t->user_data);
    /* one shot timer, we return false */
    return false;
}
#endif

bool ml_hrtimer_init(ml_hrtimer_t *t, const char *name, ml_hrtimer_cb_t cb, void *user_data)
{
    if (!cb)
    {
        return false;
    }
    t->cb        = cb;
    t->user_data = user_data;
    t->running   = false;
    t->periodic  = false;
#if defined(ML_PLATFORM_PICO)
    (void)name;
#elif defined(ML_PLATFORM_ESP)
    esp_timer_create_args_t args = {
        .callback        = esp_timer_trampoline,
        .arg             = t,
        .dispatch_method = ESP_TIMER_TASK, // not ISR
        .name            = name,
    };

    if (esp_timer_create(&args, &t->h) != ESP_OK)
    {
        return false;
    }
#endif

    return true;
}

bool ml_hrtimer_start_periodic(ml_hrtimer_t *t, uint64_t period_us)
{
#if defined(ML_PLATFORM_ESP)
    if (!t || !t->h || period_us == 0)
    {
        return false;
    }

    (void)esp_timer_stop(t->h);
    t->periodic = true;
    t->running  = esp_timer_start_periodic(t->h, period_us) == ESP_OK;
#elif defined(ML_PLATFORM_PICO)
    if (!t || period_us == 0)
    {
        return false;
    }
    cancel_repeating_timer(&t->rt);
    t->rt.user_data = t;
    t->running      = add_repeating_timer_us(period_us, pico_timer_trampoline_periodic, t, &t->rt);
#endif
    return t->running;
}

bool ml_hrtimer_start_once(ml_hrtimer_t *t, uint64_t period_us)
{
#if defined(ML_PLATFORM_ESP)
    if (!t || !t->h || period_us == 0)
    {
        return false;
    }

    (void)esp_timer_stop(t->h);
    t->periodic = false;
    t->running  = esp_timer_start_once(t->h, period_us) == ESP_OK;
#elif defined(ML_PLATFORM_PICO)
    if (!t || period_ms == 0)
    {
        return false;
    }
    cancel_repeating_timer(&t->rt);
    t->rt.user_data = t;
    t->running      = add_repeating_timer_us(period_us, pico_timer_trampoline_once, t, &t->rt);
#endif
    return t->running;
}

bool ml_hrtimer_stop(ml_hrtimer_t *t)
{
#if defined(ML_PLATFORM_ESP)
    if (!t || !t->h)
    {
        return false;
    }
    esp_err_t err = esp_timer_stop(t->h);
    t->running    = false;
    return (err == ESP_OK || err == ESP_ERR_INVALID_STATE);
#elif defined(ML_PLATFORM_PICO)
    if (!t)
    {
        return false;
    }
    t->running = false;
    return cancel_repeating_timer(&t->rt);
#endif
}

bool ml_hrtimer_is_running(ml_hrtimer_t *t)
{
    return t->running;
}

void ml_hrtimer_deinit(ml_hrtimer_t *t)
{
    if (!t)
    {
        return;
    }
#if defined(ML_PLATFORM_ESP)
    if (t->h)
    {
        esp_timer_stop(t->h);
        esp_timer_delete(t->h);
    }
#elif defined(ML_PLATFORM_PICO)
    cancel_repeating_timer(&t->rt);
#endif
}
