/*
 * rest_routes.c
 *
 * HTTP route registration and JSON serialization.
 */

#include "rest_api.h"

#include "esp_http_server.h"
#include "system_snapshot.h"

#include <inttypes.h>
#include <stdio.h>

static esp_err_t rest_send_json(httpd_req_t *req, const char *payload, size_t len)
{
    esp_err_t err = httpd_resp_set_type(req, "application/json");
    if (err != ESP_OK) {
        return err;
    }

    return httpd_resp_send(req, payload, len);
}

size_t rest_api_build_time_json(const system_snapshot_t *snapshot,
                                char *out_buf,
                                size_t out_len)
{
    if (snapshot == NULL || out_buf == NULL || out_len == 0U) {
        return 0U;
    }

    int len = snprintf(out_buf,
                       out_len,
                       "{\"iso8601\":\"%s\",\"epoch\":%" PRIi64 "}",
                       snapshot->iso8601,
                       snapshot->epoch_seconds);

    if (len < 0 || (size_t)len >= out_len) {
        return 0U;
    }

    return (size_t)len;
}

size_t rest_api_build_temperature_json(const system_snapshot_t *snapshot,
                                       char *out_buf,
                                       size_t out_len)
{
    if (snapshot == NULL || out_buf == NULL || out_len == 0U) {
        return 0U;
    }

    int len = snprintf(out_buf,
                       out_len,
                       "{\"celsius\":%.2f}",
                       (double)snapshot->temperature_c);

    if (len < 0 || (size_t)len >= out_len) {
        return 0U;
    }

    return (size_t)len;
}

size_t rest_api_build_status_json(const system_snapshot_t *snapshot,
                                  char *out_buf,
                                  size_t out_len)
{
    if (snapshot == NULL || out_buf == NULL || out_len == 0U) {
        return 0U;
    }

    const char *wifi_connected = snapshot->wifi_connected ? "true" : "false";

    int len = snprintf(out_buf,
                       out_len,
                       "{\"uptime_ms\":%" PRIu32
                       ",\"free_heap_bytes\":%" PRIu32
                       ",\"wifi_connected\":%s,\"wifi_rssi_dbm\":%d}",
                       snapshot->uptime_ms,
                       snapshot->free_heap_bytes,
                       wifi_connected,
                       (int)snapshot->wifi_rssi_dbm);

    if (len < 0 || (size_t)len >= out_len) {
        return 0U;
    }

    return (size_t)len;
}

static esp_err_t rest_time_get_handler(httpd_req_t *req)
{
    system_snapshot_t snapshot;
    char buffer[REST_API_TIME_JSON_LEN];

    system_snapshot_read(&snapshot);
    size_t len = rest_api_build_time_json(&snapshot, buffer, sizeof(buffer));
    if (len == 0U) {
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "snapshot_error");
    }

    return rest_send_json(req, buffer, len);
}

static esp_err_t rest_temperature_get_handler(httpd_req_t *req)
{
    system_snapshot_t snapshot;
    char buffer[REST_API_TEMP_JSON_LEN];

    system_snapshot_read(&snapshot);
    size_t len = rest_api_build_temperature_json(&snapshot, buffer, sizeof(buffer));
    if (len == 0U) {
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "snapshot_error");
    }

    return rest_send_json(req, buffer, len);
}

static esp_err_t rest_status_get_handler(httpd_req_t *req)
{
    system_snapshot_t snapshot;
    char buffer[REST_API_STATUS_JSON_LEN];

    system_snapshot_read(&snapshot);
    size_t len = rest_api_build_status_json(&snapshot, buffer, sizeof(buffer));
    if (len == 0U) {
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "snapshot_error");
    }

    return rest_send_json(req, buffer, len);
}

bool rest_routes_register(httpd_handle_t server)
{
    if (server == NULL) {
        return false;
    }

    const httpd_uri_t time_uri = {
        .uri = "/api/v1/time",
        .method = HTTP_GET,
        .handler = rest_time_get_handler,
        .user_ctx = NULL
    };

    const httpd_uri_t temperature_uri = {
        .uri = "/api/v1/temperature",
        .method = HTTP_GET,
        .handler = rest_temperature_get_handler,
        .user_ctx = NULL
    };

    const httpd_uri_t status_uri = {
        .uri = "/api/v1/status",
        .method = HTTP_GET,
        .handler = rest_status_get_handler,
        .user_ctx = NULL
    };

    if (httpd_register_uri_handler(server, &time_uri) != ESP_OK) {
        return false;
    }

    if (httpd_register_uri_handler(server, &temperature_uri) != ESP_OK) {
        return false;
    }

    if (httpd_register_uri_handler(server, &status_uri) != ESP_OK) {
        return false;
    }

    return true;
}
