# Firmware unit tests

Ceedling compiles production C on your computer using small hardware and service
stubs. DRD, MDI, TEL, and STR have tests; HVC and MST do not yet.

## Run

Install Ruby 3.2 or newer, Bundler, Make, and a native C compiler available as
`gcc`. On macOS, install Xcode Command Line Tools. CI uses Ruby 3.2 on Ubuntu.

```sh
bundle install                    # Uses the pinned Ceedling 1.1.8 dependency
make utest                        # All four boards
make utest UTEST_BOARDS=drd        # One board
make utest UTEST_BOARDS="tel str"  # Selected boards
```

If Bundler is missing, install the locked version:
`gem install bundler -v 4.0.20 --no-document`.
The old vendored Ceedling in `tools/ceedling/` is unused.

`make utest` cleans each board's test output, runs Ceedling, and checks that at
least one test passed. Failed tests, compile errors, and empty suites fail the
command. CI runs the same command. Generated files and JSON reports are under
`firmware/components/<board>/build_unit/`.

To run a single test file:

```sh
cd firmware/components/tel
bundle exec ceedling test:rtc_driver
```

On macOS, `make utest` selects the SDK automatically. If direct Ceedling commands
have an SDK/linker mismatch, set
`export SDKROOT="$(xcrun --sdk macosx --show-sdk-path)"` first.

## Tests and support

There are five tests across four boards:

| Board | Tests |
| --- | --- |
| DRD | Braked forward state change and safe motor output; ADC timeout diagnostic and zero throttle |
| MDI | Command decoding, DAC writes, and direction/eco outputs |
| TEL | RTC time fields |
| STR | Speed formatting and display I2C writes |

Each board has a small `project.yml` listing the production files it tests,
and test cases in `test/test_ceedling_*.c`. DRD's service stubs and captured
outputs live directly in its test file. TEL currently tests RTC only; IMU
testing is deferred.
`firmware/test/support/` is the only shared helper directory; it supplies the fake HAL.
Keep this support on test include/source paths; never add it to target CMake.
The fake HAL rejects compilation without `TEST`.

Tests set inputs, call production functions, and check recorded outputs.
`TestHalReset()` resets peripheral state. Each test resets its local captured
outputs in `setUp()`. ADC polling defaults to `HAL_OK`; set
`test_hal.adc_poll_status` to exercise a failure.

To add a test, create a `test_ceedling_<module>.c` file, include `unity.h` and the
production/support headers, reset state in `setUp()`, and assert the result.
Use `test_*` names for Unity test cases; use module-prefixed PascalCase for
helpers. Run `make utest` afterward. Add a new board to `UTEST_BOARDS` once it has
a `project.yml` and tests.

These tests cover software logic. Check target integration separately with
`make all debug` and `make all release`; electrical behavior, CAN transport,
and RTOS scheduling require hardware validation.
