#include <fm_hw_uart.h>
#include "main.h"

static UART_HandleTypeDef huart1;

/* Internal accessor for the configured UART handle. */
static UART_HandleTypeDef *FM_HW_UART_HandleGet(void)
{
    return &huart1;
}

void FM_HW_UART_Init(void)
{
    UART_HandleTypeDef *huart = FM_HW_UART_HandleGet();

    huart->Instance = USART1;
    huart->Init.BaudRate = 115200;
    huart->Init.WordLength = UART_WORDLENGTH_8B;
    huart->Init.StopBits = UART_STOPBITS_1;
    huart->Init.Parity = UART_PARITY_NONE;
    huart->Init.Mode = UART_MODE_TX_RX;
    huart->Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart->Init.OverSampling = UART_OVERSAMPLING_16;
    huart->Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart->Init.ClockPrescaler = UART_PRESCALER_DIV1;
    huart->AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

    if (HAL_UART_Init(huart) != HAL_OK)
    {
        Error_Handler();
    }
    if (HAL_UARTEx_SetTxFifoThreshold(huart, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
    {
        Error_Handler();
    }
    if (HAL_UARTEx_SetRxFifoThreshold(huart, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
    {
        Error_Handler();
    }
    if (HAL_UARTEx_DisableFifoMode(huart) != HAL_OK)
    {
        Error_Handler();
    }
}

bool FM_HW_UART_Transmit(const uint8_t *p_data, uint32_t len, uint32_t timeout_ms)
{
    if ((p_data == NULL) || (len == 0U))
    {
        return false;
    }

    return (HAL_UART_Transmit(FM_HW_UART_HandleGet(), (uint8_t *) p_data, len, timeout_ms) == HAL_OK);
}
