# Native firmware component tests (SIL)

DRD, MDI, TEL, and STR have Ceedling projects that compile real production C on
the development computer. No STM32, probe, ARM toolchain, or running RTOS is
needed. HVC and MST do not yet have native suites.

## Install and run

Install Ruby 3.2 or newer, RubyGems, GNU Make, and a native C compiler available as
`gcc` (Apple clang's `gcc` alias works). CI uses Ruby 3.2 and Ubuntu's
`build-essential`. On Ubuntu, install `ruby ruby-dev build-essential`; on macOS,
install Xcode Command Line Tools (`xcode-select --install`) and a current Ruby,
for example `brew install ruby`, with its `bin` directory on `PATH`.

From the repository root:

```sh
bundle install                    # Installs the versions in Gemfile.lock
make utest                        # DRD, MDI, TEL, STR
make utest UTEST_BOARDS=mdi        # One board
make utest UTEST_BOARDS="tel str"  # Selected boards
```

`Gemfile` pins Ceedling 1.1.8; `Gemfile.lock` locks its dependencies for local
use and CI. Use Bundler 4 (Ruby 3.2 or newer). If needed, install the lockfile's
Bundler version with `gem install bundler -v 4.0.20 --no-document`.
The legacy vendored Ceedling under `tools/ceedling/` is not used.

Ceedling runs directly from the board directory, with Bundler finding the root
Gemfile automatically. There are no per-board launcher scripts or custom Ruby
runner:

```sh
cd firmware/components/tel
bundle exec ceedling test:imu_app
bundle exec ceedling test:rtc_driver
bundle exec ceedling test:all
```

In the other board directories, use `test:drive_state` (DRD), `test:mdi_driver`
(MDI), or `test:hex_app` (STR). Ceedling owns test discovery, compilation, test
runners, assertion failures, and reports. See its
[command reference](https://docs.throwtheswitch.org/latest/getting-started/command-line/).

`make utest` loops over the selected boards and runs
`bundle exec ceedling clobber test:all`. After each successful run, one report
check requires a positive passed-test count. This catches empty or all-ignored
suites, which must not make CI green. Compile errors and failed assertions use
Ceedling's own nonzero exit status. Missing reports and invalid selections also
fail. No custom version selection, test runner, or duplicate result aggregation
is needed.

SIL artifacts live in `firmware/components/<board>/build_sil/`, separate from
STM32 output in `build/`. Ceedling's `clobber` removes the SIL artifacts, including
old reports, before each full Make run. Reports are
`build_sil/artifacts/test/tests_report.json` under each board.

Do not use `make utest mdi`: `mdi` is still the ARM firmware build target.
The old `./ceedling` and `cube/ceedling` launchers have been removed. TEL's
hardware scripts remain in `test/test_scripts/`; run their setup there.

On macOS, `make utest` selects the SDK with
`xcrun --sdk macosx --show-sdk-path` if `SDKROOT` is unset/empty. This prevents
mixing an older Xcode linker with a newer Command Line Tools SDK. Explicit
`SDKROOT` and `DEVELOPER_DIR` settings are respected. The setting is scoped to
the test command and does not affect normal ARM builds. For direct Ceedling
commands on a Mac with that SDK mismatch, set this once in your shell:

```sh
export SDKROOT="$(xcrun --sdk macosx --show-sdk-path)"
```

GitHub Actions installs the locked gems through `ruby/setup-ruby` with
`bundler-cache: true`, installs the native compiler, then runs the same
`make utest`. Push/PR filters include `firmware/**`, `Makefile`, `Gemfile`,
`Gemfile.lock`, and the workflow. Manual dispatch is also available. Committing
locally alone does not trigger GitHub Actions; push the changes first.
The separate ARM jobs still cover all six boards in `debug` and `release`.
There is no automated hardware flashing/deployment in this workflow.

To add a SIL board, create its Ceedling `project.yml` and tests, then add its
name to `UTEST_BOARDS` in the Makefile.

## Test design and sources

The suites link real production modules together and replace hardware and
selected external services with reusable, stateful C test doubles. CMock generation is
disabled; there are no `mock_*.h` includes or generated mock files to manage.
Ceedling still discovers tests, compiles sources, runs Unity, and reports results.
These are small host component tests, not a simulation of the entire board.

This choice draws on published engineering guidance, not a universal industry
rule:

- [Google: Don't Overuse Mocks](https://testing.googleblog.com/2013/05/testing-on-toilet-dont-overuse-mocks.html)
  recommends real implementations where practical and test doubles for external services.
- [Memfault: Embedded Unit Testing Basics](https://interrupt.memfault.com/blog/unit-testing-basics)
  describes reusable test doubles/stubs and keeping hardware validation separate.
- [Memfault: Unit Testing with Mocks](https://interrupt.memfault.com/blog/unit-test-mocking)
  explains both isolated module tests and combining real modules into integration
  tests. Mocks remain useful for precise interaction and failure-path tests.
- [Zephyr Ztest](https://docs.zephyrproject.org/latest/develop/test/ztest.html)
  also supports test drivers; adopting Zephyr itself is unnecessary here.

## Folder layout

```text
firmware/
  test/support/                    # Shared native HAL declarations and implementation
    stm32f1xx_hal.h
    test_hal.h
    test_hal.c
  components/<board>/
    test/
      test_ceedling_<module>.c      # Ceedling test cases
      support/                     # Board-specific helpers (DRD and TEL currently)
        test_<board>_services.h
        test_<board>_services.c
      test_scripts/                # Hardware scripts (DRD and TEL currently)
    project.yml
```

Shared HAL support is kept outside
`firmware/common/` so production CMake source globs cannot pick it up.
Each board's project searches `test/` for `test_ceedling_*.c`; support files
are on native source/include paths, and hardware scripts are not SIL cases.
The `test_` support names do not match the `test_ceedling_` discovery prefix.

## Real code and test boundaries

| Board | Real production modules linked | Replacements |
| --- | --- | --- |
| DRD | `drive_state.c`, `accel_driver.c`, `gpio_driver.c`, `cyclic_data_handler.c` | HAL GPIO/ADC/tick; CAN send service, LCD speed-unit setting, diagnostic reporting |
| MDI | `mdi_driver.c` | HAL I2C/GPIO |
| TEL RTC | `rtc_driver.c` | HAL RTC/tick |
| TEL IMU | `imu_app.c`, `imu_driver.c`, `can_driver.c` | HAL I2C/GPIO/time; CAN enqueue, telemetry forwarding, and `osDelay` |
| STR | `hex_app.c`, `hex_driver.c`, `cyclic_data_handler.c` | HAL I2C/tick |

`firmware/test/support/test_hal.c` and `.h` provide the shared host environment:
GPIO state, an ADC sample, a manually controlled tick, an I2C write log, and RTC
fields, plus queued I2C responses. Reset it with `TestHalReset()` before every test. Tests set inputs and
inspect recorded output after running production functions. I2C payloads are
copied, not retained as pointers to expired stack buffers.

The test HAL supports GPIO ports A/B/C, ADC1, I2C1/2, and one RTC. Unknown ports or
handles, unsupported RTC format, and I2C log/payload overflow fail the test.
Unimplemented HAL functions fail to link. ADC and RTC operations succeed;
`test_hal.i2c_status` controls I2C transmit status. I2C transfers support up to
128 bytes, with 16 entries per read/write log. Queue read responses with
`TestHalQueueI2cRead(bus, address, bytes, size, status)` and call
`TestHalVerifyI2cReads()` in `tearDown`. Reads check the bus, address, requested
size, and timeout (`HAL_MAX_DELAY` by default). Missing/unused responses fail,
and failed responses return their status without copying data to the caller.
`HAL_Delay` advances test time without sleeping. Set `test_hal.tick_step` to a
nonzero value when exercising a polling timeout; otherwise time changes only
when explicitly advanced. RTC fields do not advance with the tick.
This is a limited test double, not peripheral emulation.

DRD's `test/support/test_drd_services.c` and `.h` capture motor commands and
diagnostic values and supply LCD units. They implement only the interfaces the
drive component needs and include the production declarations to check their
signatures. CAN packing/queues, LCD rendering, and diagnostic transmission are
outside this component test. They need their own tests; grouping their test doubles
into one support file does not count as testing their real implementations.

TEL's `test/support/test_tel_services.c` captures CAN and telemetry messages by
value in separate bounded logs. It implements `osDelay` using the real vendored
CMSIS declaration, records delays, and advances the test clock at TEL's configured
1000 Hz tick rate without scheduling threads. It does not implement RTOS queues,
UART/radio transport, or CAN bus behavior. The real `can_driver.c` provides the
message headers, and the real IMU driver packs their payloads.

`firmware/test/support/stm32f1xx_hal.h` declares the HAL subset. Signatures match the
vendored HAL; handles and ports are identity tokens. It rejects builds without
`TEST` and is only on native include paths. DRD/MDI/TEL use the actual Cube
headers and GPIO pin definitions. `CAN_comms.h` guards its RTOS includes with
`#ifndef TEST`. Firmware builds retain those includes. TEL includes the portable
CMSIS-RTOS2 API header for declarations but does not link the RTOS implementation.

## Add a test

1. Create `firmware/components/<board>/test/test_ceedling_<module>.c`.
2. Include `unity.h`, the production module headers to link, and `test_hal.h`.
   DRD controller tests also include `test_drd_services.h`; TEL IMU tests include
   `test_tel_services.h`. Reset these with `TestDrdServicesReset()` or
   `TestTelServicesReset()` as appropriate. Ceedling selects the
   matching `.c` files; it does not compile all firmware or traverse every header
   dependency into another source automatically.
3. In `setUp`, reset test doubles and arrange the real module state through existing APIs.
   DRD currently resets its existing global controller context without changing
   its production interface. Initialize the cyclic values used by the test.
4. Set input values, call real production functions, and assert their results
   and captured outputs. Avoid copying production algorithms into the test doubles.
5. Run the module command above, then `make utest`. To include another portable
   dependency, include its header in the test or use Ceedling's
   `TEST_SOURCE_FILE("module.c")` directive. If it needs hardware, extend the
   test support deliberately, matching the vendored HAL declarations.

All four suites use the same pattern. Keep test support in native source/include
paths; never add it to STM32 CMake. More focused CMock suites can be added later,
but must not link a test implementation and a generated mock defining the same functions.

## Starter tests

There is one scenario per component: DRD, MDI, STR, TEL RTC, and TEL IMU.
Deeper coverage is deferred.

| Board | File in `test/` | Check |
| --- | --- | --- |
| DRD | `test_ceedling_drive_state.c` | Brake plus a forward request changes PARK to FORWARD, publishes the state, and sends zero throttle/regen with the brake light on. |
| MDI | `test_ceedling_mdi_driver.c` | Decode a command and check actual DAC bytes, I2C buses, and direction/eco outputs. |
| TEL | `test_ceedling_rtc_driver.c` | Setting time updates the test RTC with the correct hours, minutes, seconds, and binary format. |
| TEL | `test_ceedling_imu_app.c` | Initialize the sensor, decode known acceleration/gyro/magnetometer reports, and check the six CAN and telemetry payloads after one task iteration. |
| STR | `test_ceedling_hex_app.c` | Stored speed 42 passes through the real display driver as two I2C register writes. |

`make utest` should report **5 passed tests across 4 boards**. These tests do not
validate electrical behavior, RTOS scheduling, CAN transport, interrupt timing,
or complete firmware behavior. HVC and MST do not yet have native suites.

DRD's `drive_state.c` and `drive_state.h` remain unchanged. No production source
changes are needed for the switch from generated mocks to these component tests.

Local validation on macOS: all five component tests passed after a clean `make utest`.
Temporary IMU support checks verified I2C read failure, no data ready, boot
timeout, and receive-buffer preservation on failure. An incorrect expected IMU
payload failed as intended. Earlier probes also checked an incorrect assertion
on each board. All temporary probes were removed/restored, and the full suite
passed again. Build logs confirm the real modules listed above were compiled.
The simplified `make utest` also passed all five tests in a disposable Linux
container using Ruby 3.2, GCC, and a frozen install of `Gemfile.lock`. Local
failure checks confirmed assertion errors, compile errors, zero-case suites,
ignored-only suites, missing test files, and empty/unknown board selections
return nonzero. Temporary probes were restored and the final suite passed on
both macOS and Linux. The workflow YAML was checked; hosted GitHub Actions and
ARM builds were not rerun for this test-tooling change.

The IMU extension preserves the existing CAN headers: standard IDs `0x800` through
`0x805`. These exceed the 11-bit standard-ID range (`0x7FF` maximum). The host test
checks existing packing/forwarding behavior; this pre-existing CAN configuration
issue needs a separate firmware fix before claiming valid physical transmission.
GPS checksum tests are not included because no GPS checksum implementation was
found in TEL's application/driver sources.

## Target builds and hardware validation

To check target integration when the ARM toolchain is installed:

```sh
make all debug
make all release
```

Use lowercase `debug` and `release`, as CI does. The existing Makefile's
uppercase `Release` selector silently chooses Debug. CI is configured for all
six boards in both modes plus the four-board SIL job. Hosted execution and
hardware flashing are separate from local SIL validation.

The pre-existing DRD Release getter declaration issue is deferred: the LCD
calls `DriveStateGetDriveMode`, but its header declares it only under `DEBUG`.
No workaround is added to `lcd_handler.c`.
