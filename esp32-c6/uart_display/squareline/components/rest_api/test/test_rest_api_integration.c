#include "rest_api.h"

#include "esp_http_client.h"
#include "esp_netif.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "esp_heap_caps.h"
#include "system_snapshot.h"
#include "unity.h"

#include "lwip/ip4_addr.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

static void fill_snapshot(system_snapshot_t *snapshot)
{
    memset(snapshot, 0, sizeof(*snapshot));
    snapshot->epoch_seconds = 1726000000;
    (void)strlcpy(snapshot->iso8601, "2024-09-11T08:12:00Z", sizeof(snapshot->iso8601));
    snapshot->temperature_c = 26.75f;
    snapshot->uptime_ms = 4321U;
    snapshot->free_heap_bytes = 12345U;
    snapshot->wifi_connected = true;
    snapshot->wifi_rssi_dbm = -40;
}

TEST_CASE("rest_api serialization no heap growth", "[rest_api]")
{
    system_snapshot_t snapshot;
    char buffer[REST_API_STATUS_JSON_LEN];

    fill_snapshot(&snapshot);

    size_t heap_before = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    for (int i = 0; i < 1000; ++i) {
        size_t len = rest_api_build_status_json(&snapshot, buffer, sizeof(buffer));
        TEST_ASSERT_NOT_EQUAL(0U, len);
    }
    size_t heap_after = heap_caps_get_free_size(MALLOC_CAP_8BIT);

    TEST_ASSERT_EQUAL_UINT32((uint32_t)heap_before, (uint32_t)heap_after);
}

TEST_CASE("rest_api serialization deterministic timing", "[rest_api]")
{
    system_snapshot_t snapshot;
    char buffer[REST_API_TIME_JSON_LEN];
    int64_t min_us = INT64_MAX;
    int64_t max_us = 0;

    fill_snapshot(&snapshot);

    for (int i = 0; i < 200; ++i) {
        int64_t start = esp_timer_get_time();
        size_t len = rest_api_build_time_json(&snapshot, buffer, sizeof(buffer));
        int64_t end = esp_timer_get_time();

        TEST_ASSERT_NOT_EQUAL(0U, len);
        int64_t delta = end - start;
        if (delta < min_us) {
            min_us = delta;
        }
        if (delta > max_us) {
            max_us = delta;
        }
    }

    TEST_ASSERT_TRUE((max_us - min_us) <= 200);
}

static bool get_sta_ip(char *out_ip, size_t len)
{
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (netif == NULL) {
        return false;
    }

    esp_netif_ip_info_t ip_info;
    if (esp_netif_get_ip_info(netif, &ip_info) != ESP_OK) {
        return false;
    }

    (void)snprintf(out_ip, len, IPSTR, IP2STR(&ip_info.ip));
    return true;
}

static void prime_snapshot_for_http(void)
{
    system_snapshot_init();
    system_snapshot_update_time(1726000000, "2024-09-11T08:12:00Z");
    system_snapshot_update_temperature(26.75f);
    system_snapshot_update_metrics(4321U, 12345U, -40, true);
}

TEST_CASE("rest_api http endpoints", "[rest_api][integration]")
{
    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) != ESP_OK) {
        TEST_IGNORE_MESSAGE("Wi-Fi STA not connected");
    }

    rest_api_config_t config;
    rest_api_get_default_config(&config);
    config.server_port = 8080U;

    prime_snapshot_for_http();

    TEST_ASSERT_TRUE(rest_api_start(&config));

    char ip_str[16];
    if (!get_sta_ip(ip_str, sizeof(ip_str))) {
        rest_api_stop();
        TEST_IGNORE_MESSAGE("No STA IP available");
    }

    char url[96];
    (void)snprintf(url, sizeof(url), "http://%s:8080/api/v1/time", ip_str);

    esp_http_client_config_t client_config = {
        .url = url,
        .timeout_ms = 2000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&client_config);
    TEST_ASSERT_NOT_NULL(client);

    esp_err_t err = esp_http_client_perform(client);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    TEST_ASSERT_EQUAL(200, esp_http_client_get_status_code(client));

    esp_http_client_cleanup(client);
    rest_api_stop();
}
