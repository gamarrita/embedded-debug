/**
 * @file    fm_debug.c
 * @brief   Debug services: event capture, logging, LEDs, and counters.
 *
 * @details
 *  - Captures compact events (ISR-safe) into a fixed ring buffer.
 *  - Optional DWT timestamping.
 *  - Flushes buffered events over UART in non-critical context.
 *  - Keeps legacy LED control and error counters as support features.
 *
 * @section fm_debug_usage_pattern Usage pattern
 *  - Boot: call FM_DEBUG_Init() then FM_DEBUG_RefreshJumpers().
 *  - ISR: enqueue via FM_DEBUG_LogISR / FM_DEBUG_Log2ISR / FM_DEBUG_LogConstISR (no formatting/UART).
 *  - Foreground: periodically call FM_DEBUG_Flush() outside ISRs to drain the ring.
 *  - Metrics: inspect FM_DEBUG_DroppedCount(), FM_DEBUG_HighWatermark(), and FM_DEBUG_Error*() to tune cadence.
 *
 * @section fm_debug_design_model Design model
 *  - ISR-safe capture: fixed-size ring, constant-time enqueue, drop-on-full.
 *  - Deferred flush: snprintf + UART only in foreground.
 *  - Optional timestamps using the board DWT helper when available.
 *  - Lightweight per-error counters and bitmask for quick health snapshots.
 *
 * @section fm_debug_concurrency Concurrency model
 *  - Single producer (ISRs/time-critical code) pushes events.
 *  - Single consumer (foreground) flushes events.
 *  - Lock-free head/tail indices; interrupts are not disabled.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "fm_debug.h"
#include "fm_board.h"

/* Design notes:
 * - Single-producer (ISRs) pushes into a fixed ring; single-consumer (flush) drains.
 * - ISR path avoids printf/UART; enqueue is constant-time and drop-on-full.
 * - Flush performs formatting + blocking UART in foreground only.
 * - Error counters provide a quick summary; ring holds ordered detail until flushed.
 */

/* Private Defines */
#define MSG_BUFFER_LENGTH        (96U)
#define UART_TIMEOUT_MS          (10U)
#define FM_DEBUG_EVT_CAPACITY    (64U)  /* Power-of-two for fast masking. */
#define FM_DEBUG_EVT_MASK        (FM_DEBUG_EVT_CAPACITY - 1U)
#define FM_DEBUG_FLUSH_TEXT_MAX  (128U)

#define FM_DEBUG_FLAG_HAS_PARAM1   (1U << 0) /* Indicates param1 field is valid. */
#define FM_DEBUG_FLAG_CONST_TEXT   (1U << 1) /* Entry carries a const text pointer instead of params. */

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

/* Single-producer (ISR) / single-consumer (flush) ring buffer.
 * Lock-free: head advances in producer, tail in consumer; overflow drops newest.
 */
static fm_debug_ring_entry_t fm_debug_ring[FM_DEBUG_EVT_CAPACITY];
static volatile uint32_t fm_debug_evt_head = 0U;
static volatile uint32_t fm_debug_evt_tail = 0U;
static volatile uint32_t fm_debug_evt_dropped = 0U;   /* Count of entries rejected when full. */
static volatile uint32_t fm_debug_evt_high_water = 0U;/* Max depth observed. */

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
static uint32_t FM_DEBUG_TimestampCyclesInternal(void);
static bool FM_DEBUG_Enqueue(uint16_t code, uint16_t flags, int32_t param0, int32_t param1, const char *p_text);

/* Private Bodies */
static uint32_t FM_DEBUG_TimestampCyclesInternal(void)
{
	/* Returns 0 when DWT was not enabled by the board layer. */
	if (!fm_debug_dwt_ready)
	{
		return 0U;
	}

	return FM_BOARD_DwtGetCycles();
}

static bool FM_DEBUG_Enqueue(uint16_t code, uint16_t flags, int32_t param0, int32_t param1, const char *p_text)
{
	uint32_t head = fm_debug_evt_head;
	uint32_t tail = fm_debug_evt_tail;
	uint32_t queued = head - tail;

	if (queued >= FM_DEBUG_EVT_CAPACITY)
	{
		/* Drop-on-full to keep ISR bounded and non-blocking. */
		fm_debug_evt_dropped++;
		return false;
	}

	fm_debug_ring_entry_t *p_evt = &fm_debug_ring[head & FM_DEBUG_EVT_MASK];
	/* No formatting or UART in ISR: just copy fields and advance head. */
	p_evt->ts_cycles = FM_DEBUG_TimestampCyclesInternal();
	p_evt->code = code;
	p_evt->flags = flags;
	p_evt->param0 = param0;
	p_evt->param1 = param1;
	p_evt->p_text = p_text;

	fm_debug_evt_head = head + 1U;

	if ((queued + 1U) > fm_debug_evt_high_water)
	{
		/* Track maximum fill level to tune flush rate or buffer size. */
		fm_debug_evt_high_water = queued + 1U;
	}

	return true;
}

/* Public Bodies */
/**
 * @brief Initialize debug module state and board hooks.
 *
 * Enables DWT cycle counting when available, resets buffers/counters,
 * and refreshes jumper-controlled enables so UART/LED output follows hardware gating.
 *
 * @note Call once after clocks and board peripherals are configured.
 */
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

/**
 * @brief Check if any debug feature (messages or LEDs) is enabled.
 *
 * Uses the last sampled jumper/config state; call FM_DEBUG_RefreshJumpers() if hardware can change.
 *
 * @return true when UART messages or debug LEDs are permitted.
 */
bool FM_DEBUG_IsEnabled(void)
{
	return (fm_debug_msg_enable || fm_debug_leds_enable);
}

/**
 * @brief Re-sample hardware jumpers or runtime configuration that gate debug output.
 *
 * Call after FM_DEBUG_Init() and whenever jumpers or settings may change.
 */
void FM_DEBUG_RefreshJumpers(void)
{
	fm_debug_msg_enable = FM_BOARD_DebugMsgEnabled();
	fm_debug_leds_enable = FM_BOARD_DebugLedsEnabled();
}

/**
 * @brief Check if buffered debug messages are permitted.
 *
 * @note Uses last sampled jumper/config; refresh if hardware changes.
 */
bool FM_DEBUG_MsgIsEnabled(void)
{
	return fm_debug_msg_enable;
}

/**
 * @brief Check if debug LEDs are permitted.
 *
 * @note Uses last sampled jumper/config; refresh if hardware changes.
 */
bool FM_DEBUG_LedsAreEnabled(void)
{
	return fm_debug_leds_enable;
}

/**
 * @brief Record an error without an attached parameter.
 *
 * Increments the error counter, sets the mask bit, lights the error LED,
 * and enqueues an FM_DEBUG_EVT_ERROR entry.
 *
 * @warning No effect if err is FM_DEBUG_ERR_NONE or FM_DEBUG_ERR_COUNT.
 */
void FM_DEBUG_ReportError(fm_debug_error_t err)
{
	FM_DEBUG_ReportErrorWithParam(err, 0);
}

/**
 * @brief Retrieve how many times a specific error was reported.
 *
 * @param err Error code to query.
 * @return Count for err, or 0 when err is out of range.
 *
 * @note Counter is monotonic until FM_DEBUG_ClearErrors().
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
 * @brief Most recently reported error code.
 *
 * @return Last error, or FM_DEBUG_ERR_NONE if none logged.
 */
fm_debug_error_t FM_DEBUG_LastError(void)
{
	return fm_debug_last_error;
}

/**
 * @brief Bitmask of all errors that occurred since the last clear.
 *
 * Bit position matches fm_debug_error_t; bits persist until FM_DEBUG_ClearErrors().
 */
uint32_t FM_DEBUG_ErrorMask(void)
{
	return fm_debug_error_mask;
}

/**
 * @brief Clear counters, parameters, mask, and turn off the error LED.
 *
 * @note Does not clear events already queued in the ring buffer.
 */
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

	FM_BOARD_LedErrorOff();
}

/**
 * @brief Drive the error indicator LED.
 *
 * @param state Desired LED state.
 *
 * @note Does not check FM_DEBUG_LedsAreEnabled(); gate externally if needed.
 */
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

/**
 * @brief Drive the run/status LED.
 *
 * @param state Desired LED state.
 */
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

/**
 * @brief Drive the signal/activity LED.
 *
 * @param state Desired LED state.
 */
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

/**
 * @brief Transmit a raw message buffer over UART when debug messages are enabled.
 *
 * @note Legacy helper; not IRQ-safe. Length is clamped to the internal buffer size.
 *
 * @param p_msg Pointer to message buffer.
 * @param len   Number of bytes to send.
 * @return true if transmitted; false if disabled or input invalid.
 */
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

/**
 * @brief Transmit an unsigned 32-bit value with trailing newline.
 *
 * @warning Not IRQ-safe; intended for foreground diagnostics.
 *
 * @param num Value to print.
 * @return true if transmitted; false if messages disabled.
 */
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

/**
 * @brief Record an error and attach one signed 32-bit parameter.
 *
 * Increments the counter, updates the mask, stores the parameter, lights the error LED,
 * and enqueues an FM_DEBUG_EVT_ERROR entry.
 *
 * @warning No effect if err is FM_DEBUG_ERR_NONE or FM_DEBUG_ERR_COUNT.
 */
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

	(void) FM_DEBUG_Enqueue((uint16_t) FM_DEBUG_EVT_ERROR, 0U, param, 0, NULL);
	FM_DEBUG_LedError(FM_DEBUG_LED_ON);
}

/**
 * @brief Last parameter recorded for the given error code.
 *
 * @param err Error code to query.
 * @return Stored parameter or 0 if err is invalid or unused.
 */
int32_t FM_DEBUG_ErrorParam(fm_debug_error_t err)
{
	if ((err <= FM_DEBUG_ERR_NONE) || (err >= FM_DEBUG_ERR_COUNT))
	{
		return 0;
	}

	return fm_debug_error_param[err];
}

/**
 * @brief Human-readable string for the given error code.
 *
 * @param err Error code to stringify.
 * @return Pointer to a static string; "UNKNOWN" when err is out of range.
 */
const char *FM_DEBUG_ErrorString(fm_debug_error_t err)
{
	if ((err < FM_DEBUG_ERR_NONE) || (err >= FM_DEBUG_ERR_COUNT))
	{
		return "UNKNOWN";
	}

	return fm_debug_error_str[err];
}

/**
 * @brief Transmit a signed 32-bit value with trailing newline.
 *
 * @warning Not IRQ-safe; intended for foreground diagnostics.
 */
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

/**
 * @brief Transmit a float with two decimal places and trailing newline.
 *
 * @warning Uses snprintf and blocking UART; avoid inside ISRs.
 */
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

/**
 * @brief Read the current DWT cycle counter if supported.
 *
 * @return Cycle count, or 0 when the board does not expose DWT.
 *
 * @note Used internally for event timestamps; can be polled for profiling.
 */
uint32_t FM_DEBUG_TimestampCycles(void)
{
	return FM_DEBUG_TimestampCyclesInternal();
}

/**
 * @brief Log a compact event from ISR or time-critical code.
 *
 * @param code   Application-defined event code (or fm_debug_event_t).
 * @param param0 Primary parameter to capture with the event.
 * @return true if enqueued; false if the ring buffer was full (event dropped).
 *
 * @note Designed to be IRQ-safe; event is transmitted later by FM_DEBUG_Flush().
 */
bool FM_DEBUG_LogISR(uint16_t code, int32_t param0)
{
	return FM_DEBUG_Enqueue(code, 0U, param0, 0, NULL);
}

/**
 * @brief Log an event with two parameters from ISR or time-critical code.
 *
 * @param code   Application-defined event code.
 * @param param0 First parameter.
 * @param param1 Second parameter.
 * @return true if enqueued; false if dropped due to full buffer.
 */
bool FM_DEBUG_Log2ISR(uint16_t code, int32_t param0, int32_t param1)
{
	return FM_DEBUG_Enqueue(code, FM_DEBUG_FLAG_HAS_PARAM1, param0, param1, NULL);
}

/**
 * @brief Log a constant text string from ISR without formatting.
 *
 * @param p_msg Pointer to a static, long-lived string literal.
 * @return true if enqueued; false if buffer full or p_msg is NULL.
 *
 * @warning p_msg must live longer than the log entry; never pass stack buffers.
 */
bool FM_DEBUG_LogConstISR(const char *p_msg)
{
	if (p_msg == NULL)
	{
		return false;
	}

	return FM_DEBUG_Enqueue((uint16_t) FM_DEBUG_EVT_TEXT, FM_DEBUG_FLAG_CONST_TEXT, 0, 0, p_msg);
}

/**
 * @brief Number of events dropped because the ring buffer was full.
 *
 * @return Cumulative drop count since FM_DEBUG_Init().
 */
uint32_t FM_DEBUG_DroppedCount(void)
{
	return fm_debug_evt_dropped;
}

/**
 * @brief Current number of queued events waiting to be flushed.
 *
 * @return Pending entries in the ring buffer.
 */
uint32_t FM_DEBUG_QueuedCount(void)
{
	return (fm_debug_evt_head - fm_debug_evt_tail);
}

/**
 * @brief Peak queued depth observed since FM_DEBUG_Init().
 *
 * @return Maximum ring occupancy recorded.
 */
uint32_t FM_DEBUG_HighWatermark(void)
{
	return fm_debug_evt_high_water;
}

/**
 * @brief Flush queued debug events over the board UART.
 *
 * @details Performs snprintf and blocking UART transfers; intended for non-ISR context
 *          such as the main loop or a low-priority task.
 *
 * @note Flush frequently to minimize drops. Not ISR-safe.
 */
void FM_DEBUG_Flush(void)
{
	if (!fm_debug_msg_enable)
	{
		return;
	}

	while (fm_debug_evt_tail != fm_debug_evt_head)
	{
		/* Pop one entry; consumer side only advances tail. */
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

			/* Blocking UART transmit and snprintf live here, not in ISR. */
			(void) FM_BOARD_UartTransmit((const uint8_t *) fm_debug_flush_buffer,
					(uint32_t) len,
					UART_TIMEOUT_MS);
		}
	}
}
