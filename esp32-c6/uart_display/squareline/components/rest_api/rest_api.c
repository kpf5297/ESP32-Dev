/*
 * rest_api.c
 *
 * HTTP REST API for exposing cached system snapshot data.
 */

#include "rest_api.h"

#include "esp_http_server.h"
#include "esp_log.h"
#include "rest_routes.h"

static const char *TAG = "REST_API";

#define REST_API_DEFAULT_PORT 80U
#define REST_API_DEFAULT_STACK_SIZE 4096U
#define REST_API_DEFAULT_TASK_PRIORITY (tskIDLE_PRIORITY + 1)

static httpd_handle_t s_server = NULL;

void rest_api_get_default_config(rest_api_config_t *config)
{
    if (config == NULL) {
        return;
    }

    config->server_port = REST_API_DEFAULT_PORT;
    config->stack_size = REST_API_DEFAULT_STACK_SIZE;
    config->task_priority = REST_API_DEFAULT_TASK_PRIORITY;
    config->core_id = tskNO_AFFINITY;
}

bool rest_api_start(const rest_api_config_t *config)
{
    if (config == NULL) {
        return false;
    }

    if (s_server != NULL) {
        return true;
    }

    httpd_config_t server_config = HTTPD_DEFAULT_CONFIG();
    server_config.server_port = config->server_port;
    server_config.stack_size = config->stack_size;
    server_config.task_priority = config->task_priority;
    server_config.core_id = config->core_id;

    esp_err_t err = httpd_start(&s_server, &server_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "httpd_start failed: %s", esp_err_to_name(err));
        s_server = NULL;
        return false;
    }

    if (!rest_routes_register(s_server)) {
        httpd_stop(s_server);
        s_server = NULL;
        return false;
    }

    ESP_LOGI(TAG, "REST API started on port %u", (unsigned)config->server_port);
    return true;
}

void rest_api_stop(void)
{
    if (s_server == NULL) {
        return;
    }

    httpd_stop(s_server);
    s_server = NULL;
    ESP_LOGI(TAG, "REST API stopped");
}
