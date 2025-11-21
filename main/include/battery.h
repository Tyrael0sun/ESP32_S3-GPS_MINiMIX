#ifndef BATTERY_H
#define BATTERY_H

#include "esp_err.h"

/**
 * @brief Initialize Battery ADC (ADC2 CH1)
 *
 * @return esp_err_t
 */
esp_err_t battery_init(void);

/**
 * @brief Read Battery Voltage
 *
 * Assumes 1:1 voltage divider (V_bat = V_adc * 2)
 *
 * @param voltage_mv Pointer to store voltage in mV
 * @return esp_err_t
 */
esp_err_t battery_read_voltage(uint32_t *voltage_mv);

#endif // BATTERY_H
