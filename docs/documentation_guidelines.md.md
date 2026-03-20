# Documentation Guidelines

## General Principles

- Comments must explain intent, not restate code.
- Use engineer-to-engineer tone.
- Keep comments concise but meaningful.
- Avoid decorative or redundant comments.

---

## Layer-Based Documentation Rules

### 1. Board Layer (`fm_board.*`)

#### Header (`fm_board.h`)
- Contains public API contract.
- Use short, clear descriptions.
- No architectural rationale.

#### Source (`fm_board.c`)
- Contains higher-level documentation.
- Explain:
  - API role
  - integration behavior
  - initialization order
  - relationships with `fm_hw_*`

- Group functions by responsibility:
  - initialization
  - debug enable
  - LED control
  - UART access
  - DWT access

---

### 2. Hardware Layer (`fm_hw_*.*`)

#### Headers
- Minimal technical descriptions.
- No board-level explanations.

#### Source files
- Only document:
  - hardware constraints
  - HAL sequences
  - non-obvious implementation details
  - power-saving behavior
  - overflow or precision considerations

---

## Commenting Structures

### Structs

Document:
- purpose of the struct
- meaning of each field
- constraints (e.g. ownership, lifetime)

Example:

/**

@brief Entry stored in the debug ring buffer.

Represents a debug event with timestamp, parameters,

and optional text.

Notes:

p_text must point to persistent memory.

Designed for low-overhead logging.
*/



---

### Global/Static Variables

Document when:
- accessed from multiple contexts (ISR + main)
- represent system state or configuration
- have non-obvious meaning

Include:
- purpose
- origin (e.g. board config)
- concurrency notes if applicable

---

### Functions

#### Public API
- describe what the function provides
- do not describe implementation

#### Internal functions
- document only if behavior is not obvious

---

## Concurrency Notes

- Variables accessed from ISR and foreground must be `volatile`.
- Comments should explain why concurrency exists, not just that it exists.

---

## What to Avoid

- Repeating function names in comments
- Explaining obvious assignments
- Large blocks of text in headers
- Mixing architecture explanation in low-level modules






