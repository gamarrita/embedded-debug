/**
 * @file    fm_board.c
 * @brief   Board-level facade for debug services on Nucleo-U575ZI-Q.
 *
 * @details
 *  - Wraps fm_hw_gpio / fm_hw_uart / fm_hw_dwt so applications avoid HAL.
 *  - Owns bring-up order (GPIO → UART → DWT); no fm_hw.c aggregator by design.
 *  - GPIO init is manual (CubeMX call generation disabled) to keep LED/jumper control.
 *  - Jumper sampling uses temporary pull-ups then returns pins to analog to minimize leakage.
 */

#include <fm_hw_dwt.h>
#include <fm_hw_gpio.h>
#include <fm_hw_uart.h>
#include "fm_board.h"

/* Private Defines */
/* (none) */

/* Private Types */
/* (none) */

/* Private Data */
/* (none) */

/* Private Prototypes */
/* (none) */

/* Private Bodies */
/* (none) */

/* Public Bodies */
/* Debug bring-up sequence: GPIO (LEDs/jumpers) before UART uses those pins, DWT last after clock tree is stable. */
void FM_BOARD_Init(void)
{
    FM_HW_GPIO_Init();
    FM_HW_UART_Init();
    FM_HW_DWT_Init();
}

/* Debug enable sampling: gate debug messages/LEDs via jumpers with low-leakage pulls. */
bool FM_BOARD_DebugMsgEnabled(void)
{
    return FM_HW_GPIO_IsDbgMsgEnabled();
}

bool FM_BOARD_DebugLedsEnabled(void)
{
	bool enabled = FM_HW_GPIO_IsDbgLedEnabled();

	if(!enabled)
	{
		FM_BOARD_LedErrorOff();
		FM_BOARD_LedRunOff();
		FM_BOARD_LedSignalOff();
	}

    return FM_HW_GPIO_IsDbgLedEnabled();
}

/* LED control wrappers */
void FM_BOARD_LedErrorOn(void)
{
    FM_HW_GPIO_LedErrorOn();
}

void FM_BOARD_LedErrorOff(void)
{
    FM_HW_GPIO_LedErrorOff();
}

void FM_BOARD_LedRunOn(void)
{
    FM_HW_GPIO_LedRunOn();
}

void FM_BOARD_LedRunOff(void)
{
    FM_HW_GPIO_LedRunOff();
}

void FM_BOARD_LedSignalOn(void)
{
    FM_HW_GPIO_LedSignalOn();
}

void FM_BOARD_LedSignalOff(void)
{
    FM_HW_GPIO_LedSignalOff();
}

/* UART wrapper: blocks using HAL UART configured in fm_hw_uart. */
bool FM_BOARD_UartTransmit(const uint8_t *p_data, uint32_t len, uint32_t timeout_ms)
{
    return FM_HW_UART_Transmit(p_data, len, timeout_ms);
}

/* DWT wrappers: expose cycle counter when hardware supports CYCCNT (returns false/0 otherwise). */
bool FM_BOARD_DwtInit(void)
{
    return FM_HW_DWT_Init();
}

uint32_t FM_BOARD_DwtGetCycles(void)
{
    return FM_HW_DWT_GetCycles();
}

/* Interrupts */
/* (none) */

