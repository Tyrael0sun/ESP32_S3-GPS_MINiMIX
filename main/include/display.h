#ifndef DISPLAY_H
#define DISPLAY_H

#include "esp_err.h"
#include "lvgl.h"

/**
 * @brief Initialize Display (SPI + ST7789) and LVGL
 *
 * @return esp_err_t ESP_OK on success
 */
esp_err_t display_init(void);

/**
 * @brief Lock the LVGL mutex (for thread safety if needed in future)
 * @param timeout_ms
 * @return true if lock acquired
 */
bool display_lock(int timeout_ms);

/**
 * @brief Unlock the LVGL mutex
 */
void display_unlock(void);

#endif // DISPLAY_H
