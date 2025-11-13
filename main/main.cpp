/**
 * @file main.cpp
 * @brief Main application entry point for ESP32-S3 GPS MINiMIX
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "driver/i2c.h"

#include "config.h"
#include "hardware/display_driver.h"
#include "hardware/imu_driver.h"
#include "hardware/mag_driver.h"
#include "hardware/baro_driver.h"
#include "hardware/gnss_driver.h"
#include "hardware/sdcard_driver.h"
#include "hardware/encoder_driver.h"
#include "hardware/battery_monitor.h"
#include "core/sensor_fusion.h"
#include "core/gps_logger.h"
#include "core/calibration.h"
#include "core/rtc_manager.h"
#include "ui/ui_manager.h"
#include "app/bike_computer_app.h"
#include "app/pbox_app.h"
#include "app/gps_logger_app.h"
#include "utils/diagnostics.h"

static const char* TAG = "MAIN";

// I2C initialization
static bool init_i2c(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA_GPIO,
        .scl_io_num = I2C_SCL_GPIO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master = {
            .clk_speed = I2C_FREQ_HZ,
        },
        .clk_flags = 0,
    };
    
    esp_err_t err = i2c_param_config(I2C_NUM, &conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure I2C: %s", esp_err_to_name(err));
        return false;
    }
    
    err = i2c_driver_install(I2C_NUM, conf.mode, 0, 0, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install I2C driver: %s", esp_err_to_name(err));
        return false;
    }
    
    ESP_LOGI(TAG, "I2C initialized at %d Hz", I2C_FREQ_HZ);
    return true;
}

// Key event handler
static void key_event_handler(KeyEvent event) {
    // Temporarily disable mode switching to prevent crashes during debugging
    // AppMode current_mode = ui_manager_get_mode();
    
    switch (event) {
        case KEY_SHORT_PRESS:
            ESP_LOGI(TAG, "KEY: Short press");
            // Mode switching disabled temporarily
            /*
            switch (current_mode) {
                case MODE_BIKE_COMPUTER:
                    ui_manager_switch_mode(MODE_GPS_LOGGER);
                    break;
                case MODE_GPS_LOGGER:
                    ui_manager_switch_mode(MODE_PBOX);
                    break;
                case MODE_PBOX:
                    ui_manager_switch_mode(MODE_GNSS_INFO);
                    break;
                case MODE_GNSS_INFO:
                    ui_manager_switch_mode(MODE_BIKE_COMPUTER);
                    break;
                case MODE_SETTINGS:
                    ui_manager_switch_mode(MODE_BIKE_COMPUTER);
                    break;
            }
            */
            break;
            
        case KEY_MEDIUM_PRESS:
            ESP_LOGI(TAG, "KEY: Medium press");
            // Recording control disabled temporarily
            /*
            if (gps_logger_is_logging()) {
                gps_logger_stop();
            } else {
                gps_logger_start();
            }
            */
            break;
            
        case KEY_LONG_PRESS:
            ESP_LOGI(TAG, "KEY: Long press");
            // Settings toggle disabled temporarily
            /*
            if (current_mode == MODE_SETTINGS) {
                ui_manager_switch_mode(MODE_BIKE_COMPUTER);
            } else {
                ui_manager_switch_mode(MODE_SETTINGS);
            }
            */
            break;
            
        case KEY_DOUBLE_CLICK:
            ESP_LOGI(TAG, "KEY: Double click");
            break;
            
        default:
            break;
    }
}

// Encoder rotation handler
static void encoder_event_handler(int32_t count) {
    ESP_LOGI(TAG, "Encoder: %ld", count);
}

// RTC sync task
static void rtc_sync_task(void* arg) {
    bool synced = false;
    
    while (!synced) {
        if (rtc_sync_with_gps()) {
            synced = true;
            ui_manager_show_time_sync();
            ESP_LOGI(TAG, "RTC synced with GPS");
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    vTaskDelete(NULL);
}

// Main application task
static void app_task(void* arg) {
    uint32_t last_update = 0;
    const uint32_t UPDATE_INTERVAL_MS = 100; // 10Hz
    
    while (1) {
        uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
        
        if (now - last_update >= UPDATE_INTERVAL_MS) {
            last_update = now;
            
            // Update sensor fusion
            sensor_fusion_update(UPDATE_INTERVAL_MS / 1000.0f);
            
            // Update current app
            AppMode mode = ui_manager_get_mode();
            switch (mode) {
                case MODE_BIKE_COMPUTER:
                    bike_computer_update();
                    break;
                case MODE_GPS_LOGGER:
                    gps_logger_app_update();
                    break;
                case MODE_PBOX:
                    pbox_update();
                    break;
                case MODE_GNSS_INFO:
                    // GNSS info handled by UI
                    break;
                case MODE_SETTINGS:
                    // Settings handled by UI
                    break;
            }
            
            // Update UI
            ui_manager_update();
        }
        
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  ESP32-S3 GPS MINiMIX v0.0.1");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "");
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Initialize I2C bus
    if (!init_i2c()) {
        ESP_LOGE(TAG, "Failed to initialize I2C");
        return;
    }
    
    // Initialize hardware
    ESP_LOGI(TAG, "Initializing hardware...");
    
    if (!imu_init()) ESP_LOGW(TAG, "IMU init failed");
    if (!mag_init()) ESP_LOGW(TAG, "Magnetometer init failed");
    if (!baro_init()) ESP_LOGW(TAG, "Barometer init failed");
    if (!gnss_init()) ESP_LOGW(TAG, "GNSS init failed");
    if (!sdcard_init()) ESP_LOGW(TAG, "SD card init failed");
    if (!encoder_init()) ESP_LOGW(TAG, "Encoder init failed");
    if (!battery_init()) ESP_LOGW(TAG, "Battery monitor init failed");
    
    // Initialize core systems
    ESP_LOGI(TAG, "Initializing core systems...");
    
    sensor_fusion_init();
    gps_logger_init();
    calibration_init();
    rtc_init();
    diagnostics_init();
    
    // Initialize UI
    ESP_LOGI(TAG, "Initializing UI...");
    ui_manager_init();
    
    // Initialize apps
    bike_computer_init();
    pbox_init();
    gps_logger_app_init();
    
    // Register input callbacks
    key_register_callback(key_event_handler);
    encoder_register_callback(encoder_event_handler);
    
    // Start tasks
    xTaskCreate(app_task, "app", STACK_SIZE_UI, NULL, TASK_PRIORITY_UI, NULL);
    xTaskCreate(rtc_sync_task, "rtc_sync", 4096, NULL, 2, NULL);  // Increased from 2048 to 4096 to prevent stack overflow
    diagnostics_start_task();
    
    ESP_LOGI(TAG, "System started successfully!");
    ESP_LOGI(TAG, "");
}
