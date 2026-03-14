#include "fm_board_uart.h"

#include "usart.h"

static UART_HandleTypeDef *fm_board_uart_handle(void)
{
    return &huart1;
}

void FM_BOARD_UART_Init(void)
{
    /* Use CubeMX-generated init to keep clocks, GPIOs and FIFO config consistent. */
    MX_USART1_UART_Init();
}

bool FM_BOARD_UART_Transmit(const uint8_t *p_data, uint32_t len, uint32_t timeout_ms)
{
    if ((p_data == NULL) || (len == 0U))
    {
        return false;
    }

    return (HAL_UART_Transmit(fm_board_uart_handle(), (uint8_t *) p_data, len, timeout_ms) == HAL_OK);
}