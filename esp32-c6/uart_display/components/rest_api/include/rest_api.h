#pragma once

/*
 * rest_api.h
 *
 * HTTP REST API for exposing cached system snapshot data.
 * No business logic or hardware access lives here.
 */

#include "freertos/FreeRTOS.h"
#include "system_snapshot.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define REST_API_TIME_JSON_LEN 96
#define REST_API_TEMP_JSON_LEN 64
#define REST_API_STATUS_JSON_LEN 160

#ifdef REST_API_ENABLE_TRACE
#include "esp_log.h"
#define REST_API_TRACE(fmt, ...) ESP_LOGI("REST_API", fmt, ##__VA_ARGS__)
#else
#define REST_API_TRACE(...) do { } while (0)
#endif

typedef struct {
    uint16_t server_port;
    uint32_t stack_size;
    UBaseType_t task_priority;
    BaseType_t core_id;
} rest_api_config_t;

void rest_api_get_default_config(rest_api_config_t *config);
bool rest_api_start(const rest_api_config_t *config);
void rest_api_stop(void);

size_t rest_api_build_time_json(const system_snapshot_t *snapshot,
                                char *out_buf,
                                size_t out_len);
size_t rest_api_build_temperature_json(const system_snapshot_t *snapshot,
                                       char *out_buf,
                                       size_t out_len);
size_t rest_api_build_status_json(const system_snapshot_t *snapshot,
                                  char *out_buf,
                                  size_t out_len);

#ifdef __cplusplus
}
#endif
