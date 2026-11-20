#include "display.h"
#include "hw_conf.h"

#include <ml_hrtimer.h>
#include <ml_i2c.h>
#include <ml_ssd1306.h>
#include <ml_ssd1306_font5x8_full.h>
#include <ml_ssd1306_font8x8_full.h>

#include "../gfx/img_fan.h"
#include "../gfx/img_heater.h"
#include "../gfx/img_light.h"

#define TAG "display"

static ml_ssd1306_t ssd;
static ml_hrtimer_t ssd_sleep_timer;
static bool         ssd_sleeping        = false;

static int8_t       selected_item_index = -1;
static bool         editing             = false;

static uint8_t      cur_temp_index      = 2;
static uint8_t      cur_duration_index  = 2;
static uint8_t      cur_start_index     = 0;
static uint8_t      cur_stop_index      = 0;
static uint8_t      cur_reset_index     = 0;

typedef struct
{
    char   *text;
    int32_t value;
} item_value_t;

/* screens / items tables */

static const item_value_t temp_table[] = {
    {"40°C", 40},
    {"45°C", 45},
    {"50°C", 50},
    {"55°C", 55},
    {"60°C", 60},
    {"65°C", 65},
    {"70°C", 70},
    {"75°C", 75}
}; /* 8 items */

static const item_value_t duration_table[] = {
    {"1H00",  60 * 60},
    {"1H30",  90 * 60},
    {"2H00", 120 * 60},
    {"2H30", 150 * 60},
    {"3H00", 180 * 60},
    {"3H30", 210 * 60},
    {"4H00", 240 * 60},
    {"4H30", 270 * 60},
    {"5H00", 300 * 60},
    {"5H30", 330 * 60},
    {"6H00", 360 * 60},
    {"6H30", 390 * 60},
    {"7H00", 420 * 60},
    {"7H30", 450 * 60},
    {"8H00", 480 * 60}
}; /* 15 items */

static const char *const start_table[2] = {"START [NO]", "START [YES]"};
static const char *const reset_table[2] = {"RESET [NO]", "RESET [YES]"};
static const char *const stop_table[2]  = {"STOP [NO]", "STOP [YES]"};

static bool ssd_timer_cb(void *user_data)
{
    ml_ssd1306_power_off(&ssd);
    display_off();
    ssd_sleeping = true;
    ESP_LOGI(TAG, "SSD1306 sleeping");
    return false;
}

static void ssd_wakeup(void)
{
    ml_ssd1306_power_on(&ssd);
    ml_hrtimer_start_once(&ssd_sleep_timer, SSD1306_SLEEP_US);
    ssd_sleeping = false;
    display_on();
}

static uint8_t inc_clamp_index(uint8_t index, uint8_t min, uint8_t max)
{
    index++;
    if (index > max) index = min;
    return index;
}

static uint8_t dec_clamp_index(uint8_t index, uint8_t min, uint8_t max)
{
    if (index > min)
        index--;
    else
        index = max;
    return index;
}

static void cycle_value(uint8_t *value, ml_ec11_event_e event, uint8_t min, uint8_t max)
{
    if (event == EC11_EVENT_ROTATE_RIGHT)
        *value = inc_clamp_index(*value, min, max);
    else
        *value = dec_clamp_index(*value, min, max);
}

/* draw a menu item at the specified position and with the required state */
static void draw_menu_item(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const char *text, bool selected, bool editing)
{
    uint8_t flags = ML_SSD1306_FLAG_VCENTER | ML_SSD1306_FLAG_HCENTER;
    if (selected)
    {
        /* selected items are boxed */
        flags |= ML_SSD1306_FLAG_BOXED;
        /* if this item is being edited, invert it */
        if (editing)
        {
            flags |= ML_SSD1306_FLAG_INVERT;
        }
    }
    ml_ssd1306_draw_str_ex(&ssd, x, y, w, h, text, &font8x8_full_font, flags);
}

static void cycle_screen_button_event(const dcs_runtime_t *runtime, ml_button_event_e event)
{
    if (!editing && event == BTN_EVENT_SINGLE)
    {
        if (selected_item_index == 0)
        {
            editing = true;
        }
    }
    else if (editing && event == BTN_EVENT_SINGLE)
    {
        editing = false;
        /* do action */
        switch (selected_item_index)
        {
            /* temp & duration */
            /* nothing to do */
            case 0:
                if (cur_stop_index == 1)
                {
                    stop_cycle();
                    cur_stop_index      = 0;
                    selected_item_index = -1;
                }
                break;
        }
    }
}

static void cycle_screen_ec11_event(const dcs_runtime_t *runtime, ml_ec11_event_e event, int32_t value)
{
    /* only stop button can be selected / edited  */
    if (!editing)
    {
        selected_item_index = (selected_item_index == 0) ? -1 : 0;
    }
    else
    {
        switch (selected_item_index)
        {
            /* temp */
            case 0:
                cycle_value(&cur_stop_index, event, 0, 1);
                break;
        }
    }
}

static void cycle_screen_draw(const dcs_runtime_t *runtime)
{
    char buf[32];

    /* running / blink effect */
    draw_menu_item(4,
                   18,
                   SSD1306_WIDTH - 8,
                   10,
                   runtime->cycle_remaining_s % 2 == 0 ? "< RUNNING >" : "> RUNNING <",
                   runtime->cycle_remaining_s % 2 == 0 ? true : false,
                   false);

    /* setpoint temp / cycle duration recap */
    sprintf(buf, "T: %s D: %s", temp_table[cur_temp_index].text, duration_table[cur_duration_index].text);
    draw_menu_item(4, 28, SSD1306_WIDTH - 8, 10, buf, false, false);

    /* remaining time */
    sprintf(buf,
            " %02d H %02d M %02d S",
            runtime->cycle_remaining_s / 3600,
            (runtime->cycle_remaining_s / 60) % 60,
            runtime->cycle_remaining_s % 60);
    draw_menu_item(4, 38, SSD1306_WIDTH - 8, 10, buf, false, false);

    /* stop button */
    draw_menu_item(4, 48, SSD1306_WIDTH - 8, 10, stop_table[cur_stop_index], selected_item_index == 0, editing);
}

/* main screen */

static void main_screen_ec11_event(const dcs_runtime_t *runtime, ml_ec11_event_e event, int32_t value)
{
    if (!editing)
    {
        if (event == EC11_EVENT_ROTATE_RIGHT)
        {
            selected_item_index++;
            if (selected_item_index > 3) selected_item_index = 0;
        }
        else if (event == EC11_EVENT_ROTATE_LEFT)
        {
            if (selected_item_index > 0)
                selected_item_index--;
            else
                selected_item_index = 3;
        }
    }
    else
    {
        switch (selected_item_index)
        {
            /* temp */
            case 0:
                cycle_value(&cur_temp_index, event, 0, 7);
                break;
            /* duration */
            case 1:
                cycle_value(&cur_duration_index, event, 0, 14);
                break;

            /* start */
            case 2:
                cur_start_index = cur_start_index == 0 ? 1 : 0;
                break;

            /* reset */
            case 3:
                cur_reset_index = cur_reset_index == 0 ? 1 : 0;
                break;
        }
    }
}

static void main_screen_button_event(const dcs_runtime_t *runtime, ml_button_event_e event)
{
    /* enter edit mode */
    if (!editing && event == BTN_EVENT_SINGLE)
    {
        if (selected_item_index == 0 || selected_item_index == 1 || selected_item_index == 2 ||
            selected_item_index == 3)
        {
            editing = true;
        }
    }
    /* validate field */
    else if (editing && event == BTN_EVENT_SINGLE)
    {
        editing = false;
        /* do action */
        switch (selected_item_index)
        {
            /* temp & duration */
            /* nothing to do */
            case 0:
            case 1:
                break;
            /* start */
            case 2:
                if (cur_start_index == 1)
                {
                    start_cycle(temp_table[cur_temp_index].value, duration_table[cur_duration_index].value);
                    write_indexes_to_nvs(cur_duration_index, cur_temp_index);
                    cur_start_index = 0;
                }
                break;
            case 3:
                if (cur_reset_index == 1)
                {
                    cur_temp_index     = 2;
                    cur_duration_index = 2;
                    cur_reset_index    = 0;
                    write_indexes_to_nvs(cur_duration_index, cur_temp_index);
                }
                break;
        }
    }
}

static void main_screen_draw(const dcs_runtime_t *runtime)
{
    char                buf[32];

    /* temp */
    const item_value_t *t = &temp_table[cur_temp_index];
    snprintf(buf, sizeof(buf), "Temperat. %s", t->text);
    draw_menu_item(4, 18, SSD1306_WIDTH - 8, 10, buf, selected_item_index == 0, editing);

    /* duration */
    const item_value_t *d = &duration_table[cur_duration_index];
    snprintf(buf, sizeof(buf), "Duration %s", d->text);
    draw_menu_item(4, 28, SSD1306_WIDTH - 8, 10, buf, selected_item_index == 1, editing);

    /* start */
    draw_menu_item(4, 38, SSD1306_WIDTH - 8, 10, start_table[cur_start_index], selected_item_index == 2, editing);

    /* reset */
    draw_menu_item(4, 48, SSD1306_WIDTH - 8, 10, reset_table[cur_reset_index], selected_item_index == 3, editing);
}

bool display_init(void)
{
    ml_ssd1306_config_t ssd_config = {
        .width        = SSD1306_WIDTH,
        .height       = SSD1306_HEIGHT,
        .i2c_addr     = SSD1306_I2C_ADDR,
        .i2c_port     = I2C_NUM_0,
        .external_vcc = false,
        .alloc        = NULL,
        .free         = NULL,
    };

    if (!ml_ssd1306_init(&ssd, &ssd_config))
    {
        ESP_LOGE(TAG, "Failed to initialize SSD1306!");
        return false;
    }
    else
    {
        ESP_LOGI(TAG, "SSD1306 initialized successfully.");
    }

    /* create screen saver timer & start it */
    ml_hrtimer_init(&ssd_sleep_timer, "ssd_timer", ssd_timer_cb, NULL);

    editing = false;

    ssd_wakeup();

    read_indexes_from_nvs(&cur_duration_index, &cur_temp_index);

    return true;
}

void display_update(const dcs_runtime_t *runtime)
{
    char buf[64];

    /* clear the screen */
    ml_ssd1306_clear(&ssd);

    /***************/
    /* status zone */
    /***************/

    /* icons */
    if (runtime->heater_state) ml_ssd1306_draw_image(&ssd, 0, 0, &img_heater);
    if (runtime->fan_state) ml_ssd1306_draw_image(&ssd, 16, 0, &img_fan);
    if (runtime->light_state) ml_ssd1306_draw_image(&ssd, 32, 0, &img_light);

    /* temp & humidity */
    snprintf(buf, sizeof(buf), "%.1f°C %3d%%", (float)runtime->temp_x10 / 10.0f, runtime->hum);
    ml_ssd1306_draw_str(&ssd, 52, 4, buf, &font5x8_full_font);

    ml_ssd1306_draw_rect(&ssd, 0, 0, 128, 16);

    /* draw the current screen into the buffer */
    if (runtime->cycle_running)
    {
        cycle_screen_draw(runtime);
    }
    else
    {
        main_screen_draw(runtime);
    }

    /* send the buffer to the device */
    ml_ssd1306_show(&ssd);
}

/**
 *  If a field is currently edited, we validate its value
 *  We wake up the display
 */
void display_button_event(const dcs_runtime_t *runtime, ml_button_event_e event)
{
    /* on any button event, we wake up the display */
    bool sleeping = ssd_sleeping;
    ssd_wakeup();
    /* screen was off, wake it up and do not proceed the event */
    if (sleeping) return;
    if (event != BTN_EVENT_SINGLE) return;

    /* react to the event */
    if (runtime->cycle_running)
    {
        cycle_screen_button_event(runtime, event);
    }
    else
    {
        main_screen_button_event(runtime, event);
    }
}

/**
 *  If a field is currently edited, we validate its value
 *  We wake up the display
 */
void display_ec11_event(const dcs_runtime_t *runtime, ml_ec11_event_e event, int32_t value)
{
    if (ssd_sleeping) return;
    ssd_wakeup();
    if (runtime->cycle_running)
    {
        cycle_screen_ec11_event(runtime, event, value);
    }
    else
    {
        main_screen_ec11_event(runtime, event, value);
    }
}
