/**
 * @file pbox_app.cpp
 * @brief P-Box application implementation
 */

#include "pbox_app.h"
#include "config.h"
#include "../hardware/gnss_driver.h"
#include "../core/sensor_fusion.h"
#include "esp_log.h"

static const char* TAG = "PBOX_APP";

static PBoxData app_data = {0};
static uint32_t test_start_time = 0;

void pbox_init(void) {
    app_data.state = PBOX_IDLE;
    app_data.current_speed = 0.0f;
    app_data.test_time = 0.0f;
    app_data.start_speed = PBOX_TARGET_SPEED_DEFAULT_START;
    app_data.target_speed = PBOX_TARGET_SPEED_DEFAULT_END;
    
    ESP_LOGI(TAG, "P-Box app initialized");
}

void pbox_update(void) {
    GnssData gps;
    if (!gnss_read(&gps) || !gps.fix_valid) {
        app_data.state = PBOX_IDLE;
        return;
    }
    
    app_data.current_speed = gps.speed;
    
    FusedData fusion;
    sensor_fusion_get_data(&fusion);
    float forward_accel = fusion.forward_accel_g;
    
    switch (app_data.state) {
        case PBOX_IDLE:
        case PBOX_READY:
            // Check auto-start conditions (F-PBOX-02)
            if (app_data.current_speed < PBOX_START_SPEED_KMPH && 
                forward_accel > PBOX_START_ACCEL_G) {
                app_data.state = PBOX_TESTING;
                test_start_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
                ESP_LOGI(TAG, "Test started! Accel: %.3fG", forward_accel);
            } else {
                app_data.state = PBOX_READY;
            }
            break;
            
        case PBOX_TESTING:
            // Calculate elapsed time
            uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
            app_data.test_time = (now - test_start_time) / 1000.0f;
            
            // Check if target speed reached
            if (app_data.current_speed >= app_data.target_speed) {
                app_data.state = PBOX_FINISHED;
                ESP_LOGI(TAG, "Test finished! Time: %.3f s", app_data.test_time);
            }
            break;
            
        case PBOX_FINISHED:
            // Stay in finished state until reset
            break;
    }
}

PBoxData pbox_get_data(void) {
    return app_data;
}

void pbox_reset(void) {
    app_data.state = PBOX_IDLE;
    app_data.test_time = 0.0f;
    test_start_time = 0;
}

void pbox_set_test_params(float start_speed, float target_speed) {
    app_data.start_speed = start_speed;
    app_data.target_speed = target_speed;
    ESP_LOGI(TAG, "Test params: %.1f -> %.1f km/h", start_speed, target_speed);
}
