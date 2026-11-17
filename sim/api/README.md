# api: what tests can use

The `Board` test API. Tests import only from here (`from api import Board, CanFrame, ...`) and get
a `Board` through the `board` fixture. The call reference for test writers is in `../README.md`
("Write a test"); `ROADMAP.md` lists what's missing and the order to add it in.

```
api/
  __init__.py            Board, CanFrame, PinEdge, SimTimeout, find_elf: the public names
  board.py               Board: time, lifecycle, debugging; attaches the parts below
  can.py uart.py gpio.py board.can, board.uart, board.gpio ...
  adc.py mem.py rtos.py  ... board.adc, board.mem, board.rtos
  results.py             CanFrame, PinEdge, SimTimeout (what the calls return)
  ROADMAP.md             what the API can do, what's missing, what's next
```

Dependency direction: **tests -> api -> emulation**. `api` is the only code that imports
`emulation` (through `emulation.renode`); `tests/conftest.py` refuses a test that imports it.
Keep Renode types out of what `api` returns, so a hardware (HIL) backend could sit behind the
same calls later.

Adding to the API is a normal change, but every test depends on it: keep calls backwards
compatible (old names stay as aliases) and give it its own PR, separate from tests.

## How it talks to Renode

Implemented with **pyrenode3**, which loads Renode (a .NET program) *inside* the pytest process via
pythonnet. Python then holds the same objects a `.resc` uses (machine, CPU, CAN controller, UARTs):
it calls their methods, reads their properties and subscribes to their events. Monitor commands
(`include`, `emulation RunFor`, `Clear`) go through pyrenode3's `Monitor`.

What happens in `Board("tel")`:
1. `find_elf()` picks the ELF (see "Which ELF" in `../emulation/README.md`).
2. Renode is loaded, once per pytest run (`emulation.renode.load()`). It's found through `PYRENODE_PATH`, or
   the `renode` on `PATH`.
3. `Clear` drops the previous test's machine; `logLevel 3` hides Renode's warnings.
4. `$elf = @<elf>` then `include @boards/tel.resc` builds the machine. Nothing runs yet.
5. The parts are created (`board.can`, `board.uart`, `board.gpio`, `board.adc`, `board.mem`,
   `board.rtos`). Each subscribes to its Renode events now, before any firmware runs: `can1.FrameSent`,
   every UART's `CharReceived`, a state-change hook on every GPIO pin (A–E × 16), CPU hooks on
   `osThreadNew` and the RTOS wait functions. Every event is stored with its virtual time.
6. With `gdb=True`: `machine StartGdbServer 3333 false` (`false` = connecting doesn't start the
   emulation), then wait until a debugger is attached.

How time moves: `wait(ms)` is `emulation RunFor "<seconds>"`, which blocks until that much virtual
time has passed. The `wait_for()` calls run 10 ms steps repeatedly, checking the captured data after each
step, until the event shows up or `within_ms` of virtual time has passed (then `SimTimeout`, an
`AssertionError`, so pytest reports a failed test). The emulation may run up to 10 ms past the event;
results carry their own timestamps, so timing checks stay exact.

**Verified: virtual time stops while GDB holds the CPU at a breakpoint.** With GDB stopped in TEL's
`TimeSinceStartup` for ~4.5 s of real time, virtual time stayed at 2.50 ms and `RunFor` just waited,
then finished at exactly 1000 ms after GDB continued. So a firmware breakpoint pauses the test with
no extra code.

pythonnet gotchas (all handled in the code):
- Peripherals come back as interfaces (`IPeripheral`, `IBusController`); cast with
  `interface_to_class` to see their real methods and events.
- CPU hooks must be wrapped in `CpuAddressHook(...)` and the address in `UInt64(...)`; the hook gets a
  generic CPU interface, so read registers from the `CortexM` object instead.
- **An exception inside a Renode callback kills the whole process.** Every callback catches its
  exceptions and stores them; `wait()` re-raises the first one in the test.
- `SystemBus` has no `GetSymbolAddress` from Python; use `GetAllSymbolAddresses(name)[0]`.
- Symbol names at a function's first instruction end in ` (entry)`; `_symbol()` strips it.
- .NET namespaces (`System`, `Antmicro.*`) can only be imported after pyrenode3 has loaded Renode, so
  all those imports live inside `emulation.renode.load()`.
- `gpioPortX.Connections[pin]` comes back as `IGPIO`, which has no hook method: cast it, then
  `AddStateChangedHook(Action[Boolean](fn))`. Inputs driven with `OnGPIO` show up there too (that's
  how EXTI hears them), so `get`/`edges` work for inputs as well as outputs.
- `TryFindSymbolAt(UInt64(addr), None, None, cpu, False)`: the two `out` parameters are passed as
  `None` and come back in the result tuple `(found, name, symbol)`; `symbol.Length.RawValue` is the
  variable's size.

Rules the implementation keeps:
- **Tests control time.** The emulation only runs inside `wait()` and the `wait_for()` calls; otherwise
  it is paused. A Python breakpoint then freezes the firmware, and a firmware breakpoint blocks the
  test.
- **Timeouts are in virtual time** (`within_ms`), never wall clock.
- **Every captured event is timestamped in virtual time** (`CanFrame.time_ms`).
- **`gdb=True`** starts Renode's GDB server on `GDB_PORT` (3333) and waits for a debugger before any
  firmware runs.
- **One fresh machine per `Board`**, fully released in `close()`.
- **UART waits consume output**: each `uart.wait_for()` continues after the previous match.
- **`rtos.task_loops()`** hooks `osThreadNew` (records each task function from R0) and the RTOS wait
  functions (`osDelay`, `osDelayUntil`, `osMessageQueueGet`, `osSemaphoreAcquire`,
  `osThreadFlagsWait`, `osEventFlagsWait`), counting calls per caller (from LR). Works as long as a
  task's loop calls a wait function directly.

Renode calls `Board` uses:
- CAN TX: `can1.FrameSent += handler` (frame: `Id`, `Data`, `ExtendedFormat`).
- CAN RX: `can1.OnFrameReceived(CANMessageFrame(id, Array[Byte](data), extended, False, False, False))`.
- UART TX: `usartN.CharReceived += handler`. UART RX: `usartN.WriteChar(byte)` (no baud timing).
- GPIO: `gpioPortX.OnGPIO(pin, level)` drives an input; `cast(Connections[pin]).IsSet` reads a pin.
- ADC: `sysbus.WriteDoubleWord(adc_base + 0x100 + 4 * channel, raw)` writes the stub's backdoor.
- Variables: `GetAllSymbolAddresses(name)`, `TryFindSymbolAt` for the size, `ReadBytes`/`WriteBytes`.
- Virtual time: `machine.ElapsedVirtualTime.TimeElapsed.TotalMilliseconds`.
- GPIO input: `gpioPortX.OnGPIO(pin, level)`.
- Current function: `sysbus.FindSymbolAt(cpu.PC.RawValue, cpu)`.

Speed: TEL runs at roughly 1 s of firmware time per 5 s of real time (it busy-waits on the missing
I2C devices); the 9 TEL tests take ~40 s.

## Adding a part (`board.<thing>`)

One file per peripheral in `api/`, a class taking `board`. Use only `board._peripheral`, `_run_until`,
`_announce`/`_announce_done`, `time_ms`, `_hook_errors`, `_bus`, `_cpu`, `_address_of`, `_add_hook`.
Subscribe to events in `__init__`; wrap every callback body in `try/except` into
`board._hook_errors`; validate arguments with a `ValueError` that names the bad value; announce
every call that changes the board or runs time, so `--sim-trace` shows it. Attach it in
`Board.__init__` (`api/board.py`) and add the class annotation at the top of `Board` (autocomplete).
