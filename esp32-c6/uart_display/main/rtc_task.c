#include "rtc_task.h"

#include "RTC_Clock.h"
#include "TempSensor.h"
#include "esp_log.h"
#include "freertos/task.h"
#include <stdio.h>

static const char *TAG = "RTC_TASK";

#define RTC_QUEUE_LENGTH 10
#define RTC_QUEUE_ITEM_SIZE 16
#define DATE_QUEUE_ITEM_SIZE 16
#define TEMP_QUEUE_ITEM_SIZE 16
#define RTC_TASK_STACK_SIZE 4096
#define DATE_TASK_STACK_SIZE 4096
#define TEMP_TASK_STACK_SIZE 4096
#define RTC_TASK_PRIORITY (tskIDLE_PRIORITY + 1)
#define DATE_TASK_PRIORITY (tskIDLE_PRIORITY + 1)
#define TEMP_TASK_PRIORITY (tskIDLE_PRIORITY + 1)

QueueHandle_t rtcQueue = NULL;
QueueHandle_t dateQueue = NULL;
QueueHandle_t tempQueue = NULL;
lv_obj_t *uic_Label6 = NULL;
lv_obj_t *uic_LabelDate = NULL;
lv_obj_t *uic_LabelTemp = NULL;

static void rtcClockTask(void *arg)
{
    char tbuf[RTC_QUEUE_ITEM_SIZE];

    while (1) {
        RTC_Clock_GetTime(tbuf, sizeof(tbuf));
        (void)xQueueSend(rtcQueue, tbuf, 0);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void dateTask(void *arg)
{
    char dbuf[DATE_QUEUE_ITEM_SIZE];

    while (1) {
        RTC_Clock_GetDate(dbuf, sizeof(dbuf));
        (void)xQueueSend(dateQueue, dbuf, 0);
        vTaskDelay(pdMS_TO_TICKS(24UL * 60 * 60 * 1000));
    }
}

static void tempTask(void *arg)
{
    char tbuf[TEMP_QUEUE_ITEM_SIZE];
    float temp_c = 0.0f;

    while (1) {
        if (TempSensor_ReadCelsius(&temp_c)) {
            (void)snprintf(tbuf, sizeof(tbuf), "%.1f C", (double)temp_c);
            (void)xQueueSend(tempQueue, tbuf, 0);
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

bool rtc_task_init(void)
{
    ESP_LOGI(TAG, "Creating RTC update queue");
    rtcQueue = xQueueCreate(RTC_QUEUE_LENGTH, RTC_QUEUE_ITEM_SIZE);
    if (rtcQueue == NULL) {
        return false;
    }

    dateQueue = xQueueCreate(RTC_QUEUE_LENGTH, DATE_QUEUE_ITEM_SIZE);
    if (dateQueue == NULL) {
        vQueueDelete(rtcQueue);
        rtcQueue = NULL;
        return false;
    }

    tempQueue = xQueueCreate(RTC_QUEUE_LENGTH, TEMP_QUEUE_ITEM_SIZE);
    if (tempQueue == NULL) {
        vQueueDelete(rtcQueue);
        rtcQueue = NULL;
        vQueueDelete(dateQueue);
        dateQueue = NULL;
        return false;
    }

    return true;
}

void start_rtc_task(void)
{
    ESP_LOGI(TAG, "Starting RTC clock task");
    xTaskCreate(rtcClockTask, "rtcClockTask", RTC_TASK_STACK_SIZE, NULL, RTC_TASK_PRIORITY, NULL);
}

void start_date_task(void)
{
    ESP_LOGI(TAG, "Starting RTC date task");
    xTaskCreate(dateTask, "dateTask", DATE_TASK_STACK_SIZE, NULL, DATE_TASK_PRIORITY, NULL);
}

void start_temp_task(void)
{
    ESP_LOGI(TAG, "Starting temperature task");
    xTaskCreate(tempTask, "tempTask", TEMP_TASK_STACK_SIZE, NULL, TEMP_TASK_PRIORITY, NULL);
}

void rtc_ui_clock_update(void)
{
    char clockStr[RTC_QUEUE_ITEM_SIZE];
    char dateStr[DATE_QUEUE_ITEM_SIZE];
    char tempStr[TEMP_QUEUE_ITEM_SIZE];

    if (uic_Label6 != NULL && rtcQueue != NULL) {
        if (xQueueReceive(rtcQueue, clockStr, 0) == pdTRUE) {
            lv_label_set_text(uic_Label6, clockStr);
        }
    }

    if (uic_LabelDate != NULL && dateQueue != NULL) {
        if (xQueueReceive(dateQueue, dateStr, 0) == pdTRUE) {
            lv_label_set_text(uic_LabelDate, dateStr);
        }
    }

    if (uic_LabelTemp != NULL && tempQueue != NULL) {
        if (xQueueReceive(tempQueue, tempStr, 0) == pdTRUE) {
            lv_label_set_text(uic_LabelTemp, tempStr);
        }
    }
}
