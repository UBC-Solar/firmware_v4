"""Test API for SIL tests. Tests use only what this module exposes.

Design rule: tests control *virtual* time. Renode's clock only moves while the emulation runs, and
the emulation only runs inside run_for() / wait_for_*(). So:
- at a Python breakpoint the firmware is frozen,
- at a firmware (GDB) breakpoint run_for() doesn't return, so the test waits (verified: virtual time
  stops while GDB holds the CPU),
- every captured event is timestamped in firmware time, so timing asserts are repeatable.
Never use time.sleep() or wall-clock timeouts in tests.

How it talks to Renode: pyrenode3 loads Renode (a .NET program) *inside* this Python process via
pythonnet. Python then gets the same objects a .resc script uses (the machine, the CPU, the CAN
controller, the UARTs) as Python objects: it can call their methods, read their properties and
subscribe to their events. Monitor commands (`include`, `emulation RunFor`, ...) go through
pyrenode3's Monitor.
"""

import inspect
import os
import re
import shutil
import sys
import time
from dataclasses import dataclass
from pathlib import Path

BOARDS_DIR = Path(__file__).parent / "boards"
COMPONENTS_DIR = Path(__file__).resolve().parents[2] / "firmware" / "components"
GDB_PORT = 3333
SIM_DIR = Path(__file__).resolve().parents[1]
GDB_ELF_LINK = SIM_DIR / ".gdb-target.elf"  # see _wait_for_gdb()
STEP_MS = 10  # wait_for_*() runs the emulation in steps this long while checking for the event
UARTS = ("usart1", "usart2", "usart3", "usart4", "usart5")  # names in Renode's stm32f103.repl
RTOS_WAIT_FUNCTIONS = (
    "osDelay", "osDelayUntil", "osMessageQueueGet", "osSemaphoreAcquire",
    "osThreadFlagsWait", "osEventFlagsWait",
)
RENODE_LOG_LEVEL = 3  # 3 = errors only. Renode warns on every unmodelled register bit (very noisy)


class SimTimeout(AssertionError):
    """An expected event didn't happen within the virtual-time limit. Subclasses AssertionError so
    pytest reports it as a test failure, not a crash."""


@dataclass
class CanFrame:
    can_id: int
    data: bytes
    extended: bool
    time_ms: float  # virtual time the frame was sent/received


def find_elf(board: str, build_type: str = "Debug") -> Path:
    """Find <board>.elf built as `build_type` ("Debug" or "Release").

    A board can be built into several folders: build/Debug/ and build/Release/ (CMake presets, VS
    Code), or build/ itself (`make <board>`, Debug or Release, the folder name doesn't say). So the
    build type is read from each folder's CMakeCache.txt instead of trusting the path. If more than
    one folder matches, the newest ELF wins.
    """
    build = COMPONENTS_DIR / board / "build"
    caches = [build / "CMakeCache.txt", *sorted(build.glob("*/CMakeCache.txt"))]
    found = {}  # ELF path -> build type
    for cache in caches:
        elf = cache.parent / f"{board}.elf"
        if cache.exists() and elf.exists():
            match = re.search(r"^CMAKE_BUILD_TYPE:\w+=(\w+)", cache.read_text(), re.MULTILINE)
            found[elf] = match.group(1) if match else "unknown"

    matching = [elf for elf, kind in found.items() if kind.lower() == build_type.lower()]
    if not matching:
        seen = ", ".join(f"{elf.relative_to(COMPONENTS_DIR)} ({kind})" for elf, kind in found.items())
        raise FileNotFoundError(
            f"No {build_type} build of {board}. Found: {seen or 'none'}. "
            f"Build it: cmake --preset {build_type} -S firmware/components/{board} "
            f"&& cmake --build firmware/components/{board}/build/{build_type}"
        )
    return max(matching, key=lambda elf: elf.stat().st_mtime)


# ---- Loading Renode (once per Python process) ---------------------------------------------------

_renode = None  # namespace with the Renode classes we use, filled in by _load_renode()


def _load_renode():
    """Load Renode into this process the first time a Board is created.

    pyrenode3 finds Renode through PYRENODE_PATH. If that isn't set, use the `renode` on PATH (the
    portable install is a folder with a `renode` script in it).
    """
    global _renode
    if _renode is not None:
        return _renode

    if "PYRENODE_PATH" not in os.environ:
        renode_bin = shutil.which("renode")
        if renode_bin is None:
            raise RuntimeError(
                "Renode not found: put `renode` on PATH or set PYRENODE_PATH to the Renode folder"
            )
        os.environ["PYRENODE_PATH"] = str(Path(renode_bin).resolve().parent)

    # These imports load Renode's .NET assemblies; they only work after pyrenode3 is imported.
    from pyrenode3.conversion import interface_to_class
    from pyrenode3.wrappers import Emulation, Monitor
    from Antmicro.Renode.Core import EmulationManager
    from Antmicro.Renode.Core.CAN import CANMessageFrame
    from Antmicro.Renode.Peripherals.CPU import CpuAddressHook
    from System import Array, Byte, UInt64

    class Namespace:
        pass

    _renode = Namespace()
    Emulation()  # creates Renode's emulation (singleton)
    _renode.monitor = Monitor()  # runs Monitor commands, like typing them in Renode's window
    _renode.manager = EmulationManager.Instance
    # pythonnet returns peripherals as their interface type (IPeripheral, IBusController ...).
    # interface_to_class casts them to the real class, so their own methods/events are visible.
    _renode.cast = interface_to_class
    _renode.CANMessageFrame = CANMessageFrame
    _renode.CpuAddressHook = CpuAddressHook
    _renode.Array, _renode.Byte, _renode.UInt64 = Array, Byte, UInt64
    return _renode


def _monitor(command: str) -> str:
    """Run a Renode Monitor command; raise if Renode reports an error."""
    out, err = _renode.monitor.execute(command)
    if err:
        raise RuntimeError(f"Renode command failed: {command}\n{err}")
    return out


# ---- Board -------------------------------------------------------------------------------------


class Board:
    """One emulated board, freshly booted. `name` matches a file in boards/, e.g. "str"."""

    def __init__(self, name: str, gdb: bool = False, build_type: str = "Debug", trace: bool = False,
                 say=None):
        """Load boards/<name>.resc with the right ELF. The emulation is not running yet.

        gdb=True: start Renode's GDB server on GDB_PORT and wait for a debugger to connect before
        any firmware runs, so boot can be debugged too.
        build_type: which firmware build to run ("Debug" or "Release"), see find_elf().
        trace=True (always on with gdb): print each test step to the terminal, see _announce().
        say: function that prints a line to the terminal. The pytest fixture passes one that gets
        past pytest's output capture; the default prints to stderr.
        """
        self.name = name
        self._trace = trace or gdb
        self._say = say or (lambda line: print(line, file=sys.stderr, flush=True))
        self.resc = BOARDS_DIR / f"{name}.resc"
        if not self.resc.exists():
            raise FileNotFoundError(f"No emulator setup for board '{name}': {self.resc}")
        self.elf = find_elf(name, build_type)

        r = _load_renode()
        _monitor("Clear")  # drop any machine left over from a previous test
        _monitor(f"logLevel {RENODE_LOG_LEVEL}")
        # Set $elf first: the board file's `$elf ?= ...` only applies when $elf isn't set yet.
        _monitor(f"$elf = @{self.elf}")
        _monitor(f"include @{self.resc}")

        found, machine = r.manager.CurrentEmulation.TryGetMachineByName(name, None)
        if not found:
            raise RuntimeError(f"{self.resc} didn't create a machine called '{name}'")
        self._machine = r.cast(machine)
        self._bus = r.cast(self._machine.SystemBus)
        self._cpu = self._peripheral("cpu")
        self._hook_errors = []  # exceptions raised inside Renode callbacks, re-raised in the test

        self._capture_can()
        self._capture_uarts()
        self._track_rtos_tasks()

        if gdb:
            self._wait_for_gdb()

    def _peripheral(self, name: str):
        """sysbus.<name> as its real class, or None if this machine doesn't have it."""
        try:
            peripheral = self._machine[f"sysbus.{name}"]
        except Exception:
            return None
        return _renode.cast(peripheral) if peripheral is not None else None

    def close(self):
        """Remove the machine. The next Board starts from a clean emulation."""
        _monitor("Clear")

    # ---- time ----------------------------------------------------------------------------

    def run_for(self, ms: float):
        """Run exactly `ms` of firmware (virtual) time, then pause."""
        self._announce(f"run_for {ms} ms")
        self._step(ms)

    def _step(self, ms: float):
        # Blocks until the virtual time has passed. If GDB holds the CPU at a breakpoint, virtual
        # time stops too, so this simply waits until the debugger continues.
        _monitor(f'emulation RunFor "{ms / 1000:.6f}"')
        self._raise_hook_errors()

    @property
    def time_ms(self) -> float:
        """Current virtual time since boot."""
        return self._machine.ElapsedVirtualTime.TimeElapsed.TotalMilliseconds

    def _run_until(self, find, within_ms: float, what: str):
        """Run in STEP_MS steps until find() returns something, or `within_ms` of virtual time pass.

        The emulation may run up to STEP_MS past the event; returned results carry their own
        timestamps, so timing checks are still exact.
        """
        deadline = self.time_ms + within_ms
        while True:
            result = find()
            if result is not None:
                return result
            remaining = deadline - self.time_ms
            if remaining <= 0:
                raise SimTimeout(f"{what} not seen within {within_ms} ms (now at {self.time_ms:.1f} ms)")
            self._step(min(STEP_MS, remaining))

    # ---- CAN -----------------------------------------------------------------------------

    def _capture_can(self):
        self._can = self._peripheral("can1")
        self._can_frames = []  # every frame the board sent, oldest first
        self._can_seen = 0  # wait_for_can() looks at frames from this index on
        if self._can is None:
            return

        def on_frame_sent(frame):
            try:
                self._can_frames.append(
                    CanFrame(int(frame.Id), bytes(frame.Data), bool(frame.ExtendedFormat), self.time_ms)
                )
            except Exception as e:  # an exception escaping into Renode would kill the process
                self._hook_errors.append(e)

        self._can.FrameSent += on_frame_sent

    def send_can(self, can_id: int, data: bytes, extended: bool = False):
        """Deliver a frame to the board's CAN controller, as if another board sent it."""
        self._announce(f"send_can 0x{can_id:X} [{bytes(data).hex(' ')}]")
        r = _renode
        frame = r.CANMessageFrame(can_id, r.Array[r.Byte](bytes(data)), extended, False, False, False)
        self._can.OnFrameReceived(frame)

    def can_frames_sent(self, can_id: int | None = None) -> list[CanFrame]:
        """Frames the board has sent so far, oldest first. Optionally only one ID."""
        return [f for f in self._can_frames if can_id is None or f.can_id == can_id]

    def wait_for_can(self, can_id: int, within_ms: float) -> CanFrame:
        """Run until the board sends `can_id` (counting only frames sent from now on, or after the
        previous wait_for_can). Fails if `within_ms` of firmware time passes first."""

        def find():
            for i in range(self._can_seen, len(self._can_frames)):
                if self._can_frames[i].can_id == can_id:
                    self._can_seen = i + 1
                    return self._can_frames[i]
            return None

        self._can_seen = max(self._can_seen, len(self._can_frames))  # only new frames count
        self._announce(f"wait_for_can 0x{can_id:X} (up to {within_ms} ms)")
        frame = self._run_until(find, within_ms, f"CAN 0x{can_id:X}")
        self._announce_done(frame.time_ms)
        return frame

    # ---- UART ----------------------------------------------------------------------------

    def _capture_uarts(self):
        # For every UART: all bytes the board wrote, the virtual time of each, and how far
        # wait_for_uart() has consumed.
        self._uart_bytes = {}
        self._uart_times = {}
        self._uart_pos = {}
        for name in UARTS:
            uart = self._peripheral(name)
            if uart is None:
                continue
            self._uart_bytes[name] = bytearray()
            self._uart_times[name] = []
            self._uart_pos[name] = 0

            def on_char(byte, name=name):  # name=name: each callback keeps its own UART name
                try:
                    self._uart_bytes[name].append(byte)
                    self._uart_times[name].append(self.time_ms)
                except Exception as e:
                    self._hook_errors.append(e)

            uart.CharReceived += on_char

    def uart_bytes(self, uart: str) -> bytes:
        """Everything the board has written to `uart` so far, e.g. uart_bytes("usart2")."""
        return bytes(self._uart_bytes[uart])

    def wait_for_uart(self, uart: str, data: bytes, within_ms: float) -> float:
        """Run until `data` appears on `uart`; return the virtual time its last byte was sent.

        Each wait consumes output up to the end of its match, so consecutive waits for the same
        bytes find successive occurrences. Fails if `within_ms` of firmware time passes first.
        """
        if uart not in self._uart_bytes:
            raise ValueError(f"Unknown UART '{uart}'. Known: {', '.join(self._uart_bytes)}")

        def find():
            i = self._uart_bytes[uart].find(data, self._uart_pos[uart])
            if i < 0:
                return None
            end = i + len(data)
            self._uart_pos[uart] = end
            return self._uart_times[uart][end - 1]

        self._announce(f"wait_for_uart {uart} [{data.hex(' ')}] (up to {within_ms} ms)")
        found_at = self._run_until(find, within_ms, f"{data.hex(' ')} on {uart}")
        self._announce_done(found_at)
        return found_at

    # ---- RTOS ----------------------------------------------------------------------------

    def _symbol(self, address: int) -> str:
        """Function name at an address ('' if none). Thumb addresses have bit 0 set: clear it."""
        name = self._bus.FindSymbolAt(address & ~1, self._cpu) or ""
        return name.replace(" (entry)", "")

    def _address_of(self, symbol: str) -> int | None:
        try:
            return int(list(self._bus.GetAllSymbolAddresses(symbol))[0])
        except Exception:
            return None  # this firmware doesn't have that function

    def _add_hook(self, symbol: str, callback):
        """Call callback() every time the CPU is about to run `symbol`. Returns False if missing."""
        address = self._address_of(symbol)
        if address is None:
            return False

        def hook(cpu, pc):
            try:
                callback()
            except Exception as e:  # an exception escaping into Renode would kill the process
                self._hook_errors.append(e)

        self._cpu.AddHook(_renode.UInt64(address), _renode.CpuAddressHook(hook))
        return True

    def _track_rtos_tasks(self):
        """Count loops of each FreeRTOS task (see rtos_task_loops)."""
        self._tasks_created = []
        self._wait_calls = {}  # calling function -> number of calls to an RTOS wait function

        def on_thread_new():
            # osThreadNew(func, ...): R0 holds the task's function
            self._tasks_created.append(self._symbol(self._cpu.GetRegister(0).RawValue))

        def on_wait():
            # LR is the return address, i.e. inside the function that called the wait
            caller = self._symbol(self._cpu.LR.RawValue)
            self._wait_calls[caller] = self._wait_calls.get(caller, 0) + 1

        if not self._add_hook("osThreadNew", on_thread_new):
            return  # bare-metal board, no RTOS
        for wait in RTOS_WAIT_FUNCTIONS:
            self._add_hook(wait, on_wait)

    def rtos_task_loops(self) -> dict[str, int]:
        """Loop count of every task created with osThreadNew, by task function name.

        Counted by hooking osThreadNew (records each task function) and the RTOS wait functions
        (osDelay, osMessageQueueGet, osSemaphoreAcquire, ...: one call from a task = one loop).
        A task that was never created, hangs or starves shows up with a low count. Only counts
        waits called directly from the task function, not from helpers it calls.
        """
        return {task: self._wait_calls.get(task, 0) for task in self._tasks_created}

    # ---- GPIO ----------------------------------------------------------------------------

    def set_pin(self, port: str, pin: int, high: bool):
        """Drive an input pin from outside, e.g. set_pin("A", 2, False) for STR's NEXT_PAGE."""
        self._announce(f"set_pin P{port.upper()}{pin} {'high' if high else 'low'}")
        gpio = self._peripheral(f"gpioPort{port.upper()}")
        if gpio is None:
            raise ValueError(f"No GPIO port {port}")
        gpio.OnGPIO(pin, high)

    # ---- state ---------------------------------------------------------------------------

    def symbol_at_pc(self) -> str:
        """Function the CPU is currently in, e.g. "Error_Handler"."""
        return self._symbol(self._cpu.PC.RawValue)

    # ---- debugging -----------------------------------------------------------------------

    def _wait_for_gdb(self):
        # Point sim/.gdb-target.elf at the ELF this test runs, so one VS Code debug config fits every
        # board and build type. Written before the server starts: once port 3333 is open (what the
        # VS Code pre-launch task waits for) the link is already right.
        GDB_ELF_LINK.unlink(missing_ok=True)
        GDB_ELF_LINK.symlink_to(self.elf)
        # `false`: don't start the emulation when GDB connects; tests still control time.
        try:
            _monitor(f"machine StartGdbServer {GDB_PORT} false")
        except RuntimeError as e:
            if "AddressAlreadyInUse" in str(e):
                raise RuntimeError(
                    f"Port {GDB_PORT} is already in use: another `pytest --gdb` run or VS Code SIL "
                    "debug session is still open. Stop it (or find it: ss -ltnp | grep 3333)."
                ) from None
            raise
        self._say(f"\n[sim] {self.name}: waiting for GDB on localhost:{GDB_PORT} ({self.elf})")
        while "True" not in _monitor(f"machine IsGdbConnectedToServer {GDB_PORT}"):
            time.sleep(0.2)  # waiting for a person to attach a debugger: real time is correct here
        self._say(f"[sim] {self.name}: GDB connected")

    # ---- tracing: which test line is running ----------------------------------------------
    #
    # While the firmware sits at a GDB breakpoint, the test is blocked inside a Renode call and the
    # Python debugger can't show where. So each test step is printed *before* it runs: the last
    # [sim] line in the terminal is the step that's running when a C breakpoint hits.

    def _announce(self, action: str):
        if not self._trace:
            return
        self._say(f"[sim {self.time_ms:9.1f} ms] {_test_location()}\n{'':18}-> {action}")

    def _announce_done(self, at_ms: float):
        if self._trace:
            self._say(f"{'':18}   found at {at_ms:.1f} ms")

    def _raise_hook_errors(self):
        if self._hook_errors:
            error = self._hook_errors[0]
            self._hook_errors.clear()
            raise RuntimeError(f"Error inside a Renode callback: {error!r}") from error


def _test_location() -> str:
    """'tel/test_boot.py:10  <source line>' of the innermost frame in a sim/<board>/test_*.py file."""
    for frame in inspect.stack(context=1)[1:]:
        path = Path(frame.filename)
        if path.name.startswith("test_") and path.parent.parent == SIM_DIR:
            source = frame.code_context[0].strip() if frame.code_context else ""
            return f"{path.parent.name}/{path.name}:{frame.lineno}  {source}"
    return "(not called from a test file)"
