/**
 * @file sdcard_driver.cpp
 * @brief SD card driver implementation using SDIO
 */

#include "sdcard_driver.h"
#include "config.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"

static const char* TAG = "SDCARD";

static sdmmc_card_t* card = NULL;
static bool card_initialized = false;

bool sdcard_init(void) {
    esp_err_t ret;
    
    // Configure SDIO host
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.max_freq_khz = SDMMC_FREQ_HIGHSPEED;
    
    // Configure slot
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.width = 4; // 4-bit mode
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;
    
    // Mount filesystem
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };
    
    ret = esp_vfs_fat_sdmmc_mount(SD_MOUNT_POINT, &host, &slot_config, &mount_config, &card);
    
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem");
        } else {
            ESP_LOGE(TAG, "Failed to initialize card (%s)", esp_err_to_name(ret));
        }
        return false;
    }
    
    // Print card info
    sdmmc_card_print_info(stdout, card);
    ESP_LOGI(TAG, "SD card mounted at %s", SD_MOUNT_POINT);
    
    card_initialized = true;
    return true;
}

bool sdcard_is_present(void) {
    return card_initialized && (card != NULL);
}

uint32_t sdcard_get_size_mb(void) {
    if (!card) return 0;
    uint64_t card_size = ((uint64_t)card->csd.capacity) * card->csd.sector_size / (1024 * 1024);
    return (uint32_t)card_size;
}

uint32_t sdcard_get_free_space_mb(void) {
    if (!card_initialized) return 0;
    
    FATFS* fs;
    DWORD free_clusters;
    
    if (f_getfree("0:", &free_clusters, &fs) == FR_OK) {
        uint64_t free_bytes = (uint64_t)free_clusters * fs->csize * 512;
        return (uint32_t)(free_bytes / (1024 * 1024));
    }
    
    return 0;
}

void sdcard_deinit(void) {
    if (card_initialized) {
        esp_vfs_fat_sdcard_unmount(SD_MOUNT_POINT, card);
        card = NULL;
        card_initialized = false;
        ESP_LOGI(TAG, "SD card unmounted");
    }
}
