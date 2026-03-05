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
#include <stdint.h>
#include <stdbool.h>
// #include "fmx.h"     // Si el main necesita tipos RTOS públicos
// #include "fm_log.h" // Si el main llama a módulos de log

/* =========================== Public Macros ================================ */
// Ejemplos de sufijos de unidad
#define FM_MAIN_LOOP_PERIOD_MS        (10u)     // periodo del bucle principal
#define FM_MAIN_WATCHDOG_TIMEOUT_SEC  (2u)      // tiempo de watchdog
#define FM_MAIN_STACK_SIZE_BYTES      (2048u)   // stack de un hilo, en bytes

/* =========================== Public Types ================================= */
// Tipos públicos del módulo, con prefijo en minúsculas y sufijo _t
typedef enum
{
    FM_MAIN_OK = 0,
    FM_MAIN_ERROR
} fm_main_status_t;

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

#endif /* FM_MAIN_H_ */

/*** end of file ***/
