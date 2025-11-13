/**
 * @file encoder_driver.h
 * @brief Rotary encoder and button input driver
 */

#ifndef ENCODER_DRIVER_H
#define ENCODER_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

enum KeyEvent {
    KEY_NONE = 0,
    KEY_SHORT_PRESS,
    KEY_MEDIUM_PRESS,
    KEY_LONG_PRESS,
    KEY_DOUBLE_CLICK
};

enum RotaryEvent {
    ROTARY_NONE = 0,
    ROTARY_CW,   // Clockwise
    ROTARY_CCW   // Counter-clockwise
};

typedef void (*encoder_callback_t)(int32_t count);
typedef void (*key_callback_t)(KeyEvent event);

/**
 * @brief Initialize encoder and button
 * @return true on success, false on failure
 */
bool encoder_init(void);

/**
 * @brief Register encoder rotation callback
 * @param callback Callback function
 */
void encoder_register_callback(encoder_callback_t callback);

/**
 * @brief Register key event callback
 * @param callback Callback function
 */
void key_register_callback(key_callback_t callback);

/**
 * @brief Get encoder count with automatic debounce
 * @return Current encoder count
 * @note Auto-clears count after 500ms of inactivity to prevent drift
 */
int32_t encoder_get_count(void);

/**
 * @brief Reset encoder count
 */
void encoder_reset_count(void);

#endif // ENCODER_DRIVER_H
