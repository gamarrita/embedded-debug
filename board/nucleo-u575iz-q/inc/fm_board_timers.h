#ifndef FM_BOARD_TIMERS_H
#define FM_BOARD_TIMERS_H

#include <stdint.h>

void FM_BOARD_TIMERS_Init(void);
void FM_BOARD_TIMERS_DelayMs(uint32_t delay_ms);

#endif /* FM_BOARD_TIMERS_H */