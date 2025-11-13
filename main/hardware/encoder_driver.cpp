/**
 * @file encoder_driver.cpp
 * @brief Rotary encoder and button driver implementation using PCNT and GPIO ISR
 */

#include "encoder_driver.h"
#include "config.h"
#include "esp_log.h"
#include "driver/pulse_cnt.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

static const char* TAG = "ENCODER";

static pcnt_unit_handle_t pcnt_unit = NULL;
static encoder_callback_t encoder_cb = NULL;
static key_callback_t key_cb = NULL;

// Button state tracking
static uint32_t button_press_time = 0;
static uint32_t button_release_time = 0;
static uint32_t last_click_time = 0;
static bool button_pressed = false;
static TimerHandle_t long_press_timer = NULL;

// Encoder debounce mechanism (500ms auto-clear)
static int32_t last_encoder_count = 0;
static uint32_t last_encoder_change_time = 0;
#define ENCODER_DEBOUNCE_MS     500

static void IRAM_ATTR key_isr_handler(void* arg) {
    uint32_t now = xTaskGetTickCountFromISR() * portTICK_PERIOD_MS;
    int level = gpio_get_level((gpio_num_t)KEY_MAIN_GPIO);
    
    if (level == 0) { // Button pressed (active low with pull-up)
        if (!button_pressed && (now - button_release_time) > KEY_DEBOUNCE_MS) {
            button_pressed = true;
            button_press_time = now;
            
            // Start long press timer
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            if (long_press_timer) {
                xTimerStartFromISR(long_press_timer, &xHigherPriorityTaskWoken);
            }
        }
    } else { // Button released
        if (button_pressed && (now - button_press_time) > KEY_DEBOUNCE_MS) {
            button_pressed = false;
            button_release_time = now;
            uint32_t press_duration = button_release_time - button_press_time;
            
            // Stop long press timer
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            if (long_press_timer) {
                xTimerStopFromISR(long_press_timer, &xHigherPriorityTaskWoken);
            }
            
            // Determine event type
            if (press_duration >= KEY_LONG_PRESS_MS) {
                // Already handled by timer
            } else if (press_duration >= KEY_MEDIUM_PRESS_MS) {
                if (key_cb) {
                    key_cb(KEY_MEDIUM_PRESS);
                }
            } else if (press_duration < KEY_SHORT_PRESS_MS) {
                // Check for double click
                if ((now - last_click_time) < KEY_DOUBLE_CLICK_MS) {
                    if (key_cb) {
                        key_cb(KEY_DOUBLE_CLICK);
                    }
                    last_click_time = 0; // Reset to avoid triple click
                } else {
                    last_click_time = now;
                    if (key_cb) {
                        key_cb(KEY_SHORT_PRESS);
                    }
                }
            }
        }
    }
}

static void long_press_timer_callback(TimerHandle_t xTimer) {
    if (button_pressed && key_cb) {
        key_cb(KEY_LONG_PRESS);
    }
}

bool encoder_init(void) {
    esp_err_t ret;
    
    // Configure PCNT for encoder
    pcnt_unit_config_t unit_config = {
        .low_limit = -32768,
        .high_limit = 32767,
    };
    ret = pcnt_new_unit(&unit_config, &pcnt_unit);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create PCNT unit");
        return false;
    }
    
    pcnt_chan_config_t chan_a_config = {
        .edge_gpio_num = ENC_A_GPIO,
        .level_gpio_num = ENC_B_GPIO,
    };
    pcnt_channel_handle_t pcnt_chan_a = NULL;
    ret = pcnt_new_channel(pcnt_unit, &chan_a_config, &pcnt_chan_a);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create PCNT channel A");
        return false;
    }
    
    pcnt_chan_config_t chan_b_config = {
        .edge_gpio_num = ENC_B_GPIO,
        .level_gpio_num = ENC_A_GPIO,
    };
    pcnt_channel_handle_t pcnt_chan_b = NULL;
    ret = pcnt_new_channel(pcnt_unit, &chan_b_config, &pcnt_chan_b);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create PCNT channel B");
        return false;
    }
    
    // Configure channel actions
    pcnt_channel_set_edge_action(pcnt_chan_a, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_DECREASE);
    pcnt_channel_set_level_action(pcnt_chan_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE);
    
    pcnt_channel_set_edge_action(pcnt_chan_b, PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE);
    pcnt_channel_set_level_action(pcnt_chan_b, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE);
    
    // Enable and start
    pcnt_unit_enable(pcnt_unit);
    pcnt_unit_start(pcnt_unit);
    
    // Configure button GPIO
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << KEY_MAIN_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE
    };
    gpio_config(&io_conf);
    
    // Install GPIO ISR service
    gpio_install_isr_service(0);
    gpio_isr_handler_add((gpio_num_t)KEY_MAIN_GPIO, key_isr_handler, NULL);
    
    // Create long press timer
    long_press_timer = xTimerCreate("LongPress", pdMS_TO_TICKS(KEY_LONG_PRESS_MS), 
                                     pdFALSE, NULL, long_press_timer_callback);
    
    ESP_LOGI(TAG, "Encoder and button initialized");
    return true;
}

void encoder_register_callback(encoder_callback_t callback) {
    encoder_cb = callback;
}

void key_register_callback(key_callback_t callback) {
    key_cb = callback;
}

int32_t encoder_get_count(void) {
    int count = 0;
    if (pcnt_unit) {
        pcnt_unit_get_count(pcnt_unit, &count);
        
        // Auto-clear count after 500ms of inactivity (debounce)
        uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
        if (count != last_encoder_count) {
            last_encoder_count = count;
            last_encoder_change_time = now;
        } else if (count != 0 && (now - last_encoder_change_time) > ENCODER_DEBOUNCE_MS) {
            // Clear count after 500ms of no change
            pcnt_unit_clear_count(pcnt_unit);
            last_encoder_count = 0;
            count = 0;
            ESP_LOGD(TAG, "Encoder count auto-cleared after %dms timeout", ENCODER_DEBOUNCE_MS);
        }
    }
    return count;
}

void encoder_reset_count(void) {
    if (pcnt_unit) {
        pcnt_unit_clear_count(pcnt_unit);
    }
}
