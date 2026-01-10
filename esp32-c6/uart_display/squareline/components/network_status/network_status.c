/*
 * network_status.c
 *
 * This module is intentionally decoupled from UI and networking consumers to preserve
 * deterministic real-time behavior on ESP32-C6.
 */

#include "network_status.h"

#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "lwip/ip4_addr.h"

#include <stdio.h>
#include <string.h>

static const char *TAG = "NETWORK_STATUS";

#define NETWORK_STATUS_QUEUE_LEN 4U
#define NETWORK_STATUS_TASK_STACK_SIZE 3072U
#define NETWORK_STATUS_TASK_PRIORITY (tskIDLE_PRIORITY)

typedef enum {
    NETWORK_STATUS_EVENT_GOT_IP = 0,
    NETWORK_STATUS_EVENT_LOST_IP
} network_status_event_t;

typedef struct {
    network_status_event_t type;
    esp_ip4_addr_t ip;
} network_status_msg_t;

typedef struct {
    char ipv4[NETWORK_STATUS_IPV4_STR_LEN];
    bool connected;
} network_status_cache_t;

static bool s_inited = false;
static QueueHandle_t s_event_queue = NULL;
static TaskHandle_t s_task_handle = NULL;
static portMUX_TYPE s_cache_mux = portMUX_INITIALIZER_UNLOCKED;
static network_status_cache_t s_cache;
static network_status_task_config_t s_task_config;

static void network_status_copy_empty(char *dest, size_t len)
{
    if (len > 0U) {
        dest[0] = '\0';
    }
}

static void network_status_update_cache(const char *ipv4, bool connected)
{
    portENTER_CRITICAL(&s_cache_mux);
    s_cache.connected = connected;
    if (ipv4 != NULL) {
        (void)strlcpy(s_cache.ipv4, ipv4, sizeof(s_cache.ipv4));
    } else {
        network_status_copy_empty(s_cache.ipv4, sizeof(s_cache.ipv4));
    }
    portEXIT_CRITICAL(&s_cache_mux);
}

static void network_status_event_handler(void *arg,
                                         esp_event_base_t event_base,
                                         int32_t event_id,
                                         void *event_data)
{
    (void)arg;

    if (s_event_queue == NULL) {
        return;
    }

    network_status_msg_t msg;
    bool send = false;

    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        const ip_event_got_ip_t *event = (const ip_event_got_ip_t *)event_data;
        msg.type = NETWORK_STATUS_EVENT_GOT_IP;
        msg.ip = event->ip_info.ip;
        send = true;
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_LOST_IP) {
        msg.type = NETWORK_STATUS_EVENT_LOST_IP;
        memset(&msg.ip, 0, sizeof(msg.ip));
        send = true;
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        msg.type = NETWORK_STATUS_EVENT_LOST_IP;
        memset(&msg.ip, 0, sizeof(msg.ip));
        send = true;
    }

    if (send) {
        (void)xQueueSend(s_event_queue, &msg, 0);
    }
}

static void network_status_task(void *arg)
{
    (void)arg;

    while (1) {
        network_status_msg_t msg;
        if (xQueueReceive(s_event_queue, &msg, portMAX_DELAY) == pdTRUE) {
            if (msg.type == NETWORK_STATUS_EVENT_GOT_IP) {
                char ip_buf[NETWORK_STATUS_IPV4_STR_LEN];
                (void)snprintf(ip_buf, sizeof(ip_buf), IPSTR, IP2STR(&msg.ip));
                network_status_update_cache(ip_buf, true);
            } else {
                network_status_update_cache(NULL, false);
            }
        }
    }
}

bool network_status_init(void)
{
    if (s_inited) {
        return true;
    }

    s_event_queue = xQueueCreate(NETWORK_STATUS_QUEUE_LEN, sizeof(network_status_msg_t));
    if (s_event_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create event queue");
        return false;
    }

    network_status_update_cache(NULL, false);

    esp_err_t err = esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                               network_status_event_handler, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register GOT_IP handler: %s", esp_err_to_name(err));
        return false;
    }

    err = esp_event_handler_register(IP_EVENT, IP_EVENT_STA_LOST_IP,
                                     network_status_event_handler, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register LOST_IP handler: %s", esp_err_to_name(err));
        return false;
    }

    err = esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED,
                                     network_status_event_handler, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register WIFI disconnect handler: %s", esp_err_to_name(err));
        return false;
    }

    s_inited = true;
    return true;
}

void network_status_task_get_default_config(network_status_task_config_t *config)
{
    if (config == NULL) {
        return;
    }

    /* Low priority keeps LVGL and real-time tasks deterministic. */
    config->stack_size = NETWORK_STATUS_TASK_STACK_SIZE;
    config->task_priority = NETWORK_STATUS_TASK_PRIORITY;
    config->core_id = tskNO_AFFINITY;
}

bool network_status_start_task(const network_status_task_config_t *config)
{
    if (!s_inited || config == NULL) {
        return false;
    }

    if (s_task_handle != NULL) {
        return true;
    }

    s_task_config = *config;

    BaseType_t ret;
    if (s_task_config.core_id == tskNO_AFFINITY) {
        ret = xTaskCreate(network_status_task,
                          "network_status",
                          s_task_config.stack_size,
                          NULL,
                          s_task_config.task_priority,
                          &s_task_handle);
    } else {
        ret = xTaskCreatePinnedToCore(network_status_task,
                                      "network_status",
                                      s_task_config.stack_size,
                                      NULL,
                                      s_task_config.task_priority,
                                      &s_task_handle,
                                      s_task_config.core_id);
    }

    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to start network status task");
        s_task_handle = NULL;
        return false;
    }

    return true;
}

bool network_status_get_ipv4(char *buffer, size_t buffer_len)
{
    if (buffer == NULL || buffer_len == 0U) {
        return false;
    }

    if (buffer_len < NETWORK_STATUS_IPV4_STR_LEN) {
        network_status_copy_empty(buffer, buffer_len);
        return false;
    }

    bool connected;

    portENTER_CRITICAL(&s_cache_mux);
    connected = s_cache.connected;
    (void)strlcpy(buffer, s_cache.ipv4, buffer_len);
    portEXIT_CRITICAL(&s_cache_mux);

    return connected;
}

bool network_status_is_connected(void)
{
    bool connected;

    portENTER_CRITICAL(&s_cache_mux);
    connected = s_cache.connected;
    portEXIT_CRITICAL(&s_cache_mux);

    return connected;
}
