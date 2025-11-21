#include "battery.h"
#include "config.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"

static const char *TAG = "BATTERY";

static adc_oneshot_unit_handle_t adc_handle = NULL;

// ADC2 Channel 1 is GPIO 12 on ESP32-S3?
// Spec says: "BAT_ADC / CHRG_STATUS | 12 / 21"
// ESP32-S3: GPIO 12 is ADC2_CH1.
// Note: ADC2 usage might conflict with Wi-Fi in some IDF versions, but S3 handles it better.

esp_err_t battery_init(void) {
    ESP_LOGI(TAG, "Initializing Battery ADC...");

    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_2,
        .clk_src = ADC_DIGI_CLK_SRC_DEFAULT,
    };
    esp_err_t err = adc_oneshot_new_unit(&init_config, &adc_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ADC2 Init failed: %s", esp_err_to_name(err));
        return err;
    }

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_12,
        .atten = ADC_ATTEN_DB_12, // Allows up to ~3V input. With 1:1 divider, max Bat 6V. 4.2V / 2 = 2.1V fits.
    };

    // Channel 1 corresponds to GPIO 12 for ADC2 on S3?
    // We need to confirm channel mapping.
    // GPIO 12 is ADC2_CH1 on S3.
    err = adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_1, &config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ADC2 Channel Config failed: %s", esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}

esp_err_t battery_read_voltage(uint32_t *voltage_mv) {
    if (!adc_handle) return ESP_ERR_INVALID_STATE;

    int adc_raw;
    esp_err_t err = adc_oneshot_read(adc_handle, ADC_CHANNEL_1, &adc_raw);
    if (err != ESP_OK) return err;

    // Simple conversion without calibration curve for prototype
    // ADC_ATTEN_DB_12 typically has linear range up to ~3100mV?
    // Actually S3 max is ~3100mV with 12dB attenuation? Or 11dB.
    // Let's use rough calculation:
    // V_adc = raw * 3300 / 4095? No, it depends on attenuation.
    // With 11dB/12dB, full scale is approx 3.1V.
    // Let's assume 3100mV reference for now.
    // V_adc = raw * 3100 / 4095
    // V_bat = V_adc * 2 (1:1 divider)

    // TODO: Use esp_adc_cal for precision.
    uint32_t v_adc = (adc_raw * 3100) / 4095;
    *voltage_mv = v_adc * 2;

    return ESP_OK;
}
