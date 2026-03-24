# 03_TimerISRJitter

Example for measuring timer interrupt jitter with deferred logging.

Instead of printing from the ISR, this example:

- measures the period in CPU cycles with DWT
- computes jitter against an expected value
- enqueues only the samples whose jitter exceeds a threshold
- flushes the queued events later from foreground code

This keeps the interrupt path short and makes the timing disturbance caused by logging much smaller than a blocking `printf()` inside the ISR.

## What it logs

In [`app/fm_main.c`](app/fm_main.c):

- `EXPECTED_CYCLES = 240000`
- `JITTER_THRESHOLD_CYCLES = 1`
- event code `FM_MAIN_EVT_RTC_JITTER = FM_DEBUG_EVT_USER + 1`, which prints as `EVT=257`

Each flushed line has this format:

```text
[timestamp] EVT=257 P0=period_cycles P1=jitter_cycles
```

Meaning:

- `timestamp`: DWT timestamp captured when the event was enqueued
- `P0`: measured interrupt period in cycles
- `P1`: signed jitter in cycles relative to `EXPECTED_CYCLES`

Example:

```text
[47690278] EVT=257 P0=241999 P1=1999
```

This means the measured period was `241999` cycles, or `1999` cycles above the expected `240000`.

## Why negative and positive pairs appear

The output often alternates between positive and negative jitter:

```text
[47690278] EVT=257 P0=241999 P1=1999
[47928283] EVT=257 P0=238001 P1=-1999
```

That pattern is expected when one interrupt arrives late and the next interval compensates for it.

## Dropped events

After each `FM_DEBUG_Flush()`, the example also prints this line when the ring buffer overflowed:

```text
Dropped events since last flush: 12
```

This does not mean UART failed. It means more jitter events were produced than the deferred debug ring could hold before the next flush.

In this example, that can happen because:

- the jitter threshold is very low (`1` cycle)
- `TIM7` can generate many loggable events
- foreground code flushes in bursts instead of continuously
- `TIM6_IRQHandler()` deliberately adds extra load

## What this example demonstrates

- ISR-safe event capture with `FM_DEBUG_Log2ISR()`
- foreground-only formatting and UART transmit with `FM_DEBUG_Flush()`
- jitter measurement using DWT cycle timestamps
- visibility into overflow pressure through `FM_DEBUG_DroppedCount()`

## Where to look in the code

- [`app/fm_main.c`](app/fm_main.c): example logic, jitter calculation, thresholding, drop summary
- [`common/src/fm_debug.c`](../../../common/src/fm_debug.c): ring buffer, deferred flush, drop counter
- [`common/include/fm_debug.h`](../../../common/include/fm_debug.h): public debug API
