/**
 * @file sensor_fusion.h
 * @brief Sensor fusion for IMU, magnetometer, and GPS data
 */

#ifndef SENSOR_FUSION_H
#define SENSOR_FUSION_H

#include <stdint.h>
#include <stdbool.h>

struct FusedData {
    // Linear acceleration (without gravity)
    float linear_acc_x, linear_acc_y, linear_acc_z; // m/s^2
    
    // Gravity vector
    float gravity_x, gravity_y, gravity_z; // m/s^2
    
    // Orientation (Euler angles)
    float roll, pitch, yaw; // degrees
    
    // Heading from magnetometer
    float heading; // degrees (0-360)
    
    // Forward acceleration (in vehicle frame)
    float forward_accel_g; // G-force
    
    uint32_t timestamp; // ms
};

/**
 * @brief Initialize sensor fusion
 * @return true on success, false on failure
 */
bool sensor_fusion_init(void);

/**
 * @brief Update sensor fusion with new data
 * @param dt Time delta in seconds
 * @return true on success, false on failure
 */
bool sensor_fusion_update(float dt);

/**
 * @brief Get fused sensor data
 * @param data Pointer to FusedData structure
 * @return true on success, false on failure
 */
bool sensor_fusion_get_data(FusedData* data);

/**
 * @brief Get forward acceleration (for P-Box)
 * @return Forward acceleration in G
 */
float sensor_fusion_get_forward_accel_g(void);

#endif // SENSOR_FUSION_H
