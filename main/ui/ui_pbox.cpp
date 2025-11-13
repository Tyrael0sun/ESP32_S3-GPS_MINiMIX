/**
 * @file ui_pbox.cpp
 * @brief P-Box UI implementation
 */

#include "ui_pbox.h"
#include "ui_common.h"
#include "../hardware/gnss_driver.h"
#include "../core/sensor_fusion.h"
#include "../hardware/display_driver.h"
#include "esp_log.h"
#include "lvgl.h"
#include <stdio.h>

static const char* TAG = "UI_PBOX";

// Screen layout for 240x320 (portrait)
#define SPEED_DISPLAY_HEIGHT    140
#define TIMER_DISPLAY_HEIGHT    80
#define TARGET_INFO_HEIGHT      40
#define STATUS_HEIGHT           40

// LVGL widgets
static lv_obj_t* speed_label = NULL;
static lv_obj_t* speed_unit_label = NULL;
static lv_obj_t* timer_label = NULL;
static lv_obj_t* target_label = NULL;
static lv_obj_t* status_label = NULL;
static lv_obj_t* best_time_label = NULL;

void ui_pbox_init(void) {
    lv_obj_t* scr = lv_scr_act();
    
    // Speed display (very large, centered)
    speed_label = lv_label_create(scr);
    lv_label_set_text(speed_label, "0.0");
    lv_obj_set_style_text_font(speed_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(speed_label, lv_color_white(), 0);
    lv_obj_align(speed_label, LV_ALIGN_TOP_MID, 0, UI_STATUS_BAR_HEIGHT + 50);
    
    // Speed unit
    speed_unit_label = lv_label_create(scr);
    lv_label_set_text(speed_unit_label, "km/h");
    lv_obj_set_style_text_color(speed_unit_label, lv_color_hex(UI_COLOR_TEXT_SECONDARY), 0);
    lv_obj_align(speed_unit_label, LV_ALIGN_TOP_MID, 0, UI_STATUS_BAR_HEIGHT + 100);
    
    // Timer display
    int y_pos = UI_STATUS_BAR_HEIGHT + SPEED_DISPLAY_HEIGHT;
    timer_label = lv_label_create(scr);
    lv_label_set_text(timer_label, "00:00.00");
    lv_obj_set_style_text_font(timer_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(timer_label, lv_color_hex(0x00FF00), 0);
    lv_obj_set_pos(timer_label, 60, y_pos + 20);
    
    // Target info
    y_pos += TIMER_DISPLAY_HEIGHT;
    target_label = lv_label_create(scr);
    lv_label_set_text(target_label, "Target: 100 km/h");
    lv_obj_set_style_text_color(target_label, lv_color_white(), 0);
    lv_obj_set_pos(target_label, 40, y_pos + 10);
    
    // Best time label
    best_time_label = lv_label_create(scr);
    lv_label_set_text(best_time_label, "Best: --:--");
    lv_obj_set_style_text_color(best_time_label, lv_color_hex(0xFFFF00), 0);
    lv_obj_set_pos(best_time_label, 40, y_pos + 25);
    
    // Status message
    y_pos += TARGET_INFO_HEIGHT;
    status_label = lv_label_create(scr);
    lv_label_set_text(status_label, "Ready");
    lv_obj_set_style_text_color(status_label, lv_color_hex(UI_COLOR_TEXT_SECONDARY), 0);
    lv_obj_align(status_label, LV_ALIGN_BOTTOM_MID, 0, -10);
    
    ESP_LOGI(TAG, "P-Box UI initialized for %dx%d", UI_SCREEN_WIDTH, UI_SCREEN_HEIGHT);
}

void ui_pbox_update(void) {
    if (!speed_label) return;
    
    GnssData gps;
    gnss_read(&gps);
    
    // Update speed
    char speed_buf[16];
    snprintf(speed_buf, sizeof(speed_buf), "%.1f", gps.speed);
    lv_label_set_text(speed_label, speed_buf);
    
    // TODO: Update timer when test is running
    // static uint32_t test_start_time = 0;
    // if (test_running) {
    //     uint32_t elapsed = (xTaskGetTickCount() * portTICK_PERIOD_MS) - test_start_time;
    //     uint32_t minutes = elapsed / 60000;
    //     uint32_t seconds = (elapsed % 60000) / 1000;
    //     uint32_t centiseconds = (elapsed % 1000) / 10;
    //     char time_buf[16];
    //     snprintf(time_buf, sizeof(time_buf), "%02lu:%02lu.%02lu", minutes, seconds, centiseconds);
    //     lv_label_set_text(timer_label, time_buf);
    // }
}

void ui_pbox_deinit(void) {
    if (speed_label) lv_obj_del(speed_label);
    if (speed_unit_label) lv_obj_del(speed_unit_label);
    if (timer_label) lv_obj_del(timer_label);
    if (target_label) lv_obj_del(target_label);
    if (status_label) lv_obj_del(status_label);
    if (best_time_label) lv_obj_del(best_time_label);
    
    speed_label = NULL;
    speed_unit_label = NULL;
    timer_label = NULL;
    target_label = NULL;
    status_label = NULL;
    best_time_label = NULL;
}
