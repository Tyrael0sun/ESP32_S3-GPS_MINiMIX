/**
 * @file ui_bike_computer.cpp
 * @brief Bike computer UI implementation
 */

#include "ui_bike_computer.h"
#include "ui_common.h"
#include "../hardware/gnss_driver.h"
#include "../hardware/baro_driver.h"
#include "../core/gps_logger.h"
#include "../hardware/display_driver.h"
#include "esp_log.h"
#include <stdio.h>

static const char* TAG = "UI_BC";

// Screen layout for 240x320 (portrait)
// Layout: Status bar (20px) + Speed display (120px) + Data rows (180px)
#define SPEED_DISPLAY_HEIGHT    120
#define DATA_ROW_HEIGHT         45
#define DATA_ROWS               4

// LVGL objects
static lv_obj_t* speed_label = NULL;
static lv_obj_t* speed_unit_label = NULL;
static lv_obj_t* altitude_label = NULL;
static lv_obj_t* distance_label = NULL;
static lv_obj_t* time_label = NULL;
static lv_obj_t* record_indicator = NULL;

void ui_bike_computer_init(void) {
    lv_obj_t* scr = lv_scr_act();
    
    // Speed display (large, centered)
    speed_label = lv_label_create(scr);
    lv_label_set_text(speed_label, "0.0");
    lv_obj_set_style_text_font(speed_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(speed_label, lv_color_white(), 0);
    lv_obj_align(speed_label, LV_ALIGN_TOP_MID, 0, UI_STATUS_BAR_HEIGHT + 30);
    
    // Speed unit
    speed_unit_label = lv_label_create(scr);
    lv_label_set_text(speed_unit_label, "km/h");
    lv_obj_set_style_text_color(speed_unit_label, lv_color_hex(UI_COLOR_TEXT_SECONDARY), 0);
    lv_obj_align(speed_unit_label, LV_ALIGN_TOP_MID, 0, UI_STATUS_BAR_HEIGHT + 85);
    
    // Data rows starting position
    int y_pos = UI_STATUS_BAR_HEIGHT + SPEED_DISPLAY_HEIGHT + 10;
    
    // Altitude row
    altitude_label = lv_label_create(scr);
    lv_label_set_text(altitude_label, "Alt: --- m");
    lv_obj_set_style_text_color(altitude_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(altitude_label, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(altitude_label, 20, y_pos);
    y_pos += DATA_ROW_HEIGHT;
    
    // Distance row
    distance_label = lv_label_create(scr);
    lv_label_set_text(distance_label, "Dist: 0.00 km");
    lv_obj_set_style_text_color(distance_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(distance_label, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(distance_label, 20, y_pos);
    y_pos += DATA_ROW_HEIGHT;
    
    // Time row
    time_label = lv_label_create(scr);
    lv_label_set_text(time_label, "Time: 00:00:00");
    lv_obj_set_style_text_color(time_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(time_label, 20, y_pos);
    y_pos += DATA_ROW_HEIGHT;
    
    // Record indicator
    record_indicator = lv_obj_create(scr);
    lv_obj_set_size(record_indicator, 20, 20);
    lv_obj_set_pos(record_indicator, 110, y_pos + 10);
    lv_obj_set_style_radius(record_indicator, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(record_indicator, lv_color_hex(UI_COLOR_TEXT_SECONDARY), 0);
    lv_obj_set_style_border_width(record_indicator, 0, 0);
    
    ESP_LOGI(TAG, "Bike computer UI initialized for %dx%d", UI_SCREEN_WIDTH, UI_SCREEN_HEIGHT);
}

void ui_bike_computer_update(void) {
    if (!speed_label) return;
    
    GnssData gps;
    gnss_read(&gps);
    
    BaroData baro;
    baro_read(&baro);
    
    bool logging = gps_logger_is_logging();
    float distance = gps_logger_get_distance();
    uint32_t duration = gps_logger_get_duration();
    
    // Update speed
    char speed_buf[16];
    snprintf(speed_buf, sizeof(speed_buf), "%.1f", gps.speed);
    lv_label_set_text(speed_label, speed_buf);
    
    // Update altitude
    char alt_buf[32];
    snprintf(alt_buf, sizeof(alt_buf), "Alt: %.1f m", baro.altitude);
    lv_label_set_text(altitude_label, alt_buf);
    
    // Update distance
    char dist_buf[32];
    snprintf(dist_buf, sizeof(dist_buf), "Dist: %.2f km", distance);
    lv_label_set_text(distance_label, dist_buf);
    
    // Update time
    uint32_t hours = duration / 3600;
    uint32_t minutes = (duration % 3600) / 60;
    uint32_t seconds = duration % 60;
    char time_buf[32];
    snprintf(time_buf, sizeof(time_buf), "Time: %02lu:%02lu:%02lu", hours, minutes, seconds);
    lv_label_set_text(time_label, time_buf);
    
    // Update record indicator
    if (logging) {
        // Flashing red when recording
        static uint32_t flash_counter = 0;
        flash_counter++;
        if ((flash_counter / 5) % 2 == 0) {
            lv_obj_set_style_bg_color(record_indicator, lv_color_hex(UI_COLOR_RECORDING), 0);
        } else {
            lv_obj_set_style_bg_color(record_indicator, lv_color_hex(0x800000), 0); // Dark red
        }
    } else {
        lv_obj_set_style_bg_color(record_indicator, lv_color_hex(UI_COLOR_TEXT_SECONDARY), 0);
    }
}

void ui_bike_computer_deinit(void) {
    if (speed_label) lv_obj_del(speed_label);
    if (speed_unit_label) lv_obj_del(speed_unit_label);
    if (altitude_label) lv_obj_del(altitude_label);
    if (distance_label) lv_obj_del(distance_label);
    if (time_label) lv_obj_del(time_label);
    if (record_indicator) lv_obj_del(record_indicator);
    
    speed_label = NULL;
    speed_unit_label = NULL;
    altitude_label = NULL;
    distance_label = NULL;
    time_label = NULL;
    record_indicator = NULL;
}
