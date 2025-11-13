/**
 * @file gps_logger.h
 * @brief GPS track logging to GPX format
 */

#ifndef GPS_LOGGER_H
#define GPS_LOGGER_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Initialize GPS logger
 * @return true on success, false on failure
 */
bool gps_logger_init(void);

/**
 * @brief Start logging GPS track
 * @return true on success, false on failure
 */
bool gps_logger_start(void);

/**
 * @brief Stop logging GPS track
 * @return true on success, false on failure
 */
bool gps_logger_stop(void);

/**
 * @brief Check if currently logging
 * @return true if logging
 */
bool gps_logger_is_logging(void);

/**
 * @brief Log a GPS point
 * @return true on success, false on failure
 */
bool gps_logger_log_point(void);

/**
 * @brief Get current track distance
 * @return Distance in km
 */
float gps_logger_get_distance(void);

/**
 * @brief Get current track duration
 * @return Duration in seconds
 */
uint32_t gps_logger_get_duration(void);

#endif // GPS_LOGGER_H
