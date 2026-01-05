/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "ST7789.h"
#include "SD_Storage.h"
#include "RGB.h"
#include "UI/ui.h"
#include "WiFi_Manager.h"
#include "wifi_signal_task.h"
#include "wifi_task.h"
#include "LVGL_Example.h"
#include "ui.h"
#include "RTC_Clock.h"
#include "TempSensor.h"
#include "rtc_task.h"
#include "lvgl.h"

static void rtc_ui_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    rtc_ui_clock_update();
}

void app_main(void)
{
    Flash_Searching();
    RGB_Init();
    RGB_Example();
    LCD_Init();
    BK_Light(50);
    SD_Mount();
    WiFi_Init();
    wifi_task_start();
    wifi_signal_task_start();
    LVGL_Init();                            // returns the screen object

    ui_init();
    wifi_ui_timer_init();
    uic_Label6 = ui_Label1;
    uic_LabelDate = ui_Label2;
    uic_LabelTemp = ui_Label3;

    RTC_Clock_Init();
    TempSensor_Init();
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
        vTaskDelay(pdMS_TO_TICKS(10));
        // The task running lv_timer_handler should have lower priority than that running `lv_tick_inc`
        lv_timer_handler();
    }
}
