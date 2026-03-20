#include <fm_hw_clk.h>
#include "stm32u5xx_hal.h"

static uint32_t s_cycles_per_us = 0U; /* cached to avoid repeated division */

uint32_t FM_HW_CLK_GetHclkHz(void)
{
    return HAL_RCC_GetHCLKFreq();
}

uint32_t FM_HW_CLK_CyclesPerUs(void)
{
    if (s_cycles_per_us == 0U)
    {
        s_cycles_per_us = FM_HW_CLK_GetHclkHz() / 1000000U;
        if (s_cycles_per_us == 0U)
        {
            s_cycles_per_us = 1U; /* avoid divide-by-zero */
        }
    }

    return s_cycles_per_us;
}
