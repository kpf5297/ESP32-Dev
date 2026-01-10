#include "app_version.h"

#include "esp_log.h"
#include "lvgl.h"

static const char *TAG = "AppVersion";

// Weak declarations for UI labels - these are populated by the UI system
extern lv_obj_t *ui_NameLabel __attribute__((weak));
extern lv_obj_t *ui_VersionLabel __attribute__((weak));

const char *AppVersion_GetName(void)
{
    return APP_NAME;
}

const char *AppVersion_GetVersion(void)
{
    return APP_VERSION;
}

const char *AppVersion_GetBuildDate(void)
{
    return APP_BUILD_DATE;
}

const char *AppVersion_GetBuildTime(void)
{
    return APP_BUILD_TIME;
}

void AppVersion_PopulateSplashScreen(void)
{
    ESP_LOGI(TAG, "Application: %s %s", APP_NAME, APP_VERSION);
    ESP_LOGI(TAG, "Build: %s %s", APP_BUILD_DATE, APP_BUILD_TIME);

    // Update splash screen labels if they exist
    if (ui_NameLabel) {
        lv_label_set_text(ui_NameLabel, APP_NAME);
    }

    if (ui_VersionLabel) {
        lv_label_set_text(ui_VersionLabel, APP_VERSION);
    }
}
