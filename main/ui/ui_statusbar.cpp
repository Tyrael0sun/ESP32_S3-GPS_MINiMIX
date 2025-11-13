/**
 * @file ui_statusbar.cpp
 * @brief Status bar implementation
 */

#include "ui_statusbar.h"
#include "ui_common.h"
#include "../hardware/gnss_driver.h"
#include "../hardware/sdcard_driver.h"
#include "../hardware/battery_monitor.h"
#include "../hardware/display_driver.h"
#include "esp_log.h"
#include <stdio.h>

static const char* TAG = "UI_SB";

// Status bar layout for 240px width
// [GPS Icon(30px)] [Sat Count(40px)] [SD Icon(30px)] [Spacer] [Battery(70px)] [Charging(30px)]
#define ICON_WIDTH              30
#define SAT_COUNT_WIDTH         40
#define BATTERY_WIDTH           70

// LVGL objects
static lv_obj_t* statusbar_cont = NULL;
static lv_obj_t* gps_icon_label = NULL;
static lv_obj_t* sat_count_label = NULL;
static lv_obj_t* sd_icon_label = NULL;
static lv_obj_t* battery_bar = NULL;
static lv_obj_t* battery_label = NULL;
static lv_obj_t* charging_icon = NULL;

void ui_statusbar_init(void) {
    // Clean up existing status bar if any
    if (statusbar_cont) {
        ui_statusbar_deinit();
    }
    
    lv_obj_t* scr = lv_scr_act();
    
    // Create status bar container
    statusbar_cont = lv_obj_create(scr);
    lv_obj_set_size(statusbar_cont, UI_SCREEN_WIDTH, UI_STATUS_BAR_HEIGHT);
    lv_obj_set_pos(statusbar_cont, 0, 0);
    lv_obj_set_style_bg_color(statusbar_cont, lv_color_hex(0x202020), 0);
    lv_obj_set_style_border_width(statusbar_cont, 0, 0);
    lv_obj_set_style_pad_all(statusbar_cont, 2, 0);
    lv_obj_clear_flag(statusbar_cont, LV_OBJ_FLAG_SCROLLABLE);
    
    // GPS icon (0-30px)
    gps_icon_label = lv_label_create(statusbar_cont);
    lv_label_set_text(gps_icon_label, LV_SYMBOL_GPS);
    lv_obj_set_pos(gps_icon_label, 5, 0);
    lv_obj_set_style_text_color(gps_icon_label, lv_color_hex(UI_COLOR_GPS_NO_FIX), 0);
    
    // Satellite count (30-70px)
    sat_count_label = lv_label_create(statusbar_cont);
    lv_label_set_text(sat_count_label, "0");
    lv_obj_set_pos(sat_count_label, 30, 0);
    lv_obj_set_style_text_color(sat_count_label, lv_color_white(), 0);
    
    // SD card icon (70-100px)
    sd_icon_label = lv_label_create(statusbar_cont);
    lv_label_set_text(sd_icon_label, LV_SYMBOL_SD_CARD);
    lv_obj_set_pos(sd_icon_label, 70, 0);
    lv_obj_set_style_text_color(sd_icon_label, lv_color_hex(UI_COLOR_TEXT_SECONDARY), 0);
    
    // Battery bar (140-210px)
    battery_bar = lv_bar_create(statusbar_cont);
    lv_obj_set_size(battery_bar, 50, 12);
    lv_obj_set_pos(battery_bar, 140, 3);
    lv_bar_set_range(battery_bar, 0, 100);
    lv_bar_set_value(battery_bar, 100, LV_ANIM_OFF);
    
    // Battery percentage
    battery_label = lv_label_create(statusbar_cont);
    lv_label_set_text(battery_label, "100%");
    lv_obj_set_pos(battery_label, 192, 0);
    lv_obj_set_style_text_color(battery_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(battery_label, &lv_font_montserrat_14, 0);
    
    // Charging icon (210-240px)
    charging_icon = lv_label_create(statusbar_cont);
    lv_label_set_text(charging_icon, LV_SYMBOL_CHARGE);
    lv_obj_set_pos(charging_icon, 220, 0);
    lv_obj_add_flag(charging_icon, LV_OBJ_FLAG_HIDDEN);
    
    ESP_LOGI(TAG, "Status bar initialized for %dpx width", UI_SCREEN_WIDTH);
}

void ui_statusbar_update(void) {
    if (!statusbar_cont) return;
    
    // Get GPS status
    bool gps_fix = gnss_has_fix();
    uint8_t satellites = gnss_get_satellites();
    
    // Get SD card status
    bool sd_present = sdcard_is_present();
    
    // Get battery status
    BatteryStatus battery;
    battery_read(&battery);
    
    // Update GPS icon color
    if (gps_fix) {
        lv_obj_set_style_text_color(gps_icon_label, lv_color_hex(UI_COLOR_GPS_FIX), 0);
    } else {
        lv_obj_set_style_text_color(gps_icon_label, lv_color_hex(UI_COLOR_GPS_NO_FIX), 0);
    }
    
    // Update satellite count
    char sat_buf[8];
    snprintf(sat_buf, sizeof(sat_buf), "%d", satellites);
    lv_label_set_text(sat_count_label, sat_buf);
    
    // Update SD card icon
    if (sd_present) {
        lv_obj_set_style_text_color(sd_icon_label, lv_color_white(), 0);
    } else {
        lv_obj_set_style_text_color(sd_icon_label, lv_color_hex(UI_COLOR_TEXT_SECONDARY), 0);
    }
    
    // Update battery bar and percentage
    lv_bar_set_value(battery_bar, battery.percentage, LV_ANIM_OFF);
    char bat_buf[8];
    snprintf(bat_buf, sizeof(bat_buf), "%d%%", battery.percentage);
    lv_label_set_text(battery_label, bat_buf);
    
    // Update charging icon
    if (battery.charging) {
        lv_obj_clear_flag(charging_icon, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(charging_icon, LV_OBJ_FLAG_HIDDEN);
    }
}

void ui_statusbar_deinit(void) {
    if (statusbar_cont) {
        lv_obj_del(statusbar_cont);
        statusbar_cont = NULL;
        gps_icon_label = NULL;
        sat_count_label = NULL;
        sd_icon_label = NULL;
        battery_bar = NULL;
        battery_label = NULL;
        charging_icon = NULL;
    }
}
