#ifndef FM_DEBUG_H
#define FM_DEBUG_H

#include <stdbool.h>
#include <stdint.h>

/* ===== Public types ===== */

typedef enum
{
    FM_DEBUG_LED_OFF = 0,
    FM_DEBUG_LED_ON  = 1
} fm_debug_led_state_t;

typedef enum
{
    FM_DEBUG_ERR_NONE = 0,
    FM_DEBUG_ERR_OVERRUN,
    FM_DEBUG_ERR_TIMEOUT,
    FM_DEBUG_ERR_BACKEND,
    FM_DEBUG_ERR_BUFFER_FULL,
    FM_DEBUG_ERR_COUNT
} fm_debug_error_t;

typedef enum
{
    FM_DEBUG_EVT_NONE = 0,
    FM_DEBUG_EVT_ERROR,
    FM_DEBUG_EVT_MARK,
    FM_DEBUG_EVT_TEXT,
    FM_DEBUG_EVT_IRQ_LATE,
    FM_DEBUG_EVT_USER = 0x0100
} fm_debug_event_t;

typedef struct
{
    uint32_t ts_cycles;
    uint16_t code;
    uint16_t flags;
    int32_t  param0;
    int32_t  param1;
} fm_debug_entry_t;

/* ===== Public API ===== */

void FM_DEBUG_Init(void);

bool FM_DEBUG_IsEnabled(void);
bool FM_DEBUG_MsgIsEnabled(void);
bool FM_DEBUG_LedsAreEnabled(void);
void FM_DEBUG_RefreshJumpers(void);

void FM_DEBUG_LedError(fm_debug_led_state_t state);
void FM_DEBUG_LedRun(fm_debug_led_state_t state);
void FM_DEBUG_LedSignal(fm_debug_led_state_t state);

void FM_DEBUG_ReportError(fm_debug_error_t err);
void FM_DEBUG_ReportErrorWithParam(fm_debug_error_t err, int32_t param);
uint32_t FM_DEBUG_ErrorCount(fm_debug_error_t err);
fm_debug_error_t FM_DEBUG_LastError(void);
uint32_t FM_DEBUG_ErrorMask(void);
int32_t FM_DEBUG_ErrorParam(fm_debug_error_t err);
const char *FM_DEBUG_ErrorString(fm_debug_error_t err);
void FM_DEBUG_ClearErrors(void);

/* New logging API */
uint32_t FM_DEBUG_TimestampCycles(void);

bool FM_DEBUG_LogISR(uint16_t code, int32_t param0);
bool FM_DEBUG_Log2ISR(uint16_t code, int32_t param0, int32_t param1);
bool FM_DEBUG_LogConstISR(const char *p_msg);

uint32_t FM_DEBUG_DroppedCount(void);
uint32_t FM_DEBUG_QueuedCount(void);
uint32_t FM_DEBUG_HighWatermark(void);

void FM_DEBUG_Flush(void);

/* Legacy direct UART API: keep temporarily, but non-core */
bool FM_DEBUG_UartMsg(const char *p_msg, uint32_t len);
bool FM_DEBUG_UartUint32(uint32_t num);
bool FM_DEBUG_UartInt32(int32_t num);
bool FM_DEBUG_UartFloat(float num);

#endif /* FM_DEBUG_H */
