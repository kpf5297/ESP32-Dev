#pragma once

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <esp_wifi.h>
#include <nvs_flash.h>
#include <stdio.h>
#include <string.h>
#include <esp_system.h>

extern uint16_t BLE_NUM;
extern uint16_t WIFI_NUM;
extern bool Scan_finish;

void Wireless_Init(void);
void WIFI_Init(void *arg);
uint16_t WIFI_Scan(void);
static inline void BLE_Init(void *arg) { return; }
static inline uint16_t BLE_Scan(void) { return 0; }
