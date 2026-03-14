#ifndef FM_BOARD_GPIO_H
#define FM_BOARD_GPIO_H

#include <stdbool.h>
#include <stdint.h>

void FM_BOARD_GPIO_Init(void);

bool FM_BOARD_GPIO_IsDbgLedEnabled(void);
bool FM_BOARD_GPIO_IsDbgMsgEnabled(void);

void FM_BOARD_GPIO_LedErrorOn(void);
void FM_BOARD_GPIO_LedErrorOff(void);
void FM_BOARD_GPIO_LedRunOn(void);
void FM_BOARD_GPIO_LedRunOff(void);
void FM_BOARD_GPIO_LedSignalOn(void);
void FM_BOARD_GPIO_LedSignalOff(void);

#endif /* FM_BOARD_GPIO_H */