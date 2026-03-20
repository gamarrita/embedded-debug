# Architecture Overview

## Layering

The project is structured in two main layers:

### 1. Board Layer (`fm_board.*`)
- Public API used by application code.
- Acts as a facade over low-level hardware modules.
- Owns initialization order and integration logic.
- Defines the debug-related services exposed by the board.

### 2. Hardware Layer (`fm_hw_*.*`)
- Low-level hardware backends (GPIO, UART, DWT, CLK).
- Encapsulate direct interaction with HAL/peripherals.
- Do not expose board-level behavior or policies.

## Design Rules

- Application code must depend only on `fm_board.*`.
- `fm_board.*` may depend on any `fm_hw_*.*`.
- `fm_hw_*.*` must not depend on `fm_board.*`.

## Initialization

- `FM_BOARD_Init()` defines the initialization sequence.
- Hardware modules are initialized in a controlled order from the board layer.

## Debug System

- Debug features (UART, LEDs, timestamps) are exposed via `fm_board.*`.
- Low-level implementation is delegated to `fm_hw_*` modules.