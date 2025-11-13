/**
 * @file ui_statusbar.h
 * @brief Status bar UI showing GPS, SD card, battery
 */

#ifndef UI_STATUSBAR_H
#define UI_STATUSBAR_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Initialize status bar
 */
void ui_statusbar_init(void);

/**
 * @brief Update status bar display
 */
void ui_statusbar_update(void);

/**
 * @brief Deinitialize status bar
 */
void ui_statusbar_deinit(void);

#endif // UI_STATUSBAR_H
