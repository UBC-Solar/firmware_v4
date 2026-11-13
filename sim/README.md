# sim: software-in-the-loop (SIL) tests

Runs board firmware (the same `.elf` we flash) on an emulated STM32F103RCT6 in
[Renode](https://renode.io) and tests it with pytest. No board, probe or bench needed.

```
sim/
  <board>/test_*.py   tests, one folder per board (the folder picks the board)
  conftest.py         the `board` fixture and the --gdb option
  pytest.ini  requirements.txt
  _emulator/          emulator setup. Set up once; only touch it to fix or extend the emulator.
    __init__.py         the test API (Board, CanFrame): the only thing tests use
    chip/               STM32F103RCT6 setup shared by every board
    peripherals/        Python stubs for peripherals Renode's F103 model lacks
    devices/            fake external parts (GPS, IMU, displays ...)
    boards/<board>.resc how each board is wired: chip + ELF + devices
```

## Setup (once)

1. Install Renode (v1.16.1 used so far) and put `renode` on your `PATH`.
2. Build the board: `make str`.
3. Python environment, from the repo root:
   ```bash
   python -m venv sim/.venv
   sim/.venv/bin/pip install -r sim/requirements.txt
   ```

## Run the tests

```bash
sim/.venv/bin/pytest sim               # all boards
sim/.venv/bin/pytest sim/str           # one board
sim/.venv/bin/pytest sim/str -k boot   # one test
```

## Write a test

Add `test_<feature>.py` in the board's folder and take `board` as an argument. The folder decides
which board you get, freshly booted for each test.

```python
def test_steering_state_sent_every_100ms(board):
    board.run_for(ms=1000)
    frames = board.can_frames_sent(can_id=0x580)
    gaps = [b.time_ms - a.time_ms for a, b in zip(frames, frames[1:])]
    assert all(90 <= g <= 110 for g in gaps), gaps
```

### Tests control time
Renode has its own **virtual time**, which only moves while the emulation runs, and the emulation
only runs inside `board.run_for()` and `board.wait_for_*()`.

- Move time with `board.run_for(ms=...)`. Wait for events with `board.wait_for_can(id, within_ms=...)`:
  the limit is in **firmware** time.
- **Never use `time.sleep()` or wall-clock timeouts.** They have no relation to firmware time.
- Every captured frame has `time_ms` (virtual time), so timing asserts give the same result on any
  machine.
- Timing is reliable for timer-driven behaviour (task periods, `osDelay`, CAN intervals, watchdog).
  It is approximate for execution speed: Renode isn't cycle-accurate and its DMA/UART finish
  instantly.

### Board API

| Call | Does |
|---|---|
| `run_for(ms)` | Run exactly `ms` of firmware time |
| `time_ms` | Current virtual time |
| `send_can(id, data, extended=False)` | Deliver a frame to the board, as if another board sent it |
| `can_frames_sent(id=None)` | Frames the board has sent so far (`CanFrame`: `can_id`, `data`, `extended`, `time_ms`) |
| `wait_for_can(id, within_ms)` | Run until the board sends `id`; fail after `within_ms` |
| `set_pin(port, pin, high)` | Drive an input pin, e.g. `set_pin("A", 2, False)` |
| `symbol_at_pc()` | Function the CPU is in, e.g. `"Error_Handler"` |

If a test needs something this doesn't offer (a new helper, a fake device), that's an
`_emulator/` change: make it its own PR.

## Debug a test and the firmware together

Two debuggers run at once: **debugpy** for the Python test, **cortex-debug** attached to Renode's
GDB server (`localhost:3333`) for the firmware.

> Planned: VS Code launch configs that start both with one F5 are not in `.vscode/` yet.

Until then, by hand:
1. Start the test with the GDB server: `sim/.venv/bin/pytest sim/str/test_can.py --gdb -s`.
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
- At a **firmware** breakpoint `run_for()` doesn't return, so the test waits.
- You can't step from Python *into* C. `board.send_can()` hands the frame to Renode; put a
  breakpoint in the C code that receives it (e.g. `SteeringCanRxHandler` in `app/can_app.c`).

Open question: whether Renode's virtual time stops while the CPU sits at a GDB breakpoint. If it
doesn't, `Board` will pause the emulation on breakpoints; tests won't change.

## Run the STR sim by hand

```bash
renode sim/_emulator/boards/str.resc
# then in the Renode Monitor:
start
pause
sysbus FindSymbolAt `cpu PC`     # where is the CPU?
```
