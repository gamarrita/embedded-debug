/**
 * @file    fm_debug.h
 * @brief   Public debug API: event logging, LEDs, errors, and UART helpers.
 *
 * @details
 *  - Exposes ISR-safe logging with deferred formatting/flush.
 *  - Provides jumper-gated enable checks for messages and LEDs.
 *  - Offers legacy blocking UART helpers for foreground diagnostics.
 */
#ifndef FM_DEBUG_H
#define FM_DEBUG_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    FM_DEBUG_LED_OFF = 0,
    FM_DEBUG_LED_ON  = 1
} fm_debug_led_state_t;

typedef enum
{
    FM_DEBUG_ERR_NONE = 0,
    FM_DEBUG_ERR_OVERRUN,
    FM_DEBUG_ERR_TIMEOUT,
    FM_DEBUG_ERR_BACKEND,
    FM_DEBUG_ERR_BUFFER_FULL,
    FM_DEBUG_ERR_COUNT
} fm_debug_error_t;

typedef enum
{
    FM_DEBUG_EVT_NONE = 0,
    FM_DEBUG_EVT_ERROR,
    FM_DEBUG_EVT_MARK,
    FM_DEBUG_EVT_TEXT,
    FM_DEBUG_EVT_IRQ_LATE,
    FM_DEBUG_EVT_USER = 0x0100
} fm_debug_event_t;

typedef struct
{
    uint32_t ts_cycles;
    uint16_t code;
    uint16_t flags;
    int32_t  param0;
    int32_t  param1;
} fm_debug_entry_t;

/** @brief Initialize the debug subsystem (ring buffer, DWT, counters). Call once. */
void FM_DEBUG_Init(void);

/** @brief Refresh jumper-controlled enables for messages and LEDs. */
void FM_DEBUG_RefreshJumpers(void);

/** @brief Returns true if any debug output (messages or LEDs) is currently enabled. */
bool FM_DEBUG_IsEnabled(void);

/** @brief Returns true if UART debug messages are enabled. */
bool FM_DEBUG_MsgIsEnabled(void);

/** @brief Returns true if debug LEDs are enabled. */
bool FM_DEBUG_LedsAreEnabled(void);

/** @brief Drive the error LED. */
void FM_DEBUG_LedError(fm_debug_led_state_t state);

/** @brief Drive the run/status LED. */
void FM_DEBUG_LedRun(fm_debug_led_state_t state);

/** @brief Drive the signal/activity LED. */
void FM_DEBUG_LedSignal(fm_debug_led_state_t state);

/** @brief Record an error without a parameter and update LED/mask/counter. */
void FM_DEBUG_ReportError(fm_debug_error_t err);

/** @brief Record an error with a parameter and update LED/mask/counter. */
void FM_DEBUG_ReportErrorWithParam(fm_debug_error_t err, int32_t param);

/** @brief Retrieve how many times a specific error was reported. */
uint32_t FM_DEBUG_ErrorCount(fm_debug_error_t err);

/** @brief Most recently reported error. */
fm_debug_error_t FM_DEBUG_LastError(void);

/** @brief Bitmask of all errors seen since the last clear. */
uint32_t FM_DEBUG_ErrorMask(void);

/** @brief Last parameter recorded for the given error code. */
int32_t FM_DEBUG_ErrorParam(fm_debug_error_t err);

/** @brief Human-readable string for an error code. */
const char *FM_DEBUG_ErrorString(fm_debug_error_t err);

/** @brief Clear all error counters, mask, and LED. */
void FM_DEBUG_ClearErrors(void);

/** @brief Read the current timestamp in CPU cycles (0 if unsupported). */
uint32_t FM_DEBUG_TimestampCycles(void);

/** @brief ISR-safe logging with one parameter. */
bool FM_DEBUG_LogISR(uint16_t code, int32_t param0);

/** @brief ISR-safe logging with two parameters. */
bool FM_DEBUG_Log2ISR(uint16_t code, int32_t param0, int32_t param1);

/** @brief ISR-safe logging of a constant string pointer. */
bool FM_DEBUG_LogConstISR(const char *p_msg);

/** @brief Number of events dropped because the ring buffer was full. */
uint32_t FM_DEBUG_DroppedCount(void);

/** @brief Current number of queued events pending flush. */
uint32_t FM_DEBUG_QueuedCount(void);

/** @brief Peak queued depth observed since init. */
uint32_t FM_DEBUG_HighWatermark(void);

/** @brief Flush queued events over UART (foreground only). */
void FM_DEBUG_Flush(void);

/** @brief Blocking UART send of a raw message buffer (foreground). */
bool FM_DEBUG_UartMsg(const char *p_msg, uint32_t len);

/** @brief Blocking UART send of an unsigned 32-bit value with newline. */
bool FM_DEBUG_UartUint32(uint32_t num);

/** @brief Blocking UART send of a signed 32-bit value with newline. */
bool FM_DEBUG_UartInt32(int32_t num);

/** @brief Blocking UART send of a float with two decimals and newline. */
bool FM_DEBUG_UartFloat(float num);

#endif /* FM_DEBUG_H */

