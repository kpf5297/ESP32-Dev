/*
 * system_snapshot.c
 *
 * Owns the system snapshot used by REST handlers and other consumers.
 * Producers update this snapshot; consumers read it lock-free.
 */

#include "system_snapshot.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "SYSTEM_SNAPSHOT";

static system_snapshot_t s_snapshot;
static portMUX_TYPE s_snapshot_mux = portMUX_INITIALIZER_UNLOCKED;
static TaskHandle_t s_metrics_task_handle = NULL;
static system_snapshot_metrics_task_config_t s_metrics_task_config;

#define SYSTEM_SNAPSHOT_METRICS_STACK_SIZE 3072U
#define SYSTEM_SNAPSHOT_METRICS_TASK_PRIORITY (tskIDLE_PRIORITY + 1)
#define SYSTEM_SNAPSHOT_METRICS_INTERVAL_MS 2000U

static void system_snapshot_copy_default_iso(char *dest, size_t len)
{
    (void)strlcpy(dest, "1970-01-01T00:00:00Z", len);
}

void system_snapshot_init(void)
{
    portENTER_CRITICAL(&s_snapshot_mux);
    s_snapshot.epoch_seconds = 0;
    system_snapshot_copy_default_iso(s_snapshot.iso8601, sizeof(s_snapshot.iso8601));
    s_snapshot.temperature_c = 0.0f;
    s_snapshot.uptime_ms = 0U;
    s_snapshot.free_heap_bytes = 0U;
    s_snapshot.wifi_rssi_dbm = SYSTEM_SNAPSHOT_WIFI_RSSI_INVALID;
    s_snapshot.wifi_connected = false;
    portEXIT_CRITICAL(&s_snapshot_mux);
}

void system_snapshot_update_time(int64_t epoch_seconds, const char *iso8601)
{
    if (iso8601 == NULL) {
        return;
    }

    portENTER_CRITICAL(&s_snapshot_mux);
    s_snapshot.epoch_seconds = epoch_seconds;
    (void)strlcpy(s_snapshot.iso8601, iso8601, sizeof(s_snapshot.iso8601));
    portEXIT_CRITICAL(&s_snapshot_mux);
}

void system_snapshot_update_temperature(float temperature_c)
{
    portENTER_CRITICAL(&s_snapshot_mux);
    s_snapshot.temperature_c = temperature_c;
    portEXIT_CRITICAL(&s_snapshot_mux);
}

void system_snapshot_update_metrics(uint32_t uptime_ms,
                                    uint32_t free_heap_bytes,
                                    int8_t wifi_rssi_dbm,
                                    bool wifi_connected)
{
    portENTER_CRITICAL(&s_snapshot_mux);
    s_snapshot.uptime_ms = uptime_ms;
    s_snapshot.free_heap_bytes = free_heap_bytes;
    s_snapshot.wifi_rssi_dbm = wifi_rssi_dbm;
    s_snapshot.wifi_connected = wifi_connected;
    portEXIT_CRITICAL(&s_snapshot_mux);
}

void system_snapshot_read(system_snapshot_t *out_snapshot)
{
    if (out_snapshot == NULL) {
        return;
    }

    portENTER_CRITICAL(&s_snapshot_mux);
    memcpy(out_snapshot, &s_snapshot, sizeof(*out_snapshot));
    portEXIT_CRITICAL(&s_snapshot_mux);
}

void system_snapshot_metrics_task_get_default_config(system_snapshot_metrics_task_config_t *config)
{
    if (config == NULL) {
        return;
    }

    config->interval_ms = SYSTEM_SNAPSHOT_METRICS_INTERVAL_MS;
    config->stack_size = SYSTEM_SNAPSHOT_METRICS_STACK_SIZE;
    config->task_priority = SYSTEM_SNAPSHOT_METRICS_TASK_PRIORITY;
    config->core_id = tskNO_AFFINITY;
}

static void system_snapshot_metrics_task(void *arg)
{
    const system_snapshot_metrics_task_config_t *config = (const system_snapshot_metrics_task_config_t *)arg;
    const TickType_t delay_ticks = pdMS_TO_TICKS(config->interval_ms);

    while (1) {
        uint32_t uptime_ms = (uint32_t)(esp_timer_get_time() / 1000);
        uint32_t free_heap = esp_get_free_heap_size();
        int8_t rssi = SYSTEM_SNAPSHOT_WIFI_RSSI_INVALID;
        bool connected = false;

        wifi_ap_record_t ap_info;
        if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
            rssi = (int8_t)ap_info.rssi;
            connected = true;
        }

        system_snapshot_update_metrics(uptime_ms, free_heap, rssi, connected);
        vTaskDelay(delay_ticks);
    }
}

bool system_snapshot_start_metrics_task(const system_snapshot_metrics_task_config_t *config)
{
    if (config == NULL) {
        return false;
    }

    if (s_metrics_task_handle != NULL) {
        return true;
    }

    s_metrics_task_config = *config;

    BaseType_t ret;
    if (s_metrics_task_config.core_id == tskNO_AFFINITY) {
        ret = xTaskCreate(system_snapshot_metrics_task,
                          "snapshot_metrics",
                          s_metrics_task_config.stack_size,
                          (void *)&s_metrics_task_config,
                          s_metrics_task_config.task_priority,
                          &s_metrics_task_handle);
    } else {
        ret = xTaskCreatePinnedToCore(system_snapshot_metrics_task,
                                      "snapshot_metrics",
                                      s_metrics_task_config.stack_size,
                                      (void *)&s_metrics_task_config,
                                      s_metrics_task_config.task_priority,
                                      &s_metrics_task_handle,
                                      s_metrics_task_config.core_id);
    }

    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to start metrics task");
        s_metrics_task_handle = NULL;
        return false;
    }

    ESP_LOGI(TAG, "Metrics task started");
    return true;
}
