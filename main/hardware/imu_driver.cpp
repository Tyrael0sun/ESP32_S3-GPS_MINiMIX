/**
 * @file imu_driver.cpp
 * @brief LSM6DSR IMU driver implementation
 */

#include "imu_driver.h"
#include "config.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include <cstring>

static const char* TAG = "IMU";

// LSM6DSR registers
#define LSM6DSR_WHO_AM_I        0x0F
#define LSM6DSR_CTRL1_XL        0x10
#define LSM6DSR_CTRL2_G         0x11
#define LSM6DSR_OUT_TEMP_L      0x20
#define LSM6DSR_OUTX_L_G        0x22
#define LSM6DSR_OUTX_L_A        0x28

// Expected WHO_AM_I value
#define LSM6DSR_ID              0x6B

// Calibration offsets (X, Z axis inverted, Y unchanged per spec)
static float acc_offset[3] = {0.0f, 0.0f, 0.0f};

static bool imu_initialized = false;

static esp_err_t imu_write_reg(uint8_t reg, uint8_t data) {
    uint8_t write_buf[2] = {reg, data};
    return i2c_master_write_to_device(I2C_NUM, LSM6DSR_I2C_ADDR >> 1, write_buf, 2, pdMS_TO_TICKS(100));
}

static esp_err_t imu_read_reg(uint8_t reg, uint8_t* data, size_t len) {
    return i2c_master_write_read_device(I2C_NUM, LSM6DSR_I2C_ADDR >> 1, &reg, 1, data, len, pdMS_TO_TICKS(100));
}

bool imu_init(void) {
    uint8_t who_am_i = 0;
    
    // Read WHO_AM_I
    if (imu_read_reg(LSM6DSR_WHO_AM_I, &who_am_i, 1) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read WHO_AM_I");
        return false;
    }
    
    if (who_am_i != LSM6DSR_ID) {
        ESP_LOGE(TAG, "Unexpected WHO_AM_I: 0x%02X (expected 0x%02X)", who_am_i, LSM6DSR_ID);
        return false;
    }
    
    // Configure accelerometer: 104Hz, ±4g
    if (imu_write_reg(LSM6DSR_CTRL1_XL, 0x50) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure accelerometer");
        return false;
    }
    
    // Configure gyroscope: 104Hz, ±500dps
    if (imu_write_reg(LSM6DSR_CTRL2_G, 0x54) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure gyroscope");
        return false;
    }
    
    ESP_LOGI(TAG, "LSM6DSR initialized (WHO_AM_I=0x%02X)", who_am_i);
    imu_initialized = true;
    
    return true;
}

bool imu_read(ImuData* data) {
    if (!imu_initialized || !data) return false;
    
    uint8_t raw_data[12];
    int16_t raw_gyro[3], raw_acc[3];
    
    // Read gyroscope data
    if (imu_read_reg(LSM6DSR_OUTX_L_G, raw_data, 6) != ESP_OK) {
        return false;
    }
    raw_gyro[0] = (int16_t)(raw_data[0] | (raw_data[1] << 8));
    raw_gyro[1] = (int16_t)(raw_data[2] | (raw_data[3] << 8));
    raw_gyro[2] = (int16_t)(raw_data[4] | (raw_data[5] << 8));
    
    // Read accelerometer data
    if (imu_read_reg(LSM6DSR_OUTX_L_A, raw_data, 6) != ESP_OK) {
        return false;
    }
    raw_acc[0] = (int16_t)(raw_data[0] | (raw_data[1] << 8));
    raw_acc[1] = (int16_t)(raw_data[2] | (raw_data[3] << 8));
    raw_acc[2] = (int16_t)(raw_data[4] | (raw_data[5] << 8));
    
    // Convert to physical units (±4g range, ±500dps range)
    // Apply axis transformation: X and Z inverted, Y unchanged
    data->acc_x = -(raw_acc[0] * 4.0f / 32768.0f) * 9.81f - acc_offset[0];
    data->acc_y = (raw_acc[1] * 4.0f / 32768.0f) * 9.81f - acc_offset[1];
    data->acc_z = -(raw_acc[2] * 4.0f / 32768.0f) * 9.81f - acc_offset[2];
    
    data->gyro_x = -raw_gyro[0] * 500.0f / 32768.0f;
    data->gyro_y = raw_gyro[1] * 500.0f / 32768.0f;
    data->gyro_z = -raw_gyro[2] * 500.0f / 32768.0f;
    
    data->timestamp = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    return true;
}

float imu_get_temperature(void) {
    if (!imu_initialized) return 0.0f;
    
    uint8_t raw_temp[2];
    if (imu_read_reg(LSM6DSR_OUT_TEMP_L, raw_temp, 2) != ESP_OK) {
        return 0.0f;
    }
    
    int16_t temp_raw = (int16_t)(raw_temp[0] | (raw_temp[1] << 8));
    return 25.0f + (temp_raw / 256.0f);
}

void imu_set_acc_offset(float x, float y, float z) {
    acc_offset[0] = x;
    acc_offset[1] = y;
    acc_offset[2] = z;
}

void imu_get_acc_offset(float* x, float* y, float* z) {
    if (x) *x = acc_offset[0];
    if (y) *y = acc_offset[1];
    if (z) *z = acc_offset[2];
}
