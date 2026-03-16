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
#include "fm_irq_health.h"
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

/* Module configuration */
/* =========================== Private Types ================================ */
/* (none) */

/* =========================== Private Data ================================= */
TIM_HandleTypeDef htim6;
static fm_irq_health_t tim6_irq_health;
static volatile uint32_t s_validation_sel = 0U;
static uint32_t tim6_self_load_cycles = 0U;

/* =========================== Private Prototypes =========================== */
static void app_uart_send(const char *buf, size_t len);

/* =========================== Private Bodies =============================== */
static void app_uart_send(const char *buf, size_t len)
{
    if ((buf == NULL) || (len == 0U))
    {
        return;
    }

    if (FM_DEBUG_MsgIsEnabled())
    {
        (void) FM_BOARD_UART_Transmit((const uint8_t *) buf, (uint32_t) len, 10U);
    }
}

/* =========================== Public Bodies ================================ */

void FM_MAIN_Init(void)
{
    /* Application-level initialization.
     * Keep this module as the owner of the main control flow. */
	FM_BOARD_Init();
	FM_BOARD_DWT_Init();
	FM_IRQ_HEALTH_Init(&tim6_irq_health, TIM6_IDEAL_PERIOD_US, TIM6_JITTER_THRESHOLD_US);
}

/* Main execution loop.
 * Do not place user logic in auto-generated IDE files.
 *
 */
void FM_MAIN_Main(void)
{
    uint32_t last_report_ms = 0U;
    uint8_t led_signal_toggle = 0U;
    char line[160];

    FM_MAIN_Init();

    /*
     * TIM6: reference timer at 1 kHz used to measure interrupt health.
     * Arrival jitter and ISR execution time are collected with DWT.
     */
    __HAL_RCC_TIM6_CLK_ENABLE();
    htim6.Instance = TIM6;
    htim6.Init.Prescaler = 239; /* (Timer clk / (239 + 1)) = 100 kHz when timer clock is 24 MHz -> 10 µs tick */
    htim6.Init.Period = 99;     /* 100 kHz / (99 + 1) = 1 kHz -> 1 ms period */

    if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
    {
        Error_Handler();
    }

    /* Validation selector: choose one of the scenarios below */
    const uint32_t sel = 0U;
    s_validation_sel = sel;

    switch (sel)
	{
		case 0U:
			/* Case 0: baseline, no extra load. Expect near-zero jitter and zero violations. */
			(void) FM_BOARD_TIMERS_StopLoadTimer();
			break;
		case 1U:
		    /* Case 1: short periodic load (TIM7) to create occasional jitter spikes. */
		    if (FM_BOARD_TIMERS_ConfigLoadTimer(TIM7_LOAD_SHORT_INT_US, TIM7_LOAD_SHORT_WORK_US))
		    {
		        (void) FM_BOARD_TIMERS_StartLoadTimer();
		    }
			break;
		case 2U:
		    /* Case 2: heavier periodic load; expect more violations and wider jitter spread. */
		    if (FM_BOARD_TIMERS_ConfigLoadTimer(TIM7_LOAD_HEAVY_INT_US, TIM7_LOAD_HEAVY_WORK_US))
		    {
		        (void) FM_BOARD_TIMERS_StartLoadTimer();
		    }
			break;
        case 3U:
            /* Case 3: self-load inside TIM6 ISR to validate execution-time tracking. */
            (void) FM_BOARD_TIMERS_StopLoadTimer();
            tim6_self_load_cycles = FM_BOARD_DWT_UsToCycles(TIM6_SELF_LOAD_US);
            if (tim6_self_load_cycles == 0U)
            {
                tim6_self_load_cycles = 1U;
            }
            break;
		default:
			break;
	}

    __HAL_TIM_CLEAR_FLAG(&htim6, TIM_FLAG_UPDATE);
    HAL_TIM_Base_Start(&htim6);

    HAL_NVIC_SetPriority(TIM6_IRQn, 7, 0);
    __HAL_TIM_ENABLE_IT(&htim6, TIM_IT_UPDATE);
    HAL_NVIC_EnableIRQ(TIM6_IRQn);

    last_report_ms = HAL_GetTick();

    for (;;)
    {
        /* Activity blink for visual heartbeat (honors debug jumper) */
        led_signal_toggle ^= 1U;
        FM_DEBUG_LedSignal((led_signal_toggle != 0U) ? FM_DEBUG_LED_ON : FM_DEBUG_LED_OFF);

        uint32_t now_ms = HAL_GetTick();
        if ((now_ms - last_report_ms) >= REPORT_INTERVAL_MS)
        {
            fm_irq_health_stats_t stats;
            FM_IRQ_HEALTH_GetStats(&tim6_irq_health, &stats);

            int len = snprintf(line, sizeof(line),
                               "TIM6[1s]: n=%lu viol=%lu jit(us) last=%ld min=%ld max=%ld exec(us) last=%lu min=%lu max=%lu\r\n",
                               (unsigned long) stats.sample_count,
                               (unsigned long) stats.violation_count,
                               (long) stats.jitter_last_us,
                               (long) stats.jitter_min_us,
                               (long) stats.jitter_max_us,
                               (unsigned long) stats.exec_last_us,
                               (unsigned long) stats.exec_min_us,
                               (unsigned long) stats.exec_max_us);

            if (len > 0)
            {
                app_uart_send(line, (size_t) len);
            }

            FM_IRQ_HEALTH_ResetStats(&tim6_irq_health);
            last_report_ms = now_ms;
        }

        HAL_Delay(100);
    }
}

/* =========================== Interrupts =================================== */

/*
 * TIM6 update ISR — measures arrival jitter + execution time with minimal overhead.
 */
void TIM6_IRQHandler(void)
{
    if (TIM6->SR & TIM_SR_UIF)
    {
        FM_IRQ_HEALTH_OnEntry(&tim6_irq_health);

        if (s_validation_sel == 3U)
        {
            for (volatile uint32_t i = 0U; i < tim6_self_load_cycles; i++)
            {
                __NOP();
            }
        }

        FM_IRQ_HEALTH_OnExit(&tim6_irq_health);

        TIM6->SR &= ~TIM_SR_UIF;
    }
}

/*** end of file ***/
