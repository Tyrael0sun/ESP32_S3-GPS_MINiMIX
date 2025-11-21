#ifndef INPUT_H
#define INPUT_H

#include "esp_err.h"

/**
 * @brief Initialize Input (Encoder and Buttons) with internal pull-ups
 *
 * @return esp_err_t ESP_OK on success
 */
esp_err_t input_init(void);

#endif // INPUT_H
