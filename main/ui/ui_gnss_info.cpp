/**
 * @file ui_gnss_info.cpp
 * @brief GNSS information display UI implementation
 * 
 * Screen Layout (240x320):
 * - Status bar: 20px (shared)
 * - Line 1 (Position): 60px - Lat/Lon, Altitude, Speed
 * - Line 2 (Accuracy): 40px - HDOP, VDOP, PDOP
 * - Line 3 (Satellites): 200px - Scrollable satellite list
 */

#include "ui_gnss_info.h"
#include "ui_common.h"
#include "../hardware/gnss_driver.h"
#include "esp_log.h"
#include <stdio.h>

static const char* TAG = "UI_GNSS";

// Display constants for GNSS info screen (uses 240x320 layout)
#define SCREEN_WIDTH        UI_SCREEN_WIDTH
#define SCREEN_HEIGHT       UI_SCREEN_HEIGHT
#define STATUS_BAR_HEIGHT   UI_STATUS_BAR_HEIGHT
#define LINE1_HEIGHT        60
#define LINE2_HEIGHT        40
#define LINE3_HEIGHT        200

#define LINE1_Y             STATUS_BAR_HEIGHT
#define LINE2_Y             (LINE1_Y + LINE1_HEIGHT)
#define LINE3_Y             (LINE2_Y + LINE2_HEIGHT)

// Satellite list display
#define SAT_LIST_ITEM_HEIGHT 20
#define SAT_LIST_MAX_VISIBLE 10

static const char* get_status_text(SatelliteStatus status) {
    switch (status) {
        case SAT_SEARCHING: return "SRCH";
        case SAT_TRACKING:  return "TRK ";
        case SAT_USED:      return "USE ";
        default:            return "UNK ";
    }
}

void ui_gnss_info_init(void) {
    // TODO: Create LVGL widgets
    
    // Line 1: Position Information
    // - Label: "Lat: XX.XXXXX° Lon: XX.XXXXX°"
    // - Label: "Alt: XXXX.Xm  Spd: XXX.X km/h"
    
    // Line 2: Accuracy Information
    // - Label: "HDOP: X.X  VDOP: X.X  PDOP: X.X"
    
    // Line 3: Satellite List (scrollable)
    // - Header: "ID  TYPE  CN0  STATUS"
    // - List items: "XXX  XXX   XX   XXXX"
    
    ESP_LOGI(TAG, "GNSS info UI initialized");
    ESP_LOGI(TAG, "Screen layout: %dx%d", SCREEN_WIDTH, SCREEN_HEIGHT);
    ESP_LOGI(TAG, "  Line 1 (Position): y=%d, h=%d", LINE1_Y, LINE1_HEIGHT);
    ESP_LOGI(TAG, "  Line 2 (Accuracy): y=%d, h=%d", LINE2_Y, LINE2_HEIGHT);
    ESP_LOGI(TAG, "  Line 3 (Satellites): y=%d, h=%d", LINE3_Y, LINE3_HEIGHT);
}

void ui_gnss_info_update(void) {
    GnssData gps;
    gnss_read(&gps);
    
    // === Line 1: Position Information ===
    if (gps.fix_valid) {
        char pos_line1[64];
        char pos_line2[64];
        
        // Format: "Lat: 22.54321° Lon: 113.94210°"
        snprintf(pos_line1, sizeof(pos_line1), 
                 "Lat:%.5f Lon:%.5f", 
                 gps.latitude, gps.longitude);
        
        // Format: "Alt: 125.5m  Spd: 45.3 km/h"
        snprintf(pos_line2, sizeof(pos_line2),
                 "Alt:%.1fm Spd:%.1f km/h",
                 gps.altitude, gps.speed);
        
        ESP_LOGD(TAG, "%s", pos_line1);
        ESP_LOGD(TAG, "%s", pos_line2);
        
        // TODO: Update LVGL labels
    } else {
        ESP_LOGD(TAG, "Position: NO FIX");
        // TODO: Display "NO FIX" message
    }
    
    // === Line 2: Accuracy Information ===
    char acc_line[64];
    snprintf(acc_line, sizeof(acc_line),
             "HDOP:%.1f VDOP:%.1f PDOP:%.1f",
             gps.hdop, gps.vdop, gps.pdop);
    ESP_LOGD(TAG, "%s", acc_line);
    
    // TODO: Update LVGL label
    
    // === Line 3: Satellite List ===
    SatelliteInfo sats[MAX_SATELLITES];
    uint8_t sat_count = gnss_get_satellite_info(sats, MAX_SATELLITES);
    
    ESP_LOGD(TAG, "Satellites in view: %d", sat_count);
    ESP_LOGD(TAG, "----------------------------------------");
    ESP_LOGD(TAG, "ID   TYPE  CN0  STATUS");
    
    for (uint8_t i = 0; i < sat_count; i++) {
        const char* constellation = gnss_get_constellation_name(sats[i].constellation);
        const char* status = get_status_text(sats[i].status);
        
        ESP_LOGD(TAG, "%-3d  %-4s  %-3d  %s",
                 sats[i].sat_id,
                 constellation,
                 sats[i].cn0,
                 status);
        
        // TODO: Update LVGL list item
        // Each item format: "123  GPS   45   USE"
        // Use different colors based on status:
        // - SAT_SEARCHING: Gray
        // - SAT_TRACKING: Yellow
        // - SAT_USED: Green
    }
    
    // Visual layout check
    int required_height = sat_count * SAT_LIST_ITEM_HEIGHT;
    if (required_height > LINE3_HEIGHT) {
        ESP_LOGD(TAG, "Satellite list requires scrolling: %d > %d", 
                 required_height, LINE3_HEIGHT);
    }
}

void ui_gnss_info_deinit(void) {
    // TODO: Clean up LVGL widgets
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
