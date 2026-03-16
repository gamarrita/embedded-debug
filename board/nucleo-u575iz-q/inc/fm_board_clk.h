#ifndef FM_BOARD_CLK_H
#define FM_BOARD_CLK_H

#include <stdint.h>

/**
 * @brief Devuelve la frecuencia del reloj HCLK en Hz utilizando HAL.
 */
uint32_t FM_BOARD_CLK_GetHclkHz(void);

/**
 * @brief Devuelve el numero de ciclos de CPU por microsegundo (precalculado).
 */
uint32_t FM_BOARD_CLK_CyclesPerUs(void);

#endif /* FM_BOARD_CLK_H */
