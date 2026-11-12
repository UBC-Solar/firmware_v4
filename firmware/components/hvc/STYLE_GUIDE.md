# HVC Style Guide

The reference for HVC code style is [`driver/adc_driver.c`](driver/adc_driver.c) and
[`driver/adc_driver.h`](driver/adc_driver.h). Where this guide and the repo-root `CLAUDE.md` /
`.clang-format` disagree (Allman braces, left-aligned pointers, `PascalCase` without underscores),
**this guide wins for everything under `firmware/components/hvc/`** (excluding `cube/`, which is
CubeMX-generated).

## Formatting

- **Indent:** 4 spaces, no tabs.
- **Braces:** K&R. The opening brace always goes on the **same line as the function name** (or
  the `if` / `for` / `while` / `switch` / `struct`), never on its own line. `else` goes on the
  same line as the closing brace: `} else {`.

  ```c
  void ADC_Init(ADC_HandleTypeDef *_hadc1, TIM_HandleTypeDef *_htim3) {
      if (hadc1 == NULL) {
          return;
      }
  }
  ```
- **Always use braces**, even for one-line `if` / `for` / `while` bodies.
- **Pointers:** the `*` binds to the name: `ADC_HandleTypeDef *hadc1`, `const char *message`.
  Casts keep the `*` against the type: `(uint32_t*)adc_buffer`.
- **Line length:** aim for 100 columns. Wrap long expressions and long parameter lists rather than
  letting them run on.
- **Parameters:** one line if it fits. If not, wrap with one parameter per line, indented 4 spaces.
- **Switch:** `case` labels are indented one level inside `switch`, and `break;` is on the same
  line as nothing else, at the `case` label's indent. Separate cases with a blank line.
- **Blank lines:** one between functions, and one between logical blocks inside a function.
- **Sections:** no boxed banner comments (`/*=====...=====*/`). Mark sections with one-line
  headers instead, in two levels:

  ```c
  // ==== Hardware tests ====
  // What this section is for, if it needs explaining.

  // --- Pick the test ---
  #define HVC_TEST TEST_NONE
  ```

  `// ==== Name ====` starts a section; `// --- Name ---` starts a subsection inside one. Only
  mark a section when it groups several items in a longer file (`hvc_defs.h`, `fsm.c`). Don't
  mark single items: a struct or function already has its own `@brief` block.
- **Preprocessor directives** (`#if`, `#elif`, `#else`, `#endif`) go at column 0 on their own
  line, even inside a function body. Label every `#endif` (and `#else`) with the condition it
  closes:

  ```c
  void HVC_Main(void) {
  #if (HVC_TEST != TEST_NONE)
      TESTS_RunSelected();
  #else
      FSM_Run();
  #endif // (HVC_TEST != TEST_NONE)
  }
  ```

## Naming

| Thing | Convention | Example |
|---|---|---|
| Public functions | `FILENAME_NameOfTheFunction` (see below) | `ADC_Init`, `GPIO_Read`, `HVC_Main` |
| Private functions | `static`, PascalCase, no prefix needed, declared at the top of the `.c` | `MstHandshake`, `SendHeartbeat` |
| Types | `Module_PascalCase` for structs the module owns, or `snake_case_t` for plain data records | `ADC_Readings`, `thermistor_mapping_t` |
| File-scope variables | `snake_case`, always `static` unless shared on purpose | `adc_buffer`, `adc1_readings` |
| Local variables, struct fields | `snake_case` | `half`, `motor_precharge` |
| Macros / constants | `UPPER_SNAKE_CASE` with the module as prefix | `ADC1_NUM_CHANNELS`, `ADC_RESOLUTION` |
| Unused-name clash with a global | leading underscore on the parameter | `_hadc1`, `_htim3` |

### Public function names: `FILENAME_NameOfTheFunction`

**Every** public function (anything declared in a header) is named after the file it lives in.
The prefix is the file's name in caps, then an underscore, then the function name in PascalCase.

| File | Prefix | Examples |
|---|---|---|
| `driver/adc_driver.c` | `ADC_` | `ADC_Init()`, `ADC_GetVoltages()` |
| `driver/gpio_driver.c` | `GPIO_` | `GPIO_Read()`, `GPIO_Write()` |
| `app/hvc_main.c` | `HVC_` | `HVC_Init()`, `HVC_Main()` |
| `runtime/adc_runtime.c` | `ADC_Runtime_` | `ADC_Runtime_ProcessReadings()`, `ADC_Runtime_GetValues()` |
| `runtime/gpio_runtime.c` | `GPIO_Runtime_` | `GPIO_Runtime_Read()` |
| `runtime/shunt_runtime.c` | `SHUNT_` | `SHUNT_Init()`, `SHUNT_GetShuntCurrent_mA()` |
| `app/fsm.c` | `FSM_` | `FSM_Init()`, `FSM_TimerElapsed()` |
| `app/tests.c` | `TESTS_` | `TESTS_RunSelected()` |
| `app/can_messages.c` | `CAN_Messages_` | `CAN_Messages_SendAll()`, `CAN_Messages_HandleRx()` |
| `app/debug_output.c` | `DEBUG_Output_` | `DEBUG_Output_Init()` |

A runtime file that sits on top of a driver of the same name adds `_Runtime` to the driver's
prefix (`ADC_` → `ADC_Runtime_`), so the two never clash. A two-word filename puts the first word
in caps and the rest in PascalCase (`can_messages.c` → `CAN_Messages_`).

**Only functions called from another file are public.** Everything else is `static`, declared at
the top of its `.c` (below the includes), and left out of the header. Private functions don't need
the prefix. If a static function only compiles under a switch (`#if HVC_TEST == ...`, `FAKE_*`),
its prototype goes under the same `#if`, so no configuration warns about unused functions.

This means you can always tell from a call which file to open. No public function goes without a
prefix (not `InitTemperatureReading()`, not `FsmRun()`).

HAL callbacks (`HAL_ADC_ConvCpltCallback()`, `HAL_GPIO_EXTI_Callback()`) keep the names HAL
requires.

Units go in names or the comment when they are not obvious: `voltage_uV`, `temperature_mC`,
`ADC_VOLTAGE_REFERENCE // mV`.

## Files

- **Headers** start with `#pragma once` (not `#ifndef` guards).
- **Source files** include their own header first, then everything else.
- The header holds: includes needed by the public types, `#define`s, typedefs, then prototypes.
  Private helpers are `static` and **declared at the top of the `.c`**, below the includes.
- Prototypes with no parameters use `(void)`.
- Add new `.c` files to `CMakeLists.txt` explicitly (no globbing).

## Comments

- **Every public function gets a Doxygen block** in the `.c` file, directly above its definition:

  ```c
  /**
   * @brief Read averaged ADC values for a given half of the buffer, then store them.
   * @param half 0 for the first half, 1 for the second half.
   * @return None
   */
  ```

  `@brief` is one sentence. Add one `@param` per parameter and an `@return` for non-void functions.
- **Public types and structs** get a `@brief` block too. Say what the values mean and their units.
- **Inline comments** use `//`, either on the line above the code or trailing on short lines.
  Describe the *why* or the step (`// Sum readings`), not the syntax.
- Multi-step functions are broken into commented steps (`// 1. Clamp ...`, `// 2. Iterate ...`).
- **Do not leave commented-out code** in the tree.
- Mark vendored or copied code with its source (`// COPIED FROM MST`).

## Layering

`runtime → app → driver → HAL`, as in the repo `CLAUDE.md`. Don't call HAL from `app/` or
`runtime/`; go through a driver.

Entry points called from `cube/Core/Src/main.c` are `HVC_Init()` and `HVC_Main()` in
`app/hvc_main.c`.
