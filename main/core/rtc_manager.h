/**
 * @file rtc_manager.h
 * @brief RTC time management and GPS synchronization
 */

#ifndef RTC_MANAGER_H
#define RTC_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

/**
 * @brief Initialize RTC with compile time
 * @return true on success, false on failure
 */
bool rtc_init(void);

/**
 * @brief Sync RTC with GPS time
 * @return true if sync was successful
 */
bool rtc_sync_with_gps(void);

/**
 * @brief Get current time
 * @param timeinfo Pointer to tm structure
 * @return true on success
 */
bool rtc_get_time(struct tm* timeinfo);

/**
 * @brief Set RTC time manually
 * @param timeinfo Pointer to tm structure
 * @return true on success
 */
bool rtc_set_time(const struct tm* timeinfo);

/**
 * @brief Check if RTC has been synced with GPS
 * @return true if synced
 */
bool rtc_is_synced(void);

#endif // RTC_MANAGER_H
