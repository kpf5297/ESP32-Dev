#include "debug_console.h"
#include "app_log.h"

static bool dbg_init(void *ctx)
{
    (void)ctx;
    return DebugConsole_Init();
}

static void dbg_deinit(void *ctx)
{
    (void)ctx;
}

static void dbg_write_line(void *ctx, const char *line)
{
    (void)ctx;
    DebugConsole_WriteLine(line);
}

const app_log_backend_t *app_log_backend_debug_console(void)
{
    static const app_log_backend_t backend = {
        .init = dbg_init,
        .deinit = dbg_deinit,
        .write_line = dbg_write_line,
        .ctx = NULL,
    };

    return &backend;
}

static bool composite_init(void *ctx)
{
    (void)ctx;
    /* DebugConsole is already initialized in main.c before app_log */
    return true;
}

static void composite_deinit(void *ctx)
{
    (void)ctx;
}

static void composite_write_line(void *ctx, const char *line)
{
    (void)ctx;
    /* Route to both UART (via ROM printf which ESP_LOGI uses) and DebugConsole display */
    DebugConsole_WriteLine(line);
}

const app_log_backend_t *app_log_backend_composite(void)
{
    static const app_log_backend_t backend = {
        .init = composite_init,
        .deinit = composite_deinit,
        .write_line = composite_write_line,
        .ctx = NULL,
    };

    return &backend;
}
