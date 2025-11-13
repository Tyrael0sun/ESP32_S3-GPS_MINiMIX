/**
 * @file sensor_fusion.cpp
 * @brief Sensor fusion implementation using complementary filter
 */

#include "sensor_fusion.h"
#include "../hardware/imu_driver.h"
#include "../hardware/mag_driver.h"
#include "esp_log.h"
#include <cmath>

static const char* TAG = "FUSION";

static FusedData fused_data = {0};
static bool fusion_initialized = false;

// Complementary filter coefficients
static const float ALPHA = 0.98f;

// Current orientation estimate
static float roll_estimate = 0.0f;
static float pitch_estimate = 0.0f;
static float yaw_estimate = 0.0f;

bool sensor_fusion_init(void) {
    ESP_LOGI(TAG, "Sensor fusion initialized");
    fusion_initialized = true;
    return true;
}

bool sensor_fusion_update(float dt) {
    if (!fusion_initialized) return false;
    
    ImuData imu;
    MagData mag;
    
    // Read sensor data
    if (!imu_read(&imu)) {
        return false;
    }
    
    mag_read(&mag); // Non-critical if fails
    
    // Calculate gravity vector using low-pass filter
    static float grav_x = 0.0f, grav_y = 0.0f, grav_z = 9.81f;
    grav_x = ALPHA * grav_x + (1.0f - ALPHA) * imu.acc_x;
    grav_y = ALPHA * grav_y + (1.0f - ALPHA) * imu.acc_y;
    grav_z = ALPHA * grav_z + (1.0f - ALPHA) * imu.acc_z;
    
    fused_data.gravity_x = grav_x;
    fused_data.gravity_y = grav_y;
    fused_data.gravity_z = grav_z;
    
    // Calculate linear acceleration (remove gravity)
    fused_data.linear_acc_x = imu.acc_x - grav_x;
    fused_data.linear_acc_y = imu.acc_y - grav_y;
    fused_data.linear_acc_z = imu.acc_z - grav_z;
    
    // Calculate orientation from accelerometer
    float acc_roll = atan2f(imu.acc_y, imu.acc_z) * 180.0f / M_PI;
    float acc_pitch = atan2f(-imu.acc_x, sqrtf(imu.acc_y * imu.acc_y + imu.acc_z * imu.acc_z)) * 180.0f / M_PI;
    
    // Integrate gyroscope
    roll_estimate += imu.gyro_x * dt;
    pitch_estimate += imu.gyro_y * dt;
    yaw_estimate += imu.gyro_z * dt;
    
    // Complementary filter fusion
    roll_estimate = ALPHA * roll_estimate + (1.0f - ALPHA) * acc_roll;
    pitch_estimate = ALPHA * pitch_estimate + (1.0f - ALPHA) * acc_pitch;
    
    fused_data.roll = roll_estimate;
    fused_data.pitch = pitch_estimate;
    fused_data.yaw = yaw_estimate;
    
    // Calculate heading from magnetometer
    if (mag_read(&mag)) {
        // Tilt compensation
        float mx = mag.mag_x;
        float my = mag.mag_y;
        float mz = mag.mag_z;
        
        float roll_rad = roll_estimate * M_PI / 180.0f;
        float pitch_rad = pitch_estimate * M_PI / 180.0f;
        
        float mx_comp = mx * cosf(pitch_rad) + mz * sinf(pitch_rad);
        float my_comp = mx * sinf(roll_rad) * sinf(pitch_rad) + 
                       my * cosf(roll_rad) - 
                       mz * sinf(roll_rad) * cosf(pitch_rad);
        
        float heading = atan2f(my_comp, mx_comp) * 180.0f / M_PI;
        if (heading < 0) heading += 360.0f;
        
        fused_data.heading = heading;
    }
    
    // Calculate forward acceleration (assuming vehicle frame X-axis is forward)
    fused_data.forward_accel_g = fused_data.linear_acc_x / 9.81f;
    
    fused_data.timestamp = imu.timestamp;
    
    return true;
}

bool sensor_fusion_get_data(FusedData* data) {
    if (!fusion_initialized || !data) return false;
    *data = fused_data;
    return true;
}

float sensor_fusion_get_forward_accel_g(void) {
    return fused_data.forward_accel_g;
}
