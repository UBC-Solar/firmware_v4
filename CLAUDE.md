# firmware_v4 — UBC Solar `Cascadia`

Firmware for every custom board on UBC Solar's fourth-generation car. All boards are
**STM32F103RCTx** (Cortex-M3, 256 KB flash, 48 KB RAM, 72 MHz SYSCLK), programmed in C11
against STM32 HAL, built with CMake + Ninja + `arm-none-eabi-gcc`.

## Layout

```
firmware/components/<board>/     one folder per physical board
  cube/          STM32CubeMX-generated code — <board>.ioc, Core/, Drivers/, Middlewares/
  driver/        lowest layer: wraps HAL + external chips
  app/           board logic: FSMs, CAN handling, diagnostics
  runtime/       entry point — FreeRTOS task bodies or the bare-metal superloop
firmware/common/                 shared libraries (see below)
tools/                           CAN bus simulator, vendored Ceedling
```

Layering convention is `runtime → app → driver → HAL`. Don't call HAL directly from `app/`.

## Building

Toolchain needed: `cmake`, `ninja-build`, `gcc-arm-none-eabi`, `libnewlib-arm-none-eabi`.
No git submodules — everything (HAL, FreeRTOS) is vendored in-tree.

**From the repo root**, via the top-level Makefile:

```bash
make str                # build one board (Debug)
make str release        # Release
make all                # mdi tel drd hvc mst str  (note: NOT dist)
make clean              # wipe every build/ dir
```

Artifacts land in `firmware/components/<board>/build/<board>.elf`.

**Direct CMake** (required for `dist`, which the Makefile omits, and for clangd):

```bash
cmake --preset Debug -S firmware/components/dist    # configures into build/Debug
cmake --build firmware/components/dist/build/Debug
```

Presets `Debug` and `Release` are defined per board in `firmware/components/<board>/CMakePresets.json`.

### Build gotchas

- **`.clangd` expects `firmware/components/<board>/build/Debug/compile_commands.json`.** That's
  the preset's default `binaryDir`. The Makefile overrides it with `-B .../build` (no `Debug`
  suffix), so a `make`-only build leaves clangd/IDE indexing broken. Run the direct-CMake form
  above at least once per board you're editing.
- **`make <board> Release` silently builds Debug.** The Makefile only matches lowercase
  `debug`/`release` in `MAKECMDGOALS`. CI (`.github/workflows/firmware_all.yml`) passes the
  capitalized `Release`, so CI's "Release" builds are actually Debug.
- **`make utest` is a no-op** — the Ceedling invocations in the Makefile are commented out. CI's
  unit-test job therefore passes vacuously. `tools/ceedling/` holds a vendored Ceedling 1.0.1
  and a `project.yml`, and `drd/test/` + `tel/cube/test/` hold stub test files.
- `dist` is absent from `make all` and from `make clean`.
- `.vscode/settings.json` currently has JSON syntax errors (missing commas), so VS Code may
  ignore it.

### Flashing / debugging

`.vscode/launch.json` has three configs against `STM32F103RCT6`: J-Link + SWO (ITM console on
port 0, 72 MHz CPU / 2 MHz SWO), ST-LINK, and J-Link without SWO. `.vscode/tasks.json` has a
"J-Link Release Flash" shell task driving `JLinkExe -device STM32F103RC -if SWD -speed 4000`.

`DEBUG_IO_PRINT()` from `common/debug_io` prints over SWD/SWO and compiles to nothing unless
`DEBUG` is defined.

## Common libraries (`firmware/common/`)

| Library | What it is |
|---|---|
| `CAN_comms/` | The shared CAN layer. **Requires FreeRTOS (CMSIS_V2)** — spins up Rx/Tx tasks + queues, ~1 KB heap. Fill a `CAN_comms_config_t` (hcan, filter, Rx callback), call `CAN_comms_init()` between `osKernelInitialize()` and `osKernelStart()` (i.e. in `MX_FREERTOS_Init()`). Tx via `CAN_comms_Add_Tx_message()`. Tracks diagnostics counters (dropped/success/HAL-failure Rx & Tx). See its README. |
| `car_configs/` | Single place for build-variant switches: `CAR_CONFIG_CELLULAR` (TEL on huart2 cellular vs huart4 radio), `CAR_CONFIG_CAN_MSG_ALL` (forward all CAN vs competition whitelist), `CAR_CONFIG_SPEED_KPH` (DRD LCD units). Read the derived `car_config_*` bools in code, edit only the macros. |
| `cyclic_data/` | `CYCLIC_DATA(type, name, max_cycle_time)` macro — value + cycle time + `HAL_GetTick()` timestamp, so consumers can detect stale data. |
| `debug_io/` | `DEBUG_IO_PRINT(fmt, ...)` printf over SWO; no-op in non-DEBUG builds. |

Non-RTOS boards (`dist`, `hvc`, `mdi`, `mst`) can't use `CAN_comms` — they each roll their own
`driver/can_driver.c` on top of HAL_CAN.

## Boards

All six on-car boards share the same MCU and run CAN at **500 kbps** (two equivalent timing
setups appear: prescaler 4 / 15+2 TQ, and prescaler 8 / 4+4 TQ).

### `drd` — Driver Display (dashboard) · **FreeRTOS**
ADC1 + ADC2, CAN, IWDG, SPI1, UART4.
- **Peripherals:** ST7565 128×64 LCD over SPI1 (`DISPLAY_/CS`, `DISPLAY_SCL`, `DISPLAY_A0`,
  `DISPLAY_RESET`) with dirty-page tracking and a bundled Verdana font; dual-ADC accelerator
  pedal read (`accel_driver`, valid-range fault detection); external lights outputs
  (`BL_LIGHTS`, `BR_LIGHTS`, `LTS_OUT`, `BRK_OUT`, `RTS_OUT`); driver inputs `HAZARD`,
  `ECO/POWER`, `BRK_IN`, `DRIVE_STATE_NEXT/PREV`, `ESTOP`; `FLT_MCU` fault line; `DEBUG_LED`;
  independent watchdog.
- **Tasks** (`runtime/tasks.c`, created in `cube/Core/Src/freertos.c`, all static buffers):
  `TasksLcdUpdate` (Normal), `TasksDiagnostic` (Normal), `defaultTask` (Normal),
  `TasksDriveState`, `TasksCalculateSoc`, `TasksExtLights`, `TasksFaultLightFlash`,
  `TasksTimeSinceStartup` (all Low). Plus a `calculate_soc_flag` event flag.
- **App:** `drive_state.c` (drive FSM), `soc.c`, `external_lights.c`, `fault_handler.c`,
  `lcd_handler.c`/`lcd_app.c`, `cyclic_data_handler.c`, `diagnostic.c`, `iwdg_app.c`.
- `docs/LCD_README.md` documents the display.

### `tel` — Telemetry · **FreeRTOS**
ADC1+ADC2, CAN, DMA, I2C1+I2C2, RTC, TIM2 (prescaler 71 → 1 µs tick), UART4, UART5, USART2.
- **Peripherals:** radio module on UART4 and cellular module on USART2 (both 230400 baud;
  `car_configs.h` selects which — **radio needs ST-LINK or a bench supply, J-Link can't source
  enough current**); GPS (`G_*` pins: reset, safeboot, wake-on-motion, fix LED, direction); IMU
  over I2C (`I_NRST`, `I_BOOTN`, `I_INTN`); on-chip RTC for message timestamps;
  `MCU_WHEEL_TICK` input; `DEBUG_LED_1`.
- **Tasks:** `TasksDiagnostics` (High), `defaultTask` (Normal), `TasksIMU` and
  `TasksTimeSinceStartup` (Low).
- **App:** `telemetry_app.c` (queues 60 framed messages of `RADIO_Msg_TypeDef`), `can_app.c`,
  `imu_app.c`, `rtc_app.c`, `diagnostics.c`.

### `str` — Steering Wheel · **FreeRTOS**
CAN, I2C1, IWDG, UART5.
- **Peripherals:** hex/7-segment display (`hex_driver`), button and switch inputs — `REGEN`,
  `LTS_IN`, `NEXT_PAGE`, `HORN_MCU`, `CRUISE_INC`/`CRUISE_DEC`/`CRUISE_CONTROL`, `PTT_MCU`
  (push-to-talk); `DEBUG_LED`; independent watchdog (`iwdg_driver` flashes the LED on a
  watchdog-induced reset).
- **Tasks:** `TasksHexDisplay` and `defaultTask` (Normal), `TasksSteeringOutputs`,
  `TasksDiagnostic`, `TasksTimeSinceBootUp` (Low). Static allocation.
- LCD page selection is published on CAN ID `0x580`.

### `mdi` — Motor Drive Interface · **bare-metal superloop**
CAN, I2C1, I2C2, SPI1, UART4. `main()` calls `AppMain()` in `runtime/tasks.c`.
- **Peripherals:** DAC7571 over I2C at address `0x4C<<1` for the motor torque/regen setpoint
  (clamped to 90 % of full scale); MAX31865 RTD front-end over SPI1 (PT1000 curve) for motor
  temperature; `DIR`, `ECO_MCU`, `ROT_IP` control lines; `MC_LED`.
- **Loop:** diagnostics every 1000 ms; **stops the motor if no CAN command arrives within
  100 ms** (`MDI_MAX_TIMEOUT_VALUE`); applies queued motor commands.
- `docs/RTD_README.md` documents the temperature sensing.

### `hvc` — High Voltage Controller · **bare-metal FSM**
ADC1, CAN, DMA, I2C1, TIM3 (100 µs trigger for ADC), TIM4 CH3 (fan PWM), USART2.
`main()` calls `HVC_Main()`.
- **Peripherals:** contactor drives `POS_CTRL`, `NEG_CTRL`, `HLIM_CTRL`, `LLIM_CTRL`,
  `MOTOR_PC_CTRL`, `MPPT_PC_CTRL`, `MPPT_CTRL`, `DIST_CTRL`; precharge sense
  `MOTOR_PRECHARGE`/`MPPT_PRECHARGE`; INA228 current monitor over I2C at `0x40<<1`
  (`runtime/ina228_runtime.c`); ADC for `SUPP_SENSE`, `LV_CURR_SENSE`, `THERMISTOR`; fan
  control (`FAN_CTRL` + `FAN_PWM`); IMD (`IMD_CTRL`, `IMD_GPIO_IN`); `ESTOP`,
  `MASTERBOARD_FAULT`, `HV_CURRENT_ALERT` inputs; status LEDs `ESTOP_LED`, `FAULT_LED`,
  `SUPP_LOW_LED`, `DEBUG_LED`.
- **FSM** (`app/hvc_fsm.c`, states in `hvc_fsm_states.c`): `MVP_LV_POWERUP → MST_READY →
  MST_CHECK → FANS_POWERUP → HV_CONNECT → MOTOR_DISCHARGE → MOTOR_PRECHARGE → MPPT_PRECHARGE →
  CLOSE_LLIM → CLOSE_HLIM → LV_POWERUP → MONITORING`, plus `FAULT` and `UNKNOWN`.

### `mst` — Masterboard (BMS) · **bare-metal superloop**
CAN, CRC, DMA, SPI2, USART1. Configured for **32 modules × 13 cells**.
- **Peripherals:** ADBMS1818 battery monitors over isoSPI on SPI2 (`SPI_ADBMS_NSS`, prescaler
  64; the driver also handles LTC6811); digital I/O `BALANCE_EN_IN`, `SCRUTINEERING_EN_IN`,
  and the disable outputs `LLIM_DIS_OUT`, `HLIM_DIS_OUT`, `CONTACTOR_DIS_OUT`, `FAULT_OUT`;
  `LED_OUT`; UART logging on USART1.
- **Loop:** `CollectBoardData → CollectModuleData → AnalyzeModuleData → DriveOutputs →
  SendCanMessages`.
- **Limits/config in `app/mst_defs.h`:** `MAX_VOLTAGE_mV 4200`, `MAX_TEMP_degC 60`,
  `MAX_BALANCE_VOLT_DIFF_MV 300`, `NUM_CONSECUTIVE_COMM_ERR 3`, `NUM_INIT_MAINLOOPS 4`. It also
  holds `UNIT_TEST_*` switches (`RUN`/`SKIP`) that swap the main loop for the `Debug_*TestCycle`
  routines — check these are all `SKIP` before flashing for real.
- **CAN IDs:** status `0x200`, voltage summary `0x201`, temp summary `0x202`, balance data
  `0x203`, per-module voltage from `0x210`, temp from `0x220`, status from `0x230`.

### `dist` — Power Distribution · **bare-metal FSM**
ADC1, CAN, I2C1. `main()` calls `AppMain()`. Not built by `make all` — configure it directly.
- **Peripherals:** per-rail control and current/fuse monitoring for MDI, DRD, and spare rails
  (`*_CTRL`, `*_CURRENT`, `*_FUSE` pins); `SUPP_ADC` supply monitoring; `ESTOP_GPIO`;
  IS31FL3236 36-channel LED driver over I2C (`runtime/led_runtime.c`); `LED_TOGGLE`, `DEBUG`.
- **FSM** (`app/fsm.c`): `FSM_STATE_STARTUP → FSM_STATE_ACTIVATE_CTRL → FSM_STATE_NORMAL`,
  plus `FSM_STATE_FAULT`. Fault reporting in `runtime/faulting_runtime.c`.

### `aerosensor`
Empty placeholder directory — no firmware yet.

## Conventions

- **Style** (`.clang-format`): LLVM base, 4-space indent, 100-column limit, **Allman braces**,
  left-aligned pointers (`int* p`), no single-line ifs/loops, no arg/param bin-packing.
  `.clang-tidy` is also configured. Format-on-save is off in the repo's VS Code settings.
- **Naming:** functions are PascalCase with a module prefix — `CanAppInit()`,
  `IwdgDriverRefresh()`, `MdiStopMotor()`, `HexDisplayWriteDecimal()`. Headers use
  `__MODULE_NAME_H__` guards and Doxygen `@brief`/`@param` blocks.
- **Adding a source file:** it must be listed explicitly in the board's `CMakeLists.txt`
  `target_sources()` — there's no globbing. Shared files are added by relative path
  (`../../common/CAN_comms/CAN_comms.c`) and the common dirs by `target_include_directories()`.
- **Regenerating from CubeMX** rewrites `cube/`; keep hand-written code inside
  `/* USER CODE BEGIN */ ... /* USER CODE END */` blocks, or better, in `app/`, `driver/`,
  `runtime/`.
- **Branches:** `<name>-<project>-<feature>`, no spaces (e.g. `EvanO12-drd-drive-state-logic`).
  PRs use `.github/pull_request_template.md` and need one reviewer.

## Tools

- `tools/Simulate_CAN_Bus/` — Python CAN bus simulator. `./setup.sh`, then
  `source environment/bin/activate`, then `python main.py`. Messages, burst sizes, intervals,
  and per-board startup delays are defined in `can_messages.yaml`.
- `tools/ceedling/` — vendored Ceedling 1.0.1 for host-side unit tests (not currently wired
  into `make utest`).
