#include "fm_board_dwt.h"

bool FM_BOARD_DWT_Init(void)
{
#if FM_BOARD_DWT_HAS_CYCCNT
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    return true;
#else
    return false;
#endif
}

uint32_t FM_BOARD_DWT_GetCpuHz(void)
{
    return SystemCoreClock;
}

uint32_t FM_BOARD_DWT_CyclesPerUs(void)
{
    return FM_BOARD_DWT_GetCpuHz() / 1000000U;
}
