# ESP32-C6 UART Display - REST API Extension

## Overview
This project exposes a minimal REST API over HTTP for time, temperature, and system status. The API reads from a cached snapshot owned by the `system_snapshot` component. No HTTP handler accesses hardware or blocks real-time tasks.

## Task model
- `system_snapshot` metrics task
  - Priority: `tskIDLE_PRIORITY + 1`
  - Stack: 3072 bytes
  - Interval: 2000 ms
  - Responsibility: refresh uptime, heap, and Wi-Fi RSSI into the snapshot
- HTTP server task (ESP-IDF `esp_http_server`)
  - Priority: `tskIDLE_PRIORITY + 1` (configurable)
  - Stack: 4096 bytes (configurable)
  - Responsibility: handle REST requests and serialize snapshot data
- `network_status` task
  - Priority: `tskIDLE_PRIORITY`
  - Stack: 3072 bytes
  - Trigger: Wi-Fi/IP events only
  - Responsibility: cache IPv4 and connection state
- LVGL debug update task
  - Priority: `tskIDLE_PRIORITY`
  - Stack: 3072 bytes
  - Interval: 1000 ms
  - Responsibility: update debug screen labels using cached network status

## Data flow
- Producers (RTC task, temperature task, metrics task) call `system_snapshot_update_*()`
- Consumers (REST handlers) call `system_snapshot_read()`
- REST handlers serialize snapshot data into fixed-size JSON buffers
- Wi-Fi/IP events update `network_status` cache
- LVGL debug update task reads cached IP and updates labels

## Real-time guarantees
- No HTTP code executes in RTC, sensor, LVGL, or ISR contexts
- No dynamic allocation in REST handlers
- Snapshot access uses short critical sections only
- REST handlers never touch hardware
- LVGL debug updates read cached data only and avoid Wi-Fi calls

## REST endpoints
- `GET /api/v1/time` -> `{ "iso8601": "...", "epoch": ... }`
- `GET /api/v1/temperature` -> `{ "celsius": ... }`
- `GET /api/v1/status` -> `{ "uptime_ms": ..., "free_heap_bytes": ..., "wifi_connected": true|false, "wifi_rssi_dbm": ... }`

## Integration notes
1. Ensure Wi-Fi is initialized before `rest_api_start()`.
2. Call `system_snapshot_init()` before producer tasks start.
3. Update snapshot in producer tasks (RTC and temperature already wired in `main/rtc_task.c`).
4. Start the metrics task via `system_snapshot_start_metrics_task()`.
5. Initialize `network_status` after Wi-Fi init and start both `network_status` and LVGL debug update tasks.
6. Debug screen uses `ui_DebugLineLabel5` and `ui_DebugLineLabel6` for IP and status.

## Testing
Unit tests and integration tests use the ESP-IDF Unity framework:
- `components/system_snapshot/test/test_system_snapshot.c`
- `components/rest_api/test/test_rest_api.c`
- `components/rest_api/test/test_rest_api_integration.c`
- `components/network_status/test/test_network_status.c`
- `main/test/test_network_debug_task.c`

Run tests with:
```
idf.py -T all test
```
