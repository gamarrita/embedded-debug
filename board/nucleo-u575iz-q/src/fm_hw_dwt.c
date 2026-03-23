#include <fm_hw_dwt.h>

bool FM_HW_DWT_Init(void)
{
#if FM_HW_DWT_HAS_CYCCNT
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    return true;
#else
    return false; /* CYCCNT not present on this device */
#endif
}

uint32_t FM_HW_DWT_GetCpuHz(void)
{
    return SystemCoreClock;
}

uint32_t FM_HW_DWT_CyclesPerUs(void)
{
    return FM_HW_DWT_GetCpuHz() / 1000000U; /* returns 0 when clock is unknown */
}

uint32_t FM_HW_DWT_UsToCycles(uint32_t us)
{
    uint32_t cycles_per_us = FM_HW_DWT_CyclesPerUs();
    if (cycles_per_us == 0U)
    {
        return 0U;
    }

    /* 64-bit math avoids overflow for larger intervals */
    uint64_t cycles = (uint64_t)cycles_per_us * (uint64_t)us;
    return (uint32_t)cycles;
}

int32_t FM_HW_DWT_CyclesToUs(int32_t cycles)
{
    uint32_t cycles_per_us = FM_HW_DWT_CyclesPerUs();
    if (cycles_per_us == 0U)
    {
        return 0;
    }

    return cycles / (int32_t)cycles_per_us;
}
