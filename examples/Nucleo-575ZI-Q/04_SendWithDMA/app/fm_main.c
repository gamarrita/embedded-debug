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
#include "fm_main.h"
#include "fm_debug.h"
#include "fm_board_gpio.h"
#include "fm_board_timers.h"
#include "fm_board_uart.h"



/* =========================== Private Defines ============================== */


/* =========================== Private Types ================================ */


/* =========================== Private Data ================================= */


/* =========================== Private Prototypes =========================== */


/* =========================== Private Bodies =============================== */


/* =========================== Public Bodies ================================ */

void FM_MAIN_Init(void)
{
	FM_BOARD_GPIO_Init();
	FM_BOARD_TIMERS_Init();
	FM_BOARD_UART_Init();
	FM_DEBUG_Init();
}

/*
 * @brief   Loop infinito Flowmeet.
 * @note    El programa principal se desarrolla en este módulo; no escribir
 *          lógica de usuario en archivos generados automáticamente por el IDE.
 */
void FM_MAIN_Main(void)
{

	FM_MAIN_Init();

	fm_debug_led_state_t led_toogle = FM_DEBUG_LED_OFF;

    for (;;)
    {
    	led_toogle ^= 1; /* toggle LED state */
        FM_DEBUG_LedError(led_toogle);
        FM_BOARD_TIMERS_DelayMs(250U);
    }
}

/* =========================== Interrupts =================================== */

void FM_MAIN_OnRtcWakeup(void)
{
	char msg[] = "Wake Up Timer callback\n";

	FM_DEBUG_UartMsg(msg, sizeof(msg) - 1U);
}


/*** end of file ***/
