#ifndef FM_HW_DWT_H
#define FM_HW_DWT_H

#include <stdbool.h>
#include <stdint.h>
#include "main.h"

#if defined(DWT) && defined(CoreDebug) &&     defined(DWT_CTRL_CYCCNTENA_Msk) && defined(CoreDebug_DEMCR_TRCENA_Msk)
#define FM_HW_DWT_HAS_CYCCNT 1
#else
#define FM_HW_DWT_HAS_CYCCNT 0
#endif

/* Optional CYCCNT support; helpers return 0 when not present. */
bool FM_HW_DWT_Init(void);
uint32_t FM_HW_DWT_GetCpuHz(void);
uint32_t FM_HW_DWT_CyclesPerUs(void);
uint32_t FM_HW_DWT_UsToCycles(uint32_t us);
int32_t FM_HW_DWT_CyclesToUs(int32_t cycles);

static inline uint32_t FM_HW_DWT_GetCycles(void)
{
#if FM_HW_DWT_HAS_CYCCNT
    return DWT->CYCCNT;
#else
    return 0U; /* no cycle counter */
#endif
}

static inline void FM_HW_DWT_Reset(void)
{
#if FM_HW_DWT_HAS_CYCCNT
    DWT->CYCCNT = 0;
#endif
}

#endif /* FM_HW_DWT_H */
