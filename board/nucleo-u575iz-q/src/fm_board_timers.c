#include "fm_board_timers.h"

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

/* HAL Callbacks ------------------------------------------------------------ */
void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc)
{
    UNUSED(hrtc);
    FM_MAIN_OnRtcWakeup();
}