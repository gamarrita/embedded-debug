#include "jitter_meter.h"
#include "fm_board_dwt.h"

void FM_JitterMeter_Init(FM_JitterMeter_t *ctx, uint32_t period_us)
{
    if (ctx == NULL)
    {
        return;
    }

    ctx->expected_period_cycles = FM_BOARD_DWT_UsToCycles(period_us);
    ctx->prev_cycles = 0U;
    ctx->has_reference = 0U;
}

bool FM_JitterMeter_Sample(FM_JitterMeter_t *ctx, int32_t *error_us)
{
    if ((ctx == NULL) || (error_us == NULL))
    {
        return false;
    }

    uint32_t now_cycles = FM_BOARD_DWT_GetCycles();

    if (ctx->has_reference == 0)
    {
        ctx->prev_cycles = now_cycles; /* prime reference */
        ctx->has_reference = 1;
        return false;
    }

    /* uint32_t subtraction naturally handles DWT wraparound */
    uint32_t interval_cycles = now_cycles - ctx->prev_cycles;
    ctx->prev_cycles = now_cycles;

    int32_t err_cycles = (int32_t)interval_cycles - (int32_t)ctx->expected_period_cycles;
    *error_us = FM_BOARD_DWT_CyclesToUs(err_cycles);

    return true;
}
