/**
 * @file    fm_irq_health.c
 * @brief   Lightweight IRQ timing health monitor (arrival jitter + ISR exec time).
 */

#include "fm_irq_health.h"
#include "fm_board_dwt.h"

/* Private Helpers */
static bool fm_irq_health_ready(fm_irq_health_t *ctx)
{
    if (ctx == NULL)
    {
        return false;
    }

    if (ctx->cycles_per_us == 0U)
    {
        ctx->cycles_per_us = FM_BOARD_DWT_CyclesPerUs();
        if (ctx->cycles_per_us != 0U)
        {
            ctx->ideal_period_cycles = FM_BOARD_DWT_UsToCycles(ctx->ideal_period_us);
        }
    }

    return (ctx->cycles_per_us != 0U);
}

/* Public Bodies */
void FM_IRQ_HEALTH_Init(fm_irq_health_t *ctx,
                        uint32_t ideal_period_us,
                        uint32_t jitter_threshold_us)
{
    if (ctx == NULL)
    {
        return;
    }

    ctx->ideal_period_us = ideal_period_us;
    ctx->cycles_per_us = FM_BOARD_DWT_CyclesPerUs();
    ctx->ideal_period_cycles = (ctx->cycles_per_us != 0U)
                               ? FM_BOARD_DWT_UsToCycles(ideal_period_us)
                               : 0U;
    ctx->jitter_threshold_us = jitter_threshold_us;
    ctx->entry_cycles = 0U;
    ctx->prev_entry_cycles = 0U;
    ctx->has_reference = false;
    FM_IRQ_HEALTH_ResetStats(ctx);
}

void FM_IRQ_HEALTH_OnEntry(fm_irq_health_t *ctx)
{
    if (!fm_irq_health_ready(ctx))
    {
        return;
    }

    uint32_t now_cycles = FM_BOARD_DWT_GetCycles();
    ctx->entry_cycles = now_cycles;

    if (!ctx->has_reference)
    {
        ctx->prev_entry_cycles = now_cycles; /* prime reference without counting a sample */
        ctx->has_reference = true;
        return;
    }

    /* uint32_t subtraction naturally tolerates DWT wraparound */
    uint32_t interval_cycles = now_cycles - ctx->prev_entry_cycles;
    ctx->prev_entry_cycles = now_cycles;

    int64_t jitter_cycles = (int64_t) interval_cycles - (int64_t) ctx->ideal_period_cycles;
    int32_t jitter_us = (int32_t) (jitter_cycles / (int32_t) ctx->cycles_per_us); /* truncates toward zero */

    ctx->stats.jitter_last_us = jitter_us;

    if (!ctx->jitter_valid)
    {
        ctx->stats.jitter_min_us = jitter_us;
        ctx->stats.jitter_max_us = jitter_us;
        ctx->jitter_valid = true;
    }
    else
    {
        if (jitter_us < ctx->stats.jitter_min_us)
        {
            ctx->stats.jitter_min_us = jitter_us;
        }
        if (jitter_us > ctx->stats.jitter_max_us)
        {
            ctx->stats.jitter_max_us = jitter_us;
        }
    }

    ctx->stats.sample_count++;

    int32_t abs_jitter = (jitter_us < 0) ? -jitter_us : jitter_us;
    if ((uint32_t) abs_jitter > ctx->jitter_threshold_us)
    {
        ctx->stats.violation_count++;
    }
}

void FM_IRQ_HEALTH_OnExit(fm_irq_health_t *ctx)
{
    if (!fm_irq_health_ready(ctx) || (!ctx->has_reference))
    {
        return;
    }

    uint32_t now_cycles = FM_BOARD_DWT_GetCycles();
    uint32_t exec_cycles = now_cycles - ctx->entry_cycles;
    uint32_t exec_us = exec_cycles / ctx->cycles_per_us; /* truncates toward zero */

    ctx->stats.exec_last_us = exec_us;

    if (!ctx->exec_valid)
    {
        ctx->stats.exec_min_us = exec_us;
        ctx->stats.exec_max_us = exec_us;
        ctx->exec_valid = true;
    }
    else
    {
        if (exec_us < ctx->stats.exec_min_us)
        {
            ctx->stats.exec_min_us = exec_us;
        }
        if (exec_us > ctx->stats.exec_max_us)
        {
            ctx->stats.exec_max_us = exec_us;
        }
    }
}

void FM_IRQ_HEALTH_GetStats(const fm_irq_health_t *ctx, fm_irq_health_stats_t *out_stats)
{
    if ((ctx == NULL) || (out_stats == NULL))
    {
        return;
    }

    *out_stats = ctx->stats;

    if (!ctx->jitter_valid)
    {
        out_stats->jitter_last_us = 0;
        out_stats->jitter_min_us = 0;
        out_stats->jitter_max_us = 0;
    }

    if (!ctx->exec_valid)
    {
        out_stats->exec_last_us = 0U;
        out_stats->exec_min_us = 0U;
        out_stats->exec_max_us = 0U;
    }
}

void FM_IRQ_HEALTH_ResetStats(fm_irq_health_t *ctx)
{
    if (ctx == NULL)
    {
        return;
    }

    ctx->stats.sample_count = 0U;
    ctx->stats.violation_count = 0U;
    ctx->stats.jitter_last_us = 0;
    ctx->stats.jitter_min_us = 0;
    ctx->stats.jitter_max_us = 0;
    ctx->stats.exec_last_us = 0U;
    ctx->stats.exec_min_us = 0U;
    ctx->stats.exec_max_us = 0U;
    ctx->jitter_valid = false;
    ctx->exec_valid = false;
}

