/**
 * @file ui_gps_logger.cpp
 * @brief GPS logger UI implementation
 */

#include "ui_gps_logger.h"
#include "ui_common.h"
#include "../hardware/gnss_driver.h"
#include "../hardware/baro_driver.h"
#include "../core/gps_logger.h"
#include "esp_log.h"

static const char* TAG = "UI_LOG";

// Screen layout for 240x320 (similar to bike computer)
#define SPEED_DISPLAY_HEIGHT    100
#define MAP_VIEW_HEIGHT         120  // GPS track visualization
#define DATA_ROW_HEIGHT         40

void ui_gps_logger_init(void) {
    // TODO: Create LVGL widgets for 240x320 screen:
    // - Status bar: 20px
    // - Speed display: 100px
    // - GPS track map: 120px (visual track representation)
    // - Distance/Time info: 40px each
    // - Recording status: 40px (with red flashing indicator)
    
    ESP_LOGI(TAG, "GPS logger UI initialized for %dx%d", UI_SCREEN_WIDTH, UI_SCREEN_HEIGHT);
}

void ui_gps_logger_update(void) {
    GnssData gps;
    gnss_read(&gps);
    
    BaroData baro;
    baro_read(&baro);
    
    bool logging = gps_logger_is_logging();
    float distance = gps_logger_get_distance();
    uint32_t duration = gps_logger_get_duration();
    
    // TODO: Update LVGL widgets
}

void ui_gps_logger_deinit(void) {
    // TODO: Clean up
}
