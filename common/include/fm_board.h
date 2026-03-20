#ifndef FM_BOARD_H
#define FM_BOARD_H

#include <stdbool.h>
#include <stdint.h>

/* Board-facing API for debug-related services: init, jumpers, LEDs, UART, DWT. */

/* Initialization */
void FM_BOARD_Init(void);

/* Debug enable sampling */
bool FM_BOARD_DebugMsgEnabled(void);
bool FM_BOARD_DebugLedsEnabled(void);

/* LED control */
void FM_BOARD_LedErrorOn(void);
void FM_BOARD_LedErrorOff(void);
void FM_BOARD_LedRunOn(void);
void FM_BOARD_LedRunOff(void);
void FM_BOARD_LedSignalOn(void);
void FM_BOARD_LedSignalOff(void);

/* UART access */
bool FM_BOARD_UartTransmit(const uint8_t *p_data, uint32_t len, uint32_t timeout_ms);

/* DWT / timestamp access */
bool FM_BOARD_DwtInit(void);
uint32_t FM_BOARD_DwtGetCycles(void);

#endif /* FM_BOARD_H */
