/**
 * @file display_driver.cpp
 * @brief ST7789 LCD display driver implementation using ESP LCD + LVGL
 */

#include "display_driver.h"
#include "config.h"
#include "esp_log.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_st7789.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/ledc.h"
#include "lvgl.h"
#include <string.h>

static const char* TAG = "DISP";

// Display handles
static bool display_initialized = false;
static esp_lcd_panel_handle_t panel_handle = NULL;
static lv_disp_draw_buf_t disp_buf;
static lv_disp_drv_t disp_drv;
static lv_disp_t* disp = NULL;

// LVGL buffers (double buffering for smoother rendering)
#define LVGL_BUF_SIZE (DISP_WIDTH * 40)
static lv_color_t* lvgl_buf1 = NULL;
static lv_color_t* lvgl_buf2 = NULL;

// Forward declarations
static void lvgl_flush_cb(lv_disp_drv_t* drv, const lv_area_t* area, lv_color_t* color_map);
static bool notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t* edata, void* user_ctx);

bool display_init(void) {
    esp_err_t ret;
    
    ESP_LOGI(TAG, "Initializing ST7789 display with LVGL...");
    
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
    memset(&buscfg, 0, sizeof(buscfg));
    buscfg.mosi_io_num = DISP_MOSI_GPIO;
    buscfg.miso_io_num = -1;
    buscfg.sclk_io_num = DISP_SCK_GPIO;
    buscfg.quadwp_io_num = -1;
    buscfg.quadhd_io_num = -1;
    buscfg.max_transfer_sz = DISP_WIDTH * DISP_HEIGHT * sizeof(uint16_t);
    
    ret = spi_bus_initialize(DISP_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
        return false;
    }
    
    // Create LCD panel IO handle (SPI)
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {};
    memset(&io_config, 0, sizeof(io_config));
    io_config.cs_gpio_num = (gpio_num_t)DISP_CS_GPIO;
    io_config.dc_gpio_num = (gpio_num_t)DISP_DC_GPIO;
    io_config.spi_mode = 0;
    io_config.pclk_hz = 40 * 1000 * 1000;  // 40MHz
    io_config.trans_queue_depth = 10;
    io_config.on_color_trans_done = notify_lvgl_flush_ready;
    io_config.user_ctx = &disp_drv;
    io_config.lcd_cmd_bits = 8;
    io_config.lcd_param_bits = 8;
    
    ret = esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)DISP_SPI_HOST, &io_config, &io_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create panel IO: %s", esp_err_to_name(ret));
        return false;
    }
    
    // Create LCD panel handle (ST7789)
    esp_lcd_panel_dev_config_t panel_config = {};
    memset(&panel_config, 0, sizeof(panel_config));
    panel_config.reset_gpio_num = (gpio_num_t)DISP_RST_GPIO;
    panel_config.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR;
    panel_config.bits_per_pixel = 16;
    ret = esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create ST7789 panel: %s", esp_err_to_name(ret));
        return false;
    }
    
    // Reset and initialize panel
    esp_lcd_panel_reset(panel_handle);
    esp_lcd_panel_init(panel_handle);
    esp_lcd_panel_invert_color(panel_handle, true);
    
    // Set rotation (180 degrees)
    esp_lcd_panel_swap_xy(panel_handle, false);
    esp_lcd_panel_mirror(panel_handle, true, true);
    
    // Turn on display
    esp_lcd_panel_disp_on_off(panel_handle, true);
    
    ESP_LOGI(TAG, "LCD panel initialized");
    
    // Initialize LVGL
    lv_init();
    
    // Allocate LVGL buffers
    lvgl_buf1 = (lv_color_t*)heap_caps_malloc(LVGL_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
    lvgl_buf2 = (lv_color_t*)heap_caps_malloc(LVGL_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
    
    if (!lvgl_buf1 || !lvgl_buf2) {
        ESP_LOGE(TAG, "Failed to allocate LVGL buffers");
        return false;
    }
    
    // Initialize LVGL display buffer
    lv_disp_draw_buf_init(&disp_buf, lvgl_buf1, lvgl_buf2, LVGL_BUF_SIZE);
    
    // Initialize LVGL display driver
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = DISP_WIDTH;
    disp_drv.ver_res = DISP_HEIGHT;
    disp_drv.flush_cb = lvgl_flush_cb;
    disp_drv.draw_buf = &disp_buf;
    disp_drv.user_data = panel_handle;
    
    // Register display driver
    disp = lv_disp_drv_register(&disp_drv);
    if (!disp) {
        ESP_LOGE(TAG, "Failed to register LVGL display");
        return false;
    }
    
    ESP_LOGI(TAG, "LVGL initialized: %dx%d", DISP_WIDTH, DISP_HEIGHT);
    display_initialized = true;
    
    // Set black background
    lv_obj_t* scr = lv_disp_get_scr_act(disp);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0);
    
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
    // Legacy function - not used with LVGL integration
}

// LVGL flush callback
static void lvgl_flush_cb(lv_disp_drv_t* drv, const lv_area_t* area, lv_color_t* color_map) {
    esp_lcd_panel_handle_t panel = (esp_lcd_panel_handle_t)drv->user_data;
    
    int offsetx1 = area->x1;
    int offsetx2 = area->x2;
    int offsety1 = area->y1;
    int offsety2 = area->y2;
    
    // Send pixels to display
    esp_lcd_panel_draw_bitmap(panel, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, color_map);
}

// Notify LVGL when flush is done
static bool notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t* edata, void* user_ctx) {
    lv_disp_drv_t* disp_driver = (lv_disp_drv_t*)user_ctx;
    lv_disp_flush_ready(disp_driver);
    return false;
}

void display_fill(uint16_t color) {
    if (!display_initialized || !disp) return;
    
    lv_obj_t* scr = lv_disp_get_scr_act(disp);
    lv_obj_set_style_bg_color(scr, lv_color_hex(color), 0);
    lv_refr_now(disp);
}

void display_lvgl_tick(void) {
    if (!display_initialized) return;
    lv_tick_inc(10);  // Increment by 10ms
}

void display_lvgl_handler(void) {
    if (!display_initialized) return;
    lv_timer_handler();
}

lv_disp_t* display_get_lvgl_disp(void) {
    return disp;
}
