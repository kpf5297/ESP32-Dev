idf_component_register(
                         SRCS "main.c" 
                              "LCD_Driver/Vernon_ST7789T/Vernon_ST7789T.c" 
                              "LCD_Driver/ST7789.c"
                              "LVGL_Driver/LVGL_Driver.c"

                         INCLUDE_DIRS 
                              "./LCD_Driver/Vernon_ST7789T" 
                              "./LCD_Driver"
                              "./LVGL_Driver"
                              "."
                         REQUIRES esp_wifi esp_lcd fatfs spi_flash nvs_flash rest_api system_snapshot app_log wifi_manager sd_storage rtc_clock temp_sensor rgb_led app_version wireless debug_console network_debug lvgl_ui
                       )
