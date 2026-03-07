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

/* Error tracking */
static volatile uint32_t fm_debug_error_counts[FM_DEBUG_ERR_COUNT] = { 0U };
static volatile uint32_t fm_debug_error_mask = 0U;
static volatile fm_debug_error_t fm_debug_last_error = FM_DEBUG_ERR_NONE;

/* Private Prototypes */
/* (none) */

/* Private Bodies */
/* (none) */

/* Public Bodies */
/**
 * @brief Initializes debug control by reading enable jumpers.
 *
 * @note Call once at boot before using any FM_DEBUG_* macro.
 */
void FM_DEBUG_Init(void)
{
    FM_DEBUG_RefreshJumpers();

    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;   /* enable trace */
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;              /* enable cycle counter */
}

/**
 * @brief Re-reads the debug enable jumpers with minimum static consumption.
 *
 * @note Pins stay in analog/no-pull state outside this call.
 */
void FM_DEBUG_RefreshJumpers(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };

    /* Enable pull-up only while sampling to avoid static current. */
    GPIO_InitStruct.Pin = DBG_MSG_EN_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(DBG_MSG_EN_GPIO_Port, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = DBG_LED_EN_Pin;
    HAL_GPIO_Init(DBG_LED_EN_GPIO_Port, &GPIO_InitStruct);

    fm_debug_msg_enable = (HAL_GPIO_ReadPin(DBG_MSG_EN_GPIO_Port,
            DBG_MSG_EN_Pin) == GPIO_PIN_SET);
    fm_debug_leds_enable = (HAL_GPIO_ReadPin(DBG_LED_EN_GPIO_Port,
            DBG_LED_EN_Pin) == GPIO_PIN_SET);

    /* Return both pins to lowest-power state. */
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;

    GPIO_InitStruct.Pin = DBG_MSG_EN_Pin;
    HAL_GPIO_Init(DBG_MSG_EN_GPIO_Port, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = DBG_LED_EN_Pin;
    HAL_GPIO_Init(DBG_LED_EN_GPIO_Port, &GPIO_InitStruct);
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
 * @brief Increments the counter for a detected error type.
 *
 * @note ISR-safe: no blocking calls or UART usage.
 */
void FM_DEBUG_ReportError(fm_debug_error_t err)
{
    if ((err <= FM_DEBUG_ERR_NONE) || (err >= FM_DEBUG_ERR_COUNT))
    {
        return;
    }

    fm_debug_error_counts[err]++;
    fm_debug_last_error = err;
    fm_debug_error_mask |= (1UL << (uint32_t)err);

    FM_DEBUG_LedError(FM_DEBUG_LED_ON);
}

/**
 * @brief Returns the count for a specific error type.
 */
uint32_t FM_DEBUG_ErrorCount(fm_debug_error_t err)
{
    if ((err <= FM_DEBUG_ERR_NONE) || (err >= FM_DEBUG_ERR_COUNT))
    {
        return 0U;
    }

    return fm_debug_error_counts[err];
}

/**
 * @brief Returns the last error reported (or FM_DEBUG_ERR_NONE).
 */
fm_debug_error_t FM_DEBUG_LastError(void)
{
    return fm_debug_last_error;
}

/**
 * @brief Returns a bitmask of all errors seen since last clear.
 */
uint32_t FM_DEBUG_ErrorMask(void)
{
    return fm_debug_error_mask;
}

/**
 * @brief Clears error counters and mask; turns off error LED.
 */
void FM_DEBUG_ClearErrors(void)
{
    uint32_t i;

    for (i = 0U; i < (uint32_t)FM_DEBUG_ERR_COUNT; i++)
    {
        fm_debug_error_counts[i] = 0U;
    }

    fm_debug_error_mask = 0U;
    fm_debug_last_error = FM_DEBUG_ERR_NONE;

    HAL_GPIO_WritePin(LED_ERROR_GPIO_Port, LED_ERROR_Pin, GPIO_PIN_RESET);
}

/**
 * @brief Drives the error LED.
 *
 * @note Do not call directly; use FM_DEBUG_LED_ERROR macro to honor jumper state.
 */
void FM_DEBUG_LedError(fm_debug_led_state_t state)
{
    if (state == FM_DEBUG_LED_ON)
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

    if (state == FM_DEBUG_LED_ON)
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
    if (state == FM_DEBUG_LED_ON)
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

    if ((p_msg == NULL) || (len == 0U) || (!fm_debug_msg_enable))
    {
        return false; /* Invalid parameters */
    }

    if (len >= FM_DEBUG_MSG_BUFFER_LENGTH)
    {
        /* Truncate message if it exceeds buffer length (should not happen in normal use) */
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
    bool ret;

    if (!fm_debug_msg_enable)
    {
        return false; /* Debug messages disabled by jumper */
    }

    len = snprintf(msg_buffer, FM_DEBUG_MSG_BUFFER_LENGTH, "%lu\n", (unsigned long)num);

    ret = FM_DEBUG_UartMsg(msg_buffer, (uint32_t)len);

    return ret;

}

/**
 * @brief Sends a signed 32-bit integer over the debug UART.
 *
 * @note Do not call directly; use FM_DEBUG_UART_INT32 macro to honor jumper state.
 */
bool FM_DEBUG_UartInt32(int32_t num)
{
    int len;
    bool ret;

    if (!fm_debug_msg_enable)
    {
        return false; /* Debug messages disabled by jumper */
    }

    len = snprintf(msg_buffer, FM_DEBUG_MSG_BUFFER_LENGTH, "%ld\n", (long)num);

    ret = FM_DEBUG_UartMsg(msg_buffer, (uint32_t)len);

    return ret;

}

/**
 * @brief Sends a floating-point number (two decimals) over the debug UART.
 *
 * @note Do not call directly; use FM_DEBUG_UART_FLOAT macro to honor jumper state.
 */
bool FM_DEBUG_UartFloat(float num)
{
    int len;
    bool ret;

    if (!fm_debug_msg_enable)
    {
        return false; /* Debug messages disabled by jumper */
    }

    len = snprintf(msg_buffer, FM_DEBUG_MSG_BUFFER_LENGTH, "%0.2f\n", (double)num);

    ret = FM_DEBUG_UartMsg(msg_buffer, (uint32_t)len);

    return ret;

}

/* Interrupts */
/* (none) */
