/**
 * @file    fm_debug.c
 * @brief   Debug services: event capture, logging, LEDs, and counters.
 *
 * @details
 *  - ISR-safe producer pushes structured events into a fixed ring (drop-on-full).
 *  - Foreground consumer formats from the ring into a flush buffer, then UART.
 *  - Optional DWT timestamps when the board exposes CYCCNT; fall back to 0.
 *  - Jumper gating controls debug messages and LEDs; cached enables refreshed by caller.
 *  - Legacy direct UART helpers remain for simple foreground diagnostics.
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
#define FM_DEBUG_EVT_CAPACITY    (8U)  /* Power-of-two for fast masking. */
#define FM_DEBUG_EVT_MASK        (FM_DEBUG_EVT_CAPACITY - 1U)
#define FM_DEBUG_FLUSH_TEXT_MAX  (128U)

#define FM_DEBUG_FLAG_HAS_PARAM1   (1U << 0) /* Indicates param1 field is valid. */
#define FM_DEBUG_FLAG_CONST_TEXT   (1U << 1) /* Entry carries a const text pointer instead of params. */

/* Private Types */

/**
 * @brief Entry stored in the debug ring buffer.
 *
 * Each entry represents a debug event captured at runtime.
 * The entry is designed to be lightweight and ISR-safe to produce.
 *
 * Fields:
 * - ts_cycles: Timestamp in CPU cycles (DWT CYCCNT if available).
 * - code:      Application-defined event identifier.
 * - flags:     Optional flags describing the event (context, severity, etc.).
 * - param0:    First optional numeric parameter.
 * - param1:    Second optional numeric parameter.
 * - p_text:    Optional pointer to a constant string (may be NULL).
 *
 * Notes:
 * - p_text is not copied; it must point to persistent memory (e.g. string literal).
 * - Timestamp resolution and availability depend on DWT support.
 * - Designed for lock-free or low-overhead logging paths.
 */
typedef struct
{
    uint32_t ts_cycles;
    uint16_t code;
    uint16_t flags;
    int32_t  param0;
    int32_t  param1;
    const char *p_text;
} ring_entry_t;

/* Private Data */

/**
 * @brief Runtime enable flag for debug message output.
 *
 * Updated from board-level configuration (e.g. jumper sampling).
 * May be read from different execution contexts (foreground / ISR),
 * therefore declared volatile.
 */
static volatile bool msg_enable = false;

/**
 * @brief Runtime enable flag for debug LED activity.
 *
 * Controls whether debug-related LED signaling is active.
 * May be read from different execution contexts (foreground / ISR),
 * therefore declared volatile.
 */
static volatile bool leds_enable = false;

/**
 * @brief Indicates whether DWT cycle counter is initialized and usable.
 *
 * Set during debug initialization if the target supports DWT CYCCNT.
 * Used to guard timestamp generation.
 */
static bool dwt_ready = false;


/**
 * @brief Temporary formatting buffer for direct UART debug helpers.
 *
 * Used by legacy-style functions (e.g. FM_DEBUG_UartUint32/Int32/Float)
 * that format and transmit messages immediately in foreground context.
 *
 * Not used by the ring-buffered logging path.
 */
static char msg_buffer[MSG_BUFFER_LENGTH];

/**
 * @brief Lock-free ring buffer storing raw debug events.
 *
 * Acts as the producer/consumer boundary between ISR-safe logging
 * and deferred formatting/transmission.
 *
 * Data flow:
 *   ISR / time-critical code → enqueue → evt_ring → flush → UART
 *
 * Characteristics:
 * - Stores structured events (no text formatting here).
 * - Single-producer (ISR) / single-consumer (foreground).
 * - Lock-free using head/tail indices.
 * - Drop-on-full to keep ISR execution bounded.
 */
static ring_entry_t evt_ring[FM_DEBUG_EVT_CAPACITY];

/**
 * @brief Temporary text buffer used by the flush (consumer) stage.
 *
 * Converts structured ring entries into formatted strings before UART transmission.
 *
 * Relationship with evt_ring:
 * - evt_ring holds raw events.
 * - This buffer holds the formatted representation of one event at a time.
 *
 * Used only in the deferred path:
 *   evt_ring → snprintf → flush_buffer → UART
 *
 * @note Not ISR-safe (uses snprintf and blocking UART).
 */
static char flush_buffer[FM_DEBUG_FLUSH_TEXT_MAX];

static volatile uint32_t evt_head = 0U;
static volatile uint32_t evt_tail = 0U;
static volatile uint32_t evt_dropped = 0U;   /* Count of entries rejected when full. */
static volatile uint32_t evt_high_water = 0U;/* Max depth observed. */

/* Error tracking */
static volatile uint32_t err_counts[FM_DEBUG_ERR_COUNT] = { 0U };
static volatile uint32_t err_mask = 0U;
static volatile fm_debug_error_t last_error = FM_DEBUG_ERR_NONE;
static volatile int32_t err_param[FM_DEBUG_ERR_COUNT] = { 0 };
static const char *err_str[FM_DEBUG_ERR_COUNT] =
{
    "NONE",
    "OVERRUN",
    "TIMEOUT",
    "BACKEND",
    "BUFFER_FULL"
};

/* Private Prototypes */
static uint32_t timestamp_cycles(void);
static bool enqueue_event(uint16_t code, uint16_t flags, int32_t param0, int32_t param1, const char *p_text);

/* Private Bodies */
/**
 * @brief Retrieve a timestamp in CPU cycles for debug events.
 *
 * Provides a unified access point for timestamp generation used by the
 * debug logging system.
 *
 * Behavior:
 * - Returns the current DWT cycle counter when available.
 * - Returns 0 when DWT is not supported or not initialized.
 *
 * Rationale:
 * - Avoids conditional checks in ISR logging paths.
 * - Keeps timestamp acquisition lightweight and optional.
 *
 * @return Cycle count timestamp, or 0 when not available.
 */
static uint32_t timestamp_cycles(void)
{
    if (!dwt_ready)
    {
        return 0U;
    }

    return FM_BOARD_DwtGetCycles();
}

/**
 * @brief Enqueue a debug event into the ring buffer (ISR-safe).
 *
 * This is the single entry point for logging events into the debug system.
 * Designed to be callable from ISR or time-critical code paths.
 *
 * Behavior:
 * - Writes a structured event into the ring buffer.
 * - Does not perform formatting or UART transmission.
 * - Executes in constant time.
 *
 * Concurrency model:
 * - Single producer (ISR) updates head.
 * - Single consumer (FM_DEBUG_Flush) updates tail.
 * - Lock-free; no interrupts are disabled.
 *
 * Overflow policy:
 * - If the buffer is full, the event is dropped.
 * - A drop counter is incremented.
 * - The function returns false.
 *
 * Parameters:
 * @param code   Application-defined event identifier.
 * @param flags  Bitmask describing event format (e.g. param1 valid, const text).
 * @param param0 First numeric parameter.
 * @param param1 Second numeric parameter (used if flagged).
 * @param p_text Optional pointer to constant text (used if flagged).
 *
 * @return true if the event was enqueued, false if dropped due to full buffer.
 *
 * @note p_text is not copied; it must point to persistent memory.
 */
static bool enqueue_event(uint16_t code, uint16_t flags, int32_t param0, int32_t param1, const char *p_text)
{
    uint32_t head = evt_head;
    uint32_t tail = evt_tail;

    /* Number of queued elements (wrap-safe due to unsigned arithmetic). */
    uint32_t queued = head - tail;

    if (queued >= FM_DEBUG_EVT_CAPACITY)
    {
        /* Drop-on-full to keep ISR bounded and non-blocking. */
        evt_dropped++;
        return false;
    }

    ring_entry_t *p_evt = &evt_ring[head & FM_DEBUG_EVT_MASK];
    /* No formatting or UART in ISR: just copy fields and advance head. */
    p_evt->ts_cycles = timestamp_cycles();
    p_evt->code = code;
    p_evt->flags = flags;
    p_evt->param0 = param0;
    p_evt->param1 = param1;
    p_evt->p_text = p_text;

    evt_head = head + 1U;

    if ((queued + 1U) > evt_high_water)
    {
        /* Track maximum fill level to tune flush rate or buffer size. */
        evt_high_water = queued + 1U;
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
    dwt_ready = FM_BOARD_DwtInit();
    evt_head = 0U;
    evt_tail = 0U;
    evt_dropped = 0U;
    evt_high_water = 0U;

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
    return (msg_enable || leds_enable);
}

/**
 * @brief Refresh cached debug enable states from board configuration.
 *
 * Re-samples the board-level sources that gate debug output, such as
 * message-enable and LED-enable jumpers.
 *
 * Contract:
 * - Updates the internal cached enables used by the debug module.
 * - Does not flush pending events.
 * - Does not retroactively affect events already stored in the ring buffer.
 *
 * Call this after FM_DEBUG_Init() and whenever the external debug
 * configuration may have changed.
 */
void FM_DEBUG_RefreshJumpers(void)
{
    msg_enable = FM_BOARD_DebugMsgEnabled();
    leds_enable = FM_BOARD_DebugLedsEnabled();
}

/**
 * @brief Report whether debug message output is currently permitted.
 *
 * Returns the last cached message-enable state sampled by
 * FM_DEBUG_RefreshJumpers().
 *
 * @return true when foreground UART debug output is allowed.
 *
 * @note This function does not read hardware directly.
 *       Call FM_DEBUG_RefreshJumpers() to refresh the cached state.
 */
bool FM_DEBUG_MsgIsEnabled(void)
{
    return msg_enable;
}

/**
 * @brief Report whether debug LED signaling is currently permitted.
 *
 * Returns the last cached LED-enable state sampled by
 * FM_DEBUG_RefreshJumpers().
 *
 * @return true when debug LED activity is allowed.
 *
 * @note This function does not read hardware directly.
 *       Call FM_DEBUG_RefreshJumpers() to refresh the cached state.
 */
bool FM_DEBUG_LedsAreEnabled(void)
{
    return leds_enable;
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

    return err_counts[err];
}

/**
 * @brief Most recently reported error code.
 *
 * @return Last error, or FM_DEBUG_ERR_NONE if none logged.
 */
fm_debug_error_t FM_DEBUG_LastError(void)
{
    return last_error;
}

/**
 * @brief Bitmask of all errors that occurred since the last clear.
 *
 * Bit position matches fm_debug_error_t; bits persist until FM_DEBUG_ClearErrors().
 */
uint32_t FM_DEBUG_ErrorMask(void)
{
    return err_mask;
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
        err_counts[i] = 0U;
        err_param[i] = 0;
    }

    err_mask = 0U;
    last_error = FM_DEBUG_ERR_NONE;

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
	if(!FM_DEBUG_LedsAreEnabled())
	{
		return;
	}

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
 * @brief Drive the run/status LED through the board layer.
 *
 * @param state Desired LED state.
 *
 * Contract:
 * - Directly forwards the request to the board LED backend.
 * - Does not check FM_DEBUG_LedsAreEnabled().
 *
 * @note Gate this call externally if LED activity must follow the
 *       current debug-enable configuration.
 */
void FM_DEBUG_LedRun(fm_debug_led_state_t state)
{
	if(!FM_DEBUG_LedsAreEnabled())
	{
		return;
	}

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
 * @brief Drive the signal/activity LED through the board layer.
 *
 * @param state Desired LED state.
 *
 * Contract:
 * - Directly forwards the request to the board LED backend.
 * - Does not check FM_DEBUG_LedsAreEnabled().
 *
 * @note Gate this call externally if LED activity must follow the
 *       current debug-enable configuration.
 */
void FM_DEBUG_LedSignal(fm_debug_led_state_t state)
{
	if(!FM_DEBUG_LedsAreEnabled())
	{
		return;
	}

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
    if ((p_msg == NULL) || (len == 0U) || (!msg_enable))
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

    if (!msg_enable)
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

    err_counts[err]++;
    last_error = err;
    err_mask |= (1UL << (uint32_t) err);
    err_param[err] = param;

    (void) enqueue_event((uint16_t) FM_DEBUG_EVT_ERROR, 0U, param, 0, NULL);
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

    return err_param[err];
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

    return err_str[err];
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

    if (!msg_enable)
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

    if (!msg_enable)
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
    return timestamp_cycles();
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
    return enqueue_event(code, 0U, param0, 0, NULL);
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
    return enqueue_event(code, FM_DEBUG_FLAG_HAS_PARAM1, param0, param1, NULL);
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

    return enqueue_event((uint16_t) FM_DEBUG_EVT_TEXT, FM_DEBUG_FLAG_CONST_TEXT, 0, 0, p_msg);
}

/**
 * @brief Return how many events were dropped on enqueue.
 *
 * @return Cumulative number of events rejected because the ring buffer
 *         was full since the last FM_DEBUG_Init().
 */
uint32_t FM_DEBUG_DroppedCount(void)
{
    return evt_dropped;
}

/**
 * @brief Return the current number of pending events in the ring buffer.
 *
 * @return Number of events queued for later flush.
 *
 * @note This is a snapshot value and may change asynchronously as
 *       producers enqueue new events.
 */
uint32_t FM_DEBUG_QueuedCount(void)
{
    return (evt_head - evt_tail);
}

/**
 * @brief Return the maximum observed ring-buffer occupancy.
 *
 * @return Peak number of queued events observed since FM_DEBUG_Init().
 *
 * @note Useful for sizing the event ring and tuning flush cadence.
 */
uint32_t FM_DEBUG_HighWatermark(void)
{
    return evt_high_water;
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
    if (!msg_enable)
    {
        return;
    }

    while (evt_tail != evt_head)
    {
        /* Pop one entry; consumer side only advances tail. */
        ring_entry_t evt = evt_ring[evt_tail & FM_DEBUG_EVT_MASK];
        evt_tail++;

        if ((evt.flags & FM_DEBUG_FLAG_CONST_TEXT) != 0U)
        {
            const char *p_text = (evt.p_text != NULL) ? evt.p_text : "";
            int len = snprintf(flush_buffer, FM_DEBUG_FLUSH_TEXT_MAX,
                    "[%lu] TXT=%s\n",
                    (unsigned long) evt.ts_cycles,
                    p_text);

            if (len > 0)
            {
                if ((uint32_t) len > FM_DEBUG_FLUSH_TEXT_MAX)
                {
                    len = (int) FM_DEBUG_FLUSH_TEXT_MAX;
                }

                (void) FM_BOARD_UartTransmit((const uint8_t *) flush_buffer,
                        (uint32_t) len,
                        UART_TIMEOUT_MS);
            }
            continue;
        }

        int len;

        if ((evt.flags & FM_DEBUG_FLAG_HAS_PARAM1) != 0U)
        {
            len = snprintf(flush_buffer, FM_DEBUG_FLUSH_TEXT_MAX,
                    "[%lu] EVT=%u P0=%ld P1=%ld\n",
                    (unsigned long) evt.ts_cycles,
                    (unsigned int) evt.code,
                    (long) evt.param0,
                    (long) evt.param1);
        }
        else
        {
            len = snprintf(flush_buffer, FM_DEBUG_FLUSH_TEXT_MAX,
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
            (void) FM_BOARD_UartTransmit((const uint8_t *) flush_buffer,
                    (uint32_t) len,
                    UART_TIMEOUT_MS);
        }
    }
}

/* Interrupts */
/* (none) */





