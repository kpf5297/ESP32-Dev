#pragma once

/*
 * app_log.h
 *
 * Lightweight, reusable logging interface.
 * - Subsystem-specific filtering
 * - Priority/verbosity levels
 * - Global enable/disable
 * - Pluggable backends (UART, LED, network, etc.)
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef APP_LOG_MAX_LINE_LEN
#define APP_LOG_MAX_LINE_LEN 160
#endif

typedef enum {
    APP_LOG_LEVEL_NONE = 0,
    APP_LOG_LEVEL_CRITICAL = 1,
    APP_LOG_LEVEL_ERROR = 2,
    APP_LOG_LEVEL_WARN = 3,
    APP_LOG_LEVEL_INFO = 4,
    APP_LOG_LEVEL_DEBUG = 5,
} app_log_level_t;

typedef enum {
    APP_LOG_SUBSYS_ALL = -1,

    APP_LOG_SUBSYS_SYSTEM = 0,
    APP_LOG_SUBSYS_WIFI,
    APP_LOG_SUBSYS_DISPLAY,
    APP_LOG_SUBSYS_UI,
    APP_LOG_SUBSYS_STORAGE,
    APP_LOG_SUBSYS_SENSOR,
    APP_LOG_SUBSYS_RTC,
    APP_LOG_SUBSYS_NETWORK,
    APP_LOG_SUBSYS_REST,

    APP_LOG_SUBSYS_COUNT,
} app_log_subsystem_t;

typedef struct {
    bool (*init)(void *ctx);
    void (*deinit)(void *ctx);
    void (*write_line)(void *ctx, const char *line);
    void *ctx;
} app_log_backend_t;

bool app_log_init(const app_log_backend_t *backend);
void app_log_deinit(void);

void app_log_global_on(void);
void app_log_global_off(void);
bool app_log_is_enabled(void);

void app_log_set_output_level(app_log_subsystem_t subsystem, app_log_level_t level);
app_log_level_t app_log_get_output_level(app_log_subsystem_t subsystem);

void app_log_write(app_log_subsystem_t subsystem, app_log_level_t level, const char *msg);
void app_log_write_num(app_log_subsystem_t subsystem, app_log_level_t level, const char *msg, int32_t number);
void app_log_write_fmt(app_log_subsystem_t subsystem, app_log_level_t level, const char *fmt, ...);

const char *app_log_level_to_str(app_log_level_t level);
const char *app_log_subsystem_to_str(app_log_subsystem_t subsystem);

/* Built-in backend: UART/console via ROM printf (lowest dependency default). */
const app_log_backend_t *app_log_backend_uart(void);

#ifdef __cplusplus
}
#endif
