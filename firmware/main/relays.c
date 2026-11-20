#include <ml_gpio.h>

#include "hw_conf.h"
#include "relays.h"

#define TAG "relays"

bool relays_init(void)
{
    /* init relays GPIOs */
    ml_gpio_init(RELAY_FAN_GPIO, GPIO_DIR_OUTPUT, GPIO_PULL_DOWN);
    ml_gpio_write(RELAY_FAN_GPIO, GPIO_STATE_LOW);

    ml_gpio_init(RELAY_HEATER_GPIO, GPIO_DIR_OUTPUT, GPIO_PULL_DOWN);
    ml_gpio_write(RELAY_HEATER_GPIO, GPIO_STATE_LOW);

    ml_gpio_init(RELAY_LIGHT_GPIO, GPIO_DIR_OUTPUT, GPIO_PULL_DOWN);
    ml_gpio_write(RELAY_LIGHT_GPIO, GPIO_STATE_LOW);

    /* Relay feedback (we monitor the relay feedback instead of getting the GPIO state directly */
    ml_gpio_init(HEATER_FEEDBACK_GPIO, GPIO_DIR_INPUT, GPIO_NO_PULL);

    return true;
}

void relay_set_state(relay_id_e id, bool state)
{
    switch (id)
    {
        case RELAY_HEATER:
            ml_gpio_write(RELAY_HEATER_GPIO, state ? GPIO_STATE_HIGH : GPIO_STATE_LOW);
            ESP_LOGI(TAG, "heater %s", state ? "ON" : "OFF");
            break;
        case RELAY_LIGHT:
            ml_gpio_write(RELAY_LIGHT_GPIO, state ? GPIO_STATE_HIGH : GPIO_STATE_LOW);
            ESP_LOGI(TAG, "light %s", state ? "ON" : "OFF");
            break;
        case RELAY_FAN:
            ml_gpio_write(RELAY_FAN_GPIO, state ? GPIO_STATE_HIGH : GPIO_STATE_LOW);
            ESP_LOGI(TAG, "fan %s", state ? "ON" : "OFF");
            break;
    }
}

bool relay_get_state(relay_id_e id)
{
    switch (id)
    {
        case RELAY_HEATER:
            return ml_gpio_read(HEATER_FEEDBACK_GPIO) == GPIO_STATE_HIGH ? OPTO_ON : OPTO_OFF;
        case RELAY_LIGHT:
            return ml_gpio_read(RELAY_LIGHT_GPIO) == GPIO_STATE_HIGH;
        case RELAY_FAN:
            return ml_gpio_read(RELAY_FAN_GPIO) == GPIO_STATE_HIGH;
    }
    return false;
}
