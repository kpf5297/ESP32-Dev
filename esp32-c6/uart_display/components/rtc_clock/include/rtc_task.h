#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "lvgl.h"
#include <stdbool.h>

extern QueueHandle_t rtcQueue;
extern QueueHandle_t dateQueue;
extern QueueHandle_t tempQueue;
extern lv_obj_t *uic_Label6;
extern lv_obj_t *uic_LabelDate;
extern lv_obj_t *uic_LabelTemp;

bool rtc_task_init(void);
void start_rtc_task(void);
void start_date_task(void);
void start_temp_task(void);
void rtc_ui_clock_update(void);
