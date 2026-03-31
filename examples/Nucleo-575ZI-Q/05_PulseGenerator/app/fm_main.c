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
#include "fm_board.h"
#include "rtc.h"

/* =========================== Private Defines ============================== */
#define PULSES			1000
#define PULSES_FREQ		100



void FM_MAIN_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct =
	{ 0 };
	__HAL_RCC_GPIOC_CLK_ENABLE();

	/*Configure GPIO pin : KEY_BLUE_Pin */
	GPIO_InitStruct.Pin = KEY_BLUE_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(KEY_BLUE_GPIO_Port, &GPIO_InitStruct);

	FM_BOARD_Init();
	FM_DEBUG_Init();

	// MX_RTC_Init(); // RTC initialization is deferred until needed.
}

/*
 * @brief   Loop infinito Flowmeet.
 * @note    El programa principal se desarrolla en este módulo; no escribir
 *          lógica de usuario en archivos generados automáticamente por el IDE.
 */
void FM_MAIN_Main(void)
{

	//char msg[] = "Go to simulated sleep 2sec!!!\n";
	FM_MAIN_Init();

	fm_debug_led_state_t led_toggle = FM_DEBUG_LED_OFF;

	for (;;)
	{

		led_toggle = FM_DEBUG_LED_OFF;
		FM_DEBUG_LedSignal(led_toggle); /* Fuerza un estado idle conocido mientras espera el disparo. */

		while (HAL_GPIO_ReadPin(KEY_BLUE_GPIO_Port, KEY_BLUE_Pin) == GPIO_PIN_RESET)
		{
			/* Espera pulsación del botón. El pulsador de usuario es activo en bajo. */
			__NOP(); /* Buen punto de breakpoint para validar el trigger. */

		}

		for (uint32_t toggle_index = 0U; toggle_index < PULSES * 2; ++toggle_index)
		{

			led_toggle = (led_toggle == FM_DEBUG_LED_OFF) ? FM_DEBUG_LED_ON : FM_DEBUG_LED_OFF;
			FM_DEBUG_LedSignal(led_toggle);

			uint32_t start_cycle =  FM_BOARD_DwtGetCycles();
			uint32_t end_cycle = start_cycle + (SystemCoreClock / (PULSES_FREQ / 2U)); /* Semiperíodo en ciclos. */

			while (FM_BOARD_DwtGetCycles() < end_cycle);
		}
	}
}

/* =========================== Interrupts =================================== */

void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc)
{
	UNUSED(hrtc);

	{
		UNUSED(hrtc);
		/* Non-blocking ISR log: event will be flushed later by foreground code. */
		FM_DEBUG_LogConstISR("Wakeup event");
	}
}

/*** end of file ***/
