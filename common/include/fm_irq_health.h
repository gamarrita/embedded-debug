#ifndef FM_IRQ_HEALTH_H
#define FM_IRQ_HEALTH_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Snapshot of IRQ timing health.
 */
typedef struct
{
    uint32_t sample_count;      /**< Number of measured intervals (excludes first priming entry). */
    uint32_t violation_count;   /**< Count of intervals whose |jitter| exceeded the configured threshold. */
    int32_t  jitter_last_us;    /**< Last arrival jitter in microseconds (truncated toward zero). */
    int32_t  jitter_min_us;     /**< Minimum arrival jitter seen (signed, truncated toward zero). */
    int32_t  jitter_max_us;     /**< Maximum arrival jitter seen (signed, truncated toward zero). */
    uint32_t exec_last_us;      /**< Last ISR execution time in microseconds (truncated toward zero). */
    uint32_t exec_min_us;       /**< Minimum ISR execution time seen (truncated toward zero). */
    uint32_t exec_max_us;       /**< Maximum ISR execution time seen (truncated toward zero). */
} fm_irq_health_stats_t;

/**
 * @brief IRQ health context (ISR-safe, no dynamic memory).
 */
typedef struct
{
    fm_irq_health_stats_t stats;
    uint32_t ideal_period_us;
    uint32_t ideal_period_cycles;
    uint32_t cycles_per_us;
    uint32_t jitter_threshold_us;
    uint32_t entry_cycles;
    uint32_t prev_entry_cycles;
    bool     has_reference;
    bool     jitter_valid;
    bool     exec_valid;
} fm_irq_health_t;

/* ===== Public API ===== */

/**
 * @brief Initializes the IRQ health context.
 *
 * @param ctx                 Pointer to context storage (must be valid and zero-initialized or static).
 * @param ideal_period_us     Expected period between ISR entries (microseconds).
 * @param jitter_threshold_us Absolute jitter threshold (microseconds) used for violation counting.
 *
 * @note Uses integer division when converting cycles to microseconds (truncates toward zero).
 * @note DWT must be enabled beforehand; if cycles-per-us is zero, measurements are skipped until available.
 */
void FM_IRQ_HEALTH_Init(fm_irq_health_t *ctx,
                        uint32_t ideal_period_us,
                        uint32_t jitter_threshold_us);

/**
 * @brief Marks ISR entry and updates arrival jitter statistics.
 *
 * Call at the very start of the ISR. First invocation primes the reference and does not count a sample.
 */
void FM_IRQ_HEALTH_OnEntry(fm_irq_health_t *ctx);

/**
 * @brief Marks ISR exit and updates execution-time statistics.
 *
 * Call at the very end of the ISR. Does nothing until a valid entry timestamp exists.
 */
void FM_IRQ_HEALTH_OnExit(fm_irq_health_t *ctx);

/**
 * @brief Copies the current statistics snapshot.
 *
 * @param ctx       Pointer to context.
 * @param out_stats Destination pointer; fields are zeroed when no samples were recorded.
 */
void FM_IRQ_HEALTH_GetStats(const fm_irq_health_t *ctx, fm_irq_health_stats_t *out_stats);

/**
 * @brief Clears accumulated statistics without re-priming the timing reference.
 */
void FM_IRQ_HEALTH_ResetStats(fm_irq_health_t *ctx);

#endif /* FM_IRQ_HEALTH_H */
