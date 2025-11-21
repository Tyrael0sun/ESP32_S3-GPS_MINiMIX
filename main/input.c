#include "input.h"
#include "config.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

static const char *TAG = "INPUT";

// External function to trigger diagnostics (declared in main.c usually, or via header)
extern void diagnostics_trigger(const char *event);

static void IRAM_ATTR gpio_isr_handler(void* arg) {
    uint32_t gpio_num = (uint32_t) arg;
    // Minimal logic in ISR
    // We can use a queue or notify task.
    // For prototype, we might call a function that notifies task, or check debounce.
    // Let's assume we notify the diagnostics system.
    // Note: diagnostics_trigger prints log, which is NOT ISR safe.
    // We should use a queue.
}

// Polling task for Input (Simpler for debounce than ISR in this scope)
// User requested "timely response". ISR + Queue is best, or high freq polling.
// Given Encoder logic (state machine), polling at 1-2ms is often robust and simple.

static void input_task(void *arg) {
    int enc_a_prev = gpio_get_level(ENC_A_PIN);
    int enc_b_prev = gpio_get_level(ENC_B_PIN);
    int key_prev = gpio_get_level(KEY_MAIN_PIN);

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
        if (key != key_prev) {
            if (key == 0) { // Press (Active Low)
                diagnostics_trigger("KEY: PRESSED");
            } else {
                 diagnostics_trigger("KEY: RELEASED");
            }
            key_prev = key;
        }

        vTaskDelay(pdMS_TO_TICKS(5)); // 5ms poll
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

    // Create Polling Task
    xTaskCreate(input_task, "input_task", 2048, NULL, 10, NULL); // High priority

    ESP_LOGI(TAG, "Input configured successfully.");
    return ESP_OK;
}
