#ifndef FM_BOARD_TIMERS_H
#define FM_BOARD_TIMERS_H

#include <stdint.h>
#include <stdbool.h>
#include "main.h"

void FM_BOARD_TIMERS_Init(void);
void FM_BOARD_TIMERS_DelayMs(uint32_t delay_ms);

/**
 * @brief Configura TIM7 como generador de carga.
 * @param interval_us     Periodo de interrupcion en microsegundos.
 * @param workload_us     Tiempo de carga (NOPs) en microsegundos.
 * @return true si se configuro correctamente.
 */
bool FM_BOARD_TIMERS_ConfigLoadTimer(uint32_t interval_us,
                                     uint32_t workload_us);
/**
 * @brief Arranca TIM7 en modo interrupcion para generar carga.
 */
bool FM_BOARD_TIMERS_StartLoadTimer(void);
/**
 * @brief Detiene TIM7 y deshabilita su interrupcion (no hace nada si no fue configurado).
 */
bool FM_BOARD_TIMERS_StopLoadTimer(void);

#endif /* FM_BOARD_TIMERS_H */


