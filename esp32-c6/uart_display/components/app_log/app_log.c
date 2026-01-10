#include "app_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include <string.h>
#include <stdarg.h>
#include <stdio.h>

static bool g_enabled = true;
static app_log_level_t g_levels[APP_LOG_SUBSYS_COUNT];
static const app_log_backend_t *g_backend = NULL;

static StaticSemaphore_t g_mutex_buf;
static SemaphoreHandle_t g_mutex = NULL;

static inline bool subsys_in_range(app_log_subsystem_t s)
{
    return (s >= 0) && (s < APP_LOG_SUBSYS_COUNT);
}

static inline bool should_log(app_log_subsystem_t subsystem, app_log_level_t level)
{
    if (!g_enabled) {
        return false;
    }

    if (g_backend == NULL || g_backend->write_line == NULL) {
        return false;
    }

    if (level == APP_LOG_LEVEL_NONE) {
        return false;
    }

    if (!subsys_in_range(subsystem)) {
        /* If the caller passes an unknown subsystem, treat as SYSTEM. */
        subsystem = APP_LOG_SUBSYS_SYSTEM;
    }

    const app_log_level_t threshold = g_levels[subsystem];
    return level <= threshold;
}

static size_t append_str(char *dst, size_t cap, size_t at, const char *src)
{
    if (cap == 0 || dst == NULL) {
        return at;
    }

    if (at >= cap) {
        return cap;
    }

    if (src == NULL) {
        src = "";
    }

    while (*src && at + 1 < cap) {
        dst[at++] = *src++;
    }

    dst[at < cap ? at : (cap - 1)] = '\0';
    return at;
}

static size_t append_char(char *dst, size_t cap, size_t at, char c)
{
    if (cap == 0 || dst == NULL || at + 1 >= cap) {
        return at;
    }
    dst[at++] = c;
    dst[at] = '\0';
    return at;
}

static size_t append_i32(char *dst, size_t cap, size_t at, int32_t value)
{
    if (cap == 0 || dst == NULL || at >= cap) {
        return at;
    }

    /* Convert to decimal without printf/snprintf. */
    char tmp[12]; /* -2147483648\0 */
    size_t n = 0;

    uint32_t u = (uint32_t)value;
    if (value < 0) {
        u = (uint32_t)(-(int64_t)value);
    }

    do {
        tmp[n++] = (char)('0' + (u % 10));
        u /= 10;
    } while (u && n < sizeof(tmp) - 1);

    if (value < 0) {
        tmp[n++] = '-';
    }

    while (n && at + 1 < cap) {
        dst[at++] = tmp[--n];
    }

    dst[at < cap ? at : (cap - 1)] = '\0';
    return at;
}

const char *app_log_level_to_str(app_log_level_t level)
{
    switch (level) {
        case APP_LOG_LEVEL_CRITICAL: return "CRIT";
        case APP_LOG_LEVEL_ERROR: return "ERR";
        case APP_LOG_LEVEL_WARN: return "WARN";
        case APP_LOG_LEVEL_INFO: return "INFO";
        case APP_LOG_LEVEL_DEBUG: return "DBG";
        case APP_LOG_LEVEL_NONE:
        default: return "NONE";
    }
}

const char *app_log_subsystem_to_str(app_log_subsystem_t subsystem)
{
    switch (subsystem) {
        case APP_LOG_SUBSYS_SYSTEM: return "SYS";
        case APP_LOG_SUBSYS_WIFI: return "WIFI";
        case APP_LOG_SUBSYS_DISPLAY: return "DSP";
        case APP_LOG_SUBSYS_UI: return "UI";
        case APP_LOG_SUBSYS_STORAGE: return "SD";
        case APP_LOG_SUBSYS_SENSOR: return "SNS";
        case APP_LOG_SUBSYS_RTC: return "RTC";
        case APP_LOG_SUBSYS_NETWORK: return "NET";
        case APP_LOG_SUBSYS_REST: return "REST";
        default: return "UNK";
    }
}

bool app_log_init(const app_log_backend_t *backend)
{
    if (g_mutex == NULL) {
        g_mutex = xSemaphoreCreateMutexStatic(&g_mutex_buf);
        if (g_mutex == NULL) {
            return false;
        }
    }

    /* Set defaults once. */
    static bool defaults_set = false;
    if (!defaults_set) {
        for (int i = 0; i < APP_LOG_SUBSYS_COUNT; ++i) {
            g_levels[i] = APP_LOG_LEVEL_INFO;
        }
        defaults_set = true;
    }

    g_backend = backend;

    if (g_backend && g_backend->init) {
        return g_backend->init(g_backend->ctx);
    }

    return (g_backend && g_backend->write_line);
}

void app_log_deinit(void)
{
    if (g_backend && g_backend->deinit) {
        g_backend->deinit(g_backend->ctx);
    }
    g_backend = NULL;
}

void app_log_global_on(void)
{
    g_enabled = true;
}

void app_log_global_off(void)
{
    g_enabled = false;
}

bool app_log_is_enabled(void)
{
    return g_enabled;
}

void app_log_set_output_level(app_log_subsystem_t subsystem, app_log_level_t level)
{
    if (subsystem == APP_LOG_SUBSYS_ALL) {
        for (int i = 0; i < APP_LOG_SUBSYS_COUNT; ++i) {
            g_levels[i] = level;
        }
        return;
    }

    if (!subsys_in_range(subsystem)) {
        return;
    }

    g_levels[subsystem] = level;
}

app_log_level_t app_log_get_output_level(app_log_subsystem_t subsystem)
{
    if (!subsys_in_range(subsystem)) {
        return APP_LOG_LEVEL_NONE;
    }

    return g_levels[subsystem];
}

static void backend_write(const char *line)
{
    if (g_backend && g_backend->write_line) {
        g_backend->write_line(g_backend->ctx, line);
    }
}

void app_log_write(app_log_subsystem_t subsystem, app_log_level_t level, const char *msg)
{
    if (!should_log(subsystem, level)) {
        return;
    }

    /* Fast serialize only when emitting. */
    if (xSemaphoreTake(g_mutex, pdMS_TO_TICKS(10)) != pdTRUE) {
        return;
    }

    char line[APP_LOG_MAX_LINE_LEN];
    line[0] = '\0';
    size_t at = 0;

    at = append_char(line, sizeof(line), at, '[');
    at = append_str(line, sizeof(line), at, app_log_subsystem_to_str(subsystem));
    at = append_str(line, sizeof(line), at, "][");
    at = append_str(line, sizeof(line), at, app_log_level_to_str(level));
    at = append_str(line, sizeof(line), at, "] ");
    at = append_str(line, sizeof(line), at, msg);

    backend_write(line);

    xSemaphoreGive(g_mutex);
}

void app_log_write_num(app_log_subsystem_t subsystem, app_log_level_t level, const char *msg, int32_t number)
{
    if (!should_log(subsystem, level)) {
        return;
    }

    if (xSemaphoreTake(g_mutex, pdMS_TO_TICKS(10)) != pdTRUE) {
        return;
    }

    char line[APP_LOG_MAX_LINE_LEN];
    line[0] = '\0';
    size_t at = 0;

    at = append_char(line, sizeof(line), at, '[');
    at = append_str(line, sizeof(line), at, app_log_subsystem_to_str(subsystem));
    at = append_str(line, sizeof(line), at, "][");
    at = append_str(line, sizeof(line), at, app_log_level_to_str(level));
    at = append_str(line, sizeof(line), at, "] ");
    at = append_str(line, sizeof(line), at, msg);
    at = append_str(line, sizeof(line), at, ": ");
    at = append_i32(line, sizeof(line), at, number);

    backend_write(line);

    xSemaphoreGive(g_mutex);
}

void app_log_write_fmt(app_log_subsystem_t subsystem, app_log_level_t level, const char *fmt, ...)
{
    if (!should_log(subsystem, level)) {
        return;
    }

    if (xSemaphoreTake(g_mutex, pdMS_TO_TICKS(10)) != pdTRUE) {
        return;
    }

    char msg_buf[APP_LOG_MAX_LINE_LEN / 2];
    va_list args;
    va_start(args, fmt);
    vsnprintf(msg_buf, sizeof(msg_buf), fmt, args);
    va_end(args);

    char line[APP_LOG_MAX_LINE_LEN];
    line[0] = '\0';
    size_t at = 0;

    at = append_char(line, sizeof(line), at, '[');
    at = append_str(line, sizeof(line), at, app_log_subsystem_to_str(subsystem));
    at = append_str(line, sizeof(line), at, "][");
    at = append_str(line, sizeof(line), at, app_log_level_to_str(level));
    at = append_str(line, sizeof(line), at, "] ");
    at = append_str(line, sizeof(line), at, msg_buf);

    backend_write(line);

    xSemaphoreGive(g_mutex);
}
