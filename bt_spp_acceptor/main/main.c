/*
 * SPDX-FileCopyrightText: 2021-2024 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <inttypes.h>
#include "nvs.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_bt_device.h"
#include "esp_spp_api.h"
#include "driver/uart.h"
#include "sys/time.h"
#include "driver/gpio.h"     


/* ---------- UART CONFIG ---------- */
#define UART_PORT      UART_NUM_1            // leave UART0 for logs/flash
#define UART_TX_PIN    GPIO_NUM_17
#define UART_RX_PIN    GPIO_NUM_16
#define UART_BAUD      115200
#define UART_BUF_SIZE  2048                  // RX & TX ring-buffer size

/* ---------- SPP CONFIG ---------- */
#define SPP_TAG            "BT_UART_BRIDGE"
#define SPP_SERVER_NAME    "SPP_SERVER"
#define SPP_SHOW_SPEED     1                 // set 0 to dump data instead

static const char local_device_name[] = CONFIG_EXAMPLE_LOCAL_DEVICE_NAME;
static const esp_spp_mode_t esp_spp_mode           = ESP_SPP_MODE_CB;
static const bool           esp_spp_enable_l2cap_ertm = true;

static const esp_spp_sec_t  sec_mask   = ESP_SPP_SEC_AUTHENTICATE;
static const esp_spp_role_t role_slave = ESP_SPP_ROLE_SLAVE;

static uint32_t spp_con_handle = 0;          // 0 = no active SPP link
static long     data_cnt       = 0;
static struct timeval t_old    = {0};

static char *bda2str(uint8_t *bda, char *str, size_t size)
{
    if (!bda || !str || size < 18) return NULL;
    sprintf(str, "%02x:%02x:%02x:%02x:%02x:%02x",
            bda[0], bda[1], bda[2], bda[3], bda[4], bda[5]);
    return str;
}

/* ---------- UART ➜ BT TASK ---------- */
static void uart_to_bt_task(void *arg)
{
    uint8_t *buf = malloc(256);
    while (1) {
        int rx = uart_read_bytes(UART_PORT, buf, 256, 20 / portTICK_PERIOD_MS);
        if (rx > 0 && spp_con_handle) {
            esp_spp_write(spp_con_handle, rx, buf);
        }
    }
}

/* ---------- SPP CALLBACK ---------- */
static void esp_spp_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
{
    char bda_str[18];

    switch (event) {

    case ESP_SPP_INIT_EVT:
        ESP_LOGI(SPP_TAG, "SPP init (%s)", param->init.status == ESP_SPP_SUCCESS ? "OK" : "FAIL");
        if (param->init.status == ESP_SPP_SUCCESS)
            esp_spp_start_srv(sec_mask, role_slave, 0, SPP_SERVER_NAME);
        break;

    case ESP_SPP_SRV_OPEN_EVT:                              /* client connected */
        spp_con_handle = param->srv_open.handle;
        ESP_LOGI(SPP_TAG, "Client connected handle=%"PRIu32" (%s)",
                 spp_con_handle,
                 bda2str(param->srv_open.rem_bda, bda_str, sizeof bda_str));
        gettimeofday(&t_old, NULL);
        break;

    case ESP_SPP_CLOSE_EVT:
        ESP_LOGI(SPP_TAG, "Client disconnected, handle=%"PRIu32, param->close.handle);
        spp_con_handle = 0;
        break;

    case ESP_SPP_DATA_IND_EVT:                              /* BT ➜ UART */
        if (spp_con_handle == param->data_ind.handle) {
            uart_write_bytes(UART_PORT,
                             (const char *)param->data_ind.data,
                             param->data_ind.len);
            #if SPP_SHOW_SPEED
            struct timeval t_now;
            gettimeofday(&t_now, NULL);
            data_cnt += param->data_ind.len;
            if (t_now.tv_sec - t_old.tv_sec >= 3) {
                float dt = (t_now.tv_sec - t_old.tv_sec) +
                           (t_now.tv_usec - t_old.tv_usec)/1e6f;
                float kbps = data_cnt * 8 / dt / 1000.0f;
                ESP_LOGI(SPP_TAG, "Throughput: %.1f kbit/s", kbps);
                data_cnt = 0;
                t_old = t_now;
            }
            #endif
        }
        break;

    default:
        break;
    }
}

/* ---------- GAP CALLBACK ---------- (unchanged, only minimal logging) */
static void esp_bt_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    if (event == ESP_BT_GAP_AUTH_CMPL_EVT && param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS)
        ESP_LOGI(SPP_TAG, "Paired with %s", param->auth_cmpl.device_name);
}

/* ---------- APP MAIN ---------- */
void app_main(void)
{
    /* NVS init (unchanged) */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    /* UART driver */
    uart_config_t ucfg = {
        .baud_rate  = UART_BAUD,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE
    };
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT, UART_BUF_SIZE, UART_BUF_SIZE, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config (UART_PORT, &ucfg));
    ESP_ERROR_CHECK(uart_set_pin      (UART_PORT, UART_TX_PIN, UART_RX_PIN,
                                                     UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    /* BT stack */
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT));

    esp_bluedroid_config_t bluedroid_cfg = BT_BLUEDROID_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bluedroid_init_with_cfg(&bluedroid_cfg));
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    ESP_ERROR_CHECK(esp_bt_gap_register_callback(esp_bt_gap_cb));
    ESP_ERROR_CHECK(esp_spp_register_callback(esp_spp_cb));

    esp_spp_cfg_t spp_cfg = {
        .mode              = esp_spp_mode,
        .enable_l2cap_ertm = esp_spp_enable_l2cap_ertm,
        .tx_buffer_size    = 0
    };
    ESP_ERROR_CHECK(esp_spp_enhanced_init(&spp_cfg));

    esp_bt_gap_set_device_name(local_device_name);
    esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);

    /* Start the UART ➜ BT bridge once everything else is ready */
    xTaskCreate(uart_to_bt_task, "uart_to_bt", 4096, NULL, 10, NULL);

    ESP_LOGI(SPP_TAG, "Bridge ready – pair and open SPP port.");
}
