#include "display.h"
#include "config.h"
#include "esp_log.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "freertos/semphr.h"
#include "esp_timer.h"

static const char *TAG = "DISPLAY";

// LVGL Configuration
#define LVGL_TICK_PERIOD_MS    2
#define DISP_BUF_SIZE          (240 * 40) // 40 lines buffer

static SemaphoreHandle_t lvgl_mux = NULL;
static esp_lcd_panel_handle_t panel_handle = NULL;
static lv_disp_drv_t disp_drv; // Static instance

static bool notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx) {
    lv_disp_drv_t *disp_driver = (lv_disp_drv_t *)user_ctx;
    lv_disp_flush_ready(disp_driver);
    return false;
}

static void lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map) {
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t) drv->user_data;
    int offsetx1 = area->x1;
    int offsetx2 = area->x2;
    int offsety1 = area->y1;
    int offsety2 = area->y2;

    esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, color_map);
}

static void lvgl_tick_task(void *arg) {
    lv_tick_inc(LVGL_TICK_PERIOD_MS);
}

esp_err_t display_init(void) {
    ESP_LOGI(TAG, "Initializing Display...");

    lvgl_mux = xSemaphoreCreateMutex();

    // 1. Initialize SPI Bus
    spi_bus_config_t buscfg = {
        .sclk_io_num = DISP_SCK_PIN,
        .mosi_io_num = DISP_MOSI_PIN,
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 240 * 320 * 2 + 8
    };
    ESP_ERROR_CHECK(spi_bus_initialize(DISP_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO));

    // 2. Install Panel IO
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = DISP_DC_PIN,
        .cs_gpio_num = DISP_CS_PIN,
        .pclk_hz = 40 * 1000 * 1000, // 40 MHz
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
        .on_color_trans_done = notify_lvgl_flush_ready,
        .user_ctx = &disp_drv, // Pass the static driver instance
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)DISP_SPI_HOST, &io_config, &io_handle));

    // 3. Install ST7789 Panel Driver
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = DISP_RST_PIN,
        .rgb_endian = LCD_RGB_ENDIAN_RGB, // Default
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));

    // 4. Reset and Initialize Panel
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true)); // Often needed for ST7789

    // Spec: Vertical, Rotated 180.
    // Standard vertical is usually 240x320. 180 rotation might mean mirror X and Y.
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, false)); // Keep vertical
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, true)); // 180 degree rotation typically means mirroring both axes or specific command.

    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    // 5. Initialize Backlight (PWM)
    gpio_config_t bl_conf = {
        .pin_bit_mask = (1ULL << DISP_BL_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&bl_conf);
    gpio_set_level(DISP_BL_PIN, 1);

    // 6. Initialize LVGL
    lv_init();

    // Alloc draw buffers
    lv_color_t *buf1 = heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
    lv_color_t *buf2 = heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);

    static lv_disp_draw_buf_t disp_buf;
    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, DISP_BUF_SIZE);

    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = 240;
    disp_drv.ver_res = 320;
    disp_drv.flush_cb = lvgl_flush_cb;
    disp_drv.draw_buf = &disp_buf;
    disp_drv.user_data = panel_handle;

    lv_disp_drv_register(&disp_drv);

    // 7. Tick Timer
    const esp_timer_create_args_t tick_timer_args = {
        .callback = &lvgl_tick_task,
        .name = "lvgl_tick"
    };
    esp_timer_handle_t tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&tick_timer_args, &tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(tick_timer, LVGL_TICK_PERIOD_MS * 1000));

    return ESP_OK;
}

bool display_lock(int timeout_ms) {
    if (lvgl_mux) {
        return xSemaphoreTake(lvgl_mux, pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
    }
    return false;
}

void display_unlock(void) {
    if (lvgl_mux) {
        xSemaphoreGive(lvgl_mux);
    }
}
