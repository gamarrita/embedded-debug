# FLOWMEET Firmware C Style Guide (ADAS-Lite)

## 1. Scope

**Language:** C99 (project-wide fixed standard)  
**Domain:** Embedded firmware (safety-oriented, ADAS-ready)  
**Goals:**

- Deterministic behavior  
- High readability (human + AI friendly)  
- Static-analysis friendly  
- MISRA-aligned without bureaucracy  

This guide defines structure, naming, documentation rules, and core safety constraints.

---

## 2. File Structure

All `.c` files shall follow this exact section order:

```text
1. File header (Doxygen)
2. Includes
3. Private Defines
4. Private Types
5. Private Data
6. Private Prototypes
7. Private Bodies
8. Public Bodies
9. Interrupts
```

If a section is unused, include:

```c
/* (none) */
```

---

## 3. Formatting Rules

- Indentation: **4 spaces**, no tabs.
- Brace style: **K&R**.
- Braces required for all `if`, `for`, `while`.
- One declaration per line.
- No trailing whitespace.
- Recommended max line length: 100–120 chars.

Example:

```c
if (condition)
{
    do_action();
}
```

---

## 4. Documentation Rules

### 4.1 File Header (Mandatory)

```c
/**
 * @file    module_name.c
 * @brief   Short module description.
 *
 * @details
 *  - Responsibilities
 *  - Design constraints
 *  - Invariants or safety notes
 */
```

Keep `@details` concise (3–6 bullets max).

### 4.2 Function Documentation

- Must appear immediately before the function.
- Required for all public functions.
- Describe:
  - What it does
  - Assumptions
  - Side effects
  - Units/ranges (if applicable)

Example:

```c
/**
 * @brief Initializes the module.
 *
 * @note Must be called before any other API function.
 */
void FM_MAIN_Init(void);
```

Do not describe obvious operations.

### 4.3 Internal Comments

Allowed only to explain:

- Why something is done  
- Hardware constraints  
- Non-obvious decisions  

Avoid restating the code.

---

## 5. Naming Conventions

### 5.1 Modules

Public API:

- `FM_<MODULE>_<Action>`

Example:

- `FM_MAIN_Init`
- `FM_CAN_SendFrame`

Private symbols:

- `static snake_case`

### 5.2 Types

Use:

- `fm_<name>_t`

Example:

- `fm_status_t`

Enum values:

- `FM_<DOMAIN>_<VALUE>`

### 5.3 Macros

- Uppercase
- Module prefix required
- Use numeric suffixes (`U`, `UL`)

Example:

```c
#define FM_MAIN_LED_BLINK_MS   (250U)
```

---

## 6. Types and Conversions (Core Safety Rules)

### 6.1 Fixed Width Types

- Use `<stdint.h>` types in all interfaces.
- Avoid `int`, `long`, `short` in public APIs.

### 6.2 Conversions

- No implicit narrowing conversions.
- Explicit casts required for sign/width changes.
- Avoid mixing signed and unsigned arithmetic.

### 6.3 Magic Numbers

Prohibited in logic.

Allowed only if:
- Hardware-specific
- Immediately documented

---

## 7. Module Design Rules

### 7.1 Encapsulation

- `static` by default in `.c`.
- Only expose minimal API in `.h`.

### 7.2 Header Files

- Use include guards.
- Public API only.
- No internal state exposure.

### 7.3 C / C++ Compatibility

Create `fm_compiler.h`:

```c
#ifndef FM_COMPILER_H
#define FM_COMPILER_H

#ifdef __cplusplus
  #define FM_EXTERN_C_BEGIN  extern "C" {
  #define FM_EXTERN_C_END    }
#else
  #define FM_EXTERN_C_BEGIN
  #define FM_EXTERN_C_END
#endif

#endif
```

Use in public headers only.

---

## 8. ISR and Concurrency Rules

### 8.1 ISR Restrictions

ISR must:

- Be bounded
- Be non-blocking
- Not call delay functions
- Not perform heavy computation

Allowed operations:
- Set flags
- Push to ring buffer
- Capture minimal state

### 8.2 Volatile Usage

- Only for hardware registers or ISR-shared variables.
- Not a synchronization primitive.

---

## 9. Error Handling

### 9.1 Status Type

```c
typedef enum
{
    FM_OK = 0,
    FM_ERR_INVALID_ARG,
    FM_ERR_TIMEOUT,
    FM_ERR_HW
} fm_status_t;
```

- Functions that may fail return `fm_status_t`.
- Do not mix boolean and error codes.

---

## 10. Prohibited Practices

- `malloc/free` (unless explicitly justified)
- Recursion
- Uninitialized variables
- Implicit type narrowing
- `goto` (except controlled cleanup pattern)
- Blocking delays in production main loop
- ISR calling complex APIs

---

## 11. Pull Request Checklist

Before merging:

1. No compiler warnings.
2. No implicit narrowing conversions.
3. No magic numbers.
4. `static` used correctly.
5. ISR bounded and minimal.
6. Doxygen present for public API.
7. Fixed-width types in interfaces.
8. Macros use suffixes.
9. No heap usage.
10. Logic understandable without excessive comments.

---

## 12. AI Prompt for Code Generation

Use this prompt when generating or refactoring firmware:

### Firmware Code Generation Prompt

Generate embedded C code compliant with the FLOWMEET Firmware C Style Guide (ADAS-Lite).

Requirements:

- Use C99.
- Follow fixed section layout.
- Use Doxygen headers for file and public functions.
- Use module prefix `FM_<MODULE>_`.
- Use `stdint.h` fixed-width types.
- No dynamic memory.
- No recursion.
- No implicit narrowing conversions.
- No magic numbers (use macros with `U` suffix).
- ISR-safe design (bounded, non-blocking).
- `static` by default for internal symbols.
- Braces always used.

Produce production-grade embedded firmware code.  
Keep comments concise and explain design intent, not obvious operations.
