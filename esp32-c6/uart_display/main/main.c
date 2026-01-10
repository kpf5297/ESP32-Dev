/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "ST7789.h"
#include "sd_storage.h"
#include "rgb_led.h"
#include "lvgl_ui.h"
#include "wifi_manager.h"
#include "wifi_signal_task.h"
#include "wifi_task.h"
#include "rtc_clock.h"
#include "rtc_task.h"
#include "temp_sensor.h"
#include "network_debug_task.h"
#include "app_version.h"
#include "wireless.h"
// #include "network_status.h"  // Not available in this ESP-IDF version
#include "rest_api.h"
#include "lvgl.h"
#include "debug_console.h"
#include "system_snapshot.h"
#include "app_log.h"
#include "debug_console_backends.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static void rtc_ui_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    rtc_ui_clock_update();
}

void app_main(void)
{
    // Initialize DebugConsole early (for display-based output)
    DebugConsole_Init();

    // Initialize logging with composite backend (both display and UART)
    (void)app_log_init(app_log_backend_composite());
    app_log_set_output_level(APP_LOG_SUBSYS_ALL, APP_LOG_LEVEL_DEBUG);
    app_log_write(APP_LOG_SUBSYS_SYSTEM, APP_LOG_LEVEL_INFO, "Boot started");

    Flash_Searching();
    RGB_Init();
    RGB_Example();
    
    // Initialize SD card BEFORE LCD to set up shared SPI2 bus first
    // SD needs smaller max_transfer_sz which works for both devices
    SD_Mount();
    
    LCD_Init();
    BK_Light(50);
    LVGL_Init();                            // returns the screen object

    ui_init();

    /* app_log now routes all messages to both DebugConsole display and UART */

    AppVersion_PopulateSplashScreen();

    // Map UI labels from ScreenClock to RTC task
    extern lv_obj_t *ui_Label1;
    extern lv_obj_t *ui_Label2;
    extern lv_obj_t *ui_Label3;
    uic_Label6 = ui_Label1;    // Time
    uic_LabelDate = ui_Label2; // Date
    uic_LabelTemp = ui_Label3; // Temperature
    
    WiFi_Init();
    // (void)network_status_init();  // Not available
    // network_status_task_config_t net_status_config;
    // network_status_task_get_default_config(&net_status_config);
    // (void)network_status_start_task(&net_status_config);
    wifi_task_start();
    // wifi_signal_task_start();  // Commented out - causes crash when accessing WiFi AP info
    wifi_ui_timer_init();

    RTC_Clock_Init();
    TempSensor_Init();
    system_snapshot_init();
    system_snapshot_metrics_task_config_t metrics_config;
    system_snapshot_metrics_task_get_default_config(&metrics_config);
    (void)system_snapshot_start_metrics_task(&metrics_config);

    rest_api_config_t rest_config;
    rest_api_get_default_config(&rest_config);
    (void)rest_api_start(&rest_config);

    // network_debug_task_config_t debug_config;  // Uses network_status
    // network_debug_task_get_default_config(&debug_config);
    // (void)network_debug_task_start(&debug_config);

    if (rtc_task_init()) {
        start_rtc_task();
        start_date_task();
        start_temp_task();
    }

    lv_timer_create(rtc_ui_timer_cb, 1000, NULL);

/********************* Demo *********************/
    // Lvgl_Example1();

    // lv_demo_widgets();
    // lv_demo_keypad_encoder();
    // lv_demo_benchmark();
    // lv_demo_stress();
    // lv_demo_music();

    while (1) {
        // raise the task priority of LVGL and/or reduce the handler period can improve the performance
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(10));
        // The task running lv_timer_handler should have lower priority than that running `lv_tick_inc`
    }
}
