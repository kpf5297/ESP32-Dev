#pragma once

/*
 * network_status.h
 *
 * This module is intentionally decoupled from UI and networking consumers to preserve
 * deterministic real-time behavior on ESP32-C6.
 */

#include "freertos/FreeRTOS.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NETWORK_STATUS_IPV4_STR_LEN 16

typedef struct {
    uint32_t stack_size;
    UBaseType_t task_priority;
    BaseType_t core_id;
} network_status_task_config_t;

bool network_status_init(void);
void network_status_task_get_default_config(network_status_task_config_t *config);
bool network_status_start_task(const network_status_task_config_t *config);

bool network_status_get_ipv4(char *buffer, size_t buffer_len);
bool network_status_is_connected(void);

#ifdef __cplusplus
}
#endif
