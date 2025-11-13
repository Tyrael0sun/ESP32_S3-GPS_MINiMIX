/**
 * @file baro_driver.h
 * @brief BMP388 Barometer driver
 */

#ifndef BARO_DRIVER_H
#define BARO_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

struct BaroData {
    float pressure;     // hPa
    float altitude;     // m
    float temperature;  // °C
    uint32_t timestamp; // ms
};

/**
 * @brief Initialize BMP388 barometer
 * @return true on success, false on failure
 */
bool baro_init(void);

/**
 * @brief Read barometer data
 * @param data Pointer to BaroData structure
 * @return true on success, false on failure
 */
bool baro_read(BaroData* data);

/**
 * @brief Set reference pressure for altitude calculation
 * @param pressure Reference pressure in hPa (typically sea level pressure 1013.25)
 */
void baro_set_reference_pressure(float pressure);

#endif // BARO_DRIVER_H
