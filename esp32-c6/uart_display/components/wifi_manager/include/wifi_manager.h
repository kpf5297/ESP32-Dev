#pragma once

#include <stdbool.h>

#ifndef WIFI_DEFAULT_SSID
#define WIFI_DEFAULT_SSID "SSID"
#endif

#ifndef WIFI_DEFAULT_PASS
#define WIFI_DEFAULT_PASS "password"
#endif

bool WiFi_Init(void);
bool WiFi_Connect(const char *ssid, const char *pass);
bool WiFi_IsConnected(void);
