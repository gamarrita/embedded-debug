/**
 * @file    fm_debug.c
 * @brief   Debug services: ITM tracing, UART logging, and diagnostic LEDs.
 *
 * @details
 *  - Reads enable jumpers for UART and diagnostic LEDs
 *  - Provides print helpers for common numeric types
 *  - Drives three status LEDs (error, activity, signal)
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "main.h"
#include "fm_debug.h"

/* Private Defines */
#define FM_DEBUG_MSG_BUFFER_LENGTH   (32U)
#define FM_DEBUG_UART_TIMEOUT_MS     (10U)



/* Private Types */
/* (none) */

/* Private Data */


extern UART_HandleTypeDef huart1;

static volatile bool fm_debug_msg_enable = false;
static volatile bool fm_debug_leds_enable = false;
static char msg_buffer[FM_DEBUG_MSG_BUFFER_LENGTH];

/* Public Bodies */
/**
 * @brief Initializes debug control by reading enable jumpers.
 *
 * @note Call once at boot before using any FM_DEBUG_* macro.
 */
void FM_DEBUG_Init(void)
{
    fm_debug_msg_enable = (HAL_GPIO_ReadPin(DBG_MSG_EN_GPIO_Port, DBG_MSG_EN_Pin) == GPIO_PIN_SET);
    fm_debug_leds_enable = (HAL_GPIO_ReadPin(DBG_LED_EN_GPIO_Port, DBG_LED_EN_Pin) == GPIO_PIN_SET);
}

/**
 * @brief Returns true when UART debug is enabled by jumper.
 */
bool FM_DEBUG_MsgIsEnabled(void)
{
    return fm_debug_msg_enable;
}

/**
 * @brief Returns true when LED debug is enabled by jumper.
 */
bool FM_DEBUG_LedsAreEnabled(void)
{
    return fm_debug_leds_enable;
}

/**
 * @brief Drives the error LED.
 *
 * @note Do not call directly; use FM_DEBUG_LED_ERROR macro to honor jumper state.
 */
void FM_DEBUG_LedError(fm_debug_led_state_t state)
{
    if(state == FM_DEBUG_LED_ON)
    {
        HAL_GPIO_WritePin(LED_ERROR_GPIO_Port, LED_ERROR_Pin, GPIO_PIN_SET);
    }
     else
    {
         HAL_GPIO_WritePin(LED_ERROR_GPIO_Port, LED_ERROR_Pin, GPIO_PIN_RESET);
    }
}

/**
 * @brief Drives the activity LED.
 *
 * @note Do not call directly; use FM_DEBUG_LED_ACTIVE macro to honor jumper state.
 */
void FM_DEBUG_LedRun(fm_debug_led_state_t state)
{
    if(state == FM_DEBUG_LED_ON)
    {
        HAL_GPIO_WritePin(LED_RUN_GPIO_Port, LED_RUN_Pin, GPIO_PIN_SET);
    }
     else
    {
         HAL_GPIO_WritePin(LED_RUN_GPIO_Port, LED_RUN_Pin, GPIO_PIN_RESET);
    }
}

/**
 * @brief Drives the signal LED.
 *
 * @note Do not call directly; use FM_DEBUG_LED_SIGNAL macro to honor jumper state.
 */
void FM_DEBUG_LedSignal(fm_debug_led_state_t state)
{
    if(state == FM_DEBUG_LED_ON)
    {
        HAL_GPIO_WritePin(LED_SIGNAL_GPIO_Port, LED_SIGNAL_Pin, GPIO_PIN_SET);
    }
     else
    {
         HAL_GPIO_WritePin(LED_SIGNAL_GPIO_Port, LED_SIGNAL_Pin, GPIO_PIN_RESET);
    }
}

/**
 * @brief Sends a binary message over the debug UART.
 *
 * @note Do not call directly; use FM_DEBUG_UART_MSG macro to honor jumper state.
 */
bool FM_DEBUG_UartMsg(const char *p_msg, uint32_t len)
{
    if ((p_msg == NULL) || (len == 0U))
    {
        return false; // Invalid parameters
    }

    if (len >= FM_DEBUG_MSG_BUFFER_LENGTH)
    {
        // Truncate message if it exceeds buffer length (should not happen if used correctly)
        len = FM_DEBUG_MSG_BUFFER_LENGTH;
    }

    HAL_UART_Transmit(&huart1, (const uint8_t *)p_msg, len, FM_DEBUG_UART_TIMEOUT_MS);

    return true;
}

/**
 * @brief Sends an unsigned 32-bit integer over the debug UART.
 *
 * @note Do not call directly; use FM_DEBUG_UART_UINT32 macro to honor jumper state.
 */
bool FM_DEBUG_UartUint32(uint32_t num)
{
    int len;

    len = snprintf(msg_buffer, FM_DEBUG_MSG_BUFFER_LENGTH, "%lu\n", (unsigned long)num);


    if(len < 0)
    {
        return false; // Encoding error
    }

    FM_DEBUG_UartMsg(msg_buffer, (uint32_t)len);

    return true;

}

/**
 * @brief Sends a signed 32-bit integer over the debug UART.
 *
 * @note Do not call directly; use FM_DEBUG_UART_INT32 macro to honor jumper state.
 */
bool FM_DEBUG_UartInt32(int32_t num)
{
    int len;

    len = snprintf(msg_buffer, FM_DEBUG_MSG_BUFFER_LENGTH, "%ld\n", (long)num);


    if(len < 0)
    {
        return false; // Encoding error
    }

    FM_DEBUG_UartMsg(msg_buffer, (uint32_t)len);

    return true;

}

/**
 * @brief Sends a floating-point number (two decimals) over the debug UART.
 *
 * @note Do not call directly; use FM_DEBUG_UART_FLOAT macro to honor jumper state.
 */
bool FM_DEBUG_UartFloat(float num)
{
    int len;

    len = snprintf(msg_buffer, FM_DEBUG_MSG_BUFFER_LENGTH, "%0.2f\n", (double)num);

    if(len < 0)
    {
        return false; // Encoding error
    }

    FM_DEBUG_UartMsg(msg_buffer, (uint32_t)len);

    return true;

}

/* Interrupts */
/* (none) */
