/**
 * @file imu_driver.h
 * @brief LSM6DSR IMU (Accelerometer + Gyroscope) driver
 */

#ifndef IMU_DRIVER_H
#define IMU_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

struct ImuData {
    float acc_x, acc_y, acc_z;      // m/s^2
    float gyro_x, gyro_y, gyro_z;   // deg/s
    float temperature;              // °C
    uint32_t timestamp;             // ms
};

/**
 * @brief Initialize LSM6DSR IMU
 * @return true on success, false on failure
 */
bool imu_init(void);

/**
 * @brief Read IMU data
 * @param data Pointer to ImuData structure
 * @return true on success, false on failure
 */
bool imu_read(ImuData* data);

/**
 * @brief Get IMU temperature
 * @return Temperature in °C
 */
float imu_get_temperature(void);

/**
 * @brief Set accelerometer calibration offset
 */
void imu_set_acc_offset(float x, float y, float z);

/**
 * @brief Get accelerometer calibration offset
 */
void imu_get_acc_offset(float* x, float* y, float* z);

#endif // IMU_DRIVER_H
