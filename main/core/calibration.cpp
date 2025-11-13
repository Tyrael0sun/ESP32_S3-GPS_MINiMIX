/**
 * @file calibration.cpp
 * @brief Sensor calibration implementation with NVS storage
 */

#include "calibration.h"
#include "config.h"
#include "../hardware/imu_driver.h"
#include "../hardware/mag_driver.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cmath>

static const char* TAG = "CALIB";

static bool calib_running = false;
static uint8_t calib_progress = 0;
static TaskHandle_t calib_task_handle = NULL;

// Calibration task for IMU
static void imu_calibration_task(void* arg) {
    ESP_LOGI(TAG, "Starting IMU calibration...");
    calib_progress = 0;
    
    float acc_sum[3] = {0};
    int samples = 0;
    const int REQUIRED_SAMPLES = 1000;
    
    // Collect samples while device is stationary
    while (samples < REQUIRED_SAMPLES) {
        ImuData imu;
        if (imu_read(&imu)) {
            acc_sum[0] += imu.acc_x;
            acc_sum[1] += imu.acc_y;
            acc_sum[2] += imu.acc_z;
            samples++;
            calib_progress = (samples * 100) / REQUIRED_SAMPLES;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
    // Calculate offsets (assuming Z should be +9.81 m/s^2)
    float acc_offset_x = acc_sum[0] / samples;
    float acc_offset_y = acc_sum[1] / samples;
    float acc_offset_z = (acc_sum[2] / samples) - 9.81f;
    
    // Apply calibration
    imu_set_acc_offset(acc_offset_x, acc_offset_y, acc_offset_z);
    
    ESP_LOGI(TAG, "IMU calibration complete: X=%.3f Y=%.3f Z=%.3f", 
             acc_offset_x, acc_offset_y, acc_offset_z);
    
    calib_running = false;
    calib_task_handle = NULL;
    vTaskDelete(NULL);
}

// Calibration task for magnetometer
static void mag_calibration_task(void* arg) {
    ESP_LOGI(TAG, "Starting magnetometer calibration...");
    ESP_LOGI(TAG, "Please rotate device in figure-8 pattern...");
    
    calib_progress = 0;
    
    float mag_min[3] = {1000, 1000, 1000};
    float mag_max[3] = {-1000, -1000, -1000};
    
    int samples = 0;
    const int REQUIRED_SAMPLES = 500;
    
    while (samples < REQUIRED_SAMPLES) {
        MagData mag;
        if (mag_read(&mag)) {
            // Track min/max for each axis
            mag_min[0] = fminf(mag_min[0], mag.mag_x);
            mag_min[1] = fminf(mag_min[1], mag.mag_y);
            mag_min[2] = fminf(mag_min[2], mag.mag_z);
            
            mag_max[0] = fmaxf(mag_max[0], mag.mag_x);
            mag_max[1] = fmaxf(mag_max[1], mag.mag_y);
            mag_max[2] = fmaxf(mag_max[2], mag.mag_z);
            
            samples++;
            calib_progress = (samples * 100) / REQUIRED_SAMPLES;
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
    
    // Calculate offset (center of range) and scale
    float offset_x = (mag_max[0] + mag_min[0]) / 2.0f;
    float offset_y = (mag_max[1] + mag_min[1]) / 2.0f;
    float offset_z = (mag_max[2] + mag_min[2]) / 2.0f;
    
    float range_x = mag_max[0] - mag_min[0];
    float range_y = mag_max[1] - mag_min[1];
    float range_z = mag_max[2] - mag_min[2];
    
    float avg_range = (range_x + range_y + range_z) / 3.0f;
    
    float scale_x = avg_range / range_x;
    float scale_y = avg_range / range_y;
    float scale_z = avg_range / range_z;
    
    // Apply calibration
    mag_set_calibration(offset_x, offset_y, offset_z, scale_x, scale_y, scale_z);
    
    ESP_LOGI(TAG, "Mag calibration complete: Offset=(%.2f,%.2f,%.2f) Scale=(%.3f,%.3f,%.3f)",
             offset_x, offset_y, offset_z, scale_x, scale_y, scale_z);
    
    calib_running = false;
    calib_task_handle = NULL;
    vTaskDelete(NULL);
}

bool calibration_init(void) {
    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    
    // Load calibration from NVS
    calibration_load();
    
    ESP_LOGI(TAG, "Calibration system initialized");
    return true;
}

bool calibration_start_imu(void) {
    if (calib_running) {
        ESP_LOGW(TAG, "Calibration already in progress");
        return false;
    }
    
    calib_running = true;
    xTaskCreate(imu_calibration_task, "imu_calib", 4096, NULL, 3, &calib_task_handle);
    
    return true;
}

bool calibration_start_mag(void) {
    if (calib_running) {
        ESP_LOGW(TAG, "Calibration already in progress");
        return false;
    }
    
    calib_running = true;
    xTaskCreate(mag_calibration_task, "mag_calib", 4096, NULL, 3, &calib_task_handle);
    
    return true;
}

bool calibration_is_running(void) {
    return calib_running;
}

uint8_t calibration_get_progress(void) {
    return calib_progress;
}

bool calibration_save(void) {
    nvs_handle_t nvs_handle;
    esp_err_t err;
    
    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS");
        return false;
    }
    
    // Save IMU calibration
    float acc_offset[3];
    imu_get_acc_offset(&acc_offset[0], &acc_offset[1], &acc_offset[2]);
    err = nvs_set_blob(nvs_handle, NVS_KEY_ACC_OFFSET, acc_offset, sizeof(acc_offset));
    
    // Save magnetometer calibration
    float mag_offset[3], mag_scale[3];
    mag_get_calibration(&mag_offset[0], &mag_offset[1], &mag_offset[2],
                        &mag_scale[0], &mag_scale[1], &mag_scale[2]);
    nvs_set_blob(nvs_handle, NVS_KEY_MAG_OFFSET, mag_offset, sizeof(mag_offset));
    nvs_set_blob(nvs_handle, NVS_KEY_MAG_SCALE, mag_scale, sizeof(mag_scale));
    
    // Commit
    err = nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
    
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Calibration saved to NVS");
        return true;
    }
    
    return false;
}

bool calibration_load(void) {
    nvs_handle_t nvs_handle;
    esp_err_t err;
    
    err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "No calibration data found in NVS");
        return false;
    }
    
    // Load IMU calibration
    float acc_offset[3];
    size_t size = sizeof(acc_offset);
    err = nvs_get_blob(nvs_handle, NVS_KEY_ACC_OFFSET, acc_offset, &size);
    if (err == ESP_OK) {
        imu_set_acc_offset(acc_offset[0], acc_offset[1], acc_offset[2]);
        ESP_LOGI(TAG, "Loaded IMU calibration");
    }
    
    // Load magnetometer calibration
    float mag_offset[3], mag_scale[3];
    size = sizeof(mag_offset);
    err = nvs_get_blob(nvs_handle, NVS_KEY_MAG_OFFSET, mag_offset, &size);
    if (err == ESP_OK) {
        size = sizeof(mag_scale);
        nvs_get_blob(nvs_handle, NVS_KEY_MAG_SCALE, mag_scale, &size);
        mag_set_calibration(mag_offset[0], mag_offset[1], mag_offset[2],
                           mag_scale[0], mag_scale[1], mag_scale[2]);
        ESP_LOGI(TAG, "Loaded magnetometer calibration");
    }
    
    nvs_close(nvs_handle);
    return true;
}
