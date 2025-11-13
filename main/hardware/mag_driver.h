/**
 * @file mag_driver.h
 * @brief LIS2MDL Magnetometer driver
 */

#ifndef MAG_DRIVER_H
#define MAG_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

struct MagData {
    float mag_x, mag_y, mag_z;  // µT
    float temperature;          // °C
    uint32_t timestamp;         // ms
};

/**
 * @brief Initialize LIS2MDL magnetometer
 * @return true on success, false on failure
 */
bool mag_init(void);

/**
 * @brief Read magnetometer data
 * @param data Pointer to MagData structure
 * @return true on success, false on failure
 */
bool mag_read(MagData* data);

/**
 * @brief Get magnetometer temperature
 * @return Temperature in °C
 */
float mag_get_temperature(void);

/**
 * @brief Set magnetometer calibration
 */
void mag_set_calibration(float offset_x, float offset_y, float offset_z,
                         float scale_x, float scale_y, float scale_z);

/**
 * @brief Get magnetometer calibration
 */
void mag_get_calibration(float* offset_x, float* offset_y, float* offset_z,
                         float* scale_x, float* scale_y, float* scale_z);

#endif // MAG_DRIVER_H
