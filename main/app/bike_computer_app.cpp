/**
 * @file bike_computer_app.cpp
 * @brief Bike computer application implementation
 */

#include "bike_computer_app.h"
#include "../hardware/gnss_driver.h"
#include "../hardware/baro_driver.h"
#include "../core/gps_logger.h"
#include "esp_log.h"

static const char* TAG = "BC_APP";

static BikeComputerData app_data = {};

void bike_computer_init(void) {
    app_data.current_speed = 0.0f;
    app_data.altitude = 0.0f;
    app_data.trip_distance = 0.0f;
    app_data.trip_time = 0;
    app_data.recording = false;
    
    ESP_LOGI(TAG, "Bike computer app initialized");
}

void bike_computer_update(void) {
    GnssData gps;
    if (gnss_read(&gps) && gps.fix_valid) {
        app_data.current_speed = gps.speed;
    }
    
    BaroData baro;
    if (baro_read(&baro)) {
        app_data.altitude = baro.altitude;
    }
    
    // Update trip data if recording
    app_data.recording = gps_logger_is_logging();
    if (app_data.recording) {
        app_data.trip_distance = gps_logger_get_distance();
        app_data.trip_time = gps_logger_get_duration();
    }
}

BikeComputerData bike_computer_get_data(void) {
    return app_data;
}

void bike_computer_reset_trip(void) {
    app_data.trip_distance = 0.0f;
    app_data.trip_time = 0;
}
