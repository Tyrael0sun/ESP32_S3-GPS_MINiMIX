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

#endif // DIAGNOSTICS_H
