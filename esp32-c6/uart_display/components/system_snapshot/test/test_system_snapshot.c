#include "system_snapshot.h"

#include "unity.h"

TEST_CASE("system_snapshot roundtrip", "[system_snapshot]")
{
    system_snapshot_t snapshot;

    system_snapshot_init();
    system_snapshot_update_time(1726000000, "2024-09-11T08:12:00Z");
    system_snapshot_update_temperature(23.5f);
    system_snapshot_update_metrics(1234U, 5678U, -42, true);

    system_snapshot_read(&snapshot);

    TEST_ASSERT_EQUAL_INT64(1726000000, snapshot.epoch_seconds);
    TEST_ASSERT_EQUAL_STRING("2024-09-11T08:12:00Z", snapshot.iso8601);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 23.5f, snapshot.temperature_c);
    TEST_ASSERT_EQUAL_UINT32(1234U, snapshot.uptime_ms);
    TEST_ASSERT_EQUAL_UINT32(5678U, snapshot.free_heap_bytes);
    TEST_ASSERT_EQUAL_INT8(-42, snapshot.wifi_rssi_dbm);
    TEST_ASSERT_TRUE(snapshot.wifi_connected);
}
