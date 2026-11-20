#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <ml_button.h>
#include <ml_ec11.h>

#include "display.h"

typedef struct
{
    /* actual rounded temperature * 10 (455 = 45.5°C*/
    uint16_t temp_x10;
    /* actual rounded humidity percent */
    uint8_t  hum;
    /* current setpoint temperature */
    uint8_t  setpoint_temp;
    /* actual cycle duration (in seconds) */
    uint16_t cycle_duration_s;
    /* remaining cycle duration (in seconds) */
    uint16_t cycle_remaining_s;
    /* light relay state */
    bool     light_state;
    /* fan relay state */
    bool     fan_state;
    /* heater relay state (feedback) */
    bool     heater_state;
    /* cycle running state - true if a cycle is running */
    bool     cycle_running;
    /* true when the PID is running (which is delayed from the cycle) */ 
    bool     pid_running;
    /* display state */
    bool     display_on;
} dcs_runtime_t;


void start_cycle(uint8_t setpoint_temp_c, uint16_t duration_s);
void stop_cycle(void);
bool is_cycle_running(void);

void display_off(void);
void display_on(void);

void write_indexes_to_nvs(uint8_t duration_index, uint8_t temp_index);
bool read_indexes_from_nvs(uint8_t *duration_index, uint8_t *temp_index);
