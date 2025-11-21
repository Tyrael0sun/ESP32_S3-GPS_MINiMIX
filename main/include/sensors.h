#ifndef SENSORS_H
#define SENSORS_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

// I2C Addresses (from config or spec)
#include "config.h"

// Sensor WHO_AM_I default values
#define LSM6DSR_WHO_AM_I_VAL    0x6B
#define LIS2MDL_WHO_AM_I_VAL    0x40
#define BMP388_WHO_AM_I_VAL     0x50

/**
 * @brief Initialize I2C bus and sensors
 * @return esp_err_t ESP_OK on success
 */
esp_err_t sensors_init(void);

bool sensors_check_imu(void);
bool sensors_check_mag(void);
bool sensors_check_baro(void);

/**
 * @brief Read IMU data with axis transformation
 *
 * IMU: LSM6DSR (Z axis inverted, Y axis unchanged)
 * Assuming X axis is unchanged to maintain coordinate system or as default.
 *
 * @param ax Accel X (g)
 * @param ay Accel Y (g)
 * @param az Accel Z (g)
 * @param gx Gyro X (dps)
 * @param gy Gyro Y (dps)
 * @param gz Gyro Z (dps)
 * @param temp Temperature (deg C)
 * @return esp_err_t
 */
esp_err_t sensors_read_imu(float *ax, float *ay, float *az, float *gx, float *gy, float *gz, float *temp);

/**
 * @brief Read Magnetometer data with axis transformation
 *
 * Mag: LIS2MDL (X axis normal, Y axis inverted, Z axis inverted)
 * Note: Spec said "Y axis exchange and reverse", assuming "Y inverted".
 *
 * @param mx Mag X (uT)
 * @param my Mag Y (uT)
 * @param mz Mag Z (uT)
 * @param temp Temperature (deg C)
 * @return esp_err_t
 */
esp_err_t sensors_read_mag(float *mx, float *my, float *mz, float *temp);

/**
 * @brief Read Barometer data
 *
 * @param pressure Pressure (hPa)
 * @param temp Temperature (deg C)
 * @return esp_err_t
 */
esp_err_t sensors_read_baro(float *pressure, float *temp);

#endif // SENSORS_H
