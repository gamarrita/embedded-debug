#include "fm_board.h"
#include "fm_board_gpio.h"
#include "fm_board_uart.h"
#include "fm_board_dwt.h"

void FM_BOARD_Init(void)
{
    /* Initialize board-level peripherals in the expected order. */
    FM_BOARD_GPIO_Init();
    FM_BOARD_UART_Init();
    (void) FM_BOARD_DWT_Init();
}

bool FM_BOARD_DebugMsgEnabled(void)
{
    return FM_BOARD_GPIO_IsDbgMsgEnabled();
}

bool FM_BOARD_DebugLedsEnabled(void)
{
    return FM_BOARD_GPIO_IsDbgLedEnabled();
}

void FM_BOARD_LedErrorOn(void)
{
    FM_BOARD_GPIO_LedErrorOn();
}

void FM_BOARD_LedErrorOff(void)
{
    FM_BOARD_GPIO_LedErrorOff();
}

void FM_BOARD_LedRunOn(void)
{
    FM_BOARD_GPIO_LedRunOn();
}

void FM_BOARD_LedRunOff(void)
{
    FM_BOARD_GPIO_LedRunOff();
}

void FM_BOARD_LedSignalOn(void)
{
    FM_BOARD_GPIO_LedSignalOn();
}

void FM_BOARD_LedSignalOff(void)
{
    FM_BOARD_GPIO_LedSignalOff();
}

bool FM_BOARD_UartTransmit(const uint8_t *p_data, uint32_t len, uint32_t timeout_ms)
{
    return FM_BOARD_UART_Transmit(p_data, len, timeout_ms);
}

bool FM_BOARD_DwtInit(void)
{
    return FM_BOARD_DWT_Init();
}

uint32_t FM_BOARD_DwtGetCycles(void)
{
    return FM_BOARD_DWT_GetCycles();
}
