#ifndef JITTER_METER_H
#define JITTER_METER_H

#include <stdint.h>
#include <stdbool.h>

typedef struct
{
    uint32_t expected_period_cycles;
    uint32_t prev_cycles;
    uint8_t  has_reference;
} FM_JitterMeter_t;

void FM_JitterMeter_Init(FM_JitterMeter_t *ctx, uint32_t period_us);
bool FM_JitterMeter_Sample(FM_JitterMeter_t *ctx, int32_t *error_us);

#endif /* JITTER_METER_H */
