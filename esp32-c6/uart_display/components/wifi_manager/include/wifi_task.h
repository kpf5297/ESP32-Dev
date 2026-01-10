#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#define WIFI_STATUS_MSG_LEN 32

typedef struct {
    char text[WIFI_STATUS_MSG_LEN];
} wifi_status_msg_t;

extern QueueHandle_t wifiStatusQueue;

void wifi_task_start(void);
void wifi_ui_timer_init(void);
const char *wifi_get_ssid(void);
