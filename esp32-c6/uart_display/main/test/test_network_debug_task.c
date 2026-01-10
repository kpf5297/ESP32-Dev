#include "network_debug_task.h"

#include "network_status.h"
#include "UI/screens/ui_ScreenDebug.h"

#include "esp_event.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "unity.h"

#include "lwip/ip4_addr.h"

#include <string.h>

static void dummy_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p)
{
    (void)drv;
    (void)area;
    (void)color_p;
    lv_disp_flush_ready(drv);
}

static void lvgl_test_init(void)
{
    static bool initialized = false;
    if (initialized) {
        return;
    }

    lv_init();

    static lv_color_t buf[16];
    static lv_disp_draw_buf_t draw_buf;
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, (uint32_t)(sizeof(buf) / sizeof(buf[0])));

    lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = 10;
    disp_drv.ver_res = 10;
    disp_drv.flush_cb = dummy_flush_cb;
    disp_drv.draw_buf = &draw_buf;
    (void)lv_disp_drv_register(&disp_drv);

    initialized = true;
}

static void network_debug_test_init(void)
{
    esp_err_t err = esp_netif_init();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        TEST_FAIL_MESSAGE("esp_netif_init failed");
    }

    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        TEST_FAIL_MESSAGE("esp_event_loop_create_default failed");
    }

    TEST_ASSERT_TRUE(network_status_init());
    network_status_task_config_t status_cfg;
    network_status_task_get_default_config(&status_cfg);
    TEST_ASSERT_TRUE(network_status_start_task(&status_cfg));

    network_debug_task_config_t debug_cfg;
    network_debug_task_get_default_config(&debug_cfg);
    debug_cfg.interval_ms = 50U;
    TEST_ASSERT_TRUE(network_debug_task_start(&debug_cfg));
}

TEST_CASE("network_debug_task updates labels", "[network_debug_task][integration]")
{
    lvgl_test_init();
    network_debug_test_init();

    lv_obj_t *label_ip = lv_label_create(lv_scr_act());
    lv_obj_t *label_state = lv_label_create(lv_scr_act());

    ui_DebugLineLabel5 = label_ip;
    ui_DebugLineLabel6 = label_state;

    ip_event_got_ip_t event;
    memset(&event, 0, sizeof(event));
    IP4_ADDR(&event.ip_info.ip, 192, 168, 1, 50);

    TEST_ASSERT_EQUAL(ESP_OK,
                      esp_event_post(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                     &event, sizeof(event), portMAX_DELAY));

    vTaskDelay(pdMS_TO_TICKS(120));

    TEST_ASSERT_EQUAL_STRING("IP: 192.168.1.50", lv_label_get_text(label_ip));
    TEST_ASSERT_EQUAL_STRING("WiFi: Connected", lv_label_get_text(label_state));

    TEST_ASSERT_EQUAL(ESP_OK,
                      esp_event_post(IP_EVENT, IP_EVENT_STA_LOST_IP,
                                     NULL, 0, portMAX_DELAY));

    vTaskDelay(pdMS_TO_TICKS(120));

    TEST_ASSERT_EQUAL_STRING("IP: --", lv_label_get_text(label_ip));
    TEST_ASSERT_EQUAL_STRING("WiFi: Disconnected", lv_label_get_text(label_state));
}
