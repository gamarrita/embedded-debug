#ifndef FM_BOARD_DWT_H
#define FM_BOARD_DWT_H

#include <stdbool.h>
#include <stdint.h>
#include "main.h"

#if defined(DWT) && defined(CoreDebug) &&     defined(DWT_CTRL_CYCCNTENA_Msk) && defined(CoreDebug_DEMCR_TRCENA_Msk)
#define FM_BOARD_DWT_HAS_CYCCNT 1
#else
#define FM_BOARD_DWT_HAS_CYCCNT 0
#endif

bool FM_BOARD_DWT_Init(void);
uint32_t FM_BOARD_DWT_GetCpuHz(void);
uint32_t FM_BOARD_DWT_CyclesPerUs(void);
uint32_t FM_BOARD_DWT_UsToCycles(uint32_t us);
int32_t FM_BOARD_DWT_CyclesToUs(int32_t cycles);

static inline uint32_t FM_BOARD_DWT_GetCycles(void)
{
#if FM_BOARD_DWT_HAS_CYCCNT
    return DWT->CYCCNT;
#else
    return 0U;
#endif
}

static inline void FM_BOARD_DWT_Reset(void)
{
#if FM_BOARD_DWT_HAS_CYCCNT
    DWT->CYCCNT = 0;
#endif
}

#endif /* FM_BOARD_DWT_H */
