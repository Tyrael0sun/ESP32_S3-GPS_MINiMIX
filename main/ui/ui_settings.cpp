/**
 * @file ui_settings.cpp
 * @brief Settings UI implementation
 */

#include "ui_settings.h"
#include "ui_common.h"
#include "../core/calibration.h"
#include "../hardware/gnss_driver.h"
#include "../hardware/display_driver.h"
#include "esp_log.h"
#include "lvgl.h"
#include <stdio.h>

static const char* TAG = "UI_SET";

// Screen layout for 240x320 (portrait)
#define MENU_ITEM_HEIGHT        40
#define MENU_MAX_VISIBLE        7

// LVGL widgets
static lv_obj_t* menu_list = NULL;
static lv_obj_t* title_label = NULL;
static lv_obj_t* progress_bar = NULL;
static lv_obj_t* progress_label = NULL;

void ui_settings_init(void) {
    lv_obj_t* scr = lv_scr_act();
    
    // Title
    title_label = lv_label_create(scr);
    lv_label_set_text(title_label, "Settings");
    lv_obj_set_style_text_color(title_label, lv_color_white(), 0);
    lv_obj_set_pos(title_label, 10, UI_STATUS_BAR_HEIGHT + 5);
    
    // Menu list (scrollable)
    menu_list = lv_list_create(scr);
    lv_obj_set_size(menu_list, 220, 260);
    lv_obj_set_pos(menu_list, 10, UI_STATUS_BAR_HEIGHT + 30);
    lv_obj_set_style_bg_color(menu_list, lv_color_hex(0x000000), 0);
    lv_obj_set_style_border_width(menu_list, 1, 0);
    lv_obj_set_style_border_color(menu_list, lv_color_hex(UI_COLOR_TEXT_SECONDARY), 0);
    
    // Add menu items
    lv_obj_t* btn;
    
    btn = lv_list_add_btn(menu_list, LV_SYMBOL_SETTINGS, "IMU Calibration");
    lv_obj_set_style_text_color(btn, lv_color_white(), 0);
    
    btn = lv_list_add_btn(menu_list, LV_SYMBOL_SETTINGS, "Mag Calibration");
    lv_obj_set_style_text_color(btn, lv_color_white(), 0);
    
    btn = lv_list_add_btn(menu_list, LV_SYMBOL_GPS, "GNSS Rate");
    lv_obj_set_style_text_color(btn, lv_color_white(), 0);
    
    btn = lv_list_add_btn(menu_list, LV_SYMBOL_GPS, "GNSS Constellation");
    lv_obj_set_style_text_color(btn, lv_color_white(), 0);
    
    btn = lv_list_add_btn(menu_list, LV_SYMBOL_IMAGE, "Display Brightness");
    lv_obj_set_style_text_color(btn, lv_color_white(), 0);
    
    btn = lv_list_add_btn(menu_list, LV_SYMBOL_HOME, "System Info");
    lv_obj_set_style_text_color(btn, lv_color_white(), 0);
    
    btn = lv_list_add_btn(menu_list, LV_SYMBOL_WARNING, "Factory Reset");
    lv_obj_set_style_text_color(btn, lv_color_hex(UI_COLOR_RECORDING), 0);
    
    // Progress bar (hidden by default)
    progress_bar = lv_bar_create(scr);
    lv_obj_set_size(progress_bar, 200, 20);
    lv_obj_set_pos(progress_bar, 20, 200);
    lv_obj_add_flag(progress_bar, LV_OBJ_FLAG_HIDDEN);
    lv_bar_set_range(progress_bar, 0, 100);
    lv_bar_set_value(progress_bar, 0, LV_ANIM_OFF);
    
    progress_label = lv_label_create(scr);
    lv_label_set_text(progress_label, "Calibrating...");
    lv_obj_set_style_text_color(progress_label, lv_color_white(), 0);
    lv_obj_set_pos(progress_label, 60, 230);
    lv_obj_add_flag(progress_label, LV_OBJ_FLAG_HIDDEN);
    
    ESP_LOGI(TAG, "Settings UI initialized for %dx%d", UI_SCREEN_WIDTH, UI_SCREEN_HEIGHT);
}

void ui_settings_update(void) {
    if (!menu_list) return;
    
    // Check if calibration is running
    if (calibration_is_running()) {
        uint8_t progress = calibration_get_progress();
        
        // Show progress bar
        lv_obj_clear_flag(progress_bar, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(progress_label, LV_OBJ_FLAG_HIDDEN);
        
        // Update progress
        lv_bar_set_value(progress_bar, progress, LV_ANIM_ON);
        
        char prog_text[32];
        snprintf(prog_text, sizeof(prog_text), "Calibrating... %d%%", progress);
        lv_label_set_text(progress_label, prog_text);
        
        // Hide menu during calibration
        lv_obj_add_flag(menu_list, LV_OBJ_FLAG_HIDDEN);
    } else {
        // Hide progress bar
        lv_obj_add_flag(progress_bar, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(progress_label, LV_OBJ_FLAG_HIDDEN);
        
        // Show menu
        lv_obj_clear_flag(menu_list, LV_OBJ_FLAG_HIDDEN);
    }
}

void ui_settings_deinit(void) {
    if (title_label) lv_obj_del(title_label);
    if (menu_list) lv_obj_del(menu_list);
    if (progress_bar) lv_obj_del(progress_bar);
    if (progress_label) lv_obj_del(progress_label);
    
    title_label = NULL;
    menu_list = NULL;
    progress_bar = NULL;
    progress_label = NULL;
}
