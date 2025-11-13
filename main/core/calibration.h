/**
 * @file calibration.h
 * @brief Sensor calibration routines
 */

#ifndef CALIBRATION_H
#define CALIBRATION_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Initialize calibration system (load from NVS)
 * @return true on success, false on failure
 */
bool calibration_init(void);

/**
 * @brief Start IMU calibration process
 * @return true on success, false on failure
 */
bool calibration_start_imu(void);

/**
 * @brief Start magnetometer calibration process
 * @return true on success, false on failure
 */
bool calibration_start_mag(void);

/**
 * @brief Check if calibration is in progress
 * @return true if calibrating
 */
bool calibration_is_running(void);

/**
 * @brief Get calibration progress
 * @return Progress percentage (0-100)
 */
uint8_t calibration_get_progress(void);

/**
 * @brief Save calibration to NVS
 * @return true on success, false on failure
 */
bool calibration_save(void);

/**
 * @brief Load calibration from NVS
 * @return true on success, false on failure
 */
bool calibration_load(void);

#endif // CALIBRATION_H
