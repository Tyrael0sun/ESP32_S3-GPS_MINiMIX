#include "input.h"
#include "config.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

static const char *TAG = "INPUT";

// External function to trigger diagnostics
extern void diagnostics_trigger(const char *event);

// Button State Machine
typedef enum {
    BTN_IDLE,
    BTN_PRESSED,
    BTN_RELEASED,
    BTN_WAIT_DOUBLE
} btn_state_t;

static btn_state_t key_state = BTN_IDLE;
static int64_t key_press_time = 0;
static int64_t key_release_time = 0;

static void process_key_logic(int key_level) {
    int64_t now = esp_timer_get_time() / 1000; // ms

    // Active Low
    bool pressed = (key_level == 0);

    switch (key_state) {
        case BTN_IDLE:
            if (pressed) {
                key_state = BTN_PRESSED;
                key_press_time = now;
            }
            break;

        case BTN_PRESSED:
            if (!pressed) {
                // Released
                int64_t duration = now - key_press_time;
                key_state = BTN_RELEASED;
                key_release_time = now;

                if (duration > 2000) {
                    diagnostics_trigger("KEY: LONG PRESS");
                    key_state = BTN_IDLE; // Reset
                } else if (duration > 500) {
                    diagnostics_trigger("KEY: MEDIUM PRESS");
                    key_state = BTN_IDLE; // Reset
                }
                // else: Short press candidate, wait for potential double click
            }
            break;

        case BTN_RELEASED:
            // Waiting for double click or timeout
            if (pressed) {
                // Second press
                key_state = BTN_PRESSED; // Go back to pressed, but we need to track it's 2nd
                // Simplified: Just trigger double click immediately on 2nd press for now?
                // Or better: Treat as new press but check gap.
                int64_t gap = now - key_release_time;
                if (gap < 300) { // 300ms double click window
                    diagnostics_trigger("KEY: DOUBLE CLICK");
                    key_state = BTN_WAIT_DOUBLE; // Wait for release of 2nd press to reset
                } else {
                    // Too slow, previous was short press
                    diagnostics_trigger("KEY: SHORT PRESS");
                    key_state = BTN_PRESSED;
                    key_press_time = now;
                }
            } else {
                int64_t gap = now - key_release_time;
                if (gap > 300) {
                    diagnostics_trigger("KEY: SHORT PRESS");
                    key_state = BTN_IDLE;
                }
            }
            break;

        case BTN_WAIT_DOUBLE:
            if (!pressed) {
                key_state = BTN_IDLE;
            }
            break;
    }
}

static void input_task(void *arg) {
    int enc_a_prev = gpio_get_level(ENC_A_PIN);
    // int key_prev = gpio_get_level(KEY_MAIN_PIN); // Handled by state machine

    const TickType_t poll_delay = pdMS_TO_TICKS(10) > 0 ? pdMS_TO_TICKS(10) : 1;

    while (1) {
        int enc_a = gpio_get_level(ENC_A_PIN);
        int enc_b = gpio_get_level(ENC_B_PIN);
        int key = gpio_get_level(KEY_MAIN_PIN);

        // Encoder Logic (Simplified)
        if (enc_a != enc_a_prev) {
            if (enc_a == 0) { // Falling edge A
                if (enc_b == 1) {
                    diagnostics_trigger("ENC: CW");
                } else {
                    diagnostics_trigger("ENC: CCW");
                }
            }
            enc_a_prev = enc_a;
        }

        // Key Logic
        process_key_logic(key);

        vTaskDelay(poll_delay);
    }
}

esp_err_t input_init(void) {
    ESP_LOGI(TAG, "Initializing Input (Encoder & Keys)...");

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << ENC_A_PIN) | (1ULL << ENC_B_PIN) | (1ULL << KEY_MAIN_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    esp_err_t err = gpio_config(&io_conf);
    if (err != ESP_OK) return err;

    xTaskCreate(input_task, "input_task", 2048, NULL, 5, NULL);

    ESP_LOGI(TAG, "Input configured successfully.");
    return ESP_OK;
}
