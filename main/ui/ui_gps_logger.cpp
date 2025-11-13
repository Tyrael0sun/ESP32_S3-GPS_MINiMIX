/**
 * @file ui_gps_logger.cpp
 * @brief GPS logger UI implementation
 */

#include "ui_gps_logger.h"
#include "ui_common.h"
#include "../hardware/gnss_driver.h"
#include "../hardware/baro_driver.h"
#include "../core/gps_logger.h"
#include "../hardware/display_driver.h"
#include "esp_log.h"
#include "lvgl.h"
#include <stdio.h>

static const char* TAG = "UI_LOG";

// Screen layout
#define SPEED_DISPLAY_HEIGHT    100
#define MAP_VIEW_HEIGHT         120
#define DATA_ROW_HEIGHT         40

// LVGL widgets
static lv_obj_t* speed_label = NULL;
static lv_obj_t* speed_unit_label = NULL;
static lv_obj_t* map_canvas = NULL;
static lv_obj_t* distance_label = NULL;
static lv_obj_t* time_label = NULL;
static lv_obj_t* altitude_label = NULL;
static lv_obj_t* record_status_label = NULL;
static lv_obj_t* record_indicator = NULL;

void ui_gps_logger_init(void) {
    lv_obj_t* scr = lv_scr_act();
    
    // Speed display
    speed_label = lv_label_create(scr);
    lv_label_set_text(speed_label, "0.0");
    lv_obj_set_style_text_font(speed_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(speed_label, lv_color_white(), 0);
    lv_obj_align(speed_label, LV_ALIGN_TOP_MID, 0, UI_STATUS_BAR_HEIGHT + 30);
    
    speed_unit_label = lv_label_create(scr);
    lv_label_set_text(speed_unit_label, "km/h");
    lv_obj_set_style_text_color(speed_unit_label, lv_color_hex(UI_COLOR_TEXT_SECONDARY), 0);
    lv_obj_align(speed_unit_label, LV_ALIGN_TOP_MID, 0, UI_STATUS_BAR_HEIGHT + 70);
    
    // Map view placeholder (for future track visualization)
    int y_pos = UI_STATUS_BAR_HEIGHT + SPEED_DISPLAY_HEIGHT;
    map_canvas = lv_obj_create(scr);
    lv_obj_set_size(map_canvas, 220, 110);
    lv_obj_set_pos(map_canvas, 10, y_pos + 5);
    lv_obj_set_style_bg_color(map_canvas, lv_color_hex(0x1a1a1a), 0);
    lv_obj_set_style_border_width(map_canvas, 1, 0);
    lv_obj_set_style_border_color(map_canvas, lv_color_hex(UI_COLOR_TEXT_SECONDARY), 0);
    
    lv_obj_t* map_label = lv_label_create(map_canvas);
    lv_label_set_text(map_label, "GPS Track");
    lv_obj_set_style_text_color(map_label, lv_color_hex(UI_COLOR_TEXT_SECONDARY), 0);
    lv_obj_center(map_label);
    
    // Data rows
    y_pos += MAP_VIEW_HEIGHT;
    
    distance_label = lv_label_create(scr);
    lv_label_set_text(distance_label, "Dist: 0.00 km");
    lv_obj_set_style_text_color(distance_label, lv_color_white(), 0);
    lv_obj_set_pos(distance_label, 20, y_pos + 5);
    
    altitude_label = lv_label_create(scr);
    lv_label_set_text(altitude_label, "Alt: --- m");
    lv_obj_set_style_text_color(altitude_label, lv_color_white(), 0);
    lv_obj_set_pos(altitude_label, 140, y_pos + 5);
    
    y_pos += DATA_ROW_HEIGHT;
    
    time_label = lv_label_create(scr);
    lv_label_set_text(time_label, "Time: 00:00:00");
    lv_obj_set_style_text_color(time_label, lv_color_white(), 0);
    lv_obj_set_pos(time_label, 20, y_pos + 5);
    
    // Recording status
    record_status_label = lv_label_create(scr);
    lv_label_set_text(record_status_label, "STOP");
    lv_obj_set_style_text_color(record_status_label, lv_color_hex(UI_COLOR_TEXT_SECONDARY), 0);
    lv_obj_set_pos(record_status_label, 170, y_pos + 5);
    
    record_indicator = lv_obj_create(scr);
    lv_obj_set_size(record_indicator, 15, 15);
    lv_obj_set_pos(record_indicator, 150, y_pos + 10);
    lv_obj_set_style_radius(record_indicator, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(record_indicator, lv_color_hex(UI_COLOR_TEXT_SECONDARY), 0);
    lv_obj_set_style_border_width(record_indicator, 0, 0);
    
    ESP_LOGI(TAG, "GPS logger UI initialized for %dx%d", UI_SCREEN_WIDTH, UI_SCREEN_HEIGHT);
}

void ui_gps_logger_update(void) {
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
    
    // Update distance
    char dist_buf[32];
    snprintf(dist_buf, sizeof(dist_buf), "Dist: %.2f km", distance);
    lv_label_set_text(distance_label, dist_buf);
    
    // Update altitude
    char alt_buf[32];
    snprintf(alt_buf, sizeof(alt_buf), "Alt: %.1f m", baro.altitude);
    lv_label_set_text(altitude_label, alt_buf);
    
    // Update time
    uint32_t hours = duration / 3600;
    uint32_t minutes = (duration % 3600) / 60;
    uint32_t seconds = duration % 60;
    char time_buf[32];
    snprintf(time_buf, sizeof(time_buf), "Time: %02lu:%02lu:%02lu", hours, minutes, seconds);
    lv_label_set_text(time_label, time_buf);
    
    // Update recording status
    if (logging) {
        lv_label_set_text(record_status_label, "REC");
        lv_obj_set_style_text_color(record_status_label, lv_color_hex(UI_COLOR_RECORDING), 0);
        
        // Flashing indicator
        static uint32_t flash_counter = 0;
        flash_counter++;
        if ((flash_counter / 5) % 2 == 0) {
            lv_obj_set_style_bg_color(record_indicator, lv_color_hex(UI_COLOR_RECORDING), 0);
        } else {
            lv_obj_set_style_bg_color(record_indicator, lv_color_hex(0x800000), 0);
        }
    } else {
        lv_label_set_text(record_status_label, "STOP");
        lv_obj_set_style_text_color(record_status_label, lv_color_hex(UI_COLOR_TEXT_SECONDARY), 0);
        lv_obj_set_style_bg_color(record_indicator, lv_color_hex(UI_COLOR_TEXT_SECONDARY), 0);
    }
}

void ui_gps_logger_deinit(void) {
    if (speed_label) lv_obj_del(speed_label);
    if (speed_unit_label) lv_obj_del(speed_unit_label);
    if (map_canvas) lv_obj_del(map_canvas);
    if (distance_label) lv_obj_del(distance_label);
    if (time_label) lv_obj_del(time_label);
    if (altitude_label) lv_obj_del(altitude_label);
    if (record_status_label) lv_obj_del(record_status_label);
    if (record_indicator) lv_obj_del(record_indicator);
    
    speed_label = NULL;
    speed_unit_label = NULL;
    map_canvas = NULL;
    distance_label = NULL;
    time_label = NULL;
    altitude_label = NULL;
    record_status_label = NULL;
    record_indicator = NULL;
}
