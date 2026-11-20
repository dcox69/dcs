#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "runtime.h"

bool display_init(void);
void display_update(const dcs_runtime_t *runtime);
void display_button_event(const dcs_runtime_t *runtime, ml_button_event_e event);
void display_ec11_event(const dcs_runtime_t *runtime, ml_ec11_event_e event, int32_t value);
