/**
 * @file sdcard_driver.h
 * @brief SD card driver (SDIO 4-bit mode)
 */

#ifndef SDCARD_DRIVER_H
#define SDCARD_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Initialize SD card
 * @return true on success, false on failure
 */
bool sdcard_init(void);

/**
 * @brief Check if SD card is present
 * @return true if card is present
 */
bool sdcard_is_present(void);

/**
 * @brief Get SD card size
 * @return Card size in MB
 */
uint32_t sdcard_get_size_mb(void);

/**
 * @brief Get free space on SD card
 * @return Free space in MB
 */
uint32_t sdcard_get_free_space_mb(void);

/**
 * @brief Deinitialize SD card
 */
void sdcard_deinit(void);

#endif // SDCARD_DRIVER_H
