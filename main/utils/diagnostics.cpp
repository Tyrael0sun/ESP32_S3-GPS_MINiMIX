/**
 * @file diagnostics.cpp
 * @brief Diagnostics implementation per F-DIAG requirements
 */

#include "diagnostics.h"
#include "config.h"
#include "../hardware/gnss_driver.h"
#include "../hardware/imu_driver.h"
#include "../hardware/mag_driver.h"
#include "../hardware/baro_driver.h"
#include "../core/sensor_fusion.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"

static const char* TAG = "DIAG";

static uint32_t boot_time = 0;
static TaskHandle_t diag_task_handle = NULL;

void diagnostics_init(void) {
    boot_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    ESP_LOGI(TAG, "Diagnostics initialized");
}

void diagnostics_run(void) {
    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    // Get GNSS status
    GnssData gps;
    bool gps_ok = gnss_read(&gps);
    
    // Get IMU data
    ImuData imu;
    bool imu_ok = imu_read(&imu);
    float imu_temp = imu_get_temperature();
    
    // Get magnetometer data
    MagData mag;
    bool mag_ok = mag_read(&mag);
    float mag_temp = mag_get_temperature();
    
    // Get barometer data
    BaroData baro;
    bool baro_ok = baro_read(&baro);
    
    // Get sensor fusion data
    FusedData fusion;
    sensor_fusion_get_data(&fusion);
    
    // Get MCU temperature
    float mcu_temp = 0.0f; // ESP32-S3 internal temp sensor
    
    // Log diagnostic report
    ESP_LOGI(TAG, "[DIAG][T=%lu ms]", now);
    
    if (gps_ok && gps.fix_valid) {
        ESP_LOGI(TAG, "GNSS: OK, %d sats, (%.4f,%.4f)", 
                 gps.satellites, gps.latitude, gps.longitude);
    } else {
        ESP_LOGI(TAG, "GNSS: NO FIX, %d sats", gnss_get_satellites());
    }
    
    if (imu_ok) {
        ESP_LOGI(TAG, "IMU: ACC(L:%.2f,%.2f,%.2f) GRAV(%.2f,%.2f,%.2f)",
                 fusion.linear_acc_x, fusion.linear_acc_y, fusion.linear_acc_z,
                 fusion.gravity_x, fusion.gravity_y, fusion.gravity_z);
        ESP_LOGI(TAG, "GYR(%.1f,%.1f,%.1f)",
                 imu.gyro_x, imu.gyro_y, imu.gyro_z);
    } else {
        ESP_LOGI(TAG, "IMU: ERROR");
    }
    
    if (mag_ok) {
        ESP_LOGI(TAG, "MAG: (%.1f,%.1f,%.1f) Heading: %.1f°",
                 mag.mag_x, mag.mag_y, mag.mag_z, fusion.heading);
    } else {
        ESP_LOGI(TAG, "MAG: ERROR");
    }
    
    if (baro_ok) {
        ESP_LOGI(TAG, "BARO: %.1fhPa, Alt: %.1fm", baro.pressure, baro.altitude);
    } else {
        ESP_LOGI(TAG, "BARO: ERROR");
    }
    
    ESP_LOGI(TAG, "TEMP: MCU=%.1f°C, IMU=%.1f°C, BARO=%.1f°C, MAG=%.1f°C",
             mcu_temp, imu_temp, baro.temperature, mag_temp);
    
    ESP_LOGI(TAG, "RESULT: %s", 
             (gps_ok && imu_ok && mag_ok && baro_ok) ? "OK" : "DEGRADED");
    ESP_LOGI(TAG, ""); // Blank line
}

static void diagnostics_task(void* arg) {
    uint32_t interval_ms;
    
    while (1) {
        uint32_t elapsed = (xTaskGetTickCount() * portTICK_PERIOD_MS) - boot_time;
        
        // High frequency logging for first 5 seconds (F-DIAG-01)
        if (elapsed < DIAG_FAST_LOG_DURATION_MS) {
            interval_ms = DIAG_FAST_LOG_INTERVAL_MS;
        } else {
            interval_ms = DIAG_SLOW_LOG_INTERVAL_MS;
        }
        
        diagnostics_run();
        vTaskDelay(pdMS_TO_TICKS(interval_ms));
    }
}

void diagnostics_start_task(void) {
    xTaskCreate(diagnostics_task, "diagnostics", 
                STACK_SIZE_DIAG, NULL, 
                TASK_PRIORITY_DIAG, &diag_task_handle);
    ESP_LOGI(TAG, "Diagnostics task started");
}

void diagnostics_trigger(const char* reason) {
    ESP_LOGI(TAG, "=== TRIGGERED: %s ===", reason ? reason : "Manual");
    diagnostics_run();
}
