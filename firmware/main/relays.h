#pragma once

#include <stdbool.h>

typedef enum {
    RELAY_LIGHT = 0,
    RELAY_FAN,
    RELAY_HEATER,
} relay_id_e;

/* despite the internal command logic, true = on, false = off */

bool relays_init(void);
void relay_set_state(relay_id_e id, bool state);
bool relay_get_state(relay_id_e id);
