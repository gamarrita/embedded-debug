/**
 * @file    fm_main.h
 * @brief   Flowmeet application entry point (APP layer).
 *
 * @details
 *  - Exposes ONLY the public API of this module.
 *  - No HAL details or Flash addresses shall be included here.
 *  - Include this header in the IDE-generated main.c and call FM_MAIN_Main().
 */

#ifndef FM_MAIN_H_
#define FM_MAIN_H_

/* =========================== Includes ==================================== */

#include <stdint.h>
#include <stdbool.h>

/* Optional public dependencies */
/* #include "fmx.h"     *//* If main requires public RTOS types */
/* #include "fm_log.h"  *//* If main calls logging modules       */

/* =========================== Public Macros ================================ */

/* Unit-suffixed configuration constants */

#define FM_MAIN_LOOP_PERIOD_MS        (10U)    /* Main loop period (ms) */
#define FM_MAIN_WATCHDOG_TIMEOUT_SEC  (2U)     /* Watchdog timeout (s)  */
#define FM_MAIN_STACK_SIZE_BYTES      (2048U)  /* Thread stack size     */

/* =========================== Public Types ================================= */

/**
 * @brief Module status codes.
 */
typedef enum {
    FM_MAIN_OK = 0, FM_MAIN_ERROR
} fm_main_status_t;

/* =========================== Public API =================================== */

/**
 * @brief Initializes the application.
 *
 * This function initializes drivers, modules, and RTOS (if applicable).
 */
void FM_MAIN_Init(void);

/**
 * @brief Application main loop.
 *
 * Used when no scheduler/RTOS is present.
 * If ThreadX or another RTOS is used, this function may remain minimal
 * and delegate control to application threads.
 */
void FM_MAIN_Main(void);

#endif /* FM_MAIN_H_ */

/*** end of file ***/
