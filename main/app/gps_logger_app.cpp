/**
 * @file gps_logger_app.cpp
 * @brief GPS logger app implementation
 */

#include "gps_logger_app.h"
#include "../core/gps_logger.h"
#include "esp_log.h"

static const char* TAG = "LOG_APP";

void gps_logger_app_init(void) {
    ESP_LOGI(TAG, "GPS logger app initialized");
}

void gps_logger_app_update(void) {
    // Log point if logging is active
    if (gps_logger_is_logging()) {
        gps_logger_log_point();
    }
}
