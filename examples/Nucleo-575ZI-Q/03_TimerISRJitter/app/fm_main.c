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
#include "stm32u5xx_ll_rcc.h"

/* =========================== Private Defines ============================== */

/* Module configuration */
#define CPU_CYCLES_PER_US    (24U)

/* =========================== Private Types ================================ */
/* (none) */

/* =========================== Private Data ================================= */
TIM_HandleTypeDef htim6;
TIM_HandleTypeDef htim7;

/* =========================== Private Prototypes =========================== */
/* (none) */

/* =========================== Private Bodies =============================== */
/* (none) */

/* =========================== Public Bodies ================================ */

void FM_MAIN_Init(void)
{
    /* Application-level initialization.
     * Keep this module as the owner of the main control flow. */

    FM_DEBUG_Init();

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

    /*
     * TIM7: interfering workload timer (~100 Hz).
     * Injects a bounded ISR load to induce measurable jitter on TIM6.
     * Tune ARR/PSC or the NOP loop length in TIM7_IRQHandler to change hit rate.
     */
    __HAL_RCC_TIM7_CLK_ENABLE();
    htim7.Instance = TIM7;
    htim7.Init.Prescaler = 239;  /* 10 µs per tick */
    htim7.Init.Period = 1000;     /* ~10 ms period (100 Hz) */

    if (HAL_TIM_Base_Init(&htim7) != HAL_OK)
    {
        Error_Handler();
    }

    HAL_TIM_Base_Start(&htim7);

    HAL_NVIC_SetPriority(TIM7_IRQn, 6, 0);
    __HAL_TIM_ENABLE_IT(&htim7, TIM_IT_UPDATE);
    HAL_NVIC_EnableIRQ(TIM7_IRQn);

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
    const uint32_t period_cycles = SystemCoreClock / 1000U; /* 1 ms in cycles */

    if (TIM6->SR & TIM_SR_UIF)
    {
        uint32_t now_cycles = DWT->CYCCNT;

        if (!has_reference)
        {
            prev_tick_cycles = now_cycles; /* prime reference */
            has_reference = 1U;
            TIM6->SR &= ~TIM_SR_UIF;
            return;
        }

        int32_t interval_cycles = (int32_t)(now_cycles - prev_tick_cycles);
        prev_tick_cycles = now_cycles; /* store for next tick */

        int32_t err_cycles = interval_cycles - (int32_t)period_cycles; /* current-interval error */

        if ((err_cycles >= (int32_t)CPU_CYCLES_PER_US)
            || (err_cycles <= -(int32_t)CPU_CYCLES_PER_US))
        {
            FM_DEBUG_LedError(FM_DEBUG_LED_ON);
            FM_DEBUG_ReportError(FM_DEBUG_ERR_JITTER);
        }

        TIM6->SR &= ~TIM_SR_UIF;
    }
}

/*
 * TIM7 workload ISR — injects controlled latency to create measurable jitter.
 * - Uses DWT to timestamp entry/exit for quick inspection via debugger.
 * - Workload is a short NOP loop; adjust its length or TIM7 frequency to tune jitter rate.
 */
void TIM7_IRQHandler(void)
{
    static volatile uint32_t t_start_cycles = 0;
    static volatile uint32_t t_end_cycles = 0;
    static volatile uint32_t t_prev_cycles = 0;
    static volatile uint32_t t_min_cycles = 0xFFFFFFFFU;
    static volatile uint32_t t_max_cycles = 0;
    static volatile uint32_t t_interval_cycles = 0; /* cycles between consecutive TIM7 IRQs */
    static volatile uint32_t dur_cycles = 0;

    if (TIM7->SR & TIM_SR_UIF)
    {
        t_start_cycles = DWT->CYCCNT;

        /* Controlled workload: adjust loop length to tune injected load. */
        for (volatile uint32_t i = 0; i < 90U; i++)
        {
            __NOP();
        }

        t_end_cycles = DWT->CYCCNT;
        t_interval_cycles = t_start_cycles - t_prev_cycles;
        t_prev_cycles = t_start_cycles;
        dur_cycles = t_end_cycles - t_start_cycles;

        (void)t_interval_cycles; /* used for live debug inspection */

        if (dur_cycles < t_min_cycles)
        {
            t_min_cycles = dur_cycles;
        }
        if (dur_cycles > t_max_cycles)
        {
            t_max_cycles = dur_cycles;
        }

        TIM7->SR &= ~TIM_SR_UIF;
    }

}

/*** end of file ***/
