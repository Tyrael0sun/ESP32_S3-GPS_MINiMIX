/**
 * @file display_driver.h
 * @brief ST7789 LCD display driver interface
 */

#ifndef DISPLAY_DRIVER_H
#define DISPLAY_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Initialize the ST7789 display
 * @return true on success, false on failure
 */
bool display_init(void);

/**
 * @brief Set backlight brightness
 * @param duty_percent Brightness percentage (0-100)
 */
void display_set_backlight(uint8_t duty_percent);

/**
 * @brief Get display width
 * @return Display width in pixels
 */
uint16_t display_get_width(void);

/**
 * @brief Get display height
 * @return Display height in pixels
 */
uint16_t display_get_height(void);

/**
 * @brief Flush display buffer (callback for LVGL)
 */
void display_flush_cb(void* disp_drv, const void* area, void* color_p);

#endif // DISPLAY_DRIVER_H
