# Embedded Low-Overhead Debug Library

Small debug infrastructure for microcontrollers that lets you capture events from ISRs and other time-critical paths without doing `printf()` or UART I/O inside the interrupt.

The core idea is simple:

- In the ISR, enqueue a compact event into a fixed ring buffer.
- Later, in foreground code, call `FM_DEBUG_Flush()` to format and transmit those events over UART.

That makes the hot path short, bounded, and much safer than blocking logging in interrupt context.

## Why this is useful in embedded

On embedded targets, debug code often becomes part of the problem:

- `printf()` is slow and usually blocking.
- UART transmission inside an ISR increases interrupt latency and jitter.
- Formatting strings in time-critical code burns cycles and stack.
- When something only fails under timing pressure, the act of logging can hide the bug.

This library takes a different approach:

- ISR-safe logging is deferred.
- The producer path is constant-time and uses a fixed-size ring buffer.
- Formatting and UART transmission happen later in foreground context.
- If the ring is full, the event is dropped and counted instead of stalling the system.
- LED signaling and message output can be gated by board-level debug enables.
- DWT cycle timestamps are used when the target supports them.

## What the library currently provides

From the public API in [`common/include/fm_debug.h`](common/include/fm_debug.h):

- Deferred event logging from ISR/time-critical code with `FM_DEBUG_LogISR()`, `FM_DEBUG_Log2ISR()`, and `FM_DEBUG_LogConstISR()`
- Foreground flush with `FM_DEBUG_Flush()`
- Drop, queue, and high-watermark counters with `FM_DEBUG_DroppedCount()`, `FM_DEBUG_QueuedCount()`, and `FM_DEBUG_HighWatermark()`
- Error counters, last-error tracking, and optional LED indication
- Blocking UART helpers for simple foreground diagnostics: `FM_DEBUG_UartMsg()`, `FM_DEBUG_UartUint32()`, `FM_DEBUG_UartInt32()`, `FM_DEBUG_UartFloat()`

From the current implementation in [`common/src/fm_debug.c`](common/src/fm_debug.c):

- Single-producer / single-consumer event ring
- Fixed-size buffers
- Overflow policy: drop-on-full with a counter
- Foreground-only formatting and UART transmit

The current reference target is STM32 Nucleo-U575ZI-Q through the board facade in [`common/include/fm_board.h`](common/include/fm_board.h) and [`board/nucleo-u575iz-q`](board/nucleo-u575iz-q).

## How it differs from blocking logging

### Traditional approach

```c
void TIM_IRQHandler(void)
{
    printf("irq=%lu\n", sample);
}
```

Problems:

- formatting happens inside the ISR
- UART transmission may block inside the ISR
- execution time grows with message size and backend behavior
- timing-sensitive bugs can become harder to reproduce

### This library's approach

```c
void TIM_IRQHandler(void)
{
    (void)FM_DEBUG_LogISR(APP_EVT_SAMPLE, sample);
}

void main_loop(void)
{
    FM_DEBUG_Flush();
}
```

What changes:

- the ISR only stores compact data in the ring buffer
- the expensive work is deferred to foreground code
- overflow is visible through counters instead of silently stretching interrupt time

## Minimal example

This is the smallest real usage pattern based on the current API:

```c
#include <stdbool.h>
#include "fm_board.h"
#include "fm_debug.h"

#define APP_EVT_SAMPLE   (FM_DEBUG_EVT_USER + 1U)

static volatile bool flush_pending = false;

void App_Init(void)
{
    FM_BOARD_Init();
    FM_DEBUG_Init();
}

void Some_IRQHandler(void)
{
    int32_t sample = 42;

    (void)FM_DEBUG_LogISR(APP_EVT_SAMPLE, sample);
    flush_pending = true;
}

int main(void)
{
    App_Init();

    for (;;)
    {
        if (flush_pending)
        {
            flush_pending = false;
            FM_DEBUG_Flush();
        }
    }
}
```

If you only need a constant text marker from an ISR, the API also supports:

```c
(void)FM_DEBUG_LogConstISR("Wakeup event");
```

Important: `FM_DEBUG_Flush()` is foreground-only. It formats strings and sends them over UART, so it is intentionally not ISR-safe.

## Current repository structure

```text
common/
  include/   public debug and board-facing headers
  src/       debug core and board facade

board/
  nucleo-u575iz-q/  current low-level GPIO/UART/DWT backend

examples/
  Nucleo-575ZI-Q/
    01_BlinkLEDs
    02_BlinkLEDStop2Mode
    03_TimerISRJitter
```

## Start with these examples

If you want to explore the repo quickly:

- [`examples/Nucleo-575ZI-Q/01_BlinkLEDs`](examples/Nucleo-575ZI-Q/01_BlinkLEDs): simplest starting point, combines LED control, blocking UART helpers, and a deferred ISR text event
- [`examples/Nucleo-575ZI-Q/02_BlinkLEDStop2Mode`](examples/Nucleo-575ZI-Q/02_BlinkLEDStop2Mode): shows deferred logging around STOP2 wake/sleep flow
- [`examples/Nucleo-575ZI-Q/03_TimerISRJitter`](examples/Nucleo-575ZI-Q/03_TimerISRJitter): best example of the library's main value, measuring timer jitter with DWT timestamps, logging ISR events through the ring buffer, and flushing later in foreground code

If you only read one example first, start with `03_TimerISRJitter`.

## Scope and portability

This repository is already structured with a portability boundary, but today it is not a generic, board-agnostic package out of the box.

What is portable:

- the debug API in `common/`
- the deferred logging model
- the error and event tracking logic

What is board-specific today:

- UART backend
- LED control
- debug-enable inputs
- DWT setup/access

To adapt this to another target, the main porting surface is the board layer:

- [`common/include/fm_board.h`](common/include/fm_board.h)
- [`common/src/fm_board.c`](common/src/fm_board.c)
- [`board/nucleo-u575iz-q`](board/nucleo-u575iz-q) as the reference implementation

## A few practical notes

- `FM_DEBUG_LogConstISR()` stores a pointer to constant text; pass string literals or other long-lived storage, not stack buffers.
- The current implementation uses a fixed event ring and reports dropped events with `FM_DEBUG_DroppedCount()`.
- Direct UART helpers are still useful, but they are foreground tools, not a substitute for deferred ISR logging.

## Why this repo is worth exploring

This is not just a UART wrapper. It is a concrete embedded debugging pattern:

- capture timing-sensitive events without blocking in the ISR
- preserve ordering through a compact event ring
- inspect overflow pressure with counters
- keep the expensive debug path where it belongs: outside the interrupt

If you work on low-power, timing-sensitive, or interrupt-heavy firmware, the examples in this repository show a practical way to keep observability without paying the full runtime cost of traditional logging.
