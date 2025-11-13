/**
 * @file display_driver.cpp
 * @brief ST7789 LCD display driver implementation using LovyanGFX
 */

#include "display_driver.h"
#include "config.h"
#include "esp_log.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

static const char* TAG = "DISP";

// Display initialization flag
static bool display_initialized = false;

bool display_init(void) {
    esp_err_t ret;
    
    // Initialize backlight PWM
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = DISP_BL_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK,
        .deconfigure = false
    };
    ret = ledc_timer_config(&ledc_timer);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure LEDC timer");
        return false;
    }
    
    ledc_channel_config_t ledc_channel = {
        .gpio_num = DISP_BL_GPIO,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = (DISP_BL_DEFAULT_DUTY * 255) / 100,
        .hpoint = 0,
        .sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD,
        .flags = {},
        .deconfigure = false
    };
    ret = ledc_channel_config(&ledc_channel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure LEDC channel");
        return false;
    }
    
    // Configure SPI bus
    spi_bus_config_t buscfg = {};
    buscfg.mosi_io_num = DISP_MOSI_GPIO;
    buscfg.miso_io_num = -1;
    buscfg.sclk_io_num = DISP_SCK_GPIO;
    buscfg.quadwp_io_num = -1;
    buscfg.quadhd_io_num = -1;
    buscfg.max_transfer_sz = DISP_WIDTH * DISP_HEIGHT * 2;
    buscfg.flags = 0;
    buscfg.isr_cpu_id = ESP_INTR_CPU_AFFINITY_AUTO;
    buscfg.intr_flags = 0;
    ret = spi_bus_initialize(DISP_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus");
        return false;
    }
    
    // Configure DC and RST pins
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << DISP_DC_GPIO) | (1ULL << DISP_RST_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    
    // Reset display
    gpio_set_level((gpio_num_t)DISP_RST_GPIO, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level((gpio_num_t)DISP_RST_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(120));
    
    ESP_LOGI(TAG, "Display initialized: %dx%d", DISP_WIDTH, DISP_HEIGHT);
    display_initialized = true;
    
    return true;
}

void display_set_backlight(uint8_t duty_percent) {
    if (duty_percent > 100) duty_percent = 100;
    uint32_t duty = (duty_percent * 255) / 100;
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

uint16_t display_get_width(void) {
    return DISP_WIDTH;
}

uint16_t display_get_height(void) {
    return DISP_HEIGHT;
}

void display_flush_cb(void* disp_drv, const void* area, void* color_p) {
    // This would be implemented with actual LovyanGFX or LVGL driver
    // Placeholder for now
}
