/**
 * @file bike_computer_app.h
 * @brief Bike computer application logic
 */

#ifndef BIKE_COMPUTER_APP_H
#define BIKE_COMPUTER_APP_H

#include <stdint.h>
#include <stdbool.h>

struct BikeComputerData {
    float current_speed;    // km/h
    float altitude;         // m
    float trip_distance;    // km
    uint32_t trip_time;     // seconds
    bool recording;
};

/**
 * @brief Initialize bike computer app
 */
void bike_computer_init(void);

/**
 * @brief Update bike computer state
 */
void bike_computer_update(void);

/**
 * @brief Get bike computer data
 */
BikeComputerData bike_computer_get_data(void);

/**
 * @brief Reset trip data
 */
void bike_computer_reset_trip(void);

#endif // BIKE_COMPUTER_APP_H
