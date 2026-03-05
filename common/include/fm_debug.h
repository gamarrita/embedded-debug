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
void FM_DEBUG_LedActive(fm_debug_led_state_t state);

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


/* ===== Compile-time control ===== */

/* Use only these macros from application code to honor the jumper state */

/* UART helpers */
#define FM_DEBUG_UART_MSG(buf) \
    do { if (FM_DEBUG_MsgIsEnabled()) FM_DEBUG_UartMsg((buf), (strlen(buf))); } while (0)
#define FM_DEBUG_UART_UINT32(x) \
    do { if (FM_DEBUG_MsgIsEnabled()) FM_DEBUG_UartUint32((x)); } while (0)
#define FM_DEBUG_UART_INT32(x) \
    do { if (FM_DEBUG_MsgIsEnabled()) FM_DEBUG_UartInt32((x)); } while (0)
#define FM_DEBUG_UART_FLOAT(x) \
    do { if (FM_DEBUG_MsgIsEnabled()) FM_DEBUG_UartFloat((x)); } while (0)

/* LED helpers */
#define FM_DEBUG_LED_ERROR(state) \
    do { if (FM_DEBUG_LedsAreEnabled()) FM_DEBUG_LedError((state)); } while (0)
#define FM_DEBUG_LED_ACTIVE(state) \
    do { if (FM_DEBUG_LedsAreEnabled()) FM_DEBUG_LedActive((state)); } while (0)
#define FM_DEBUG_LED_SIGNAL(state) \
    do { if (FM_DEBUG_LedsAreEnabled()) FM_DEBUG_LedSignal((state)); } while (0)


#endif /* FM_DEBUG_H */
