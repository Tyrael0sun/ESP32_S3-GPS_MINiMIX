/**
 * @file ui_gnss_info.cpp
 * @brief GNSS information display UI implementation
 */

#include "ui_gnss_info.h"
#include "ui_common.h"
#include "../hardware/gnss_driver.h"
#include "../hardware/display_driver.h"
#include "esp_log.h"
#include "lvgl.h"
#include <stdio.h>

static const char* TAG = "UI_GNSS";

// Display constants
#define LINE1_HEIGHT        60
#define LINE2_HEIGHT        40
#define LINE3_HEIGHT        200
#define LINE1_Y             UI_STATUS_BAR_HEIGHT
#define LINE2_Y             (LINE1_Y + LINE1_HEIGHT)
#define LINE3_Y             (LINE2_Y + LINE2_HEIGHT)

// LVGL widgets
static lv_obj_t* pos_line1_label = NULL;
static lv_obj_t* pos_line2_label = NULL;
static lv_obj_t* acc_label = NULL;
static lv_obj_t* sat_list = NULL;
static lv_obj_t* sat_header_label = NULL;

static const char* get_status_text(SatelliteStatus status) {
    switch (status) {
        case SAT_SEARCHING: return "SRCH";
        case SAT_TRACKING:  return "TRK ";
        case SAT_USED:      return "USE ";
        default:            return "UNK ";
    }
}

void ui_gnss_info_init(void) {
    lv_obj_t* scr = lv_scr_act();
    
    // Line 1: Position Information
    pos_line1_label = lv_label_create(scr);
    lv_label_set_text(pos_line1_label, "Lat: --- Lon: ---");
    lv_obj_set_style_text_color(pos_line1_label, lv_color_white(), 0);
    lv_obj_set_pos(pos_line1_label, 10, LINE1_Y + 5);
    
    pos_line2_label = lv_label_create(scr);
    lv_label_set_text(pos_line2_label, "Alt: --- Spd: ---");
    lv_obj_set_style_text_color(pos_line2_label, lv_color_white(), 0);
    lv_obj_set_pos(pos_line2_label, 10, LINE1_Y + 30);
    
    // Line 2: Accuracy Information
    acc_label = lv_label_create(scr);
    lv_label_set_text(acc_label, "HDOP:-- VDOP:-- PDOP:--");
    lv_obj_set_style_text_color(acc_label, lv_color_hex(UI_COLOR_TEXT_SECONDARY), 0);
    lv_obj_set_pos(acc_label, 10, LINE2_Y + 10);
    
    // Line 3: Satellite List Header
    sat_header_label = lv_label_create(scr);
    lv_label_set_text(sat_header_label, "ID  TYPE CN0 ST");
    lv_obj_set_style_text_color(sat_header_label, lv_color_hex(UI_COLOR_TEXT_SECONDARY), 0);
    lv_obj_set_pos(sat_header_label, 10, LINE3_Y + 5);
    
    // Satellite list (scrollable)
    sat_list = lv_list_create(scr);
    lv_obj_set_size(sat_list, 220, LINE3_HEIGHT - 30);
    lv_obj_set_pos(sat_list, 10, LINE3_Y + 25);
    lv_obj_set_style_bg_color(sat_list, lv_color_hex(0x000000), 0);
    lv_obj_set_style_border_width(sat_list, 1, 0);
    lv_obj_set_style_border_color(sat_list, lv_color_hex(UI_COLOR_TEXT_SECONDARY), 0);
    
    ESP_LOGI(TAG, "GNSS info UI initialized for %dx%d", UI_SCREEN_WIDTH, UI_SCREEN_HEIGHT);
}

void ui_gnss_info_update(void) {
    if (!pos_line1_label) return;
    
    GnssData gps;
    gnss_read(&gps);
    
    // === Line 1: Position Information ===
    if (gps.fix_valid) {
        char pos_line1[64];
        char pos_line2[64];
        
        snprintf(pos_line1, sizeof(pos_line1), 
                 "Lat:%.5f Lon:%.5f", 
                 gps.latitude, gps.longitude);
        
        snprintf(pos_line2, sizeof(pos_line2),
                 "Alt:%.1fm Spd:%.1f",
                 gps.altitude, gps.speed);
        
        lv_label_set_text(pos_line1_label, pos_line1);
        lv_label_set_text(pos_line2_label, pos_line2);
    } else {
        lv_label_set_text(pos_line1_label, "NO FIX");
        lv_label_set_text(pos_line2_label, "---");
    }
    
    // === Line 2: Accuracy Information ===
    char acc_line[64];
    snprintf(acc_line, sizeof(acc_line),
             "HDOP:%.1f VDOP:%.1f PDOP:%.1f",
             gps.hdop, gps.vdop, gps.pdop);
    lv_label_set_text(acc_label, acc_line);
    
    // === Line 3: Satellite List ===
    SatelliteInfo sats[MAX_SATELLITES];
    uint8_t sat_count = gnss_get_satellite_info(sats, MAX_SATELLITES);
    
    // Clear existing list items
    lv_obj_clean(sat_list);
    
    for (uint8_t i = 0; i < sat_count; i++) {
        const char* constellation = gnss_get_constellation_name(sats[i].constellation);
        const char* status = get_status_text(sats[i].status);
        
        char item_text[32];
        snprintf(item_text, sizeof(item_text), "%-3d %-4s %2d  %s",
                 sats[i].sat_id, constellation, sats[i].cn0, status);
        
        lv_obj_t* btn = lv_list_add_btn(sat_list, NULL, item_text);
        
        // Color based on status
        lv_color_t color;
        if (sats[i].status == SAT_USED) {
            color = lv_color_hex(0x00FF00); // Green
        } else if (sats[i].status == SAT_TRACKING) {
            color = lv_color_hex(0xFFFF00); // Yellow
        } else {
            color = lv_color_hex(UI_COLOR_TEXT_SECONDARY); // Gray
        }
        lv_obj_set_style_text_color(btn, color, 0);
    }
}

void ui_gnss_info_deinit(void) {
    if (pos_line1_label) lv_obj_del(pos_line1_label);
    if (pos_line2_label) lv_obj_del(pos_line2_label);
    if (acc_label) lv_obj_del(acc_label);
    if (sat_header_label) lv_obj_del(sat_header_label);
    if (sat_list) lv_obj_del(sat_list);
    
    pos_line1_label = NULL;
    pos_line2_label = NULL;
    acc_label = NULL;
    sat_header_label = NULL;
    sat_list = NULL;
}

/*
 * UI Design Analysis for 240x320 LCD:
 * 
 * Status Bar (20px):
 * - GPS icon, battery, etc (shared with other screens)
 * 
 * Line 1 - Position Info (60px):
 * - Font size: ~12-14px for readability
 * - 2 lines of text:
 *   Line 1: "Lat: 22.54321° Lon: 113.94210°" (~38 chars, fits in 240px with 6px font)
 *   Line 2: "Alt: 125.5m  Spd: 45.3 km/h"   (~30 chars)
 * - Padding: 5px top/bottom
 * 
 * Line 2 - Accuracy Info (40px):
 * - Font size: ~10-12px
 * - Single line: "HDOP:1.2 VDOP:1.5 PDOP:1.8" (~27 chars)
 * - Padding: 5px top/bottom
 * 
 * Line 3 - Satellite List (200px):
 * - Font size: ~8-10px (monospace recommended)
 * - Header: "ID  TYPE  CN0  STATUS" (fixed at top)
 * - List items: "123  GPS   45   USE" (~20 chars)
 * - Item height: 20px
 * - Visible items: 200/20 = 10 satellites
 * - Scrollable for >10 satellites
 * - Color coding:
 *   * Gray: Searching (CN0 < 20)
 *   * Yellow: Tracking (CN0 20-35)
 *   * Green: Used (CN0 > 35)
 * 
 * Font Requirements:
 * - Line 1: 12-14px bold for clarity
 * - Line 2: 10-12px regular
 * - Line 3: 8-10px monospace for alignment
 * 
 * Total height: 20 + 60 + 40 + 200 = 320px ✓ Perfect fit!
 * Width: 240px accommodates all text with proper font sizes ✓
 * 
 * Recommendations:
 * 1. Use LVGL table or list for satellite display
 * 2. Enable vertical scrolling for satellite list
 * 3. Update satellite list every 1 second
 * 4. Use color gradients for CN0 visualization
 * 5. Consider adding a signal strength bar chart
 */
