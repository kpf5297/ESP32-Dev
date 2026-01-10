# ESP32-C6 UART Display - Architecture & Component APIs

A modular ESP32 firmware for 320×172 LCD display with WiFi connectivity, SD card storage, real-time clock, temperature sensing, and REST API endpoints.

## System Architecture

### Component Dependency Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                     main.c (Application)                     │
└──────────────┬──────────────────────────────────────────────┘
               │
       ┌───────┴────────┬──────────────┬──────────┬────────┐
       │                │              │          │        │
    ┌──▼───┐        ┌───▼─┐        ┌──▼──┐   ┌──▼───┐   │
    │ WiFi │        │ RTC │        │ Temp│   │ RGB  │   │
    │Manager        │Clock│        │Sens │   │  LED │   │
    └──────┘        └──┬──┘        └──┬──┘   └──┬───┘   │
          │            │              │        │        │
          │         ┌──▼──────────────▼────────▼────┐   │
          │         │  Debug Console (display logs)  │   │
          │         └──────────────────────────────┘   │
          │                                            │
    ┌─────▼───────────────────────────────┬────────┐   │
    │         System Snapshot              │        │   │
    │  (RTC, Temp, WiFi, Heap, Uptime)    │        │   │
    └─────┬───────────────────────────────┘        │   │
          │                                        │   │
    ┌─────▼──────────────┐            ┌───────────▼──┐
    │   REST API         │            │   LVGL UI    │
    │  (HTTP Endpoints)  │            │  (LCD Driver)│
    └────────────────────┘            └──────────────┘

    ┌──────────────┐    ┌───────────────┐    ┌─────────┐
    │  SD Storage  │    │  App Version  │    │Wireless │
    └──────────────┘    └───────────────┘    └─────────┘
```

### Task Model

| Component | Task Name | Priority | Stack | Interval | Purpose |
|-----------|-----------|----------|-------|----------|---------|
| RTC Clock | rtc_task, date_task, temp_task | 2 | 2048 | 1s, 1s, 500ms | Update system snapshot |
| WiFi Manager | wifi_signal_task | 1 | 4096 | 5s | Monitor RSSI strength |
| REST API | httpd (ESP-IDF) | 2 | 4096 | Event-driven | Handle HTTP requests |
| Debug Console | debugConsoleTask | 2 | 4096 | Queue-based | Display log lines on LCD |
| System Snapshot | metrics_task | 1 | 3072 | 2s | Refresh heap/uptime |

## Component APIs

### 1. WiFi Manager (`wifi_manager`)

Handles WiFi connectivity, credential management, and connection status.

**Public API:**
```c
// Initialize WiFi subsystem
void WiFi_Init(void);

// Connect to WiFi with credentials
esp_err_t WiFi_Connect(const char *ssid, const char *password);

// Check if connected
bool WiFi_IsConnected(void);

// Get RSSI signal strength
int8_t WiFi_GetRSSI(void);

// Start background tasks
void wifi_task_start(void);
void wifi_signal_task_start(void);
```

**Usage Example:**
```c
WiFi_Init();
WiFi_Connect("MySSID", "MyPassword");
if (WiFi_IsConnected()) {
    int rssi = WiFi_GetRSSI();
}
wifi_task_start();
```

---

### 2. SD Storage (`sd_storage`)

Manages SD card mounting and file I/O operations.

**Public API:**
```c
// Initialize and mount SD card
bool SD_Mount(void);

// Read file from SD card
bool SD_ReadFile(const char *path, uint8_t *buffer, size_t max_len, size_t *len_out);

// Delete file from SD card
bool SD_DeleteFile(const char *path);

// Unmount SD card
void SD_Unmount(void);
```

**Usage Example:**
```c
if (SD_Mount()) {
    uint8_t buffer[512];
    size_t len;
    if (SD_ReadFile("/WIFI.TXT", buffer, sizeof(buffer), &len)) {
        printf("Read %d bytes\n", len);
    }
}
```

---

### 3. RTC Clock (`rtc_clock`)

Real-time clock with periodic time/date/temperature updates to system snapshot.

**Public API:**
```c
// Initialize RTC
void RTC_Clock_Init(void);

// Get current time (HH:MM:SS)
void RTC_Clock_GetTime(char *buf, size_t len);

// Get current date (YYYY-MM-DD)
void RTC_Clock_GetDate(char *buf, size_t len);

// Initialize background update tasks
void rtc_task_init(void);
void start_rtc_task(void);
void start_date_task(void);
void start_temp_task(void);

// Update UI labels with current time
void rtc_ui_clock_update(void);
```

**Queues Exported:**
```c
extern QueueHandle_t rtcQueue;   // Time updates (1 Hz)
extern QueueHandle_t dateQueue;  // Date updates (0.25 Hz)
extern QueueHandle_t tempQueue;  // Temp updates (2 Hz)
```

---

### 4. Temperature Sensor (`temp_sensor`)

Reads internal ESP32 temperature sensor.

**Public API:**
```c
// Initialize temperature sensor
void TempSensor_Init(void);

// Read temperature in Celsius
float TempSensor_ReadCelsius(void);
```

**Usage Example:**
```c
TempSensor_Init();
float temp = TempSensor_ReadCelsius();
printf("Temperature: %.2f°C\n", temp);
```

---

### 5. RGB LED (`rgb_led`)

Control addressable RGB LED with color animations.

**Public API:**
```c
// Initialize RMT device and LED strip
void RGB_Init(void);

// Set RGB color (0-255 each)
void Set_RGB(uint8_t r, uint8_t g, uint8_t b);

// Start color animation task
void RGB_Example(void);
```

**Features:**
- 192-color smooth gradient palette
- RMT driver (10 MHz)
- Single LED on GPIO 8
- Automatic animation with 20ms updates

---

### 6. App Version (`app_version`)

Version information and splash screen management.

**Public API:**
```c
// Get version strings
const char *AppVersion_GetName(void);      // "UART-SPP Display"
const char *AppVersion_GetVersion(void);   // "v1.0.0"
const char *AppVersion_GetBuildDate(void); // __DATE__
const char *AppVersion_GetBuildTime(void); // __TIME__

// Populate splash screen with version info
void AppVersion_PopulateSplashScreen(void);
```

---

### 7. Wireless (`wireless`)

WiFi network scanning and selection wrapper.

**Public API:**
```c
// Initialize wireless subsystem
void Wireless_Init(void);

// Scan for available networks
uint16_t WIFI_Scan(void);

// Get number of found networks
extern uint16_t WIFI_NUM;
extern uint16_t BLE_NUM;
extern bool Scan_finish;
```

---

### 8. Debug Console (`debug_console`)

Display-based logging system with UART backend.

**Public API:**
```c
// Initialize debug console
bool DebugConsole_Init(void);

// Write line to debug console (with timestamp)
void DebugConsole_WriteLine(const char *line);

// Deprecated: Use app_log_write() instead
void Debug_Log(const char *fmt, ...);

// Get logging backend (for app_log integration)
const app_log_backend_t *app_log_backend_debug_console(void);
const app_log_backend_t *app_log_backend_composite(void);
```

**Features:**
- Queue-based line buffering (20 entries)
- Automatic HH:MM:SS timestamp prefixing
- Displays 7 lines on LVGL debug screen
- UART echo via ESP_LOGI

**Usage Example:**
```c
DebugConsole_Init();
DebugConsole_WriteLine("System initialized");
Debug_Log("Heap: %d bytes", esp_get_free_heap_size());
```

---

## REST API

The REST API exposes system metrics via JSON endpoints on port 80 (configurable).

### Endpoints

#### 1. `/api/v1/time` - Get System Time

**Request:**
```bash
curl http://<device-ip>/api/v1/time
```

**Response (200 OK):**
```json
{
  "iso8601": "2024-09-11T08:12:00Z",
  "epoch": 1726000000
}
```

---

#### 2. `/api/v1/temperature` - Get Temperature Reading

**Request:**
```bash
curl http://<device-ip>/api/v1/temperature
```

**Response (200 OK):**
```json
{
  "celsius": 21.25
}
```

---

#### 3. `/api/v1/status` - Get System Status

**Request:**
```bash
curl http://<device-ip>/api/v1/status
```

**Response (200 OK):**
```json
{
  "uptime_ms": 123456,
  "free_heap_bytes": 654321,
  "wifi_connected": true,
  "wifi_rssi_dbm": -45
}
```

---

## Testing the API

### Prerequisites

1. Device must be connected to WiFi
2. REST API must be started in `main.c`:
   ```c
   rest_api_config_t config;
   rest_api_get_default_config(&config);
   rest_api_start(&config);
   ```

3. Find device IP address (from UART console output or debug screen)

### Test Commands (macOS/Linux)

```bash
# Get device IP from console or scan network
DEVICE_IP="192.168.1.100"

# Test time endpoint
curl -v http://$DEVICE_IP/api/v1/time

# Test temperature endpoint
curl -v http://$DEVICE_IP/api/v1/temperature

# Test status endpoint with pretty JSON
curl -s http://$DEVICE_IP/api/v1/status | json_pp

# Monitor all endpoints every 2 seconds
watch -n 2 'curl -s http://'$DEVICE_IP'/api/v1/status | json_pp'
```

### Test with Python

```python
import requests
import json
import time

DEVICE_IP = "192.168.1.100"
BASE_URL = f"http://{DEVICE_IP}"

def test_api():
    # Test time endpoint
    resp = requests.get(f"{BASE_URL}/api/v1/time", timeout=2)
    print("Time:", resp.json())
    
    # Test temperature endpoint
    resp = requests.get(f"{BASE_URL}/api/v1/temperature", timeout=2)
    print("Temperature:", resp.json())
    
    # Test status endpoint
    resp = requests.get(f"{BASE_URL}/api/v1/status", timeout=2)
    status = resp.json()
    print(f"Status: {status['uptime_ms']}ms uptime, "
          f"{status['free_heap_bytes']} bytes free, "
          f"WiFi: {status['wifi_connected']} (RSSI: {status['wifi_rssi_dbm']} dBm)")

# Test every 5 seconds
while True:
    try:
        test_api()
    except Exception as e:
        print(f"Error: {e}")
    time.sleep(5)
```

### Test with curl (All Endpoints)

```bash
#!/bin/bash
DEVICE_IP="192.168.1.100"

echo "=== ESP32 REST API Test ==="
echo ""

echo "1. Getting time..."
curl -s -w "\nStatus: %{http_code}\n" http://$DEVICE_IP/api/v1/time | python3 -m json.tool
echo ""

echo "2. Getting temperature..."
curl -s -w "\nStatus: %{http_code}\n" http://$DEVICE_IP/api/v1/temperature | python3 -m json.tool
echo ""

echo "3. Getting status..."
curl -s -w "\nStatus: %{http_code}\n" http://$DEVICE_IP/api/v1/status | python3 -m json.tool
```

---

## Data Flow

```
Sensors & Timers
    │
    ├─ RTC Task ──────┐
    ├─ Temp Sensor ───┤
    ├─ WiFi RSSI ─────┤
    └─ Heap Metrics ──┤
                      │
                      ▼
            ┌─────────────────────┐
            │ System Snapshot     │
            │ (Shared Memory)     │
            └──────────┬──────────┘
                       │
        ┌──────────────┼──────────────┐
        │              │              │
        ▼              ▼              ▼
    ┌────────┐  ┌──────────┐   ┌──────────┐
    │REST API│  │Debug     │   │Network   │
    │HTTP    │  │Console   │   │Status    │
    │Server  │  │Display   │   │Cache     │
    └────────┘  └──────────┘   └──────────┘
        │              │              │
        └──────────────┼──────────────┘
                       │
                       ▼
                ┌──────────────┐
                │LVGL UI       │
                │(LCD Display) │
                └──────────────┘
```

---

## Integration Checklist

- [x] WiFi Manager initialized before REST API
- [x] System Snapshot initialized before metric producers
- [x] RTC Clock updates snapshot with time/temperature
- [x] Temperature Sensor wired into RTC tasks
- [x] Debug Console initialized for logging output
- [x] REST API started after WiFi connection
- [x] RGB LED animates on startup
- [x] App Version displayed on splash screen

---

## Component Status

| Component | Status | Lines | Files | Dependencies |
|-----------|--------|-------|-------|--------------|
| wifi_manager | ✅ Complete | 450 | 5 | esp_wifi, esp_netif, lvgl, app_log |
| sd_storage | ✅ Complete | 280 | 3 | fatfs, driver, spi_flash |
| rtc_clock | ✅ Complete | 350 | 4 | freertos, lvgl, temp_sensor |
| temp_sensor | ✅ Complete | 80 | 2 | driver |
| rgb_led | ✅ Complete | 220 | 2 | driver, led_strip, freertos |
| app_version | ✅ Complete | 70 | 2 | lvgl |
| wireless | ✅ Complete | 70 | 2 | esp_wifi, esp_netif, nvs_flash |
| debug_console | ✅ Complete | 180 | 3 | freertos, lvgl, app_log |
| network_debug | ⏳ Pending | - | - | - |
| lcd_driver | ⏳ Pending | - | - | esp_lcd, driver |
| lvgl_ui | ⏳ Pending | - | - | lvgl, all others |

---

## Build & Flash

```bash
# Clean build
idf.py fullclean

# Build project
idf.py build

# Flash to device
idf.py -p /dev/tty.usbserial-XXXXX flash

# Monitor serial output
idf.py -p /dev/tty.usbserial-XXXXX monitor

# Combined build-flash-monitor
idf.py -p /dev/tty.usbserial-XXXXX reconfigure build flash monitor
```

---

## Troubleshooting

### REST API Returns 500 Error
- Verify `system_snapshot_init()` was called
- Check that system_snapshot metrics task is running
- Confirm WiFi is connected before testing

### Debug Console Not Showing
- Verify `DebugConsole_Init()` called before app_log
- Check LVGL UI labels exist (`ui_DebugLineLabel*`)
- Confirm debug screen is active on LCD

### Temperature Always 0°C
- Verify `TempSensor_Init()` called in RTC task
- Check internal temperature sensor is enabled in Kconfig
- Confirm RTC clock component is initialized

---

## Next Steps

1. Create `network_debug` component (move network_debug_task)
2. Create `lcd_driver` component (move LCD_Driver/ directory)
3. Create `lvgl_ui` component (consolidate all UI files)
4. Update main.c to remove all device-specific initialization
5. Add comprehensive unit tests for each component
