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

/* =========================== Private Defines ============================== */

/* Module configuration */
#ifndef FM_MAIN_LED_BLINK_MS
#define FM_MAIN_LED_BLINK_MS   (250U)
#endif

/* =========================== Private Types ================================ */
/* (none) */

/* =========================== Private Data ================================= */
/* (none) */

/* =========================== Private Prototypes =========================== */
/* (none) */

/* =========================== Private Bodies =============================== */
/* (none) */

/* =========================== Public Bodies ================================ */

void FM_MAIN_Init(void)
{
    /* Application-level initialization.
     Keep this module as the owner of the main control flow. */
}

void FM_MAIN_Main(void)
{
    /* Main execution loop.
     Do not place user logic in auto-generated IDE files. */

    char msg[] = "Alive";



    for (;;)
    {
        HAL_GPIO_WritePin(LED1_GPIO_PORT, LED1_PIN, GPIO_PIN_SET);
        FM_DEBUG_UartMsg(msg, sizeof(msg)-1);
        HAL_Delay(10);
        HAL_GPIO_WritePin(LED1_GPIO_PORT, LED1_PIN, GPIO_PIN_RESET);

        HAL_SuspendTick();
        HAL_PWREx_EnterSTOP2Mode(PWR_STOPENTRY_WFI);
        HAL_ResumeTick();

    }
}

/* =========================== Interrupts =================================== */
/* Keep ISR work minimal: set flags, capture timestamps/state, and return. */

/**
 * @brief  Wake Up Timer callback.
 * @param  hrtc RTC handle
 * @retval None
 */
void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(hrtc);


}

/*** end of file ***/
