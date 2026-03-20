#include <fm_hw_dwt.h>
#include <fm_hw_gpio.h>
#include <fm_hw_uart.h>
#include "fm_board.h"

/*
 * fm_board: public facade for board-level debug services on Nucleo-U575ZI-Q.
 * - Abstracts low-level fm_hw_gpio / fm_hw_uart / fm_hw_dwt so applications avoid HAL.
 * - Owns initialization order; there is intentionally no fm_hw.c aggregator.
 * - Samples debug enable jumpers through fm_hw_gpio with temporary pull-ups, returning
 *   pins to analog mode between reads to minimize leakage.
 * - UART (USART1 115200 8N1) and DWT setup live in fm_hw_uart/dwt; this layer sequences
 *   them and exposes a stable API surface.
 * - Application code should depend on fm_board.* only.
 */

/* Board initialization */
/* Debug bring-up sequence: GPIO (LEDs/jumpers) before UART uses those pins, DWT last after clock tree is stable. */
void FM_BOARD_Init(void)
{
    FM_HW_GPIO_Init();
    FM_HW_UART_Init();
    FM_HW_DWT_Init();
}

/* Debug enable / jumper sampling: gate debug features via hardware jumpers. */
bool FM_BOARD_DebugMsgEnabled(void)
{
    return FM_HW_GPIO_IsDbgMsgEnabled();
}

bool FM_BOARD_DebugLedsEnabled(void)
{
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

/* UART wrapper */
bool FM_BOARD_UartTransmit(const uint8_t *p_data, uint32_t len, uint32_t timeout_ms)
{
    return FM_HW_UART_Transmit(p_data, len, timeout_ms);
}

/* DWT wrappers */
bool FM_BOARD_DwtInit(void)
{
    return FM_HW_DWT_Init();
}

uint32_t FM_BOARD_DwtGetCycles(void)
{
    return FM_HW_DWT_GetCycles();
}
