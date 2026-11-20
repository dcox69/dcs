#include "ml_button.h"

#define DEF_DEBOUNCE_US     1 * 1000   // 10 * 1000  //40 * 1000
#define DEF_DOUBLE_CLICK_US 250 * 1000 // 150 * 1000
#define DEF_LONG_CLICK_US   800 * 1000

typedef enum
{
    PRESSED,
    RELEASED
} btn_state_e;

static inline btn_state_e get_logical_state(ml_button_t *bt, bool hardware_state)
{
    if (bt->logic == BTN_LOGIC_ACTIVE_LOW)
    {
        return hardware_state ? RELEASED : PRESSED;
    }
    else
    {
        return hardware_state ? PRESSED : RELEASED;
    }
}

void ml_button_deinit(ml_button_t *bt)
{
    /* nothing to do here */
}

void ml_button_init(ml_button_t *bt)
{
    bt->last_state_change  = 0;
    bt->last_press_time    = 0;
    bt->last_release_time  = 0;
    bt->logic              = BTN_LOGIC_ACTIVE_LOW;

    /* To reverse the logic, you would need to set this field to false and reverse cur_state in the poll */
    bt->last_button_state  = true; /* active low logic -> true = button release - false = button pressed */
    bt->click_count        = 0;
    bt->longclick_detected = false;

    bt->debounce_us        = DEF_DEBOUNCE_US;
    bt->dblclick_us        = DEF_DOUBLE_CLICK_US;
    bt->longclick_us       = DEF_LONG_CLICK_US;
}

void ml_button_set_debounce_ms(ml_button_t *bt, uint32_t debounce_ms)
{
    bt->debounce_us = debounce_ms ? debounce_ms * 1000 : DEF_DEBOUNCE_US;
}

void ml_button_set_dblclick_ms(ml_button_t *bt, uint32_t dblclick_ms)
{
    bt->dblclick_us = (dblclick_ms) ? dblclick_ms * 1000 : DEF_DOUBLE_CLICK_US;
}

void ml_button_set_longclick_ms(ml_button_t *bt, uint32_t longclick_ms)
{
    bt->longclick_us = (longclick_ms) ? longclick_ms * 1000 : DEF_LONG_CLICK_US;
}

void ml_button_set_logic(ml_button_t *bt, ml_button_logic_e logic)
{
    bt->logic = logic;
}

ml_button_event_e ml_button_process(ml_button_t *bt, bool cur_state, uint64_t timestamp_us)
{

    ml_button_event_e event       = BTN_EVENT_NONE;

    btn_state_e       logic_state = get_logical_state(bt, cur_state);

    /* Change detected ? */
    if (cur_state != bt->last_button_state)
    {
        /* Debounce time */
        if (timestamp_us - bt->last_state_change > bt->debounce_us)
        {
            bt->last_state_change = timestamp_us;
            if (logic_state == PRESSED)
            {
                /* Press */
                bt->last_press_time    = timestamp_us;
                bt->longclick_detected = false;
            }
            else
            {
                /* Release */
                bt->last_release_time = timestamp_us;
                if (!bt->longclick_detected)
                {
                    bt->click_count++;
                }
            }
        }
    }

    /* Long click handling */
    if (logic_state == PRESSED && !bt->longclick_detected && (timestamp_us - bt->last_press_time) > bt->longclick_us)
    {
        event                  = BTN_EVENT_LONG;
        bt->longclick_detected = true;
        bt->click_count        = 0; /* Clicks are ignored when there is a long press */
    }

    /* Simple & double click */
    if (bt->click_count > 0 && (timestamp_us - bt->last_release_time > bt->dblclick_us))
    {
        if (bt->click_count == 1)
            event = BTN_EVENT_SINGLE;
        else if (bt->click_count == 2)
            event = BTN_EVENT_DOUBLE;
        bt->click_count = 0;
    }

    /* Store current hardware state (not logic state) */
    bt->last_button_state = cur_state;
    return event;
}