/**
 * @file ui_settings.cpp
 * @brief Settings UI implementation
 */

#include "ui_settings.h"
#include "ui_common.h"
#include "../core/calibration.h"
#include "../hardware/gnss_driver.h"
#include "esp_log.h"

static const char* TAG = "UI_SET";

// Screen layout for 240x320 (portrait)
// Menu-based interface
#define MENU_ITEM_HEIGHT        40
#define MENU_MAX_VISIBLE        7  // (300px / 40px)

void ui_settings_init(void) {
    // TODO: Create LVGL menu for 240x320 screen:
    // - Status bar: 20px
    // - Menu items (40px each, max 7 visible):
    //   * IMU Calibration
    //   * Magnetometer Calibration
    //   * GNSS Rate (1/5/10/25 Hz)
    //   * GNSS Constellation (GPS/GLO/GAL/BDS)
    //   * Display Brightness
    //   * About/Version
    // - Progress bar during calibration
    
    ESP_LOGI(TAG, "Settings UI initialized for %dx%d", UI_SCREEN_WIDTH, UI_SCREEN_HEIGHT);
}

void ui_settings_update(void) {
    // Check if calibration is running
    if (calibration_is_running()) {
        uint8_t progress = calibration_get_progress();
        // TODO: Update progress bar
    }
}

void ui_settings_deinit(void) {
    // TODO: Clean up
}
