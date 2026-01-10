#pragma once

#include <stdbool.h>

// Application version information
#define APP_NAME        "UART-SPP Display"
#define APP_VERSION     "v1.0.0"
#define APP_BUILD_DATE  __DATE__
#define APP_BUILD_TIME  __TIME__

// Function to get version strings
const char *AppVersion_GetName(void);
const char *AppVersion_GetVersion(void);
const char *AppVersion_GetBuildDate(void);
const char *AppVersion_GetBuildTime(void);

// Function to populate splash screen labels
void AppVersion_PopulateSplashScreen(void);
