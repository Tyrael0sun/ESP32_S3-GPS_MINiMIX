/**
 * @file ui_manager.cpp
 * @brief UI manager implementation (placeholder for LVGL integration)
 */

#include "ui_manager.h"
#include "ui_statusbar.h"
#include "ui_bike_computer.h"
#include "ui_gps_logger.h"
#include "ui_pbox.h"
#include "ui_gnss_info.h"
#include "ui_settings.h"
#include "../hardware/display_driver.h"
#include "esp_log.h"

static const char* TAG = "UI_MGR";

static AppMode current_mode = MODE_BIKE_COMPUTER;

bool ui_manager_init(void) {
    // Initialize display (includes LVGL initialization)
    if (!display_init()) {
        ESP_LOGE(TAG, "Failed to initialize display");
        return false;
    }
    
    // Initialize status bar
    ui_statusbar_init();
    
    // Initialize default mode (bike computer)
    ui_bike_computer_init();
    
    ESP_LOGI(TAG, "UI manager initialized with LVGL");
    return true;
}

void ui_manager_switch_mode(AppMode mode) {
    if (mode == current_mode) return;
    
    ESP_LOGI(TAG, "Switching from mode %d to mode %d", current_mode, mode);
    
    // Deinit current mode (but keep status bar)
    switch (current_mode) {
        case MODE_BIKE_COMPUTER:
            ui_bike_computer_deinit();
            break;
        case MODE_GPS_LOGGER:
            ui_gps_logger_deinit();
            break;
        case MODE_PBOX:
            ui_pbox_deinit();
            break;
        case MODE_GNSS_INFO:
            ui_gnss_info_deinit();
            break;
        case MODE_SETTINGS:
            ui_settings_deinit();
            break;
    }
    
    // Update current mode
    current_mode = mode;
    
    // Re-initialize status bar (will auto-cleanup if exists)
    ui_statusbar_init();
    
    // Init new mode
    switch (mode) {
        case MODE_BIKE_COMPUTER:
            ui_bike_computer_init();
            break;
        case MODE_GPS_LOGGER:
            ui_gps_logger_init();
            break;
        case MODE_PBOX:
            ui_pbox_init();
            break;
        case MODE_GNSS_INFO:
            ui_gnss_info_init();
            break;
        case MODE_SETTINGS:
            ui_settings_init();
            break;
    }
    
    ESP_LOGI(TAG, "Mode switch to %d complete", mode);
}

AppMode ui_manager_get_mode(void) {
    return current_mode;
}

void ui_manager_update(void) {
    // Update status bar
    ui_statusbar_update();
    
    // Update current mode UI
    switch (current_mode) {
        case MODE_BIKE_COMPUTER:
            ui_bike_computer_update();
            break;
        case MODE_GPS_LOGGER:
            ui_gps_logger_update();
            break;
        case MODE_PBOX:
            ui_pbox_update();
            break;
        case MODE_GNSS_INFO:
            ui_gnss_info_update();
            break;
        case MODE_SETTINGS:
            ui_settings_update();
            break;
    }
    
    // Call LVGL timer handler
    display_lvgl_handler();
}

void ui_manager_show_time_sync(void) {
    // TODO: Show "Time Synced" message for 2 seconds
    ESP_LOGI(TAG, "Time Synced");
}
