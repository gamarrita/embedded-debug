/**
 * @file    fm_main.c
 * @brief   Application entry point and main execution loop.
 *
 * @date    2026-02-28
 * @author  Daniel H Sagarra
 *
 * @details
 *  - Fixed section layout (template-friendly).
 *  - Public API: FM_MAIN_*.
 *  - Private symbols: static snake_case.
 *  - ISRs shall remain bounded and non-blocking (set flags only).
 */

/* ===========================     Includes    ============================== */
#include "main.h"
#include "fm_main.h"
#include "fm_debug.h"
#include "fm_board.h"
#include "string.h"
#include <stdio.h>
#include "stm32u5xx_ll_rcc.h"

/* =========================== Private Defines ============================== */
#define TIM6_IDEAL_PERIOD_US      (1000U)
#define TIM6_JITTER_THRESHOLD_US  (10U)

#define TIM7_LOAD_SHORT_INT_US    (10000U)
#define TIM7_LOAD_SHORT_WORK_US   (50U)
#define TIM7_LOAD_HEAVY_INT_US    (5000U)
#define TIM7_LOAD_HEAVY_WORK_US   (150U)

#define TIM6_SELF_LOAD_US         (40U)
#define REPORT_INTERVAL_MS        (1000U)
#define REPORT_POLL_MS            (10U)

/* Module configuration */
/* =========================== Private Types ================================ */
/* (none) */

/* =========================== Private Data ================================= */
TIM_HandleTypeDef htim6;
static volatile uint32_t s_validation_sel = 0U;

/* =========================== Private Prototypes =========================== */

/* =========================== Private Bodies =============================== */


/* =========================== Public Bodies ================================ */

void FM_MAIN_Init(void)
{
    /* Application-level initialization.
     * Keep this module as the owner of the main control flow. */
	FM_BOARD_Init();
	FM_DEBUG_Init();
}

/* Main execution loop.
 * Do not place user logic in auto-generated IDE files.
 *
 */
void FM_MAIN_Main(void)
{

    FM_MAIN_Init();



    for (;;)
    {


    }
}

/* =========================== Interrupts =================================== */

/*** end of file ***/
