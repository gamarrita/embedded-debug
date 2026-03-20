/**
 * @file    fm_debug.c
 * @brief   Debug services: event capture, logging, LEDs, and counters.
 *
 * @details
 *  - Captures compact events (ISR-safe) into a fixed ring buffer.
 *  - Optional DWT timestamping.
 *  - Flushes buffered events over UART in non-critical context.
 *  - Keeps legacy LED control and error counters as support features.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "fm_debug.h"
#include "fm_board.h"

/* Private Defines */
#define MSG_BUFFER_LENGTH        (96U)
#define UART_TIMEOUT_MS          (10U)
#define FM_DEBUG_EVT_CAPACITY    (64U)
#define FM_DEBUG_EVT_MASK        (FM_DEBUG_EVT_CAPACITY - 1U)
#define FM_DEBUG_FLUSH_TEXT_MAX  (128U)

#define FM_DEBUG_FLAG_HAS_PARAM1   (1U << 0)
#define FM_DEBUG_FLAG_CONST_TEXT   (1U << 1)

/* Private Types */
typedef struct
{
	uint32_t ts_cycles;
	uint16_t code;
	uint16_t flags;
	int32_t  param0;
	int32_t  param1;
	const char *p_text;
} fm_debug_ring_entry_t;

/* Private Data */
static volatile bool fm_debug_msg_enable = false;
static volatile bool fm_debug_leds_enable = false;
static bool fm_debug_dwt_ready = false;

static char msg_buffer[MSG_BUFFER_LENGTH];
static char fm_debug_flush_buffer[FM_DEBUG_FLUSH_TEXT_MAX];

static fm_debug_ring_entry_t fm_debug_ring[FM_DEBUG_EVT_CAPACITY];
static volatile uint32_t fm_debug_evt_head = 0U;
static volatile uint32_t fm_debug_evt_tail = 0U;
static volatile uint32_t fm_debug_evt_dropped = 0U;
static volatile uint32_t fm_debug_evt_high_water = 0U;

/* Error tracking */
static volatile uint32_t fm_debug_error_counts[FM_DEBUG_ERR_COUNT] = { 0U };
static volatile uint32_t fm_debug_error_mask = 0U;
static volatile fm_debug_error_t fm_debug_last_error = FM_DEBUG_ERR_NONE;
static volatile int32_t fm_debug_error_param[FM_DEBUG_ERR_COUNT] = { 0 };
static const char *fm_debug_error_str[FM_DEBUG_ERR_COUNT] =
{
	"NONE",
	"OVERRUN",
	"TIMEOUT",
	"BACKEND",
	"BUFFER_FULL"
};

/* Private Prototypes */
static uint32_t fm_debug_timestamp_cycles_internal(void);
static bool fm_debug_enqueue(uint16_t code, uint16_t flags, int32_t param0, int32_t param1, const char *p_text);

/* Private Bodies */
static uint32_t fm_debug_timestamp_cycles_internal(void)
{
	if (!fm_debug_dwt_ready)
	{
		return 0U;
	}

	return FM_BOARD_DwtGetCycles();
}

static bool fm_debug_enqueue(uint16_t code, uint16_t flags, int32_t param0, int32_t param1, const char *p_text)
{
	uint32_t head = fm_debug_evt_head;
	uint32_t tail = fm_debug_evt_tail;
	uint32_t queued = head - tail;

	if (queued >= FM_DEBUG_EVT_CAPACITY)
	{
		fm_debug_evt_dropped++;
		return false;
	}

	fm_debug_ring_entry_t *p_evt = &fm_debug_ring[head & FM_DEBUG_EVT_MASK];
	p_evt->ts_cycles = fm_debug_timestamp_cycles_internal();
	p_evt->code = code;
	p_evt->flags = flags;
	p_evt->param0 = param0;
	p_evt->param1 = param1;
	p_evt->p_text = p_text;

	fm_debug_evt_head = head + 1U;

	if ((queued + 1U) > fm_debug_evt_high_water)
	{
		fm_debug_evt_high_water = queued + 1U;
	}

	return true;
}

/* Public Bodies */
void FM_DEBUG_Init(void)
{
	fm_debug_dwt_ready = FM_BOARD_DwtInit();
	fm_debug_evt_head = 0U;
	fm_debug_evt_tail = 0U;
	fm_debug_evt_dropped = 0U;
	fm_debug_evt_high_water = 0U;

	FM_DEBUG_ClearErrors();
	FM_DEBUG_RefreshJumpers();
}

bool FM_DEBUG_IsEnabled(void)
{
	return (fm_debug_msg_enable || fm_debug_leds_enable);
}

void FM_DEBUG_RefreshJumpers(void)
{
	fm_debug_msg_enable = FM_BOARD_DebugMsgEnabled();
	fm_debug_leds_enable = FM_BOARD_DebugLedsEnabled();
}

bool FM_DEBUG_MsgIsEnabled(void)
{
	return fm_debug_msg_enable;
}

bool FM_DEBUG_LedsAreEnabled(void)
{
	return fm_debug_leds_enable;
}

void FM_DEBUG_ReportError(fm_debug_error_t err)
{
	FM_DEBUG_ReportErrorWithParam(err, 0);
}

uint32_t FM_DEBUG_ErrorCount(fm_debug_error_t err)
{
	if ((err <= FM_DEBUG_ERR_NONE) || (err >= FM_DEBUG_ERR_COUNT))
	{
		return 0U;
	}

	return fm_debug_error_counts[err];
}

fm_debug_error_t FM_DEBUG_LastError(void)
{
	return fm_debug_last_error;
}

uint32_t FM_DEBUG_ErrorMask(void)
{
	return fm_debug_error_mask;
}

void FM_DEBUG_ClearErrors(void)
{
	uint32_t i;

	for (i = 0U; i < (uint32_t) FM_DEBUG_ERR_COUNT; i++)
	{
		fm_debug_error_counts[i] = 0U;
		fm_debug_error_param[i] = 0;
	}

	fm_debug_error_mask = 0U;
	fm_debug_last_error = FM_DEBUG_ERR_NONE;

	FM_BOARD_DebugLedErrorOff();
}

void FM_DEBUG_LedError(fm_debug_led_state_t state)
{
	if (state == FM_DEBUG_LED_ON)
	{
		FM_BOARD_LedErrorOn();
	}
	else
	{
		FM_BOARD_LedErrorOff();
	}
}

void FM_DEBUG_LedRun(fm_debug_led_state_t state)
{
	if (state == FM_DEBUG_LED_ON)
	{
		FM_BOARD_LedRunOn();
	}
	else
	{
		FM_BOARD_LedRunOff();
	}
}

void FM_DEBUG_LedSignal(fm_debug_led_state_t state)
{
	if (state == FM_DEBUG_LED_ON)
	{
		FM_BOARD_LedSignalOn();
	}
	else
	{
		FM_BOARD_LedSignalOff();
	}
}

bool FM_DEBUG_UartMsg(const char *p_msg, uint32_t len)
{
	if ((p_msg == NULL) || (len == 0U) || (!fm_debug_msg_enable))
	{
		return false;
	}

	if (len >= MSG_BUFFER_LENGTH)
	{
		len = MSG_BUFFER_LENGTH;
	}

	(void) FM_BOARD_UartTransmit((const uint8_t*) p_msg, len, UART_TIMEOUT_MS);

	return true;
}

bool FM_DEBUG_UartUint32(uint32_t num)
{
	int len;
	bool ret;

	if (!fm_debug_msg_enable)
	{
		return false;
	}

	len = snprintf(msg_buffer, MSG_BUFFER_LENGTH, "%lu\n", (unsigned long) num);

	ret = FM_DEBUG_UartMsg(msg_buffer, (uint32_t) len);

	return ret;
}

void FM_DEBUG_ReportErrorWithParam(fm_debug_error_t err, int32_t param)
{
	if ((err <= FM_DEBUG_ERR_NONE) || (err >= FM_DEBUG_ERR_COUNT))
	{
		return;
	}

	fm_debug_error_counts[err]++;
	fm_debug_last_error = err;
	fm_debug_error_mask |= (1UL << (uint32_t) err);
	fm_debug_error_param[err] = param;

	(void) fm_debug_enqueue((uint16_t) FM_DEBUG_EVT_ERROR, 0U, param, 0, NULL);
	FM_DEBUG_LedError(FM_DEBUG_LED_ON);
}

int32_t FM_DEBUG_ErrorParam(fm_debug_error_t err)
{
	if ((err <= FM_DEBUG_ERR_NONE) || (err >= FM_DEBUG_ERR_COUNT))
	{
		return 0;
	}

	return fm_debug_error_param[err];
}

const char *FM_DEBUG_ErrorString(fm_debug_error_t err)
{
	if ((err < FM_DEBUG_ERR_NONE) || (err >= FM_DEBUG_ERR_COUNT))
	{
		return "UNKNOWN";
	}

	return fm_debug_error_str[err];
}

bool FM_DEBUG_UartInt32(int32_t num)
{
	int len;
	bool ret;

	if (!fm_debug_msg_enable)
	{
		return false;
	}

	len = snprintf(msg_buffer, MSG_BUFFER_LENGTH, "%ld\n", (long) num);

	ret = FM_DEBUG_UartMsg(msg_buffer, (uint32_t) len);

	return ret;
}

bool FM_DEBUG_UartFloat(float num)
{
	int len;
	bool ret;

	if (!fm_debug_msg_enable)
	{
		return false;
	}

	len = snprintf(msg_buffer, MSG_BUFFER_LENGTH, "%0.2f\n", (double) num);

	ret = FM_DEBUG_UartMsg(msg_buffer, (uint32_t) len);

	return ret;
}

uint32_t FM_DEBUG_TimestampCycles(void)
{
	return fm_debug_timestamp_cycles_internal();
}

bool FM_DEBUG_LogISR(uint16_t code, int32_t param0)
{
	return fm_debug_enqueue(code, 0U, param0, 0, NULL);
}

bool FM_DEBUG_Log2ISR(uint16_t code, int32_t param0, int32_t param1)
{
	return fm_debug_enqueue(code, FM_DEBUG_FLAG_HAS_PARAM1, param0, param1, NULL);
}

bool FM_DEBUG_LogConstISR(const char *p_msg)
{
	if (p_msg == NULL)
	{
		return false;
	}

	return fm_debug_enqueue((uint16_t) FM_DEBUG_EVT_TEXT, FM_DEBUG_FLAG_CONST_TEXT, 0, 0, p_msg);
}

uint32_t FM_DEBUG_DroppedCount(void)
{
	return fm_debug_evt_dropped;
}

uint32_t FM_DEBUG_QueuedCount(void)
{
	return (fm_debug_evt_head - fm_debug_evt_tail);
}

uint32_t FM_DEBUG_HighWatermark(void)
{
	return fm_debug_evt_high_water;
}

void FM_DEBUG_Flush(void)
{
	if (!fm_debug_msg_enable)
	{
		return;
	}

	while (fm_debug_evt_tail != fm_debug_evt_head)
	{
		fm_debug_ring_entry_t evt = fm_debug_ring[fm_debug_evt_tail & FM_DEBUG_EVT_MASK];
		fm_debug_evt_tail++;

		if ((evt.flags & FM_DEBUG_FLAG_CONST_TEXT) != 0U)
		{
			const char *p_text = (evt.p_text != NULL) ? evt.p_text : "";
			int len = snprintf(fm_debug_flush_buffer, FM_DEBUG_FLUSH_TEXT_MAX,
					"[%lu] TXT=%s\n",
					(unsigned long) evt.ts_cycles,
					p_text);

			if (len > 0)
			{
				if ((uint32_t) len > FM_DEBUG_FLUSH_TEXT_MAX)
				{
					len = (int) FM_DEBUG_FLUSH_TEXT_MAX;
				}

				(void) FM_BOARD_UartTransmit((const uint8_t *) fm_debug_flush_buffer,
						(uint32_t) len,
						UART_TIMEOUT_MS);
			}
			continue;
		}

		int len;

		if ((evt.flags & FM_DEBUG_FLAG_HAS_PARAM1) != 0U)
		{
			len = snprintf(fm_debug_flush_buffer, FM_DEBUG_FLUSH_TEXT_MAX,
					"[%lu] EVT=%u P0=%ld P1=%ld\n",
					(unsigned long) evt.ts_cycles,
					(unsigned int) evt.code,
					(long) evt.param0,
					(long) evt.param1);
		}
		else
		{
			len = snprintf(fm_debug_flush_buffer, FM_DEBUG_FLUSH_TEXT_MAX,
					"[%lu] EVT=%u P0=%ld\n",
					(unsigned long) evt.ts_cycles,
					(unsigned int) evt.code,
					(long) evt.param0);
		}

		if (len > 0)
		{
			if ((uint32_t) len > FM_DEBUG_FLUSH_TEXT_MAX)
			{
				len = (int) FM_DEBUG_FLUSH_TEXT_MAX;
			}

			(void) FM_BOARD_UartTransmit((const uint8_t *) fm_debug_flush_buffer,
					(uint32_t) len,
					UART_TIMEOUT_MS);
		}
	}
}
