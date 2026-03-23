#ifndef FM_HW_GPIO_H
#define FM_HW_GPIO_H

#include <stdbool.h>
#include <stdint.h>

/* Low-level GPIO for debug LEDs and jumper sampling. */

void FM_HW_GPIO_Init(void);

bool FM_HW_GPIO_IsDbgLedEnabled(void);
bool FM_HW_GPIO_IsDbgMsgEnabled(void);

void FM_HW_GPIO_LedErrorOn(void);
void FM_HW_GPIO_LedErrorOff(void);
void FM_HW_GPIO_LedRunOn(void);
void FM_HW_GPIO_LedRunOff(void);
void FM_HW_GPIO_LedSignalOn(void);
void FM_HW_GPIO_LedSignalOff(void);

#endif /* FM_HW_GPIO_H */
