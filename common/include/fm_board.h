/**
 * @file    fm_board.h
 * @brief   Public board-level API for debug peripherals.
 *
 * @details
 *  - Facade for GPIO jumpers/LEDs, UART, and DWT helpers.
 *  - Applications use this surface; low-level lives in fm_hw_* modules.
 *  - FM_BOARD_Init owns bring-up order; there is no fm_hw.c aggregator.
 */
#ifndef FM_BOARD_H
#define FM_BOARD_H

#include <stdbool.h>
#include <stdint.h>

/** @brief Initialize board-level debug peripherals (GPIO → UART → DWT). */
void FM_BOARD_Init(void);

/** @brief Sample jumper or config that enables UART debug messages. */
bool FM_BOARD_DebugMsgEnabled(void);

/** @brief Sample jumper or config that enables debug LEDs. */
bool FM_BOARD_DebugLedsEnabled(void);

/** @brief Drive the error indicator LED. */
void FM_BOARD_LedErrorOn(void);
void FM_BOARD_LedErrorOff(void);

/** @brief Drive the run/status LED. */
void FM_BOARD_LedRunOn(void);
void FM_BOARD_LedRunOff(void);

/** @brief Drive the signal/activity LED. */
void FM_BOARD_LedSignalOn(void);
void FM_BOARD_LedSignalOff(void);

/** @brief Blocking UART transmit through the board debug channel. */
bool FM_BOARD_UartTransmit(const uint8_t *p_data, uint32_t len, uint32_t timeout_ms);

/** @brief Initialize the DWT cycle counter if present. */
bool FM_BOARD_DwtInit(void);

/** @brief Read the current DWT cycle count (0 if unavailable). */
uint32_t FM_BOARD_DwtGetCycles(void);

#endif /* FM_BOARD_H */

