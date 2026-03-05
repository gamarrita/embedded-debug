/*
 * @file    fm_main.c
 * @brief   Implementación del punto de entrada de la aplicación.
 * @date    2025-09-14
 * @author  Daniel H Sagarra
 *
 * @details
 *   - Mantiene el estilo de secciones fijas.
 *   - Ejemplifica nombres: públicas FM_MAIN_*, privadas static snake_case.
 *   - Evita lógica pesada en ISR (solo flags).
 */


/* ===========================     Includes    ============================== */
#include "main.h"
#include "fm_main.h"
#include "fm_debug.h"



/* =========================== Private Defines ============================== */


/* =========================== Private Types ================================ */


/* =========================== Private Data ================================= */


/* =========================== Private Prototypes =========================== */


/* =========================== Private Bodies =============================== */


/* =========================== Public Bodies ================================ */

void FM_MAIN_Init(void)
{

}

/*
 * @brief   Loop infinito Flowmeet.
 * @note    El programa principal se desarrolla en este módulo; no escribir
 *          lógica de usuario en archivos generados automáticamente por el IDE.
 */
void FM_MAIN_Main(void)
{
    FM_DEBUG_Init();

    for (;;)
    {
        HAL_GPIO_TogglePin(LED1_GPIO_PORT, LED1_PIN);
        HAL_Delay(50);
    }
}

/* =========================== Interrupts =================================== */

/**
 * @brief  Wake Up Timer callback.
 * @param  hrtc RTC handle
 * @retval None
 */
void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(hrtc);
    FM_DEBUG_UART_MSG("Wake Up Timer callback\n");
}


/*** end of file ***/
