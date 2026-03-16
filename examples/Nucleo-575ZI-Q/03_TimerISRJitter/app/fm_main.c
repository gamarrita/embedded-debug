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
#include "fm_main.h"
#include "fm_debug.h"
#include "fm_board_gpio.h"
#include "fm_board_timers.h"
#include "fm_board_uart.h"
#include "fm_board.h"
#include "fm_board_dwt.h"
#include "fm_board_clk.h"
#include "jitter_meter.h"
#include "string.h"
#include "stm32u5xx_ll_rcc.h"

/* =========================== Private Defines ============================== */
#define JITTER_THRESHOLD_US   (1U)

/* Module configuration */
/* =========================== Private Types ================================ */
/* (none) */

/* =========================== Private Data ================================= */
TIM_HandleTypeDef htim6;
static FM_JitterMeter_t tim6_jitter_ctx;

/* =========================== Private Prototypes =========================== */
/* (none) */

/* =========================== Private Bodies =============================== */
/* (none) */

/* =========================== Public Bodies ================================ */

void FM_MAIN_Init(void)
{
    /* Application-level initialization.
     * Keep this module as the owner of the main control flow. */
	FM_BOARD_Init();
	FM_BOARD_DWT_Init();
	FM_JitterMeter_Init(&tim6_jitter_ctx, 1000U); /* ideal period: 1 ms */
}

/* Main execution loop.
 * Do not place user logic in auto-generated IDE files.
 *
 */
void FM_MAIN_Main(void)
{
    char msg[] = "Entry: FM_MAIN_Main\n";
    int led_signal_toggle = 0;
    FM_MAIN_Init();


    /*
     * TIM6: reference timer at 1 kHz used to measure interrupt jitter.
     * Deviation versus the ideal 1 ms period is computed in its ISR using DWT.
     */
    __HAL_RCC_TIM6_CLK_ENABLE();
    htim6.Instance = TIM6;
    htim6.Init.Prescaler = 239; /* 24 MHz / (239 + 1) = 100 kHz → 10 µs per tick */
    htim6.Init.Period = 99;     /* 100 kHz / (99 + 1) = 1 kHz → 1 ms period */

    if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
    {
        Error_Handler();
    }

    HAL_TIM_Base_Start(&htim6);

    HAL_NVIC_SetPriority(TIM6_IRQn, 7, 0);
    __HAL_TIM_ENABLE_IT(&htim6, TIM_IT_UPDATE);
    HAL_NVIC_EnableIRQ(TIM6_IRQn);

    /* Configura y arranca TIM7 como generador de carga */
    FM_BOARD_TIMERS_ConfigLoadTimer(10000U, 1U);
    FM_BOARD_TIMERS_StartLoadTimer();

    FM_DEBUG_UartMsg(msg, strlen(msg));

    for (;;)
    {
        led_signal_toggle ^= 1;
        FM_DEBUG_LedSignal(led_signal_toggle);

        fm_debug_error_t last = FM_DEBUG_LastError();
        int32_t val_us = FM_DEBUG_ErrorParam(last);
        const char *txt = FM_DEBUG_ErrorString(last);
        FM_DEBUG_UartMsg(txt, strlen(txt));
        FM_DEBUG_UartMsg(": ", 2);
        FM_DEBUG_UartInt32(val_us);

        HAL_Delay(1000);
    }
}

/* =========================== Interrupts =================================== */

/*
 * TIM6 update ISR — per-interval jitter using DWT.
 * Measures current interval vs. ideal 1 ms; flags jitter if |error| > 1 µs.
 * No cumulative drift; only instantaneous latency is considered.
 */
void TIM6_IRQHandler(void)
{
    if (TIM6->SR & TIM_SR_UIF)
    {
        int32_t error_us;

        if (FM_JitterMeter_Sample(&tim6_jitter_ctx, &error_us))
        {
            if ((error_us >= (int32_t)JITTER_THRESHOLD_US)
                || (error_us <= -(int32_t)JITTER_THRESHOLD_US))
            {
                FM_DEBUG_ReportErrorWithParam(FM_DEBUG_ERR_JITTER, error_us);
            }
        }

        TIM6->SR &= ~TIM_SR_UIF;
    }
}

/*** end of file ***/
