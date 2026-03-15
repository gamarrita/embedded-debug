#include "fm_board.h"

void FM_BOARD_Init(void)
{
    /* Initialize board-level peripherals in the expected order. */
    FM_BOARD_GPIO_Init();
    FM_BOARD_TIMERS_Init();
    FM_BOARD_UART_Init();
    FM_DEBUG_Init();
}
