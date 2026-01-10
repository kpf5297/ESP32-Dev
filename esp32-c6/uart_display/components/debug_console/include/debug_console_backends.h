#pragma once

#include "app_log.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Backend that routes lines to DebugConsole (LVGL debug screen + ESP_LOGI). */
const app_log_backend_t *app_log_backend_debug_console(void);

/* Composite backend that routes log lines to both UART and DebugConsole. */
const app_log_backend_t *app_log_backend_composite(void);

#ifdef __cplusplus
}
#endif
