/*
 * @file    fm_main.h
 * @brief   Punto de entrada de la aplicación Flowmeet (capa APP).
 * @date    2025-09-14
 * @author  Flowmeet
 *
 * @details
 *   - Este header expone SOLO la API pública del módulo.
 *   - No incluir detalles de HAL ni direcciones de Flash aquí.
 *   - Incluir este header en main.c generado por el IDE y llamar FM_MAIN_Main().
 *
 *   (Basado en tu plantilla actual y comentarios de integración con el IDE.) 
 */

#ifndef FM_MAIN_H_
#define FM_MAIN_H_

/* =========================== Includes ==================================== */
#include "main.h"
#include <stdint.h>

/* =========================== Public API =================================== */
/**
 * @brief  Inicializa la aplicación (drivers, módulos, RTOS si aplica).
 */
void FM_MAIN_Init(void);

/**
 * @brief  Bucle principal de la app (si no usás scheduler/RTOS).
 * @note   Si usás ThreadX, este loop puede quedar mínimo y ceder control a hilos.
 */
void FM_MAIN_Main(void);

/**
 * @brief  Callback invoked by board layer on RTC WakeUp event.
 */
void FM_MAIN_OnRtcWakeup(void);

#endif /* FM_MAIN_H_ */

/*** end of file ***/
