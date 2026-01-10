#include "app_log.h"

#include <stddef.h>

/* Minimal-dependency UART backend.
 * Uses ROM printf routed to the default console UART.
 */
#include "esp_rom_sys.h"

static bool uart_init(void *ctx)
{
    (void)ctx;
    return true;
}

static void uart_deinit(void *ctx)
{
    (void)ctx;
}

static void uart_write_line(void *ctx, const char *line)
{
    (void)ctx;
    if (!line) {
        return;
    }
    esp_rom_printf("%s\n", line);
}

const app_log_backend_t *app_log_backend_uart(void)
{
    static const app_log_backend_t backend = {
        .init = uart_init,
        .deinit = uart_deinit,
        .write_line = uart_write_line,
        .ctx = NULL,
    };

    return &backend;
}
