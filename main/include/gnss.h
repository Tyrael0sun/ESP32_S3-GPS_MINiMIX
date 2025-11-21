#ifndef GNSS_H
#define GNSS_H

#include "esp_err.h"

/**
 * @brief Initialize GNSS UART and switch baud rate to 115200
 *
 * @return esp_err_t ESP_OK on success
 */
esp_err_t gnss_init(void);

/**
 * @brief Main loop for GNSS task
 * @param pvParameters
 */
void gnss_task_entry(void *pvParameters);

#endif // GNSS_H
