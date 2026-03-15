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
#include "fm_board_gpio.h"
#include "fm_board_uart.h"
#include "fm_board_dwt.h"
#include "fm_debug.h"

/* Private Defines */
#define TX_BUFFER_LENGTH	(1024U)
#define MSG_BUFFER_LENGTH	(32U)
#define UART_TIMEOUT_MS		(10U)

/* Private Types */
/* (none) */

/* Private Data */

static volatile bool fm_debug_msg_enable = false;
static volatile bool fm_debug_leds_enable = false;
static char msg_buffer[MSG_BUFFER_LENGTH];
static TIM_HandleTypeDef s_loadgen_htim;
static uint32_t s_loadgen_workload_cycles = 0U;

/* Error tracking */
static volatile uint32_t fm_debug_error_counts[FM_DEBUG_ERR_COUNT] =
{ 0U };
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
	FM_BOARD_GPIO_Init();
	FM_DEBUG_RefreshJumpers();
}

/**
 * @brief Re-reads the debug enable jumpers with minimum static consumption.
 *
 * @note Pins stay in analog/no-pull state outside this call.
 */
void FM_DEBUG_RefreshJumpers(void)
{
	fm_debug_msg_enable = FM_BOARD_GPIO_IsDbgMsgEnabled();
	fm_debug_leds_enable = FM_BOARD_GPIO_IsDbgLedEnabled();
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
	fm_debug_error_mask |= (1UL << (uint32_t) err);

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

	for (i = 0U; i < (uint32_t) FM_DEBUG_ERR_COUNT; i++)
	{
		fm_debug_error_counts[i] = 0U;
	}

	fm_debug_error_mask = 0U;
	fm_debug_last_error = FM_DEBUG_ERR_NONE;

	FM_BOARD_GPIO_LedErrorOff();
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
		FM_BOARD_GPIO_LedErrorOn();
	}
	else
	{
		FM_BOARD_GPIO_LedErrorOff();
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
		FM_BOARD_GPIO_LedRunOn();
	}
	else
	{
		FM_BOARD_GPIO_LedRunOff();
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
		FM_BOARD_GPIO_LedSignalOn();
	}
	else
	{
		FM_BOARD_GPIO_LedSignalOff();
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

	if (len >= MSG_BUFFER_LENGTH)
	{
		/* Truncate message if it exceeds buffer length (should not happen in normal use) */
		len = MSG_BUFFER_LENGTH;
	}

	(void) FM_BOARD_UART_Transmit((const uint8_t*) p_msg, len, UART_TIMEOUT_MS);

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

	len = snprintf(msg_buffer, MSG_BUFFER_LENGTH, "%lu\n", (unsigned long) num);

	ret = FM_DEBUG_UartMsg(msg_buffer, (uint32_t) len);

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

	len = snprintf(msg_buffer, MSG_BUFFER_LENGTH, "%ld\n", (long) num);

	ret = FM_DEBUG_UartMsg(msg_buffer, (uint32_t) len);

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

	len = snprintf(msg_buffer, MSG_BUFFER_LENGTH, "%0.2f\n", (double) num);

	ret = FM_DEBUG_UartMsg(msg_buffer, (uint32_t) len);

	return ret;

}

void FM_DEBUG_LoadGenInit(void)
{
	__HAL_RCC_TIM7_CLK_ENABLE();

	s_loadgen_htim.Instance = TIM7;
	s_loadgen_htim.Init.Prescaler = 0U;
	s_loadgen_htim.Init.CounterMode = TIM_COUNTERMODE_UP;
	s_loadgen_htim.Init.Period = 0U;
	s_loadgen_htim.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	s_loadgen_htim.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

	(void) HAL_TIM_Base_Init(&s_loadgen_htim);
}

void FM_DEBUG_LoadGenConfigure(uint32_t interval_us, uint32_t workload_cycles, uint32_t priority)
{
	uint32_t prescaler = (FM_BOARD_DWT_GetCpuHz() / 1000000U) - 1U;
	uint32_t period = (interval_us > 0U) ? (interval_us - 1U) : 0U;

	s_loadgen_htim.Init.Prescaler = prescaler;
	s_loadgen_htim.Init.CounterMode = TIM_COUNTERMODE_UP;
	s_loadgen_htim.Init.Period = period;
	s_loadgen_htim.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	s_loadgen_htim.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

	(void) HAL_TIM_Base_Init(&s_loadgen_htim);

	s_loadgen_workload_cycles = workload_cycles;

	HAL_NVIC_SetPriority(TIM7_IRQn, priority, 0);
}

void FM_DEBUG_LoadGenStart(void)
{
	__HAL_TIM_CLEAR_IT(&s_loadgen_htim, TIM_IT_UPDATE);
	(void) HAL_TIM_Base_Start_IT(&s_loadgen_htim);
	HAL_NVIC_EnableIRQ(TIM7_IRQn);
}

void FM_DEBUG_LoadGenStop(void)
{
	(void) HAL_TIM_Base_Stop_IT(&s_loadgen_htim);
	HAL_NVIC_DisableIRQ(TIM7_IRQn);
}

/* Interrupts */
void TIM7_IRQHandler(void)
{
	if ((TIM7->SR & TIM_SR_UIF) != 0U)
	{
		for (volatile uint32_t i = 0U; i < s_loadgen_workload_cycles; i++)
		{
			__NOP();
		}

		TIM7->SR &= ~TIM_SR_UIF;
	}
}
