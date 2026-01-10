#include "wifi_task.h"

#include <string.h>
#include <arpa/inet.h>

#include "sd_storage.h"
#include "wifi_manager.h"
#include "app_log.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "freertos/task.h"
#include "lvgl.h"

#define WIFI_TASK_STACK_SIZE 4096
#define WIFI_TASK_PRIORITY 2
#define WIFI_STATUS_QUEUE_LEN 8
#define WIFI_CRED_PATH "/sdcard/WIFI.TXT"
#define WIFI_SSID_MAX_LEN 32
#define WIFI_PASS_MAX_LEN 64
#define WIFI_FILE_BUF_LEN 256

static const char *WIFI_TASK_TAG = "wifiTask";

QueueHandle_t wifiStatusQueue = NULL;

static char s_wifi_ssid[WIFI_SSID_MAX_LEN + 1];
static char s_wifi_pass[WIFI_PASS_MAX_LEN + 1];
static char s_wifi_ip[16];

extern lv_obj_t *ui_LabelSSID __attribute__((weak));
extern lv_obj_t *ui_LabelWiFiStatus __attribute__((weak));
extern lv_obj_t *ui_LabelWiFiIP __attribute__((weak));

static void wifi_send_status(const char *status)
{
    if (!wifiStatusQueue || !status) {
        return;
    }

    wifi_status_msg_t msg;
    memset(&msg, 0, sizeof(msg));
    strlcpy(msg.text, status, sizeof(msg.text));
    xQueueSend(wifiStatusQueue, &msg, 0);
}

static void wifi_trim_whitespace(char *str)
{
    if (!str) {
        return;
    }

    char *end = str + strlen(str);
    while (end > str && (*(end - 1) == '\r' || *(end - 1) == '\n' || *(end - 1) == ' ' || *(end - 1) == '\t')) {
        *(end - 1) = '\0';
        end--;
    }

    while (*str == ' ' || *str == '\t') {
        memmove(str, str + 1, strlen(str));
    }
}

static bool wifi_load_from_sd(char *ssid_out, size_t ssid_len, char *pass_out, size_t pass_len)
{
    char file_buf[WIFI_FILE_BUF_LEN];
    if (!SD_ReadFile(WIFI_CRED_PATH, file_buf, sizeof(file_buf))) {
        ESP_LOGE(WIFI_TASK_TAG, "Failed to read WIFI credentials from SD card: %s", WIFI_CRED_PATH);
        app_log_write(APP_LOG_SUBSYS_WIFI, APP_LOG_LEVEL_WARN, "failed to read credentials from SD");
        return false;
    }

    ESP_LOGI(WIFI_TASK_TAG, "Successfully read WIFI file, parsing contents");

    bool has_ssid = false;
    bool has_pass = false;

    char *saveptr = NULL;
    char *line = strtok_r(file_buf, "\n", &saveptr);
    while (line) {
        char *cr = strchr(line, '\r');
        if (cr) {
            *cr = '\0';
        }
        if (strncmp(line, "SSID:", 5) == 0) {
            const char *value = line + 5;
            while (*value == ' ' || *value == '\t') {
                value++;
            }
            strlcpy(ssid_out, value, ssid_len);
            wifi_trim_whitespace(ssid_out);
            has_ssid = ssid_out[0] != '\0';
            if (has_ssid) {
                ESP_LOGI(WIFI_TASK_TAG, "Loaded SSID: %s", ssid_out);
            }
        } else if (strncmp(line, "PASS:", 5) == 0) {
            const char *value = line + 5;
            while (*value == ' ' || *value == '\t') {
                value++;
            }
            strlcpy(pass_out, value, pass_len);
            wifi_trim_whitespace(pass_out);
            has_pass = pass_out[0] != '\0';
            if (has_pass) {
                ESP_LOGI(WIFI_TASK_TAG, "Loaded password (length: %zu)", strlen(pass_out));
            }
        }
        line = strtok_r(NULL, "\n", &saveptr);
    }

    if (has_ssid) {
        if (!has_pass) {
            pass_out[0] = '\0';
        }
        // SD_DeleteFile(WIFI_CRED_PATH);
        ESP_LOGI(WIFI_TASK_TAG, "Loaded Wi-Fi SSID from SD");
        app_log_write(APP_LOG_SUBSYS_WIFI, APP_LOG_LEVEL_INFO, "loaded credentials from SD");
        return true;
    }

    ESP_LOGE(WIFI_TASK_TAG, "No valid SSID found in WIFI.TXT file");
    app_log_write(APP_LOG_SUBSYS_WIFI, APP_LOG_LEVEL_WARN, "no valid SSID in WIFI.TXT");
    return false;
}

static void wifi_get_ip_address(void)
{
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (!netif) {
        memset(s_wifi_ip, 0, sizeof(s_wifi_ip));
        return;
    }

    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(netif, &ip_info);
    
    if (ip_info.ip.addr != 0) {
        snprintf(s_wifi_ip, sizeof(s_wifi_ip), "%d.%d.%d.%d", 
                 IP2STR(&ip_info.ip));
    } else {
        memset(s_wifi_ip, 0, sizeof(s_wifi_ip));
    }
}

static void wifi_ui_timer_cb(lv_timer_t *timer)
{
    (void)timer;

    if (ui_LabelSSID) {
        static char last_ssid[WIFI_SSID_MAX_LEN + 1];
        if (strcmp(last_ssid, s_wifi_ssid) != 0) {
            strlcpy(last_ssid, s_wifi_ssid, sizeof(last_ssid));
            lv_label_set_text(ui_LabelSSID, last_ssid);
        }
    }

    if (!wifiStatusQueue || !ui_LabelWiFiStatus) {
        return;
    }

    wifi_status_msg_t msg;
    while (xQueueReceive(wifiStatusQueue, &msg, 0) == pdTRUE) {
        if (strcmp(msg.text, "WIFI:USING_DEFAULTS") == 0) {
            lv_label_set_text(ui_LabelWiFiStatus, "USING_DEFAULTS");
        } else if (strcmp(msg.text, "WIFI:CONNECTED") == 0) {
            lv_label_set_text(ui_LabelWiFiStatus, "CONNECTED");
            wifi_get_ip_address();
            if (ui_LabelWiFiIP && s_wifi_ip[0] != '\0') {
                lv_label_set_text(ui_LabelWiFiIP, s_wifi_ip);
            }
        } else if (strcmp(msg.text, "WIFI:DISCONNECTED") == 0) {
            lv_label_set_text(ui_LabelWiFiStatus, "DISCONNECTED");
            if (ui_LabelWiFiIP) {
                lv_label_set_text(ui_LabelWiFiIP, "Not Connected");
            }
            memset(s_wifi_ip, 0, sizeof(s_wifi_ip));
        }
    }
}

static void wifi_task(void *arg)
{
    (void)arg;

    memset(s_wifi_ssid, 0, sizeof(s_wifi_ssid));
    memset(s_wifi_pass, 0, sizeof(s_wifi_pass));

    ESP_LOGI(WIFI_TASK_TAG, "WiFi task starting - attempting to load credentials from SD card");

    if (wifi_load_from_sd(s_wifi_ssid, sizeof(s_wifi_ssid), s_wifi_pass, sizeof(s_wifi_pass))) {
        wifi_send_status("WIFI:LOADED_FROM_SD");
        ESP_LOGI(WIFI_TASK_TAG, "Successfully loaded WiFi credentials from SD card");
    } else {
        strlcpy(s_wifi_ssid, WIFI_DEFAULT_SSID, sizeof(s_wifi_ssid));
        strlcpy(s_wifi_pass, WIFI_DEFAULT_PASS, sizeof(s_wifi_pass));
        wifi_send_status("WIFI:USING_DEFAULTS");
        ESP_LOGW(WIFI_TASK_TAG, "Failed to load from SD, using default credentials: %s", WIFI_DEFAULT_SSID);
        app_log_write(APP_LOG_SUBSYS_WIFI, APP_LOG_LEVEL_WARN, "using default Wi-Fi credentials");
    }

    bool last_connected = !WiFi_IsConnected();

    while (1) {
        bool connected = WiFi_IsConnected();
        if (!connected && s_wifi_ssid[0] != '\0') {
            WiFi_Connect(s_wifi_ssid, s_wifi_pass);
        }

        if (connected != last_connected) {
            wifi_send_status(connected ? "WIFI:CONNECTED" : "WIFI:DISCONNECTED");
            if (connected) {
                app_log_write_fmt(APP_LOG_SUBSYS_WIFI, APP_LOG_LEVEL_INFO, "connected to %s", s_wifi_ssid);
            } else {
                app_log_write(APP_LOG_SUBSYS_WIFI, APP_LOG_LEVEL_INFO, "disconnected");
            }
            last_connected = connected;
        }

        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

void wifi_task_start(void)
{
    if (!wifiStatusQueue) {
        wifiStatusQueue = xQueueCreate(WIFI_STATUS_QUEUE_LEN, sizeof(wifi_status_msg_t));
    }

    xTaskCreate(wifi_task, "wifiTask", WIFI_TASK_STACK_SIZE, NULL, WIFI_TASK_PRIORITY, NULL);
}

void wifi_ui_timer_init(void)
{
    lv_timer_create(wifi_ui_timer_cb, 200, NULL);
}

const char *wifi_get_ssid(void)
{
    return s_wifi_ssid;
}
