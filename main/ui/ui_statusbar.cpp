/**
 * @file ui_statusbar.cpp
 * @brief Status bar implementation
 */

#include "ui_statusbar.h"
#include "ui_common.h"
#include "../hardware/gnss_driver.h"
#include "../hardware/sdcard_driver.h"
#include "../hardware/battery_monitor.h"
#include "esp_log.h"

static const char* TAG = "UI_SB";

// Status bar layout for 240px width
// [GPS Icon(30px)] [Sat Count(40px)] [SD Icon(30px)] [Spacer] [Battery(70px)] [Charging(30px)]
#define ICON_WIDTH              30
#define SAT_COUNT_WIDTH         40
#define BATTERY_WIDTH           70

void ui_statusbar_init(void) {
    // TODO: Create LVGL status bar widgets for 240x20px:
    // Layout (left to right):
    // - GPS icon: 30px (red=no fix, green=fix)
    // - Satellite count: 40px ("12" with icon)
    // - SD card icon: 30px (gray=none, white=present)
    // - Spacer: 40px
    // - Battery level: 70px (bar + percentage)
    // - Charging icon: 30px (lightning bolt if charging)
    
    ESP_LOGI(TAG, "Status bar initialized for %dpx width", UI_SCREEN_WIDTH);
}

void ui_statusbar_update(void) {
    // Get GPS status
    bool gps_fix = gnss_has_fix();
    uint8_t satellites = gnss_get_satellites();
    
    // Get SD card status
    bool sd_present = sdcard_is_present();
    
    // Get battery status
    BatteryStatus battery;
    battery_read(&battery);
    
    // TODO: Update LVGL widgets
    // - GPS icon color (red if no fix, green if fix)
    // - Satellite count
    // - SD card icon
    // - Battery percentage
    // - Charging icon
}

void ui_statusbar_deinit(void) {
    // TODO: Clean up LVGL widgets
}
