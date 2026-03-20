#ifndef FM_HW_CLK_H
#define FM_HW_CLK_H

#include <stdint.h>

/* HCLK frequency in Hz via HAL. */
uint32_t FM_HW_CLK_GetHclkHz(void);

/* Cached CPU cycles per microsecond. */
uint32_t FM_HW_CLK_CyclesPerUs(void);

#endif /* FM_HW_CLK_H */
