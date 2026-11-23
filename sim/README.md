# sim: software-in-the-loop (SIL) tests

Runs board firmware (the same `.elf` we flash) on an emulated STM32F103RCT6 in
[Renode](https://renode.io) and tests it with pytest. No board, probe or bench needed.

```
sim/
  tests/              WRITE TESTS HERE
    <board>/test_*.py   one folder per board (the folder picks the board)
    conftest.py         the `board` fixture, --gdb / --build-type / --sim-trace
  api/                WHAT TESTS CAN USE: `from api import Board, ...` (see api/README.md)
    board.py            Board + its parts can.py, uart.py, gpio.py, adc.py, mem.py, rtos.py
    ROADMAP.md          what's missing and what's next
  emulation/          HOW BOARDS ARE EMULATED. Don't touch unless fixing the emulator (see emulation/README.md)
    renode.py           the pyrenode3 bridge (only api/ uses it)
    chip/               STM32F103RCT6 setup shared by every board
    peripherals/        Python stubs for peripherals Renode's F103 model lacks
    devices/            fake external parts (GPS, IMU, displays ...)
    boards/<board>.resc how each board is wired: chip + ELF + devices
  pytest.ini  requirements.txt
```

## Setup (once)

1. From the repo root, run the setup script for your OS:
   ```bash
   sim/setup/setup.sh                                                  # Linux
   powershell -ExecutionPolicy Bypass -File sim\setup\setup.ps1          # Windows
   ```
   It installs everything the sim needs into gitignored folders:
   - **Renode** in `sim/.renode/`: one exact nightly build, checksum-verified. pytest and the VS Code
     "Renode:" tasks use it; a `renode` on your `PATH` is ignored, because the chip model
     (`emulation/chip/stm32f103rct6.repl`) needs this build (the tagged 1.16.1 release has no F103
     DMA). `PYRENODE_PATH=<Renode folder>` overrides it, at your own risk.
   - **Python packages** in `sim/.venv/` (pytest, pyrenode3 ...). Needs `git` and Python 3.11-3.15
     (Linux: plus `python3-venv`; Windows: from python.org). Pick the interpreter with
     `PYTHON=python3.12 sim/setup/setup.sh` or `setup.ps1 -Python C:\Python312\python.exe`.

   Re-run it after pulling: it only does work when the pinned Renode or `requirements.txt` changed.
2. Build the board, any way you normally do: `make str`, `make str release`, or the VS Code CMake
   extension / `cmake --preset Debug` (→ `build/Debug/`, `build/Release/`). Tests find the ELF
   themselves: they read `CMAKE_BUILD_TYPE` from each build folder's `CMakeCache.txt`, so the folder
   name doesn't matter.

**Windows status:** the setup script is written for Windows, but the sim hasn't been run there yet.
Two parts are Linux-only for now: the VS Code "Renode:" tasks (bash commands), and `pytest --gdb`
(it symlinks `sim/.out/gdb-target.elf`, which Windows only allows in Developer Mode). On Windows the
test runner is `sim\.venv\Scripts\pytest sim`.

## Run the tests

```bash
sim/.venv/bin/pytest sim               # all boards
sim/.venv/bin/pytest sim/tests/str     # one board
sim/.venv/bin/pytest sim/tests/str -k boot   # one test
sim/.venv/bin/pytest sim --build-type Release   # test the Release build (default: Debug)
```

## Write a test

Add `test_<feature>.py` in the board's folder and take `board` as an argument. The folder decides
which board you get, freshly booted for each test.

```python
def test_steering_state_sent_every_100ms(board):
    board.wait(ms=1000)
    frames = board.can.sent(can_id=0x580)
    gaps = [b.time_ms - a.time_ms for a, b in zip(frames, frames[1:])]
    assert all(90 <= g <= 110 for g in gaps), gaps
```

### Tests control time
Renode has its own **virtual time**, which only moves while the emulation runs, and the emulation
only runs inside `board.wait()` and the `wait_for()` calls (`board.can.wait_for()`, ...).

- Move time with `board.wait(ms=...)`. Wait for events with `board.can.wait_for(id, within_ms=...)`:
  the limit is in **firmware** time.
- **Never use `time.sleep()` or wall-clock timeouts.** They have no relation to firmware time.
- Every captured frame has `time_ms` (virtual time), so timing asserts give the same result on any
  machine.
- Timing is reliable for timer-driven behaviour (task periods, `osDelay`, CAN intervals, watchdog).
  It is approximate for execution speed: Renode isn't cycle-accurate and its DMA/UART finish
  instantly.

### Board API

Calls are grouped by peripheral, so typing `board.gpio.` in VS Code lists only the GPIO calls.
Every `wait_for` fails the test (`SimTimeout`) if `within_ms` of firmware time pass first.

**Board** (time and state)

| Call | Does |
|---|---|
| `wait(ms)` | Run exactly `ms` of firmware time |
| `time_ms` | Current virtual time |
| `symbol_at_pc()` | Function the CPU is in, e.g. `"Error_Handler"` |

**`board.can`**

| Call | Does |
|---|---|
| `send(id, data, extended=False)` | Deliver a frame to the board, as if another board sent it |
| `sent(id=None)` | Frames the board has sent so far (`CanFrame`: `can_id`, `data`, `extended`, `time_ms`) |
| `wait_for(id, within_ms)` | Run until the board sends `id` (only frames sent after the call count) |

**`board.uart`** (names `usart1` … `usart5`)

| Call | Does |
|---|---|
| `send(uart, data)` | Send bytes to the board, as if a device on that line did. All bytes arrive at once (no baud timing) |
| `received(uart)` | Everything the board wrote to the UART so far |
| `wait_for(uart, data, within_ms)` | Run until `data` appears; returns its time. Each wait continues after the previous match |

**`board.gpio`** (ports `"A"` … `"E"`, pins 0–15)

| Call | Does |
|---|---|
| `set(port, pin, high)` | Drive an input pin, e.g. `set("A", 2, False)` |
| `get(port, pin)` | Current level of any pin: an output the firmware drives (LED, contactor) or an input |
| `edges(port, pin)` | Every change since boot, oldest first (`PinEdge`: `high`, `time_ms`): blink rates, sequencing |
| `wait_for(port, pin, high, within_ms)` | Run until the pin is at that level; returns the time (now, if it already is) |

Pins start **low**: Renode doesn't model pull-ups, so set an active-low button `high` at the start
of a test before "pressing" it.

**`board.adc`**

| Call | Does |
|---|---|
| `set(adc, channel, volts=None, raw=None)` | What ADC1/ADC2 reads on a channel, e.g. `set(1, 17, raw=1489)`. `volts` is 0–3.3 V. Unset channels read 0x800 |

Single-channel conversions only (what every board except HVC does). Scan mode + DMA isn't supported yet.

**`board.mem`** (firmware variables by name, from the ELF's symbols)

| Call | Does |
|---|---|
| `read(name, size=None, signed=False)` | Variable as a little-endian int, e.g. `read("g_time_since_bootup")`. Up to 8 bytes |
| `read_bytes(name, offset=0, size=None)` | Raw bytes of any variable (structs, arrays) |
| `write(name, value, size=None)` | Overwrite a variable: force a state, inject a fault |

Struct fields by name aren't supported: use `read_bytes` with the field's offset.

**`board.rtos`**

| Call | Does |
|---|---|
| `task_loops()` | Loop count per FreeRTOS task, to catch tasks that never run, hang or starve |

**Old names** still work (existing tests use them): `send_can` = `can.send`, `can_frames_sent` =
`can.sent`, `wait_for_can` = `can.wait_for`, `uart_bytes` = `uart.received`, `wait_for_uart` =
`uart.wait_for`, `set_pin` = `gpio.set`, `rtos_task_loops` = `rtos.task_loops`.

Board-specific helpers (e.g. decoding TEL's radio frames) go in a non-test module in the board's
folder, like `tests/tel/radio.py`, imported as `from tests.tel.radio import ...`. Test file names
can repeat across boards (`tests/str/test_boot.py`, `tests/tel/test_boot.py`).

Tests import from `api` only, never `emulation`: collection fails if one does.

If a test needs something this doesn't offer, that's an `api/` change (a new call) or an
`emulation/` change (a fake device, a missing peripheral): make it its own PR.

## Debug a test and the firmware together

Two debuggers run at once: **debugpy** for the Python test, **cortex-debug** attached to Renode's
GDB server (`localhost:3333`) for the firmware.

> Planned: VS Code launch configs that start both with one F5 are not in `.vscode/` yet.

Until then, by hand:
1. Start the test with the GDB server: `sim/.venv/bin/pytest sim/tests/str/test_boot.py::test_boots_without_fault --gdb -s`.
   It waits for a debugger before running any firmware.
2. Attach a cortex-debug session to `localhost:3333` with the board's ELF
   (`"servertype": "external"`, `"request": "launch"`, `"overrideLaunchCommands": []`,
   `"runToEntryPoint": "main"`. Renode doesn't understand cortex-debug's default
   `monitor reset halt`).
3. Breakpoints in STR's `.c` files work as with a J-Link.

How it fits together:
- `pytest --gdb` makes `Board` start the GDB server and **wait for the debugger** before running any
  firmware, so boot can be debugged.
- At a **Python** breakpoint the emulation isn't running, so the firmware is frozen.
- At a **firmware** breakpoint `wait()` doesn't return, so the test waits.
- **Which test line is running?** With `--gdb` (or `--sim-trace` without it) every test step is
  printed to the terminal before it runs; the last `[sim]` line is what the test is doing when a C
  breakpoint hits:
  ```
  [sim      10.0 ms] tel/test_can_forwarding.py:22  wait_for_frame(board, 0x401, ...)
                    -> uart.wait_for usart2 [23 00 00 04 01 ab cd ...] (up to 500 ms)
                       found at 10.0 ms
  ```
- Only one debug session at a time: a second one fails with "Port 3333 is already in use".
- **One test per debug session.** Each test boots a fresh machine with its own GDB server, and the
  debugger's connection ends with the test. So `--gdb` refuses to run more than one test: use
  `pytest <file>::<test_name> --gdb` or `-k <name>`. In VS Code, F5 asks for the test's name.
- You can't step from Python *into* C. `board.can.send()` hands the frame to Renode; put a
  breakpoint in the C code that receives it (e.g. `SteeringCanRxHandler` in `app/can_app.c`).

Verified: virtual time stops while GDB holds the CPU at a breakpoint, so the test really does wait
and the firmware's timers don't run on behind your back.

## Run a board without tests

VS Code **"Renode: debug board"** (F5) runs the active CMake target (the board you'd flash) in
Renode, attaches the debugger and stops at `main`. The Renode Monitor is in the terminal (`pause`,
``sysbus FindSymbolAt `cpu PC` ``, `showAnalyzer usart2`). Renode quits when you stop debugging.

**Live Watch works here**: in the Run and Debug sidebar, add variables under **Cortex Live Watch**
and they update while the firmware runs (4 times a second).
Renode's GDB server takes only one connection, and Live Watch is a second gdb, so the task starts
`emulation/gdb_relay.py` on `:3333`, which shares Renode's connection (moved to `:3334`) between
both. The Live Watch gdb can only read and write memory; the relay refuses anything that would run,
stop or detach the target. `python3 sim/emulation/gdb_relay.py -v` prints every packet if the
debugger misbehaves. `pytest --gdb` (the SIL debug configs) starts the same relay from `Board`, so
Live Watch works there too. Live Watch evaluates expressions without a stack frame, so it shows
globals and `static`s only: local variables read "not available" (use Watch/Variables while paused).
