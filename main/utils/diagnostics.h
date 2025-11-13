/**
 * @file diagnostics.h
 * @brief System diagnostics and logging
 */

#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Initialize diagnostics system
 */
void diagnostics_init(void);

/**
 * @brief Run diagnostics check and log results
 */
void diagnostics_run(void);

/**
 * @brief Start diagnostics task
 */
void diagnostics_start_task(void);

/**
 * @brief Trigger immediate diagnostic log
 * @param reason Reason for trigger (e.g., "Key press: 150ms")
 */
void diagnostics_trigger(const char* reason);

#endif // DIAGNOSTICS_H
