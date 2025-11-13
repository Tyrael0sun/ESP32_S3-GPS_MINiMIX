/**
 * @file battery_monitor.cpp
 * @brief Battery monitoring implementation using ADC
 */

#include "battery_monitor.h"
#include "config.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "driver/gpio.h"

static const char* TAG = "BATTERY";

static adc_oneshot_unit_handle_t adc_handle = NULL;
static adc_cali_handle_t adc_cali_handle = NULL;
static bool adc_initialized = false;

bool battery_init(void) {
    // Configure ADC
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = BAT_ADC_UNIT,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));
    
    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN_DB_11,
        .bitwidth = ADC_BITWIDTH_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, BAT_ADC_CHANNEL, &config));
    
    // Calibration
    adc_cali_line_fitting_config_t cali_config = {
        .unit_id = BAT_ADC_UNIT,
        .atten = ADC_ATTEN_DB_11,
        .bitwidth = ADC_BITWIDTH_12,
    };
    esp_err_t ret = adc_cali_create_scheme_line_fitting(&cali_config, &adc_cali_handle);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "ADC calibration successful");
    }
    
    // Configure charging status GPIO
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << CHRG_STATUS_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    
    adc_initialized = true;
    ESP_LOGI(TAG, "Battery monitor initialized");
    
    return true;
}

bool battery_read(BatteryStatus* status) {
    if (!adc_initialized || !status) return false;
    
    status->voltage = battery_get_voltage();
    status->percentage = battery_get_percentage();
    status->charging = battery_is_charging();
    
    return true;
}

float battery_get_voltage(void) {
    if (!adc_initialized) return 0.0f;
    
    int raw_value = 0;
    adc_oneshot_read(adc_handle, BAT_ADC_CHANNEL, &raw_value);
    
    int voltage_mv = 0;
    if (adc_cali_handle) {
        adc_cali_raw_to_voltage(adc_cali_handle, raw_value, &voltage_mv);
    } else {
        // Fallback: simple linear conversion
        voltage_mv = (raw_value * 3300) / 4095;
    }
    
    // Apply voltage divider correction
    float voltage = (voltage_mv / 1000.0f) / BAT_VOLTAGE_DIVIDER;
    
    return voltage;
}

uint8_t battery_get_percentage(void) {
    float voltage = battery_get_voltage();
    
    if (voltage >= BAT_MAX_VOLTAGE) {
        return 100;
    } else if (voltage <= BAT_MIN_VOLTAGE) {
        return 0;
    }
    
    // Linear interpolation
    float percentage = ((voltage - BAT_MIN_VOLTAGE) / (BAT_MAX_VOLTAGE - BAT_MIN_VOLTAGE)) * 100.0f;
    
    return (uint8_t)percentage;
}

bool battery_is_charging(void) {
    // Charging status pin is active low
    return (gpio_get_level((gpio_num_t)CHRG_STATUS_GPIO) == 0);
}
