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
#include "string.h"
#include "stm32u5xx_ll_rcc.h"

/* =========================== Private Defines ============================== */

/* Module configuration */
/* =========================== Private Types ================================ */
/* (none) */

/* =========================== Private Data ================================= */
TIM_HandleTypeDef htim6;
static uint32_t cycles_per_us = 0U;
static uint32_t period_cycles = 0U;

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

	cycles_per_us = FM_BOARD_DWT_CyclesPerUs();
	period_cycles = FM_BOARD_DWT_GetCpuHz() / 1000U; /* 1 ms worth of cycles */
}

/* Main execution loop.
 * Do not place user logic in auto-generated IDE files.
 *
 */
void FM_MAIN_Main(void)
{
    char msg[] = "Entry: FM_MAIN_Main\n";
    int led_signal_toggle = 0;
    uint32_t jitters = 0;

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

    /* Inicializa y configura el generador de carga del depurador (TIM7) */
    FM_DEBUG_LoadGenInit();
    FM_DEBUG_LoadGenConfigure(10000U, 90U, 6U);
    FM_DEBUG_LoadGenStart();

    FM_DEBUG_UartMsg(msg, strlen(msg));

    for (;;)
    {
        led_signal_toggle ^= 1;
        FM_DEBUG_LedSignal(led_signal_toggle);

        jitters = FM_DEBUG_ErrorCount(FM_DEBUG_ERR_JITTER);
        FM_DEBUG_UartInt32(jitters);

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
    static uint32_t prev_tick_cycles = 0;
    static uint8_t has_reference = 0;
    const uint32_t period_cycles_local = period_cycles; /* precomputed: 1 ms in cycles */

    if (TIM6->SR & TIM_SR_UIF)
    {
        uint32_t now_cycles = FM_BOARD_DWT_GetCycles();

        if (!has_reference)
        {
            prev_tick_cycles = now_cycles; /* prime reference */
            has_reference = 1U;
            TIM6->SR &= ~TIM_SR_UIF;
            return;
        }

        int32_t interval_cycles = (int32_t)(now_cycles - prev_tick_cycles);
        prev_tick_cycles = now_cycles; /* store for next tick */

        int32_t err_cycles = interval_cycles - (int32_t)period_cycles_local; /* current-interval error */

        if ((err_cycles >= (int32_t)cycles_per_us)
            || (err_cycles <= -(int32_t)cycles_per_us))
        {
            FM_DEBUG_ReportError(FM_DEBUG_ERR_JITTER);
        }

        TIM6->SR &= ~TIM_SR_UIF;
    }
}

/*** end of file ***/
