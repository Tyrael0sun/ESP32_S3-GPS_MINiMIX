/**
 * @file baro_driver.cpp
 * @brief BMP388 barometer driver implementation
 */

#include "baro_driver.h"
#include "config.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cmath>

static const char* TAG = "BARO";

// BMP388 registers
#define BMP388_CHIP_ID          0x00
#define BMP388_PWR_CTRL         0x1B
#define BMP388_OSR              0x1C
#define BMP388_ODR              0x1D
#define BMP388_DATA_0           0x04

#define BMP388_ID               0x50

static bool baro_initialized = false;
static float reference_pressure = 1013.25f; // Sea level pressure in hPa

static esp_err_t baro_write_reg(uint8_t reg, uint8_t data) {
    uint8_t write_buf[2] = {reg, data};
    return i2c_master_write_to_device(I2C_NUM, BMP388_I2C_ADDR >> 1, write_buf, 2, pdMS_TO_TICKS(100));
}

static esp_err_t baro_read_reg(uint8_t reg, uint8_t* data, size_t len) {
    return i2c_master_write_read_device(I2C_NUM, BMP388_I2C_ADDR >> 1, &reg, 1, data, len, pdMS_TO_TICKS(100));
}

bool baro_init(void) {
    uint8_t chip_id = 0;
    
    if (baro_read_reg(BMP388_CHIP_ID, &chip_id, 1) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read CHIP_ID");
        return false;
    }
    
    if (chip_id != BMP388_ID) {
        ESP_LOGE(TAG, "Unexpected CHIP_ID: 0x%02X", chip_id);
        return false;
    }
    
    // Power on: enable pressure and temperature
    if (baro_write_reg(BMP388_PWR_CTRL, 0x33) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to power on");
        return false;
    }
    
    // Wait for sensor to stabilize
    vTaskDelay(pdMS_TO_TICKS(50));
    
    // Configure oversampling
    if (baro_write_reg(BMP388_OSR, 0x05) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure OSR");
        return false;
    }
    
    // Configure ODR (50Hz)
    if (baro_write_reg(BMP388_ODR, 0x02) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure ODR");
        return false;
    }
    
    ESP_LOGI(TAG, "BMP388 initialized");
    baro_initialized = true;
    
    return true;
}

bool baro_read(BaroData* data) {
    if (!baro_initialized || !data) return false;
    
    uint8_t raw_data[6];
    
    if (baro_read_reg(BMP388_DATA_0, raw_data, 6) != ESP_OK) {
        return false;
    }
    
    // Parse raw data (BMP388 returns 24-bit values)
    uint32_t raw_press = raw_data[0] | (raw_data[1] << 8) | (raw_data[2] << 16);
    uint32_t raw_temp = raw_data[3] | (raw_data[4] << 8) | (raw_data[5] << 16);
    
    // TODO: Implement proper BMP388 compensation using calibration coefficients
    // For now, return reasonable default values to prevent crashes
    // Typical sea level pressure is ~1013 hPa, temperature around 25°C
    if (raw_press == 0 || raw_press == 0xFFFFFF) {
        data->pressure = 1013.25f;
        data->temperature = 25.0f;
    } else {
        // Very rough approximation - needs proper calibration implementation
        data->pressure = 1013.25f + ((int32_t)raw_press - 8000000) / 10000.0f;
        data->temperature = 25.0f + ((int32_t)raw_temp - 8000000) / 100000.0f;
    }
    
    // Calculate altitude using barometric formula
    data->altitude = 44330.0f * (1.0f - powf(data->pressure / reference_pressure, 0.1903f));
    data->timestamp = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    return true;
}

void baro_set_reference_pressure(float pressure) {
    reference_pressure = pressure;
}
