#include "wifi_signal_task.h"

#include <string.h>

#include "WiFi_Manager.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "wifi_task.h"

#define WIFI_SIGNAL_TASK_STACK_SIZE 4096
#define WIFI_SIGNAL_TASK_PRIORITY 1

static volatile int s_pending_strength = 0;

extern lv_obj_t *ui_BarSignal __attribute__((weak));

static uint8_t wifi_rssi_to_percent(int rssi)
{
    if (rssi <= -90) {
        return 0;
    }
    if (rssi >= -30) {
        return 100;
    }
    return (uint8_t)(((rssi + 90) * 100) / 60);
}

static void wifi_signal_bar_update_cb(void *arg)
{
    (void)arg;
    if (ui_BarSignal) {
        lv_bar_set_value(ui_BarSignal, s_pending_strength, LV_ANIM_OFF);
    }
}

static void wifi_signal_task(void *arg)
{
    (void)arg;

    int last_strength = -1;

    while (1) {
        int strength = 0;
        if (WiFi_IsConnected()) {
            wifi_ap_record_t ap_info;
            if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
                strength = wifi_rssi_to_percent(ap_info.rssi);
            }
        }

        if (strength != last_strength) {
            last_strength = strength;
            s_pending_strength = strength;
            if (ui_BarSignal) {
                lv_async_call(wifi_signal_bar_update_cb, NULL);
            }

            if (wifiStatusQueue) {
                wifi_status_msg_t msg;
                memset(&msg, 0, sizeof(msg));
                strlcpy(msg.text, "WIFI:RSSI_UPDATE", sizeof(msg.text));
                xQueueSend(wifiStatusQueue, &msg, 0);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void wifi_signal_task_start(void)
{
    xTaskCreate(wifi_signal_task, "wifiSignalTask", WIFI_SIGNAL_TASK_STACK_SIZE, NULL, WIFI_SIGNAL_TASK_PRIORITY, NULL);
}
