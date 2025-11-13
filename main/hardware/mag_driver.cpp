/**
 * @file mag_driver.cpp
 * @brief LIS2MDL magnetometer driver implementation
 */

#include "mag_driver.h"
#include "config.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "MAG";

// LIS2MDL registers
#define LIS2MDL_WHO_AM_I        0x4F
#define LIS2MDL_CFG_REG_A       0x60
#define LIS2MDL_CFG_REG_C       0x62
#define LIS2MDL_OUTX_L_REG      0x68
#define LIS2MDL_TEMP_OUT_L_REG  0x6E

#define LIS2MDL_ID              0x40

// Calibration parameters (X/Y swap then Y invert, Z invert per spec)
static float mag_offset[3] = {0.0f, 0.0f, 0.0f};
static float mag_scale[3] = {1.0f, 1.0f, 1.0f};
static bool mag_initialized = false;

static esp_err_t mag_write_reg(uint8_t reg, uint8_t data) {
    uint8_t write_buf[2] = {reg, data};
    return i2c_master_write_to_device(I2C_NUM, LIS2MDL_I2C_ADDR >> 1, write_buf, 2, pdMS_TO_TICKS(100));
}

static esp_err_t mag_read_reg(uint8_t reg, uint8_t* data, size_t len) {
    return i2c_master_write_read_device(I2C_NUM, LIS2MDL_I2C_ADDR >> 1, &reg, 1, data, len, pdMS_TO_TICKS(100));
}

bool mag_init(void) {
    uint8_t who_am_i = 0;
    
    if (mag_read_reg(LIS2MDL_WHO_AM_I, &who_am_i, 1) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read WHO_AM_I");
        return false;
    }
    
    if (who_am_i != LIS2MDL_ID) {
        ESP_LOGE(TAG, "Unexpected WHO_AM_I: 0x%02X", who_am_i);
        return false;
    }
    
    // Configure: continuous mode, 100Hz, temperature compensation
    if (mag_write_reg(LIS2MDL_CFG_REG_A, 0x8C) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure");
        return false;
    }
    
    // Enable BDU
    if (mag_write_reg(LIS2MDL_CFG_REG_C, 0x10) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable BDU");
        return false;
    }
    
    ESP_LOGI(TAG, "LIS2MDL initialized");
    mag_initialized = true;
    
    return true;
}

bool mag_read(MagData* data) {
    if (!mag_initialized || !data) return false;
    
    uint8_t raw_data[6];
    int16_t raw_mag[3];
    
    if (mag_read_reg(LIS2MDL_OUTX_L_REG, raw_data, 6) != ESP_OK) {
        return false;
    }
    
    raw_mag[0] = (int16_t)(raw_data[0] | (raw_data[1] << 8));
    raw_mag[1] = (int16_t)(raw_data[2] | (raw_data[3] << 8));
    raw_mag[2] = (int16_t)(raw_data[4] | (raw_data[5] << 8));
    
    // Convert to µT and apply calibration
    // Axis transformation: swap X/Y then invert Y, invert Z
    float mag_x_raw = raw_mag[1] * 1.5f;  // Y -> X
    float mag_y_raw = -raw_mag[0] * 1.5f; // X -> -Y
    float mag_z_raw = -raw_mag[2] * 1.5f; // -Z
    
    data->mag_x = (mag_x_raw - mag_offset[0]) * mag_scale[0];
    data->mag_y = (mag_y_raw - mag_offset[1]) * mag_scale[1];
    data->mag_z = (mag_z_raw - mag_offset[2]) * mag_scale[2];
    data->timestamp = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    return true;
}

float mag_get_temperature(void) {
    if (!mag_initialized) return 0.0f;
    
    uint8_t raw_temp[2];
    if (mag_read_reg(LIS2MDL_TEMP_OUT_L_REG, raw_temp, 2) != ESP_OK) {
        return 0.0f;
    }
    
    int16_t temp_raw = (int16_t)(raw_temp[0] | (raw_temp[1] << 8));
    return 25.0f + (temp_raw / 8.0f);
}

void mag_set_calibration(float offset_x, float offset_y, float offset_z,
                         float scale_x, float scale_y, float scale_z) {
    mag_offset[0] = offset_x;
    mag_offset[1] = offset_y;
    mag_offset[2] = offset_z;
    mag_scale[0] = scale_x;
    mag_scale[1] = scale_y;
    mag_scale[2] = scale_z;
}

void mag_get_calibration(float* offset_x, float* offset_y, float* offset_z,
                         float* scale_x, float* scale_y, float* scale_z) {
    if (offset_x) *offset_x = mag_offset[0];
    if (offset_y) *offset_y = mag_offset[1];
    if (offset_z) *offset_z = mag_offset[2];
    if (scale_x) *scale_x = mag_scale[0];
    if (scale_y) *scale_y = mag_scale[1];
    if (scale_z) *scale_z = mag_scale[2];
}
