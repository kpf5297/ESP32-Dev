#include "rest_api.h"

#include "unity.h"

#include <string.h>

static void build_sample_snapshot(system_snapshot_t *snapshot)
{
    memset(snapshot, 0, sizeof(*snapshot));
    snapshot->epoch_seconds = 1726000000;
    (void)strlcpy(snapshot->iso8601, "2024-09-11T08:12:00Z", sizeof(snapshot->iso8601));
    snapshot->temperature_c = 21.25f;
    snapshot->uptime_ms = 123456U;
    snapshot->free_heap_bytes = 654321U;
    snapshot->wifi_connected = true;
    snapshot->wifi_rssi_dbm = -45;
}

TEST_CASE("rest_api time json", "[rest_api]")
{
    system_snapshot_t snapshot;
    char buffer[REST_API_TIME_JSON_LEN];

    build_sample_snapshot(&snapshot);
    size_t len = rest_api_build_time_json(&snapshot, buffer, sizeof(buffer));

    TEST_ASSERT_NOT_EQUAL(0U, len);
    TEST_ASSERT_EQUAL_STRING("{\"iso8601\":\"2024-09-11T08:12:00Z\",\"epoch\":1726000000}", buffer);
}

TEST_CASE("rest_api temperature json", "[rest_api]")
{
    system_snapshot_t snapshot;
    char buffer[REST_API_TEMP_JSON_LEN];

    build_sample_snapshot(&snapshot);
    size_t len = rest_api_build_temperature_json(&snapshot, buffer, sizeof(buffer));

    TEST_ASSERT_NOT_EQUAL(0U, len);
    TEST_ASSERT_EQUAL_STRING("{\"celsius\":21.25}", buffer);
}

TEST_CASE("rest_api status json", "[rest_api]")
{
    system_snapshot_t snapshot;
    char buffer[REST_API_STATUS_JSON_LEN];

    build_sample_snapshot(&snapshot);
    size_t len = rest_api_build_status_json(&snapshot, buffer, sizeof(buffer));

    TEST_ASSERT_NOT_EQUAL(0U, len);
    TEST_ASSERT_EQUAL_STRING("{\"uptime_ms\":123456,\"free_heap_bytes\":654321,\"wifi_connected\":true,\"wifi_rssi_dbm\":-45}", buffer);
}
