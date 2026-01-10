#include "sd_storage.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/unistd.h>

#include "SD_SPI.h"
#include "app_log.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define SD_MOUNT_POINT "/sdcard"

static const char *SD_STORAGE_TAG = "SD_Storage";
static bool s_sd_mounted = false;

bool SD_Mount(void)
{
    if (s_sd_mounted) {
        return true;
    }

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024,
    };

    sdmmc_card_t *card = NULL;
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    // Reduce clock frequency for large SD cards (32GB+)
    host.max_freq_khz = 10000;  // 10 MHz instead of default 20 MHz
    
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        // Increase max_transfer_sz to support shared use with LCD
        // LCD needs 320*172*2 = 110,080 bytes for full screen transfers
        .max_transfer_sz = 320 * 172 * sizeof(uint16_t),
    };

    // Initialize SPI bus (will fail if already initialized, but that's OK)
    esp_err_t ret = spi_bus_initialize(host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(SD_STORAGE_TAG, "SPI bus init failed: %s", esp_err_to_name(ret));
        app_log_write_fmt(APP_LOG_SUBSYS_STORAGE, APP_LOG_LEVEL_ERROR, "SPI bus init failed: %s", esp_err_to_name(ret));
        return false;
    }
    if (ret == ESP_ERR_INVALID_STATE) {
        ESP_LOGW(SD_STORAGE_TAG, "SPI bus already initialized (likely shared with LCD)");
    }

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = host.slot;

    // Retry mounting with delays for large cards
    for (int attempt = 1; attempt <= 3; attempt++) {
        ESP_LOGI(SD_STORAGE_TAG, "SD mount attempt %d of 3...", attempt);
        ret = esp_vfs_fat_sdspi_mount(SD_MOUNT_POINT, &host, &slot_config, &mount_config, &card);
        if (ret == ESP_OK) {
            break;
        }
        ESP_LOGW(SD_STORAGE_TAG, "Mount attempt %d failed: %s", attempt, esp_err_to_name(ret));
        if (attempt < 3) {
            vTaskDelay(pdMS_TO_TICKS(500));  // Wait 500ms before retry
        }
    }

    if (ret != ESP_OK) {
        ESP_LOGE(SD_STORAGE_TAG, "Mount failed after 3 attempts: %s", esp_err_to_name(ret));
        app_log_write_fmt(APP_LOG_SUBSYS_STORAGE, APP_LOG_LEVEL_ERROR, "mount failed: %s", esp_err_to_name(ret));
        return false;
    }

    s_sd_mounted = true;
    ESP_LOGI(SD_STORAGE_TAG, "SD card mounted successfully at %dkHz", host.max_freq_khz);
    app_log_write_fmt(APP_LOG_SUBSYS_STORAGE, APP_LOG_LEVEL_INFO, "mounted successfully at %dkHz", host.max_freq_khz);
    return true;
}

bool SD_ReadFile(const char *path, char *out, size_t len)
{
    if (!path || !out || len == 0) {
        ESP_LOGE(SD_STORAGE_TAG, "SD_ReadFile: invalid parameters");
        return false;
    }

    FILE *f = fopen(path, "r");
    if (!f) {
        ESP_LOGE(SD_STORAGE_TAG, "SD_ReadFile: failed to open file: %s", path);
        return false;
    }

    size_t read_len = fread(out, 1, len - 1, f);
    out[read_len] = '\0';
    fclose(f);
    
    if (read_len == 0) {
        ESP_LOGE(SD_STORAGE_TAG, "SD_ReadFile: no data read from: %s", path);
        return false;
    }
    
    return true;
}

bool SD_DeleteFile(const char *path)
{
    if (!path) {
        return false;
    }
    return unlink(path) == 0;
}
