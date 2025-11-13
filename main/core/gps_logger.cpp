/**
 * @file gps_logger.cpp
 * @brief GPS logger implementation with GPX format
 */

#include "gps_logger.h"
#include "config.h"
#include "../hardware/gnss_driver.h"
#include "../hardware/baro_driver.h"
#include "sensor_fusion.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <cmath>

static const char* TAG = "GPS_LOG";

static FILE* gpx_file = NULL;
static bool logging_active = false;
static float total_distance = 0.0f;
static uint32_t start_time = 0;
static double last_lat = 0.0, last_lon = 0.0;

// Calculate distance between two GPS points (Haversine formula)
static float calculate_distance(double lat1, double lon1, double lat2, double lon2) {
    const float R = 6371000.0f; // Earth radius in meters
    
    double dlat = (lat2 - lat1) * M_PI / 180.0;
    double dlon = (lon2 - lon1) * M_PI / 180.0;
    
    double a = sin(dlat / 2) * sin(dlat / 2) +
               cos(lat1 * M_PI / 180.0) * cos(lat2 * M_PI / 180.0) *
               sin(dlon / 2) * sin(dlon / 2);
    
    double c = 2 * atan2(sqrt(a), sqrt(1 - a));
    
    return R * c;
}

bool gps_logger_init(void) {
    // Create GPX directory if it doesn't exist
    struct stat st = {};
    if (stat(GPX_DIR, &st) == -1) {
        mkdir(GPX_DIR, 0700);
    }
    
    ESP_LOGI(TAG, "GPS logger initialized");
    return true;
}

bool gps_logger_start(void) {
    if (logging_active) {
        ESP_LOGW(TAG, "Logging already active");
        return false;
    }
    
    // Generate filename with timestamp
    time_t now;
    time(&now);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    
    char filename[128];
    snprintf(filename, sizeof(filename), "%s/track_%04d%02d%02d_%02d%02d%02d.gpx",
             GPX_DIR, timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
             timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    
    gpx_file = fopen(filename, "w");
    if (!gpx_file) {
        ESP_LOGE(TAG, "Failed to create GPX file: %s", filename);
        return false;
    }
    
    // Write GPX header
    fprintf(gpx_file,
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<gpx version=\"1.1\" creator=\"ESP32-S3 GPS MINiMIX\"\n"
        "  xmlns=\"http://www.topografix.com/GPX/1/1\"\n"
        "  xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
        "  xsi:schemaLocation=\"http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd\">\n"
        "  <trk>\n"
        "    <name>Track %04d-%02d-%02d %02d:%02d:%02d</name>\n"
        "    <trkseg>\n",
        timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
        timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    
    logging_active = true;
    total_distance = 0.0f;
    start_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    last_lat = 0.0;
    last_lon = 0.0;
    
    ESP_LOGI(TAG, "Started logging to: %s", filename);
    return true;
}

bool gps_logger_stop(void) {
    if (!logging_active || !gpx_file) {
        return false;
    }
    
    // Write GPX footer
    fprintf(gpx_file,
        "    </trkseg>\n"
        "  </trk>\n"
        "</gpx>\n");
    
    fclose(gpx_file);
    gpx_file = NULL;
    logging_active = false;
    
    ESP_LOGI(TAG, "Stopped logging. Total distance: %.2f km", total_distance / 1000.0f);
    return true;
}

bool gps_logger_is_logging(void) {
    return logging_active;
}

bool gps_logger_log_point(void) {
    if (!logging_active || !gpx_file) return false;
    
    GnssData gps;
    if (!gnss_read(&gps) || !gps.fix_valid) {
        return false;
    }
    
    // Calculate distance increment
    if (last_lat != 0.0 && last_lon != 0.0) {
        float dist = calculate_distance(last_lat, last_lon, gps.latitude, gps.longitude);
        total_distance += dist;
    }
    last_lat = gps.latitude;
    last_lon = gps.longitude;
    
    // Get additional sensor data
    BaroData baro;
    baro_read(&baro);
    
    FusedData fusion;
    sensor_fusion_get_data(&fusion);
    
    // Write track point
    fprintf(gpx_file,
        "      <trkpt lat=\"%.8f\" lon=\"%.8f\">\n"
        "        <ele>%.2f</ele>\n"
        "        <time>%04d-%02d-%02dT%02d:%02d:%02dZ</time>\n"
        "        <extensions>\n"
        "          <temperature>%.1f</temperature>\n"
        "          <pressure>%.2f</pressure>\n"
        "          <forward_g>%.3f</forward_g>\n"
        "        </extensions>\n"
        "      </trkpt>\n",
        gps.latitude, gps.longitude, gps.altitude,
        gps.year, gps.month, gps.day, gps.hour, gps.minute, gps.second,
        baro.temperature, baro.pressure, fusion.forward_accel_g);
    
    fflush(gpx_file); // Ensure data is written
    
    return true;
}

float gps_logger_get_distance(void) {
    return total_distance / 1000.0f; // Return in km
}

uint32_t gps_logger_get_duration(void) {
    if (!logging_active) return 0;
    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
    return (now - start_time) / 1000; // Return in seconds
}
