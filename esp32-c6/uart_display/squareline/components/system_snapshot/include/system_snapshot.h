#pragma once

/*
 * system_snapshot.h
 *
 * Owns the system snapshot used by REST handlers and other consumers.
 * Producers update this snapshot; consumers read it lock-free.
 */

#include "freertos/FreeRTOS.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SYSTEM_SNAPSHOT_ISO8601_LEN 32
#define SYSTEM_SNAPSHOT_WIFI_RSSI_INVALID (-127)

typedef struct {
    int64_t epoch_seconds;
    char iso8601[SYSTEM_SNAPSHOT_ISO8601_LEN];
    float temperature_c;
    uint32_t uptime_ms;
    uint32_t free_heap_bytes;
    int8_t wifi_rssi_dbm;
    bool wifi_connected;
} system_snapshot_t;

typedef struct {
    uint32_t interval_ms;
    uint32_t stack_size;
    UBaseType_t task_priority;
    BaseType_t core_id;
} system_snapshot_metrics_task_config_t;

void system_snapshot_init(void);

void system_snapshot_update_time(int64_t epoch_seconds, const char *iso8601);
void system_snapshot_update_temperature(float temperature_c);
void system_snapshot_update_metrics(uint32_t uptime_ms,
                                    uint32_t free_heap_bytes,
                                    int8_t wifi_rssi_dbm,
                                    bool wifi_connected);

void system_snapshot_read(system_snapshot_t *out_snapshot);

void system_snapshot_metrics_task_get_default_config(system_snapshot_metrics_task_config_t *config);
bool system_snapshot_start_metrics_task(const system_snapshot_metrics_task_config_t *config);

#ifdef __cplusplus
}
#endif
