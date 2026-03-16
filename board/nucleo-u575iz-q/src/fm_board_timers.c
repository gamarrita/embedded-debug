#include "fm_board_timers.h"
#include "fm_board_clk.h"
#include "stm32u5xx_hal.h"
#include "stm32u5xx_hal_tim.h"

static TIM_HandleTypeDef s_tim7_handle;
static uint32_t s_tim7_workload_cycles = 0U;

/* Public Bodies */
void FM_BOARD_TIMERS_Init(void)
{
    /* RTC is initialized by MX_RTC_Init() in main.c */
}

void FM_BOARD_TIMERS_DelayMs(uint32_t delay_ms)
{
    HAL_Delay(delay_ms);
}

bool FM_BOARD_TIMERS_ConfigLoadTimer(uint32_t interval_us,
                                     uint32_t workload_us)
{
    __HAL_RCC_TIM7_CLK_ENABLE();

    uint32_t cycles_per_us = FM_BOARD_CLK_CyclesPerUs();
    if (cycles_per_us == 0U)
    {
        cycles_per_us = 1U;
    }

    uint32_t prescaler = cycles_per_us - 1U;

    s_tim7_handle.Instance = TIM7;
    s_tim7_handle.Init.Prescaler = prescaler;
    s_tim7_handle.Init.CounterMode = TIM_COUNTERMODE_UP;
    s_tim7_handle.Init.Period = (interval_us > 0U) ? (interval_us - 1U) : 0U;
    s_tim7_handle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    s_tim7_handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    if (HAL_TIM_Base_Init(&s_tim7_handle) != HAL_OK)
    {
        return false;
    }

    s_tim7_workload_cycles = cycles_per_us * workload_us;

    HAL_NVIC_SetPriority(TIM7_IRQn, 6U, 0U);

    return true;
}

bool FM_BOARD_TIMERS_StartLoadTimer(void)
{
    __HAL_TIM_CLEAR_FLAG(&s_tim7_handle, TIM_FLAG_UPDATE);
    if (HAL_TIM_Base_Start_IT(&s_tim7_handle) != HAL_OK)
    {
        return false;
    }
    HAL_NVIC_EnableIRQ(TIM7_IRQn);
    return true;
}

bool FM_BOARD_TIMERS_StopLoadTimer(void)
{
    if (HAL_TIM_Base_Stop_IT(&s_tim7_handle) != HAL_OK)
    {
        return false;
    }
    HAL_NVIC_DisableIRQ(TIM7_IRQn);
    return true;
}

void TIM7_IRQHandler(void)
{
    if (__HAL_TIM_GET_FLAG(&s_tim7_handle, TIM_FLAG_UPDATE) != RESET)
    {
        if (__HAL_TIM_GET_IT_SOURCE(&s_tim7_handle, TIM_IT_UPDATE) != RESET)
        {
            __HAL_TIM_CLEAR_IT(&s_tim7_handle, TIM_IT_UPDATE);
            for (volatile uint32_t i = 0U; i < s_tim7_workload_cycles; i++)
            {
                __NOP();
            }
        }
    }
}
