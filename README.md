Embedded Low-Overhead Debug Library

A lightweight debug infrastructure designed for microcontrollers where
runtime overhead, power consumption, and flash footprint must be minimal.

Features
- Compile-time removable debug
- Hardware enable (jumper / pin)
- UART logging
- LED diagnostics
- Minimal RAM footprint
- STM32 reference implementation

Example 03 — Timer ISR Jitter
-----------------------------
This example shows how to use the debug library to detect and count jitter on an STM32 timer:
- TIM6 runs at 1 kHz as the deterministic reference. Its ISR uses DWT cycle counter to compare each interval against the ideal 1 ms and raises `FM_DEBUG_ERR_JITTER` when |error| > 1 µs.
- TIM7 runs at ~100 Hz and injects a controlled CPU load (short NOP loop) to create measurable latency. Adjust its ARR/PSC or loop length to change how often it interferes.
- The fm_debug error counters are read once per second and printed over UART (guarded by the jumper). LED_ERROR is turned on when a jitter event is detected.
- All jumpers are sampled with minimal static power: inputs are pulled up only while reading and then returned to analog/no-pull.

To tune the test:
1) Build and flash `examples/Nucleo-575ZI-Q/03_TimerISRJitter`.
2) Modify TIM7 load or frequency to reach the desired jitter hit rate (e.g., ~10 events/s with the current 90‑NOP load).
3) Observe counts via UART or query `FM_DEBUG_ErrorCount(FM_DEBUG_ERR_JITTER)` in the debugger. The hit rate should match approximately `hits ≈ fTIM6 * (dur_cycles / t_interval_cycles)`.
