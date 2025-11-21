#ifndef SENSORS_H
#define SENSORS_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

// I2C Addresses
#include "config.h"

// Sensor WHO_AM_I values
#define LSM6DSR_WHO_AM_I_VAL    0x6B
#define LIS2MDL_WHO_AM_I_VAL    0x40
#define BMP388_WHO_AM_I_VAL     0x50

esp_err_t sensors_init(void);

bool sensors_check_imu(void);
bool sensors_check_mag(void);
bool sensors_check_baro(void);

/**
 * @brief Read IMU data
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
 * @brief Read Magnetometer data
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

// Helper functions for derived data
void sensors_calc_gravity_linear(float ax, float ay, float az, float *grav_x, float *grav_y, float *grav_z, float *lin_x, float *lin_y, float *lin_z);
float sensors_calc_heading(float mx, float my);
float sensors_calc_altitude(float pressure_hpa, float temp_c);

#endif // SENSORS_H
