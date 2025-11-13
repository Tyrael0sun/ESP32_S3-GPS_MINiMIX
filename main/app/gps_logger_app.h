/**
 * @file gps_logger_app.h
 * @brief GPS logger application wrapper
 */

#ifndef GPS_LOGGER_APP_H
#define GPS_LOGGER_APP_H

#include <stdint.h>
#include <stdbool.h>

// This is a thin wrapper around core/gps_logger
// Exists to match the app architecture pattern

void gps_logger_app_init(void);
void gps_logger_app_update(void);

#endif // GPS_LOGGER_APP_H
