/*
• =<< ********* >>=
• Author : dcox69 (David GOMEZ)
• Email : dcox69@proton.me
• Organization :
• Created On : 2026-02-21
• Title : ml_gpio.c - GPIO Abstraction Layer Implementation
• Description : Multiplatform GPIO support (Pico RP2040/RP2350, ESP32)
• Copyright (c) 2026 dcox69 (David GOMEZ).
• =<< ********* >>=
*/
#include "ml_gpio.h"

// --- User data storage for ISR callbacks ---
// Simple mechanism to store user callbacks (can be extended for multiple pins)
static ml_gpio_isr_callback_t registered_callbacks[GPIO_NUM_MAX] = {NULL};
static void                  *user_data_ptrs[GPIO_NUM_MAX]       = {NULL};

// ============= PICO (RP2040 / RP2350) IMPLEMENTATION =============

#if defined(ML_PLATFORM_PICO)

void ml_gpio_init(uint gpio_num, uint direction, uint pull)
{
    gpio_init(gpio_num);
    ml_gpio_set_direction(gpio_num, direction);
    ml_gpio_set_pull(gpio_num, pull);
}

void ml_gpio_set_direction(uint gpio_num, uint direction)
{
    gpio_set_dir(gpio_num, direction == GPIO_DIR_OUTPUT ? GPIO_OUT : GPIO_IN);
}

void ml_gpio_set_pull(uint gpio_num, uint pull)
{
    if (pull == GPIO_PULL_UP)
    {
        gpio_pull_up(gpio_num);
    }
    else if (pull == GPIO_PULL_DOWN)
    {
        gpio_pull_down(gpio_num);
    }
    else
    {
        gpio_disable_pulls(gpio_num);
    }
}

void ml_gpio_write(uint gpio_num, uint state)
{
    gpio_put(gpio_num, state == GPIO_STATE_HIGH ? 1 : 0);
}

uint ml_gpio_read(uint gpio_num)
{
    return gpio_get(gpio_num) ? GPIO_STATE_HIGH : GPIO_STATE_LOW;
}

void ml_gpio_toggle(uint gpio_num)
{
    gpio_xor_mask(1 << gpio_num);
}

// ISR callback handler for Pico
static void ml_gpio_irq_handler(uint gpio_num, uint32_t events)
{
    if (gpio_num < GPIO_NUM_MAX && registered_callbacks[gpio_num] != NULL)
    {
        registered_callbacks[gpio_num](gpio_num, user_data_ptrs[gpio_num]);
    }
}

void ml_gpio_set_isr(uint gpio_num, uint event, ml_gpio_isr_callback_t callback, void *user_data)
{
    if (gpio_num >= GPIO_NUM_MAX)
    {
        return;
    }
    registered_callbacks[gpio_num] = callback;
    user_data_ptrs[gpio_num]       = user_data;

    if (callback == NULL)
    {
        // Disable interrupt
        gpio_set_irq_enabled(gpio_num, GPIO_IRQ_LEVEL_HIGH | GPIO_IRQ_LEVEL_LOW, false);
    }
    else
    {
        // Enable interrupt with appropriate edge
        uint32_t events = 0;
        if (event == GPIO_INTR_RISING)
        {
            events = GPIO_IRQ_EDGE_RISE;
        }
        else if (event == GPIO_INTR_FALLING)
        {
            events = GPIO_IRQ_EDGE_FALL;
        }
        else if (event == GPIO_INTR_BOTH)
        {
            events = GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL;
        }
        gpio_set_irq_enabled_with_callback(gpio_num, events, true, &ml_gpio_irq_handler);
    }
}

void ml_gpio_deinit(uint gpio_num)
{
    gpio_set_irq_enabled(gpio_num, GPIO_IRQ_LEVEL_HIGH | GPIO_IRQ_LEVEL_LOW, false);
    registered_callbacks[gpio_num] = NULL;
    user_data_ptrs[gpio_num]       = NULL;
}

// ============= ESP32 IMPLEMENTATION =============

#elif defined(ML_PLATFORM_ESP)

void ml_gpio_init(uint gpio_num, uint direction, uint pull)
{
    gpio_config_t conf = {
        .pin_bit_mask = (1ULL << gpio_num),
        //        .mode = direction == GPIO_DIR_OUTPUT ? GPIO_MODE_OUTPUT : GPIO_MODE_INPUT,
        .mode         = direction == GPIO_DIR_OUTPUT ? (GPIO_MODE_INPUT_OUTPUT) : GPIO_MODE_INPUT,
        .pull_up_en   = (pull == GPIO_PULL_UP) ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
        .pull_down_en = (pull == GPIO_PULL_DOWN) ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&conf);
}

void ml_gpio_set_direction(uint gpio_num, uint direction)
{
    gpio_set_direction(gpio_num, direction == GPIO_DIR_OUTPUT ? GPIO_MODE_OUTPUT : GPIO_MODE_INPUT);
}

void ml_gpio_set_pull(uint gpio_num, uint pull)
{
    if (pull == GPIO_PULL_UP)
    {
        gpio_pullup_en(gpio_num);
        gpio_pulldown_dis(gpio_num);
    }
    else if (pull == GPIO_PULL_DOWN)
    {
        gpio_pullup_dis(gpio_num);
        gpio_pulldown_en(gpio_num);
    }
    else
    {
        gpio_pullup_dis(gpio_num);
        gpio_pulldown_dis(gpio_num);
    }
}

void ml_gpio_write(uint gpio_num, uint state)
{
    gpio_set_level(gpio_num, state == GPIO_STATE_HIGH ? 1 : 0);
}

uint ml_gpio_read(uint gpio_num)
{
    return gpio_get_level(gpio_num) ? GPIO_STATE_HIGH : GPIO_STATE_LOW;
}

void ml_gpio_toggle(uint gpio_num)
{
    uint current = gpio_get_level(gpio_num);
    gpio_set_level(gpio_num, current ? 0 : 1);
}

// ISR callback handler for ESP32
static void ml_gpio_irq_handler(void *arg)
{
    uint gpio_num = (uintptr_t)arg;
    if (gpio_num < GPIO_NUM_MAX && registered_callbacks[gpio_num] != NULL)
    {
        registered_callbacks[gpio_num](gpio_num, user_data_ptrs[gpio_num]);
    }
}

void ml_gpio_set_isr(uint gpio_num, uint event, ml_gpio_isr_callback_t callback, void *user_data)
{
    if (gpio_num >= GPIO_NUM_MAX)
    {
        return;
    }
    registered_callbacks[gpio_num] = callback;
    user_data_ptrs[gpio_num]       = user_data;

    if (callback == NULL)
    {
        // Disable interrupt
        gpio_isr_handler_remove(gpio_num);
    }
    else
    {
        // Determine interrupt type
        gpio_int_type_t intr_type = GPIO_INTR_DISABLE;
        if (event == GPIO_INTR_RISING)
        {
            intr_type = GPIO_INTR_POSEDGE;
        }
        else if (event == GPIO_INTR_FALLING)
        {
            intr_type = GPIO_INTR_NEGEDGE;
        }
        else if (event == GPIO_INTR_BOTH)
        {
            intr_type = GPIO_INTR_ANYEDGE;
        }
        gpio_set_intr_type(gpio_num, intr_type);
        gpio_isr_handler_add(gpio_num, &ml_gpio_irq_handler, (void *)(uintptr_t)gpio_num);
    }
}

void ml_gpio_deinit(uint gpio_num)
{
    gpio_isr_handler_remove(gpio_num);
    registered_callbacks[gpio_num] = NULL;
    user_data_ptrs[gpio_num]       = NULL;
    gpio_reset_pin(gpio_num);
}

#else
#error "Unsupported platform for ml_gpio !"
#endif
