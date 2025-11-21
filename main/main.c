#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "config.h"
#include "sensors.h"
#include "display.h"
#include "input.h"
#include "gnss.h"
#include "battery.h" // Include Battery

static const char *TAG = "MAIN";

// Task Priorities
#define TASK_PRIO_GNSS      5
#define TASK_PRIO_UI        5
#define TASK_PRIO_LOGGER    4
#define TASK_PRIO_DIAG      3

// Task Stack Sizes
#define TASK_STACK_GNSS     4096
#define TASK_STACK_UI       8192
#define TASK_STACK_LOGGER   4096
#define TASK_STACK_DIAG     4096

// Global trigger function for diagnostics
void diagnostics_trigger(const char *event) {
    ESP_LOGI(TAG, "[EVENT] %s", event);
}

void ui_task(void *pvParameters) {
    ESP_LOGI(TAG, "UI Task Started");

    // Initialize Display and LVGL
    if (display_init() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize display!");
        vTaskDelete(NULL);
    }

    // Create a Label for testing
    if (display_lock(100)) {
        lv_obj_t *label = lv_label_create(lv_scr_act());
        lv_label_set_text(label, "ESP32-S3 GPS Logger");
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
        display_unlock();
    }

    while (1) {
        if (display_lock(10)) {
            lv_timer_handler();
            display_unlock();
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void logger_task(void *pvParameters) {
    ESP_LOGI(TAG, "Logger Task Started");
    // TODO: Initialize SD Card, Handle GPX writing
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void diagnostics_task(void *pvParameters) {
    ESP_LOGI(TAG, "Diagnostics Task Started");

    float ax, ay, az, gx, gy, gz, temp_imu;
    float mx, my, mz, temp_mag;
    float press, temp_baro;
    uint32_t bat_mv;

    // Phase 1: Startup Self-Test (0-5s)
    for (int i = 0; i < 5; i++) {
        ESP_LOGI(TAG, "--- SELF TEST T=%ds ---", i);

        // Read Sensors
        sensors_read_imu(&ax, &ay, &az, &gx, &gy, &gz, &temp_imu);
        sensors_read_mag(&mx, &my, &mz, &temp_mag);
        sensors_read_baro(&press, &temp_baro);
        battery_read_voltage(&bat_mv);

        ESP_LOGI(TAG, "IMU: A(%.2f, %.2f, %.2f) G(%.2f, %.2f, %.2f) T=%.1fC", ax, ay, az, gx, gy, gz, temp_imu);
        ESP_LOGI(TAG, "MAG: (%.2f, %.2f, %.2f)", mx, my, mz);
        ESP_LOGI(TAG, "BARO: P=%.1f hPa, T=%.1f C", press, temp_baro);
        ESP_LOGI(TAG, "BAT: %lu mV", bat_mv);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    ESP_LOGI(TAG, "Self Test Complete. Entering Heartbeat Mode.");

    // Phase 2: Runtime Heartbeat
    while (1) {
        // Heartbeat includes sensor snapshot
        sensors_read_imu(&ax, &ay, &az, &gx, &gy, &gz, &temp_imu);
        sensors_read_mag(&mx, &my, &mz, &temp_mag);
        sensors_read_baro(&press, &temp_baro);
        battery_read_voltage(&bat_mv);

        ESP_LOGI(TAG, "[DIAG] HB: IMU A(%.2f,%.2f,%.2f) MAG(%.2f,%.2f,%.2f) BARO(%.1f) BAT(%lu)",
                 ax, ay, az, mx, my, mz, press, bat_mv);

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void app_main(void) {
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "Starting ESP32-S3 GPS Logger...");

    // Initialize Sensors
    if (sensors_init() != ESP_OK) {
        ESP_LOGE(TAG, "Sensor initialization failed!");
    }

    // Initialize Input
    if (input_init() != ESP_OK) {
        ESP_LOGE(TAG, "Input initialization failed!");
    }

    // Initialize Battery
    if (battery_init() != ESP_OK) {
        ESP_LOGE(TAG, "Battery initialization failed!");
    }

    // Create Tasks
    xTaskCreate(gnss_task_entry, "gnss_task", TASK_STACK_GNSS, NULL, TASK_PRIO_GNSS, NULL);
    xTaskCreate(ui_task, "ui_task", TASK_STACK_UI, NULL, TASK_PRIO_UI, NULL);
    xTaskCreate(logger_task, "logger_task", TASK_STACK_LOGGER, NULL, TASK_PRIO_LOGGER, NULL);
    xTaskCreate(diagnostics_task, "diagnostics_task", TASK_STACK_DIAG, NULL, TASK_PRIO_DIAG, NULL);
}
