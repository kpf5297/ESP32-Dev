#include "RTC_Clock.h"

#include "esp_log.h"
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

static const char *TAG = "RTC_CLOCK";

bool RTC_Clock_Init(void)
{
    ESP_LOGI(TAG, "Initializing hardware RTC");
    setenv("TZ", "UTC0", 1);
    tzset();

    // Set default time to September 11th, 08:12:00
    struct tm default_time = {0};
    time_t now = time(NULL);
    localtime_r(&now, &default_time);
    default_time.tm_mon = 8;  // September (0-based)
    default_time.tm_mday = 11;
    default_time.tm_hour = 8;
    default_time.tm_min = 12;
    default_time.tm_sec = 0;
    time_t default_epoch = mktime(&default_time);

    struct timeval tv = { .tv_sec = default_epoch };
    settimeofday(&tv, NULL);
    return true;
}

void RTC_Clock_GetTime(char *timeStr, size_t len)
{
    time_t now;
    struct tm timeinfo;

    time(&now);
    localtime_r(&now, &timeinfo);
    strftime(timeStr, len, "%H:%M:%S", &timeinfo);
}

void RTC_Clock_GetDate(char *dateStr, size_t len)
{
    time_t now;
    struct tm timeinfo;

    time(&now);
    localtime_r(&now, &timeinfo);
    strftime(dateStr, len, "%Y-%m-%d", &timeinfo);
}
