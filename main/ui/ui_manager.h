/**
 * @file ui_manager.h
 * @brief LVGL UI manager
 */

#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

enum AppMode {
    MODE_BIKE_COMPUTER = 0,
    MODE_GPS_LOGGER,
    MODE_PBOX,
    MODE_GNSS_INFO,
    MODE_SETTINGS
};

/**
 * @brief Initialize UI manager and LVGL
 * @return true on success, false on failure
 */
bool ui_manager_init(void);

/**
 * @brief Switch to a different app mode
 * @param mode Target mode
 */
void ui_manager_switch_mode(AppMode mode);

/**
 * @brief Get current app mode
 * @return Current mode
 */
AppMode ui_manager_get_mode(void);

/**
 * @brief Update UI (call from main loop)
 */
void ui_manager_update(void);

/**
 * @brief Show time sync notification
 */
void ui_manager_show_time_sync(void);

#endif // UI_MANAGER_H
