#include "fm_board_clk.h"
#include "stm32u5xx_hal.h"

static uint32_t s_cycles_per_us = 0U;

uint32_t FM_BOARD_CLK_GetHclkHz(void)
{
    return HAL_RCC_GetHCLKFreq();
}

uint32_t FM_BOARD_CLK_CyclesPerUs(void)
{
    if (s_cycles_per_us == 0U)
    {
        s_cycles_per_us = FM_BOARD_CLK_GetHclkHz() / 1000000U;
        if (s_cycles_per_us == 0U)
        {
            s_cycles_per_us = 1U; /* evita divisi?n por cero */
        }
    }

    return s_cycles_per_us;
}
