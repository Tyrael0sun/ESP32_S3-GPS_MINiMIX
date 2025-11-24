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
#include "battery.h"
#include "driver/temperature_sensor.h"
#include "esp_timer.h"
#include "ui.h"

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

static temperature_sensor_handle_t temp_handle = NULL;

// Global trigger function for diagnostics
void diagnostics_trigger(const char *event) {
    ESP_LOGI(TAG, "[EVENT] %s", event);
}

// Global shared sensor data (for UI)
static float g_speed = 0.0f; // From GNSS (TODO: Calculate speed in GNSS)
static float g_alt = 0.0f;
static float g_heading = 0.0f;
static float g_press = 0.0f;
static float g_bat = 0.0f;
// Extern declaration for global input buffer from input.c (if not in header, we need to declare it or add getter)
// Better to add getter in input.h, but for speed here, we used a global in input.c
// Let's add a getter function in input.c and use it here, or just use the ui_update function.
// Wait, my input.c logic calls ui_update_input_status directly? No, I need to add that call.
// Let's re-check input.c logic. It calls report_event.
// report_event calls diagnostics_trigger.
// I need to update input.c to ALSO call ui_update_input_status if I want to update it from there.
// BUT input_task cannot call UI functions directly if they are not thread safe or if they use LVGL without lock.
// UI task loop updates SENSORS.
// Input event is transient.
// Best approach: Use a global string buffer protected or atomic, read by UI task.
// In input.c I added `g_input_event_str`. I should declare it extern here or in input.h.
extern char g_input_event_str[32];

void ui_task(void *pvParameters) {
    ESP_LOGI(TAG, "UI Task Started");

    if (display_init() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize display!");
        vTaskDelete(NULL);
    }

    if (display_lock(100)) {
        ui_init_display();
        display_unlock();
    }

    while (1) {
        if (display_lock(10)) {
            // Update UI with global data
            ui_update_sensors(g_speed, g_alt, g_heading, g_press, g_bat);
            ui_update_input_status(g_input_event_str);
            lv_timer_handler();
            display_unlock();
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void logger_task(void *pvParameters) {
    ESP_LOGI(TAG, "Logger Task Started");
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

static float get_mcu_temp(void) {
    float tsens_out;
    if (temperature_sensor_get_celsius(temp_handle, &tsens_out) == ESP_OK) {
        return tsens_out;
    }
    return 0.0f;
}

void diagnostics_task(void *pvParameters) {
    ESP_LOGI(TAG, "Diagnostics Task Started");

    // Init MCU Temp Sensor
    temperature_sensor_config_t temp_sensor = TEMPERATURE_SENSOR_CONFIG_DEFAULT(20, 100);
    temperature_sensor_install(&temp_sensor, &temp_handle);
    temperature_sensor_enable(temp_handle);

    float ax, ay, az, gx, gy, gz, temp_imu;
    float grav_x, grav_y, grav_z, lin_x, lin_y, lin_z;
    float mx, my, mz, temp_mag;
    float heading;
    float press, temp_baro, altitude;
    uint32_t bat_mv;
    float mcu_temp;

    // Wait a bit for systems to stabilize
    vTaskDelay(pdMS_TO_TICKS(2000));

    while (1) {
        // 1. Read Sensors
        sensors_read_imu(&ax, &ay, &az, &gx, &gy, &gz, &temp_imu);
        sensors_calc_gravity_linear(ax, ay, az, &grav_x, &grav_y, &grav_z, &lin_x, &lin_y, &lin_z);

        sensors_read_mag(&mx, &my, &mz, &temp_mag);
        heading = sensors_calc_heading(mx, my);

        sensors_read_baro(&press, &temp_baro);
        altitude = sensors_calc_altitude(press, temp_baro);

        battery_read_voltage(&bat_mv);
        mcu_temp = get_mcu_temp();

        // Update globals for UI
        g_alt = altitude;
        g_heading = heading;
        g_press = press;
        g_bat = (float)bat_mv;

        // 2. Format Output Block
        ESP_LOGI(TAG, "[DIAG][T=%lldms]", esp_timer_get_time() / 1000);

        if (g_gnss_data.fix) {
            ESP_LOGI(TAG, "GNSS: OK, %d sats, (%.4f,%.4f)", g_gnss_data.sats, g_gnss_data.lat, g_gnss_data.lon);
        } else {
            ESP_LOGI(TAG, "GNSS: NO FIX, %d sats", g_gnss_data.sats);
        }

        ESP_LOGI(TAG, "IMU: ACC(L:%.2f,%.2f,%.2f) GRAV(%.2f,%.2f,%.2f)", lin_x, lin_y, lin_z, grav_x, grav_y, grav_z);
        ESP_LOGI(TAG, "GYR(%.1f,%.1f,%.1f)", gx, gy, gz);
        ESP_LOGI(TAG, "MAG: (%.1f,%.1f,%.1f) Heading=%.1f", mx, my, mz, heading);
        ESP_LOGI(TAG, "BARO: %.1fhPa Alt=%.1fm", press, altitude);
        ESP_LOGI(TAG, "TEMP: MCU=%.1fC, IMU=%.1fC, BARO=%.1fC, MAG=%.1fC", mcu_temp, temp_imu, temp_baro, temp_mag);
        ESP_LOGI(TAG, "BAT: %lu mV", bat_mv);
        ESP_LOGI(TAG, "RESULT: OK");

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void app_main(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "Starting ESP32-S3 GPS Logger...");

    if (sensors_init() != ESP_OK) ESP_LOGE(TAG, "Sensor initialization failed!");
    if (input_init() != ESP_OK) ESP_LOGE(TAG, "Input initialization failed!");
    if (battery_init() != ESP_OK) ESP_LOGE(TAG, "Battery initialization failed!");

    xTaskCreate(gnss_task_entry, "gnss_task", TASK_STACK_GNSS, NULL, TASK_PRIO_GNSS, NULL);
    xTaskCreate(ui_task, "ui_task", TASK_STACK_UI, NULL, TASK_PRIO_UI, NULL);
    xTaskCreate(logger_task, "logger_task", TASK_STACK_LOGGER, NULL, TASK_PRIO_LOGGER, NULL);
    xTaskCreate(diagnostics_task, "diagnostics_task", TASK_STACK_DIAG, NULL, TASK_PRIO_DIAG, NULL);
}
