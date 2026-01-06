#include "SD_Storage.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/unistd.h>

#include "SD_SPI.h"
#include "DebugConsole.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

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
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };

    esp_err_t ret = spi_bus_initialize(host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(SD_STORAGE_TAG, "SPI bus init failed: %s", esp_err_to_name(ret));
        Debug_Log("SD: SPI bus init failed: %s", esp_err_to_name(ret));
        return false;
    }

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = host.slot;

    ret = esp_vfs_fat_sdspi_mount(SD_MOUNT_POINT, &host, &slot_config, &mount_config, &card);
    if (ret != ESP_OK) {
        ESP_LOGE(SD_STORAGE_TAG, "Mount failed: %s", esp_err_to_name(ret));
        Debug_Log("SD: mount failed: %s", esp_err_to_name(ret));
        return false;
    }

    s_sd_mounted = true;
    Debug_Log("SD: mounted successfully");
    return true;
}

bool SD_ReadFile(const char *path, char *out, size_t len)
{
    if (!path || !out || len == 0) {
        return false;
    }

    FILE *f = fopen(path, "r");
    if (!f) {
        return false;
    }

    size_t read_len = fread(out, 1, len - 1, f);
    out[read_len] = '\0';
    fclose(f);
    return read_len > 0;
}

bool SD_DeleteFile(const char *path)
{
    if (!path) {
        return false;
    }
    return unlink(path) == 0;
}
