#ifndef FM_DEBUG_H
#define FM_DEBUG_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/* ===== Public types ===== */
typedef enum
{
    FM_DEBUG_LED_OFF = 0,
    FM_DEBUG_LED_ON  = 1
} fm_debug_led_state_t;

typedef enum
{
    FM_DEBUG_ERR_NONE = 0,
    FM_DEBUG_ERR_JITTER,
    FM_DEBUG_ERR_OVERRUN,
    FM_DEBUG_ERR_TIMEOUT,
    FM_DEBUG_ERR_COUNT
} fm_debug_error_t;

/* ===== Public API ===== */

/**
 * @brief Initializes the debug module reading the enable jumpers.
 *
 * @note Call once at boot before using the debug macros.
 */
void FM_DEBUG_Init(void);

/**
 * @brief Drives the error LED.
 */
void FM_DEBUG_LedError(fm_debug_led_state_t state);

/**
 * @brief Drives the activity LED.
 */
void FM_DEBUG_LedRun(fm_debug_led_state_t state);

/**
 * @brief Drives the signal LED.
 */
void FM_DEBUG_LedSignal(fm_debug_led_state_t state);

/**
 * @brief Sends a binary message over the debug UART.
 *
 * @param p_msg Pointer to buffer; must not be NULL.
 * @param len  Number of bytes to transmit (1..buffer length - 1).
 */
bool FM_DEBUG_UartMsg(const char *p_msg, uint32_t len);

/**
 * @brief Sends an unsigned 32-bit integer over the debug UART.
 */
bool FM_DEBUG_UartUint32(uint32_t num);

/**
 * @brief Sends a signed 32-bit integer over the debug UART.
 */
bool FM_DEBUG_UartInt32(int32_t num);

/**
 * @brief Sends a floating-point number with two decimals over the debug UART.
 */
bool FM_DEBUG_UartFloat(float num);

/**
 * @brief Returns true when UART debug is enabled by jumper.
 */
bool FM_DEBUG_MsgIsEnabled(void);

/**
 * @brief Returns true when LED debug is enabled by jumper.
 */
bool FM_DEBUG_LedsAreEnabled(void);

/**
 * @brief Re-reads the debug enable jumpers with low-power sampling.
 *
 * @note Leaves the pins in analog/no-pull after reading.
 */
void FM_DEBUG_RefreshJumpers(void);

/**
 * @brief Registers an error occurrence (ISR-safe, non-blocking).
 */
void FM_DEBUG_ReportError(fm_debug_error_t err);

/**
 * @brief Returns how many times a given error was detected.
 */
uint32_t FM_DEBUG_ErrorCount(fm_debug_error_t err);

/**
 * @brief Returns the last error detected (or FM_DEBUG_ERR_NONE).
 */
fm_debug_error_t FM_DEBUG_LastError(void);

/**
 * @brief Bitmask of errors seen since the last clear.
 */
uint32_t FM_DEBUG_ErrorMask(void);

/**
 * @brief Clears counters and mask; turns off the error LED.
 */
void FM_DEBUG_ClearErrors(void);

/* ===== Compile-time control ===== */

/* Use only these macros from application code to honor the jumper state */



#endif /* FM_DEBUG_H */
