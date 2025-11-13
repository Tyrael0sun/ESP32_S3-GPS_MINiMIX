/**
 * @file battery_monitor.h
 * @brief Battery voltage and charging status monitor
 */

#ifndef BATTERY_MONITOR_H
#define BATTERY_MONITOR_H

#include <stdint.h>
#include <stdbool.h>

struct BatteryStatus {
    float voltage;          // V
    uint8_t percentage;     // 0-100%
    bool charging;          // Charging status
};

/**
 * @brief Initialize battery monitor
 * @return true on success, false on failure
 */
bool battery_init(void);

/**
 * @brief Read battery status
 * @param status Pointer to BatteryStatus structure
 * @return true on success, false on failure
 */
bool battery_read(BatteryStatus* status);

/**
 * @brief Get battery voltage
 * @return Voltage in V
 */
float battery_get_voltage(void);

/**
 * @brief Get battery percentage
 * @return Percentage 0-100
 */
uint8_t battery_get_percentage(void);

/**
 * @brief Check if battery is charging
 * @return true if charging
 */
bool battery_is_charging(void);

#endif // BATTERY_MONITOR_H
