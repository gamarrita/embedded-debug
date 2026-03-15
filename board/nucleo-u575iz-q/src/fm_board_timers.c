#include "fm_board_timers.h"
#include "fm_debug.h"
#include "fm_main.h"
#include "rtc.h"

/* Public Bodies */
void FM_BOARD_TIMERS_Init(void)
{
    /* RTC is initialized by MX_RTC_Init() in main.c */
}

void FM_BOARD_TIMERS_DelayMs(uint32_t delay_ms)
{
    HAL_Delay(delay_ms);
}
