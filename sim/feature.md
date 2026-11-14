# SIL feature roadmap

What the `Board` test API (`sim/_emulator/api/`) can do today, what's missing, and the order to add it in.

## How industry does it

Embedded test frameworks almost all use one **device-under-test (DUT) object** that tests call methods on:

| Framework | DUT object | Typical call |
|---|---|---|
| pytest-embedded (Espressif) | `dut` fixture | `dut.expect("Boot OK", timeout=5)` |
| Zephyr twister (pytest harness) | `DeviceAdapter` | `dut.readlines_until(...)` |
| labgrid (HIL labs) | `Target` + drivers | `target.get_driver("GpioDigitalOutputDriver").set(True)` |
| Renode Robot keywords | the emulation | `Wait For Line On Uart`, `Assert Pin Is High` |
| Vector CANoe / CAPL (automotive) | test module + signals | `TestWaitForSignalMatch(...)` |

Practices worth copying:

1. **Tests use signals and pins, not register addresses.** A DBC file turns "set `next_page` to 1" into CAN bytes.
2. **One interface, two backends.** The same tests run in SIL (Renode) and HIL (PCAN plus a GPIO/relay box). If `Board` stays hardware-neutral, the DRD HIL tests in `suntest/` can reuse the SIL tests.
3. **Methods are grouped by driver** (`board.can`, `board.gpio`, ...) rather than one flat class, so autocomplete shows only what's relevant.

## Current API

Tier 1 and the component split are done; the full table is in `sim/README.md` ("Board API").

| Part | Calls |
|---|---|
| `board` | `wait(ms)`, `time_ms`, `symbol_at_pc()` |
| `board.can` | `send`, `sent`, `wait_for` |
| `board.uart` | `send`, `received`, `wait_for` |
| `board.gpio` | `set`, `get`, `wait_for`, `edges` |
| `board.adc` | `set(adc, channel, volts=/raw=)` |
| `board.mem` | `read`, `read_bytes`, `write` |
| `board.rtos` | `task_loops()` |

Old flat names (`send_can`, `can_frames_sent`, `wait_for_can`, `uart_bytes`, `wait_for_uart`, `set_pin`, `rtos_task_loops`) are kept as aliases.

**Gaps:** reset/watchdog, DBC signals (Tier 2), and the Tier 1 limitations below.

## Verified building blocks

Checked against `Board("str")` after running it for 300 ms:

- **Output pins:** `gpioPortC.Connections[3].IsSet` is `True`, which is STR's DEBUG_LED (PC3, `str/cube/Core/Inc/main.h:60`). Each `gpioPortX` exposes `Connections`, `State`, `OnGPIO` and `ReadDoubleWord`.
- **Variables by name:** `bus.GetAllSymbolAddresses("gpio_pin_state")` gives `0x20002ce0`, and `bus.ReadDoubleWord(0x20002ce0)` gives `0x1000100`.

## Proposed structure

Split `Board` into components while keeping a few top-level shortcuts:

```python
board.wait(ms)          # time stays on Board
board.time_ms
board.reset()

board.can.send(...)        board.can.sent(...)        board.can.wait_for(...)
board.can.send_signal(...) board.can.wait_for_signal(...)
board.gpio.set(...)        board.gpio.get(...)        board.gpio.wait_for(...)   board.gpio.edges(...)
board.uart.send(...)       board.uart.received(...)   board.uart.wait_for(...)
board.adc.set(...)
board.mem.read(...)        board.mem.write(...)
board.rtos.task_loops()
```

The old flat methods (`send_can`, `set_pin`, ...) stay as aliases so existing tests keep working.

## Tier 1: needed for the obvious tests (done)

| Feature | API | Unlocks | Notes |
|---|---|---|---|
| Read output pins | `gpio.get("C", 3) -> bool` | LED, contactor, relay, horn states | Uses `Connections[pin].IsSet` (verified) |
| Wait for a pin | `gpio.wait_for("C", 3, high=True, within_ms=500)` | "Contactor closes within X ms of a command" (HVC) | Built on `_run_until` |
| Pin edge history | `gpio.edges("C", 3) -> [(time_ms, level)]` | Blink rates, sequencing order (precharge before main) | Hook GPIO state changes and timestamp them, like the CAN capture |
| Set ADC inputs | `adc.set(channel, volts)` | DRD pedal, HVC/DIST sensing | `peripherals/adc.py` always returns 0x800 for now; needs per-channel values |
| Firmware variables | `mem.read("next_page")`, `mem.write(name, value)` | White-box asserts, fault injection | Symbol reads verified. Struct fields need DWARF (pyelftools). |
| UART input | `uart.send("usart1", b"$GPGGA...")` | GPS and radio commands to TEL | Renode UART `WriteChar` |

## Tier 2: readable, robust tests

| Feature | API | Why |
|---|---|---|
| DBC-aware CAN | `can.send_signal("STR_State", next_page=1)`, `can.wait_for_signal("STR_State", "next_page", 1)` | Uses cantools with sunlink `dbc/`. No hardcoded bit positions; DBC changes break tests loudly. |
| Reset and watchdog | `reset()`, `reset_count`, IWDG model that resets the CPU | Would turn TEL's TasksIWDG starvation into a real "board reboots" failure instead of an xfail |
| Fault injection | stop a heartbeat, I2C NACK mode, CAN bus-off | Tests what happens when something on the car dies |
| Timing helper | `assert_periodic(frames, period_ms=100, tolerance_ms=10)` | Every board repeats the gap check in `str/test_can.py` |

## Tier 3: bigger infrastructure

- **Multi-board tests:** several machines on a shared Renode CAN hub, e.g. STR presses next-page and DRD changes page. This is where SIL beats unit tests.
- **Device models** in `_emulator/devices/`: AS1115 display, GPS, IMU, ADBMS. They replace write-only stubs like `peripherals/i2c_write_ok.py`, so tests can assert "the display shows 42".
- **Firmware log capture:** printf/ITM output saved to `logs/` and attached to failing tests.
- **HIL backend:** a second implementation of the same interface (PCAN plus a GPIO box) for `suntest/`.

## pytest-side needs (no Board change)

| Need | How |
|---|---|
| Wall-clock safety net if Renode hangs | `pytest-timeout` (virtual-time timeouts don't cover a hung Renode) |
| Parallel runs | `pytest-xdist -n auto`, one Renode per worker |
| CI reporting | `--junitxml`, plus a CAN/UART trace dumped in fixture teardown on failure |
| Both car configs | Parametrize over `CAR_CONFIG_CELLULAR` once both builds can be produced |

## Status

| Item | Status |
|---|---|
| Component split | Done: `_emulator/api/{can,uart,gpio,adc,mem,rtos}.py`, old names kept as aliases |
| Tier 1 | Done. Example tests: `str/test_gpio.py` (DEBUG_LED period), `str/test_mem.py` (button press in `gpio_pin_state`, write), `str/test_adc.py` (VREFINT → Vdd on CAN 0x591, under-voltage flag), `str/test_uart.py` |
| Tier 2 | Not started |
| Tier 3 | Not started |
| pytest plugins | Not started |

### Tier 1 limitations

- **ADC:** single-channel conversions only (SQR3 rank 1). Scan mode + DMA (HVC) not modelled; do it with the HVC sim.
- **Memory:** whole variables by symbol name. Struct fields need `read_bytes` + a hand-written offset; field names would need DWARF (pyelftools).
- **UART input:** bytes arrive all at once, no baud-rate timing. Nothing consumes UART input yet: the real test comes with the GPS device model.
- **GPIO:** pins start low, no pull-ups modelled. Edge times are when the firmware wrote the pin.
