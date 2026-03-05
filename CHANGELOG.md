# Changelog

All notable changes to this project will be documented in this file.

## [Unreleased]

## [0.1.0] - 2026-03-05
### Added
- UART debug output.
- LED debug indicators.
- Hardware enable control (jumper/pin).
- Designed for minimal runtime overhead.
- Example: Timer ISR jitter measurement for Nucleo-575ZI-Q (STM32U5).

### Changed
- Recreated STM32 example set; legacy BlinkLEDStopMode and TimerJitter projects were removed in favor of TimerISRJitter.
