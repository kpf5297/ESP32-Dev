idf_component_register(SRCS "main.c"
                    PRIV_REQUIRES bt nvs_flash
                    INCLUDE_DIRS "."
                    REQUIRES     driver        # <- pulls in driver/uart.h and UART HAL
                    nvs_flash     # you call nvs_flash_*()
                    bt            # classic-BT stack (esp_spp_*, esp_bt_gap_*)
                    )
