/**
 * @file pbox_app.h
 * @brief P-Box performance testing application
 */

#ifndef PBOX_APP_H
#define PBOX_APP_H

#include <stdint.h>
#include <stdbool.h>

enum PBoxState {
    PBOX_IDLE,
    PBOX_READY,
    PBOX_TESTING,
    PBOX_FINISHED
};

struct PBoxData {
    PBoxState state;
    float current_speed;    // km/h
    float test_time;        // seconds
    float start_speed;      // km/h
    float target_speed;     // km/h
};

/**
 * @brief Initialize P-Box app
 */
void pbox_init(void);

/**
 * @brief Update P-Box state
 */
void pbox_update(void);

/**
 * @brief Get P-Box data
 */
PBoxData pbox_get_data(void);

/**
 * @brief Reset test
 */
void pbox_reset(void);

/**
 * @brief Set test parameters
 */
void pbox_set_test_params(float start_speed, float target_speed);

#endif // PBOX_APP_H
