/**
 * @file    fm_main.c
 * @brief   Application entry point and main execution loop.
 *
 * @date    2026-02-28
 * @author  Daniel H Sagarra
 *
 * @details
 *  - Initializes board and debug services.
 *  - Demonstrates deferred debug logging from an RTC wakeup ISR.
 *  - Main loop flushes queued debug events in foreground context.
 *  - ISR measures wakeup period and jitter using DWT timestamps.
 */

/* ===========================     Includes    ============================== */
#include "main.h"
#include "fm_main.h"
#include "fm_debug.h"
#include "fm_board.h"

/* =========================== Private Defines ============================== */
/**
 * @brief Event code used for RTC wakeup jitter measurements.
 *
 * Event payload:
 * - param0: measured interrupt period in CPU cycles
 * - param1: signed jitter in CPU cycles
 */
#define FM_MAIN_EVT_RTC_JITTER   (FM_DEBUG_EVT_USER + 1U)

/**
 * @brief Expected RTC wakeup period in CPU cycles.
 *
 * Example value for a 1 s period at 10 MHz DWT/CPU clock.
 * Replace with a board-specific conversion if clock changes dynamically.
 */
#define RTC_EXPECTED_CYCLES      (24000000U)

/* =========================== Private Types ================================ */
/* (none) */

/* =========================== Private Data ================================= */
/**
 * @brief Foreground notification flag set by the RTC ISR.
 *
 * Used only to wake the main loop so it can flush deferred debug events.
 */
static volatile bool rtc_flag = false;

/**
 * @brief Last captured RTC interrupt timestamp in CPU cycles.
 */
static uint32_t rtc_last_ts = 0U;

/**
 * @brief Expected RTC wakeup period in CPU cycles.
 */
static uint32_t rtc_expected_cycles = 0U;

/**
 * @brief Indicates whether a previous timestamp is available.
 *
 * The first interrupt only initializes the measurement baseline.
 */
static bool rtc_jitter_ready = false;

/* =========================== Private Prototypes =========================== */
/* (none) */

/* =========================== Private Bodies =============================== */
/* (none) */

/* =========================== Public Bodies ================================ */
/**
 * @brief Initialize board services and jitter measurement state.
 *
 * Initializes the board layer first, then the debug module, and finally
 * prepares local state used by the RTC jitter example.
 */
void FM_MAIN_Init(void)
{
    FM_BOARD_Init();
    FM_DEBUG_Init();

    rtc_expected_cycles = RTC_EXPECTED_CYCLES;
    rtc_last_ts = 0U;
    rtc_jitter_ready = false;
    rtc_flag = false;
}

/**
 * @brief Main application loop.
 *
 * Waits for RTC wakeup notifications from the ISR and flushes any deferred
 * debug events from foreground context.
 *
 * @note FM_DEBUG_Flush() is intentionally executed outside interrupt context
 *       because it performs formatting and blocking UART transmission.
 */
void FM_MAIN_Main(void)
{
    FM_MAIN_Init();

    for (;;)
    {
        while (!rtc_flag)
        {
            __NOP();
        }

        rtc_flag = false;
        FM_DEBUG_Flush();
    }
}

/* =========================== Interrupts =================================== */
/**
 * @brief RTC wakeup callback used to measure interrupt period and jitter.
 *
 * This ISR:
 * - captures the current DWT timestamp
 * - computes the elapsed period since the previous wakeup
 * - computes signed jitter against the expected period
 * - enqueues the measurement using deferred debug logging
 * - notifies the foreground loop to flush pending events
 *
 * Logged event payload:
 * - param0: measured period in cycles
 * - param1: signed jitter in cycles
 */
void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc)
{
    uint32_t now;
    uint32_t delta_cycles;
    int32_t jitter_cycles;

    UNUSED(hrtc);

    now = FM_DEBUG_TimestampCycles();

    /* First sample establishes the timing baseline only. */
    if (!rtc_jitter_ready)
    {
        rtc_last_ts = now;
        rtc_jitter_ready = true;
        rtc_flag = true;
        return;
    }

    delta_cycles = now - rtc_last_ts;
    rtc_last_ts = now;

    jitter_cycles = (int32_t)delta_cycles - (int32_t)rtc_expected_cycles;

    (void)FM_DEBUG_Log2ISR(FM_MAIN_EVT_RTC_JITTER,
                           (int32_t)delta_cycles,
                           jitter_cycles);

    rtc_flag = true;
}

/*** end of file ***/
