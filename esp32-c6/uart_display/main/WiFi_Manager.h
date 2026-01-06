#pragma once

#include <stdbool.h>

#ifndef WIFI_DEFAULT_SSID
#define WIFI_DEFAULT_SSID "TP-LINK_7CE8"
#endif

#ifndef WIFI_DEFAULT_PASS
#define WIFI_DEFAULT_PASS "@butterflynet11"
#endif

bool WiFi_Init(void);
bool WiFi_Connect(const char *ssid, const char *pass);
bool WiFi_IsConnected(void);
