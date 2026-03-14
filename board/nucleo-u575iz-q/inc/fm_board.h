#ifndef FM_BOARD_H
#define FM_BOARD_H

#include <stdbool.h>
#include <stdint.h>

void FM_BOARD_Init(void);
void FM_BOARD_DelayMs(uint32_t delay_ms);

bool FM_BOARD_IsDbgLedEnabled(void);
bool FM_BOARD_IsDbgMsgEnabled(void);

void FM_BOARD_LedErrorOn(void);
void FM_BOARD_LedErrorOff(void);
void FM_BOARD_LedRunOn(void);
void FM_BOARD_LedRunOff(void);
void FM_BOARD_LedSignalOn(void);
void FM_BOARD_LedSignalOff(void);

#endif /* FM_BOARD_H */