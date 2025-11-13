/**
 * @file rtc_manager.cpp
 * @brief RTC manager implementation
 */

#include "rtc_manager.h"
#include "../hardware/gnss_driver.h"
#include "esp_log.h"
#include <sys/time.h>
#include <string.h>

static const char* TAG = "RTC";

static bool time_synced = false;

bool rtc_init(void) {
    // Set initial time to compile time
    struct tm compile_time = {0};
    
    // Parse __DATE__ and __TIME__ macros
    const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                           "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    
    char month_str[4];
    sscanf(__DATE__, "%s %d %d", month_str, &compile_time.tm_mday, &compile_time.tm_year);
    compile_time.tm_year -= 1900;
    
    for (int i = 0; i < 12; i++) {
        if (strcmp(month_str, months[i]) == 0) {
            compile_time.tm_mon = i;
            break;
        }
    }
    
    sscanf(__TIME__, "%d:%d:%d", &compile_time.tm_hour, &compile_time.tm_min, &compile_time.tm_sec);
    
    // Set system time
    time_t t = mktime(&compile_time);
    struct timeval tv = {.tv_sec = t, .tv_usec = 0};
    settimeofday(&tv, NULL);
    
    ESP_LOGI(TAG, "RTC initialized with compile time: %s", asctime(&compile_time));
    
    return true;
}

bool rtc_sync_with_gps(void) {
    GnssData gps;
    
    if (!gnss_read(&gps) || !gps.fix_valid) {
        return false;
    }
    
    // Check if GPS has valid time
    if (gps.year < 2020) {
        return false;
    }
    
    struct tm gps_time = {0};
    gps_time.tm_year = gps.year - 1900;
    gps_time.tm_mon = gps.month - 1;
    gps_time.tm_mday = gps.day;
    gps_time.tm_hour = gps.hour;
    gps_time.tm_min = gps.minute;
    gps_time.tm_sec = gps.second;
    
    time_t t = mktime(&gps_time);
    struct timeval tv = {.tv_sec = t, .tv_usec = 0};
    settimeofday(&tv, NULL);
    
    time_synced = true;
    
    ESP_LOGI(TAG, "RTC synced with GPS: %04d-%02d-%02d %02d:%02d:%02d",
             gps.year, gps.month, gps.day, gps.hour, gps.minute, gps.second);
    
    return true;
}

bool rtc_get_time(struct tm* timeinfo) {
    if (!timeinfo) return false;
    
    time_t now;
    time(&now);
    localtime_r(&now, timeinfo);
    
    return true;
}

bool rtc_set_time(const struct tm* timeinfo) {
    if (!timeinfo) return false;
    
    time_t t = mktime((struct tm*)timeinfo);
    struct timeval tv = {.tv_sec = t, .tv_usec = 0};
    settimeofday(&tv, NULL);
    
    ESP_LOGI(TAG, "RTC time set manually");
    
    return true;
}

bool rtc_is_synced(void) {
    return time_synced;
}
