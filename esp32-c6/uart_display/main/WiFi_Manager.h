#pragma once

#include <stdbool.h>

#ifndef WIFI_DEFAULT_SSID
#define WIFI_DEFAULT_SSID "YOUR_DEFAULT_SSID"
#endif

#ifndef WIFI_DEFAULT_PASS
#define WIFI_DEFAULT_PASS "YOUR_DEFAULT_PASS"
#endif

bool WiFi_Init(void);
bool WiFi_Connect(const char *ssid, const char *pass);
bool WiFi_IsConnected(void);
