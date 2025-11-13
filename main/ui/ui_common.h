/**
 * @file ui_common.h
 * @brief Common UI definitions for 240x320 LCD
 */

#ifndef UI_COMMON_H
#define UI_COMMON_H

#include <stdint.h>

// ==================== Display Dimensions ====================
#define UI_SCREEN_WIDTH         240
#define UI_SCREEN_HEIGHT        320

// ==================== Common Layout Constants ====================
#define UI_STATUS_BAR_HEIGHT    20
#define UI_CONTENT_Y            UI_STATUS_BAR_HEIGHT
#define UI_CONTENT_HEIGHT       (UI_SCREEN_HEIGHT - UI_STATUS_BAR_HEIGHT)

// ==================== Font Sizes ====================
#define UI_FONT_SIZE_SMALL      8
#define UI_FONT_SIZE_MEDIUM     12
#define UI_FONT_SIZE_LARGE      16
#define UI_FONT_SIZE_XLARGE     24

// ==================== Padding and Margins ====================
#define UI_PADDING_SMALL        5
#define UI_PADDING_MEDIUM       10
#define UI_PADDING_LARGE        15

// ==================== Color Definitions ====================
// Satellite status colors
#define UI_COLOR_SAT_SEARCHING  0x808080  // Gray
#define UI_COLOR_SAT_TRACKING   0xFFFF00  // Yellow
#define UI_COLOR_SAT_USED       0x00FF00  // Green

// Signal strength colors
#define UI_COLOR_SIGNAL_EXCELLENT   0x00FF00  // Green (>45 dBHz)
#define UI_COLOR_SIGNAL_GOOD        0x9ACD32  // Yellow-Green (35-45)
#define UI_COLOR_SIGNAL_MEDIUM      0xFFFF00  // Yellow (25-35)
#define UI_COLOR_SIGNAL_WEAK        0xFFA500  // Orange (15-25)
#define UI_COLOR_SIGNAL_POOR        0xFF0000  // Red (<15)

// Accuracy colors (HDOP)
#define UI_COLOR_ACCURACY_EXCELLENT 0x00FF00  // Green (<2.0)
#define UI_COLOR_ACCURACY_GOOD      0xFFFF00  // Yellow (2.0-5.0)
#define UI_COLOR_ACCURACY_POOR      0xFF0000  // Red (>5.0)

// General UI colors
#define UI_COLOR_GPS_NO_FIX     0xFF0000  // Red
#define UI_COLOR_GPS_FIX        0x00FF00  // Green
#define UI_COLOR_RECORDING      0xFF0000  // Red (flashing)
#define UI_COLOR_NOT_RECORDING  0x00FF00  // Green
#define UI_COLOR_WARNING        0xFFA500  // Orange
#define UI_COLOR_TEXT_PRIMARY   0xFFFFFF  // White
#define UI_COLOR_TEXT_SECONDARY 0x808080  // Gray
#define UI_COLOR_BACKGROUND     0x000000  // Black

#endif // UI_COMMON_H
