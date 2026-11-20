/**
 * @file ml_button.h
 * @brief API for handling button events with debounce, single/double click, and long click detection.
 * @author dcox69 (David GOMEZ) <dcox69@proton.me>
 *
 * @version 1.0
 * @date 2026-07-02
 * @copyright Copyright (c) 2026 dcox69 (David GOMEZ).
 *
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief button event returned by ml_button_process()
 *
 */
typedef enum
{
    BTN_EVENT_NONE,
    BTN_EVENT_SINGLE,
    BTN_EVENT_DOUBLE,
    BTN_EVENT_LONG
} ml_button_event_e;

typedef enum
{
    BTN_LOGIC_ACTIVE_LOW,
    BTN_LOGIC_ACTIVE_HIGH
} ml_button_logic_e;

/**
 * @brief Structure representing a button and its state
 *
 * @note this structure should be considered opaque and manipulated only through the provided API functions.
 *
 */
typedef struct ml_button_s
{
    uint64_t          debounce_us, dblclick_us, longclick_us;
    uint64_t          last_state_change;
    uint64_t          last_press_time;
    uint64_t          last_release_time;
    bool              last_button_state;
    bool              longclick_detected;
    uint8_t           click_count;
    ml_button_logic_e logic;
} ml_button_t;

/**
 * @brief Initialize button object
 *
 * @param bt the button object
 */
void ml_button_init(ml_button_t *bt);

/**
 * @brief Set the button debounce timing
 *
 * @param bt the button object
 * @param debounce_ms debounce delay (if 0, defaut delay is applied)
 *
 */
void ml_button_set_debounce_ms(ml_button_t *bt, uint32_t debounce_ms);

/**
 * @brief Set the button double click delay
 *
 * @param bt the button object
 * @param dbl_click_ms double click delay (if 0, default delay is applied)
 *
 */
void ml_button_set_dblclick_ms(ml_button_t *bt, uint32_t dblclick_ms);

/**
 * @brief Set the button long click delay
 *
 * @param bt the button object
 * @param longclick_ms long click delay (if 0, default delay is applied)
 *
 */
void ml_button_set_longclick_ms(ml_button_t *bt, uint32_t longclick_ms);

/**
 * @brief Set the button logic
 *
 * @param bt the button object
 * @param logic ml_button_logic_e (active low or active high)
 * @remark by default, the button is created with active low logic (LOW/0V is ACTIVE)
 *
 */
void ml_button_set_logic(ml_button_t *bt, ml_button_logic_e logic);

/**
 * @brief Poll the button
 *
 * @param bt the button object
 * @param cur_state current state (true = released, false = pushed)
 * @param timestamp_us current timestamp in microseconds
 * @return ml_button_event_e
 */
ml_button_event_e ml_button_process(ml_button_t *bt, bool cur_state, uint64_t timestamp_us);

/**
 * @brief Deinit the button
 *
 * @param bt the button object
 */
void ml_button_deinit(ml_button_t *bt);