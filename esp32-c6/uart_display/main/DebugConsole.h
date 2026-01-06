#pragma once

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lvgl.h"

#define DEBUG_MSG_MAX_LEN 64
#define DEBUG_QUEUE_LENGTH 20

bool DebugConsole_Init(void);
void Debug_Log(const char *fmt, ...);

#define DEBUG_LOG(fmt, ...) Debug_Log(fmt, ##__VA_ARGS__)
