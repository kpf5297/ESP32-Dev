#pragma once

/*
 * network_debug_task.h
 *
 * This module is intentionally decoupled from UI and networking consumers to preserve
 * deterministic real-time behavior on ESP32-C6.
 */

#include "freertos/FreeRTOS.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t interval_ms;
    uint32_t stack_size;
    UBaseType_t task_priority;
    BaseType_t core_id;
} network_debug_task_config_t;

void network_debug_task_get_default_config(network_debug_task_config_t *config);
bool network_debug_task_start(const network_debug_task_config_t *config);

#ifdef __cplusplus
}
#endif
