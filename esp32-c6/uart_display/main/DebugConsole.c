#include "DebugConsole.h"

#include <string.h>

extern lv_obj_t *ui_Screen3 __attribute__((weak));
extern lv_obj_t *ui_DebugLineLabel __attribute__((weak));
extern lv_obj_t *ui_DebugLineLabel1 __attribute__((weak));
extern lv_obj_t *ui_DebugLineLabel2 __attribute__((weak));
extern lv_obj_t *ui_DebugLineLabel3 __attribute__((weak));
extern lv_obj_t *ui_DebugLineLabel4 __attribute__((weak));
extern lv_obj_t *ui_DebugLineLabel5 __attribute__((weak));
extern lv_obj_t *ui_DebugLineLabel6 __attribute__((weak));

typedef struct {
    char text[DEBUG_MSG_MAX_LEN];
} debug_message_t;

static StaticQueue_t g_logQueueStruct;
static uint8_t g_logQueueStorage[DEBUG_QUEUE_LENGTH * sizeof(debug_message_t)];
static QueueHandle_t g_logQueue = NULL;
static TaskHandle_t g_debugTaskHandle = NULL;
static const char *TAG = "DEBUG";

static void debugConsoleTask(void *param);

static void debug_update_labels(char lines[7][DEBUG_MSG_MAX_LEN])
{
    lv_obj_t **const labels[] = {
        &ui_DebugLineLabel,
        &ui_DebugLineLabel1,
        &ui_DebugLineLabel2,
        &ui_DebugLineLabel3,
        &ui_DebugLineLabel4,
        &ui_DebugLineLabel5,
        &ui_DebugLineLabel6,
    };

    for (size_t i = 0; i < 7; ++i) {
        if (*labels[i]) {
            lv_label_set_text(*labels[i], lines[i]);
        }
    }
}

bool DebugConsole_Init(void)
{
    if (g_logQueue != NULL) {
        return true;
    }

    g_logQueue = xQueueCreateStatic(DEBUG_QUEUE_LENGTH, sizeof(debug_message_t),
                                    g_logQueueStorage, &g_logQueueStruct);
    if (g_logQueue == NULL) {
        ESP_LOGE(TAG, "Failed to create debug queue");
        return false;
    }

    BaseType_t tcreated = xTaskCreate(
        debugConsoleTask, "debugConsoleTask", 4096, NULL, 2, &g_debugTaskHandle);
    if (tcreated != pdPASS) {
        ESP_LOGE(TAG, "Failed to create debug console task");
        g_debugTaskHandle = NULL;
        return false;
    }

    ESP_LOGI(TAG, "Debug console initialized");
    return true;
}

void Debug_Log(const char *fmt, ...)
{
    debug_message_t msg;
    va_list args;
    va_start(args, fmt);
    vsnprintf(msg.text, sizeof(msg.text), fmt, args);
    va_end(args);
    msg.text[DEBUG_MSG_MAX_LEN - 1] = '\0';

    ESP_LOGI(TAG, "%s", msg.text);

    if (g_logQueue) {
        xQueueSend(g_logQueue, &msg, 0);
    }
}

static void debugConsoleTask(void *param)
{
    (void)param;

    debug_message_t msg;
    char lines[7][DEBUG_MSG_MAX_LEN] = {{0}};

    while (1) {
        if (xQueueReceive(g_logQueue, &msg, portMAX_DELAY) == pdTRUE) {
            do {
                for (int i = 6; i > 0; --i) {
                    strncpy(lines[i], lines[i - 1], DEBUG_MSG_MAX_LEN);
                    lines[i][DEBUG_MSG_MAX_LEN - 1] = '\0';
                }
                strncpy(lines[0], msg.text, DEBUG_MSG_MAX_LEN);
                lines[0][DEBUG_MSG_MAX_LEN - 1] = '\0';

                debug_update_labels(lines);
            } while (xQueueReceive(g_logQueue, &msg, 0) == pdTRUE);

            taskYIELD();
        }
    }
}
