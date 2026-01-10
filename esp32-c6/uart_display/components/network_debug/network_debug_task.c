/*
 * network_debug_task.c
 *
 * This module is intentionally decoupled from UI and networking consumers to preserve
 * deterministic real-time behavior on ESP32-C6.
 */

#include "network_debug_task.h"

// #include "network_status.h"  // Not available
#include "ui.h"
#include "UI/screens/ui_ScreenDebug.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"

#include <stdio.h>
#include <string.h>

#define NETWORK_DEBUG_TASK_STACK_SIZE 3072U
#define NETWORK_DEBUG_TASK_PRIORITY (tskIDLE_PRIORITY)
#define NETWORK_DEBUG_TASK_INTERVAL_MS 1000U

static TaskHandle_t s_debug_task_handle = NULL;
static network_debug_task_config_t s_debug_task_config;

static char s_ip_line[32];
static char s_state_line[24];

static void network_debug_task(void *arg)
{
    const network_debug_task_config_t *config = (const network_debug_task_config_t *)arg;
    const TickType_t delay_ticks = pdMS_TO_TICKS(config->interval_ms);

    while (1) {
        // char ip_buf[NETWORK_STATUS_IPV4_STR_LEN];  // Not available
        // bool connected = network_status_get_ipv4(ip_buf, sizeof(ip_buf));  // Not available

        // Set default values since network_status is not available
        // if (connected && ip_buf[0] != '\0') {
        //     (void)snprintf(s_ip_line, sizeof(s_ip_line), "IP: %s", ip_buf);
        //     (void)strlcpy(s_state_line, "WiFi: Connected", sizeof(s_state_line));
        // } else {
            (void)strlcpy(s_ip_line, "IP: --", sizeof(s_ip_line));
            (void)strlcpy(s_state_line, "WiFi: Disconnected", sizeof(s_state_line));
        // }

        if (ui_DebugLineLabel5 != NULL) {
            lv_label_set_text_static(ui_DebugLineLabel5, s_ip_line);
        }
        if (ui_DebugLineLabel6 != NULL) {
            lv_label_set_text_static(ui_DebugLineLabel6, s_state_line);
        }

        vTaskDelay(delay_ticks);
    }
}

void network_debug_task_get_default_config(network_debug_task_config_t *config)
{
    if (config == NULL) {
        return;
    }

    /* Low priority avoids interfering with the LVGL core loop. */
    config->interval_ms = NETWORK_DEBUG_TASK_INTERVAL_MS;
    config->stack_size = NETWORK_DEBUG_TASK_STACK_SIZE;
    config->task_priority = NETWORK_DEBUG_TASK_PRIORITY;
    config->core_id = tskNO_AFFINITY;
}

bool network_debug_task_start(const network_debug_task_config_t *config)
{
    if (config == NULL) {
        return false;
    }

    if (s_debug_task_handle != NULL) {
        return true;
    }

    s_debug_task_config = *config;

    BaseType_t ret;
    if (s_debug_task_config.core_id == tskNO_AFFINITY) {
        ret = xTaskCreate(network_debug_task,
                          "network_debug",
                          s_debug_task_config.stack_size,
                          (void *)&s_debug_task_config,
                          s_debug_task_config.task_priority,
                          &s_debug_task_handle);
    } else {
        ret = xTaskCreatePinnedToCore(network_debug_task,
                                      "network_debug",
                                      s_debug_task_config.stack_size,
                                      (void *)&s_debug_task_config,
                                      s_debug_task_config.task_priority,
                                      &s_debug_task_handle,
                                      s_debug_task_config.core_id);
    }

    if (ret != pdPASS) {
        s_debug_task_handle = NULL;
        return false;
    }

    return true;
}
