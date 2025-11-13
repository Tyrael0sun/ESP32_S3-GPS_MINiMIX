/**
 * @file ui_pbox.cpp
 * @brief P-Box UI implementation
 */

#include "ui_pbox.h"
#include "ui_common.h"
#include "../hardware/gnss_driver.h"
#include "../core/sensor_fusion.h"
#include "esp_log.h"

static const char* TAG = "UI_PBOX";

// Screen layout for 240x320 (portrait)
// Optimized for performance testing display
#define SPEED_DISPLAY_HEIGHT    150
#define TIMER_DISPLAY_HEIGHT    80
#define INFO_DISPLAY_HEIGHT     50

void ui_pbox_init(void) {
    // TODO: Create LVGL widgets for 240x320 screen:
    // - Status bar: 20px
    // - Current speed (very large): 150px (font size ~64px)
    // - Test timer (large): 80px (mm:ss.ms format, font ~32px)
    // - Target speed info: 50px
    // - "TEST FINISHED!!!" message: 50px (centered, flashing)
    
    ESP_LOGI(TAG, "P-Box UI initialized for %dx%d", UI_SCREEN_WIDTH, UI_SCREEN_HEIGHT);
}

void ui_pbox_update(void) {
    GnssData gps;
    gnss_read(&gps);
    
    // TODO: Update based on P-Box app state
}

void ui_pbox_deinit(void) {
    // TODO: Clean up
}
