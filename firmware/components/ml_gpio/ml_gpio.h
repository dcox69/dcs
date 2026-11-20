/*
• =<< ********* >>=
• Author : dcox69 (David GOMEZ)
• Email : dcox69@proton.me
• Organization :
• Created On : 2026-02-21
• Title : ml_gpio.h - GPIO Abstraction Layer
• Description : Multiplatform GPIO support (Pico RP2040/RP2350, ESP32)
• Copyright (c) 2026 dcox69 (David GOMEZ).
• =<< ********* >>=
*/
#pragma once

#include <ml_common.h>

/**************** Platform-specific includes ****************/

#if defined(ML_PLATFORM_PICO)
#include "hardware/gpio.h"
#elif defined(ML_PLATFORM_ESP)
#include "driver/gpio.h"
#endif

#define GPIO_NUM_MAX      64

/**************** Generic GPIO Defines (Platform Independent) ****************/

/**************** GPIO States ****************/
#define GPIO_STATE_LOW    0
#define GPIO_STATE_HIGH   1

/**************** GPIO Directions ****************/
#define GPIO_DIR_INPUT    0
#define GPIO_DIR_OUTPUT   1

/**************** GPIO Pull Resistor Configuration ****************/
#define GPIO_NO_PULL      0
#define GPIO_PULL_UP      1
#define GPIO_PULL_DOWN    2

#define GPIO_UNUSED       -1

/**************** ISR-related defines (Interrupt triggering) ****************/
/**************** Generic edge detection types ****************/
#define GPIO_INTR_DISABLE 0
#define GPIO_INTR_RISING  1 /* GPIO goes from LOW to HIGH */
#define GPIO_INTR_FALLING 2 /* GPIO goes from HIGH to LOW */
#define GPIO_INTR_BOTH    3 /* Either rising or falling */

/**************** Callback function type for GPIO interrupts ****************/
typedef void (*ml_gpio_isr_callback_t)(uint gpio_num, void *user_data);

/**************** Function Declarations ****************/

/**
 * @brief Initialize a GPIO with direction and optional pull resistor
 *
 * @param gpio_num GPIO pin number (0-28 on Pico, 0-48 on ESP32)
 * @param direction GPIO_DIR_INPUT or GPIO_DIR_OUTPUT
 * @param pull GPIO_NO_PULL, GPIO_PULL_UP, or GPIO_PULL_DOWN
 */
void ml_gpio_init(uint gpio_num, uint direction, uint pull);

/**
 * @brief Set GPIO direction (input or output)
 *
 * @param gpio_num GPIO pin number
 * @param direction GPIO_DIR_INPUT or GPIO_DIR_OUTPUT
 */
void ml_gpio_set_direction(uint gpio_num, uint direction);

/**
 * @brief Set GPIO internalpull resistor configuration
 *
 * @param gpio_num GPIO pin number
 * @param pull GPIO_NO_PULL, GPIO_PULL_UP, or GPIO_PULL_DOWN
 */
void ml_gpio_set_pull(uint gpio_num, uint pull);

/**
 * @brief Write a digital value to a GPIO (output only)
 *
 * @param gpio_num GPIO pin number
 * @param state GPIO_STATE_LOW (0) or GPIO_STATE_HIGH (1)
 */
void ml_gpio_write(uint gpio_num, uint state);

/**
 * @brief Read the digital value from a GPIO
 *
 * @param gpio_num GPIO pin number
 * @return GPIO_STATE_LOW (0) or GPIO_STATE_HIGH (1)
 */
uint ml_gpio_read(uint gpio_num);

/**
 * @brief Toggle GPIO state (HIGH -> LOW or LOW -> HIGH)
 *
 * @param gpio_num GPIO pin number
 */
void ml_gpio_toggle(uint gpio_num);

/**
 * @brief Set up GPIO interrupt with callback
 *
 * @param gpio_num GPIO pin number
 * @param event GPIO_INTR_RISING, GPIO_INTR_FALLING, or GPIO_INTR_BOTH
 * @param callback Function to call when interrupt fires (NULL to disable)
 * @param user_data Optional user pointer passed to callback
 */
void ml_gpio_set_isr(uint gpio_num, uint event, ml_gpio_isr_callback_t callback, void *user_data);

/**
 * @brief Deinitialize a GPIO (release resources)
 *
 * @param gpio_num GPIO pin number
 */
void ml_gpio_deinit(uint gpio_num);