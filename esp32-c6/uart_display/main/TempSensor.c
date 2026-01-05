#include "TempSensor.h"

#include "esp_err.h"
#include "esp_log.h"
#include "driver/temperature_sensor.h"

static const char *TAG = "TEMP_SENSOR";
static temperature_sensor_handle_t temp_handle;
static bool temp_inited = false;

bool TempSensor_Init(void)
{
    if (temp_inited) {
        return true;
    }

    temperature_sensor_config_t cfg = TEMPERATURE_SENSOR_CONFIG_DEFAULT(-10, 80);
    esp_err_t err = temperature_sensor_install(&cfg, &temp_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "temperature_sensor_install failed: %s", esp_err_to_name(err));
        return false;
    }

    err = temperature_sensor_enable(temp_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "temperature_sensor_enable failed: %s", esp_err_to_name(err));
        return false;
    }

    temp_inited = true;
    return true;
}

bool TempSensor_ReadCelsius(float *outTempC)
{
    if (outTempC == NULL) {
        return false;
    }

    if (!TempSensor_Init()) {
        return false;
    }

    esp_err_t err = temperature_sensor_get_celsius(temp_handle, outTempC);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "temperature_sensor_get_celsius failed: %s", esp_err_to_name(err));
        return false;
    }

    return true;
}
