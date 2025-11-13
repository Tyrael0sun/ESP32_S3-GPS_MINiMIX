/**
 * @file ui_bike_computer.cpp
 * @brief Bike computer UI implementation
 */

#include "ui_bike_computer.h"
#include "ui_common.h"
#include "../hardware/gnss_driver.h"
#include "../hardware/baro_driver.h"
#include "../core/gps_logger.h"
#include "esp_log.h"

static const char* TAG = "UI_BC";

// Screen layout for 240x320 (portrait)
// Layout: Status bar (20px) + Speed display (120px) + Data rows (180px)
#define SPEED_DISPLAY_HEIGHT    120
#define DATA_ROW_HEIGHT         45
#define DATA_ROWS               4

void ui_bike_computer_init(void) {
    // TODO: Create LVGL widgets for 240x320 screen:
    // - Status bar: 20px (GPS, Battery, SD)
    // - Speed (large): 120px (center, large font ~48px)
    // - Row 1: Altitude (45px)
    // - Row 2: Trip distance (45px)
    // - Row 3: Trip time (45px)
    // - Row 4: Record indicator (45px)
    
    ESP_LOGI(TAG, "Bike computer UI initialized for %dx%d", UI_SCREEN_WIDTH, UI_SCREEN_HEIGHT);
}

void ui_bike_computer_update(void) {
    GnssData gps;
    gnss_read(&gps);
    
    BaroData baro;
    baro_read(&baro);
    
    bool logging = gps_logger_is_logging();
    float distance = gps_logger_get_distance();
    uint32_t duration = gps_logger_get_duration();
    
    // TODO: Update LVGL labels with:
    // - gps.speed (km/h)
    // - baro.altitude (m)
    // - distance (km)
    // - duration (hh:mm:ss format)
    // - Record button (red flashing if logging, green circle if not)
}

void ui_bike_computer_deinit(void) {
    // TODO: Clean up LVGL widgets
}
