# Changelog

All notable changes to this project will be documented in this file.

## [0.2.0] - 2026-03-07

### Added
- README section describing Example 03 (Timer ISR Jitter) and how fm_debug is used to measure jitter with TIM6/TIM7.
- `examples/README.md` documenting the IDE layout (`examples/stm32cubeide/` as current root).

### Changed
- Comments and naming in `fm_main.c` (Example 03) and `fm_debug` headers/sources clarified and standardized to English per style guide.
- TIM7 jitter workload tuned (90 NOP load, priority 6) and documentation updated accordingly.

## [0.1.0] - 2026-03-05
### Added
- UART debug output.
- LED debug indicators.
- Hardware enable control (jumper/pin).
- Designed for minimal runtime overhead.
- Example: Timer ISR jitter measurement for Nucleo-575ZI-Q (STM32U5).

### Changed
- Recreated STM32 example set; legacy BlinkLEDStopMode and TimerJitter projects were removed in favor of TimerISRJitter.
