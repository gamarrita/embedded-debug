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
#include "string.h"
#include "fm_debug.h"

/* =========================== Private Defines ============================== */

/* Module configuration */
#ifndef FM_MAIN_LED_BLINK_MS
#define FM_MAIN_LED_BLINK_MS   (250U)
#endif

/* =========================== Private Types ================================ */
/* (none) */

/* =========================== Private Data ================================= */
TIM_HandleTypeDef htim6;

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

/* Main execution loop.
 * Do not place user logic in auto-generated IDE files.
 *
 */
void FM_MAIN_Main(void)
{
    __HAL_RCC_TIM6_CLK_ENABLE(); // (DHS ERROR #1) Primer error
    htim6.Instance = TIM6;
    htim6.Init.Prescaler = 8399;
    htim6.Init.Period = 4999;
    __HAL_RCC_TIM6_CLK_ENABLE();
    HAL_TIM_Base_Init(&htim6);
    HAL_TIM_Base_Start(&htim6);

    char msg[] = "Alive";
    FM_DEBUG_UartMsg(msg, strlen(msg));


    for (;;)
    {
        if((TIM6->CNT) < 2500)
        {
            HAL_GPIO_WritePin(LED1_GPIO_PORT, LED1_PIN, GPIO_PIN_SET);
        }
        else
        {
            HAL_GPIO_WritePin(LED1_GPIO_PORT, LED1_PIN, GPIO_PIN_RESET);
        }
    }
}

/* =========================== Interrupts =================================== */
/* Keep ISR work minimal: set flags, capture timestamps/state, and return. */

/**
 * @brief  Wake Up Timer callback.
 * @param  hrtc RTC handle
 * @retval None
 */


void TIM6_IRQHandler(void)
{
   if (TIM6->SR & TIM_SR_UIF)
   {
       TIM6->SR &= ~TIM_SR_UIF; // Clear flag
       uint32_t timestamp = HAL_GetTick();
       FM_DEBUG_UartInt32(timestamp);
   }
}

/*** end of file ***/
