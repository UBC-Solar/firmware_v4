"""Test API for SIL tests. Tests use only what this module exposes.

Design rule: tests control *virtual* time. Renode's clock only moves while the emulation runs, and
the emulation only runs inside wait() / *.wait_for(). So:
- at a Python breakpoint the firmware is frozen,
- at a firmware (GDB) breakpoint wait() doesn't return, so the test waits (verified: virtual time
  stops while GDB holds the CPU),
- every captured event is timestamped in firmware time, so timing asserts are repeatable.
Never use time.sleep() or wall-clock timeouts in tests.

Speed: by default virtual time runs as fast as the host can emulate (Renode's AdvanceImmediately),
usually many times faster than real time. With gdb=True or realtime=True it is held to wall-clock
pace instead, so a person watching (debugger, Live Watch) sees the firmware at its real speed. Test
results don't depend on it: everything is timestamped in virtual time.

Board holds time, lifecycle and debugging. Everything else is grouped by peripheral, so
autocomplete on `board.gpio.` shows only GPIO calls:
    board.can   send, sent, wait_for              (can.py)
    board.uart  send, received, wait_for          (uart.py)
    board.gpio  set, get, wait_for, edges         (gpio.py)
    board.adc   set                               (adc.py)
    board.mem   read, read_bytes, write           (mem.py)
    board.rtos  task_loops                        (rtos.py)
The older flat names (send_can, set_pin, ...) still work: see "Aliases" at the end of Board.

How it talks to Renode: pyrenode3 loads Renode (a .NET program) *inside* this Python process via
pythonnet (emulation/renode.py). Python then gets the same objects a .resc script uses (the machine, the CPU,
the CAN controller, the UARTs) as Python objects: it can call their methods, read their properties
and subscribe to their events. Monitor commands (`include`, `emulation RunFor`, ...) go through
pyrenode3's Monitor.
"""

import inspect
import os
import re
import shutil
import socket
import subprocess
import sys
import time
from pathlib import Path

from emulation import renode
from .adc import Adc
from .can import Can
from .gpio import Gpio
from .mem import Mem
from .results import CanFrame, PinEdge, SimTimeout
from .rtos import Rtos
from .uart import Uart

__all__ = ["Board", "CanFrame", "PinEdge", "SimTimeout", "find_elf"]

SIM_DIR = Path(__file__).resolve().parents[1]  # sim/
BOARDS_DIR = SIM_DIR / "emulation" / "boards"
COMPONENTS_DIR = SIM_DIR.parent / "firmware" / "components"
GDB_PORT = 3333  # debuggers connect here, to gdb_relay.py, so cortex-debug's Live Watch works
RENODE_GDB_PORT = 3334  # Renode's own GDB server, behind the relay
GDB_RELAY = SIM_DIR / "emulation" / "gdb_relay.py"
GDB_ELF_LINK = SIM_DIR / ".out" / "gdb-target.elf"  # see _wait_for_gdb()
STEP_MS = 10  # wait_for() calls run the emulation in steps this long while checking for the event


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


class Board:
    """One emulated board, freshly booted. `name` matches a file in boards/, e.g. "str"."""

    can: Can
    uart: Uart
    gpio: Gpio
    adc: Adc
    mem: Mem
    rtos: Rtos

    def __init__(self, name: str, gdb: bool = False, build_type: str = "Debug", trace: bool = False,
                 say=None, realtime: bool = False, rtos_trace: bool = False):
        """Load boards/<name>.resc with the right ELF. The emulation is not running yet.

        gdb=True: start Renode's GDB server (behind gdb_relay.py on GDB_PORT) and wait for a
        debugger to connect before any firmware runs, so boot can be debugged too.
        build_type: which firmware build to run ("Debug" or "Release"), see find_elf().
        trace=True (always on with gdb): print each test step to the terminal, see _announce().
        say: function that prints a line to the terminal. The pytest fixture passes one that gets
        past pytest's output capture; the default prints to stderr.
        realtime=True (always on with gdb): hold virtual time to wall-clock pace. Off, the emulation
        runs as fast as it can, which is what CI wants.
        rtos_trace=True: count RTOS task loops for rtos.task_loops(). Off by default: it hooks every
        osDelay/queue wait, which costs a few ms of wall time per call (several x slower overall).
        """
        self.name = name
        self._trace = trace or gdb
        self._say = say or (lambda line: print(line, file=sys.stderr, flush=True))
        self.resc = BOARDS_DIR / f"{name}.resc"
        if not self.resc.exists():
            raise FileNotFoundError(f"No emulator setup for board '{name}': {self.resc}")
        self.elf = find_elf(name, build_type)

        renode.load()
        renode.run("Clear")  # drop any machine left over from a previous test
        renode.run(f"logLevel {renode.RENODE_LOG_LEVEL}")
        # Set $elf first: the board file's `$elf ?= ...` only applies when $elf isn't set yet.
        # Paths as quoted strings: Renode's `@path` form ends at the first space (e.g.
        # "C:/.../UBC Solar/...": "Could not tokenize here"). Forward slashes so a Windows
        # backslash can't be read as an escape inside the quotes; .NET accepts them on Windows.
        renode.run(f'$elf = "{self.elf.as_posix()}"')
        renode.run(f'include "{self.resc.as_posix()}"')
        # Off: virtual time may not run ahead of the host clock (Renode's default). On: it runs as
        # fast as the host emulates it. Set every time: the setting outlives `Clear`.
        fast = not (realtime or gdb)
        renode.run(f"emulation SetGlobalAdvanceImmediately {str(fast).lower()}")

        found, machine = renode.manager.CurrentEmulation.TryGetMachineByName(name, None)
        if not found:
            raise RuntimeError(f"{self.resc} didn't create a machine called '{name}'")
        self._machine = renode.cast(machine)
        self._bus = renode.cast(self._machine.SystemBus)
        self._cpu = self._peripheral("cpu")
        self._hook_errors = []  # exceptions raised inside Renode callbacks, re-raised in the test

        # Each component subscribes to its Renode events now, before any firmware runs
        self.can = Can(self)
        self.uart = Uart(self)
        self.gpio = Gpio(self)
        self.adc = Adc(self)
        self.mem = Mem(self)
        self.rtos = Rtos(self, trace_loops=rtos_trace)

        self._relay = None
        if gdb:
            self._wait_for_gdb()

    def _peripheral(self, name: str):
        """sysbus.<name> as its real class, or None if this machine doesn't have it."""
        try:
            peripheral = self._machine[f"sysbus.{name}"]
        except Exception:
            return None
        return renode.cast(peripheral) if peripheral is not None else None

    def close(self):
        """Remove the machine. The next Board starts from a clean emulation."""
        if self._relay is not None:
            self._relay.terminate()
            self._relay.wait(timeout=5)
            self._relay = None
        renode.run("Clear")

    # ---- time ----------------------------------------------------------------------------

    def wait(self, ms: float):
        """Run exactly `ms` of firmware (virtual) time, then pause."""
        self._announce(f"wait {ms} ms")
        self._step(ms)

    def _step(self, ms: float):
        # Blocks until the virtual time has passed. If GDB holds the CPU at a breakpoint, virtual
        # time stops too, so this simply waits until the debugger continues.
        renode.run(f'emulation RunFor "{ms / 1000:.6f}"')
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

    # ---- state ---------------------------------------------------------------------------

    def symbol_at_pc(self) -> str:
        """Function the CPU is currently in, e.g. "Error_Handler"."""
        return self._symbol(self._cpu.PC.RawValue)

    # ---- symbols and CPU hooks (used by the components) ------------------------------------

    def _symbol(self, address: int) -> str:
        """Function name at an address ('' if none). Thumb addresses have bit 0 set: clear it."""
        name = self._bus.FindSymbolAt(address & ~1, self._cpu) or ""
        return name.replace(" (entry)", "")

    def _address_of(self, symbol: str) -> int | None:
        try:
            return int(list(self._bus.GetAllSymbolAddresses(symbol))[0])
        except Exception:
            return None  # this firmware doesn't have that symbol

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

        self._cpu.AddHook(renode.UInt64(address), renode.CpuAddressHook(hook))
        return True

    # ---- debugging -----------------------------------------------------------------------

    def _wait_for_gdb(self):
        # Point sim/.out/gdb-target.elf at the ELF this test runs, so one VS Code debug config fits every
        # board and build type. Written before the server starts: once port 3333 is open (what the
        # VS Code pre-launch task waits for) the link is already right.
        GDB_ELF_LINK.parent.mkdir(exist_ok=True)
        GDB_ELF_LINK.unlink(missing_ok=True)
        try:
            GDB_ELF_LINK.symlink_to(self.elf)
        except OSError:
            # Windows only allows symlinks with admin rights or Developer Mode: copy instead
            shutil.copyfile(self.elf, GDB_ELF_LINK)
        in_use = RuntimeError(
            f"Port {GDB_PORT} or {RENODE_GDB_PORT} is already in use: another `pytest --gdb` run or "
            "VS Code Renode/SIL debug session is still open. Stop it (or find it: ss -ltnp | grep 333)."
        )
        # `false`: don't start the emulation when GDB connects; tests still control time.
        try:
            renode.run(f"machine StartGdbServer {RENODE_GDB_PORT} false")
        except RuntimeError as e:
            if "AddressAlreadyInUse" in str(e):
                raise in_use from None
            raise
        # Bind GDB_PORT here and hand the socket to the relay: a busy port fails right here, and the
        # port only opens (what VS Code's pre-launch task waits for) once everything is ready.
        try:
            listener = socket.create_server(("127.0.0.1", GDB_PORT))
        except OSError:
            raise in_use from None
        with listener:
            if os.name == "nt":
                # Windows can't hand a socket to a child process (no pass_fds): free the port and
                # let the relay bind it itself, right after the check above
                listener.close()
                relay_args = ["--listen", str(GDB_PORT)]
                pass_fds = ()
            else:
                relay_args = ["--listen-fd", str(listener.fileno())]
                pass_fds = [listener.fileno()]
            self._relay = subprocess.Popen(
                [sys.executable, str(GDB_RELAY), *relay_args, "--target", str(RENODE_GDB_PORT)],
                pass_fds=pass_fds,
            )
        self._say(f"\n[sim] {self.name}: waiting for GDB on localhost:{GDB_PORT} ({self.elf})")
        # The relay connects to Renode when the debugger connects to it.
        while "True" not in renode.run(f"machine IsGdbConnectedToServer {RENODE_GDB_PORT}"):
            if self._relay.poll() is not None:
                raise RuntimeError(f"gdb_relay.py exited (code {self._relay.returncode}), see above")
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

    # ---- Aliases: the flat names from before the component split. New tests use the components.

    def send_can(self, can_id: int, data: bytes, extended: bool = False):
        """Same as can.send()."""
        self.can.send(can_id, data, extended)

    def can_frames_sent(self, can_id: int | None = None) -> list[CanFrame]:
        """Same as can.sent()."""
        return self.can.sent(can_id)

    def wait_for_can(self, can_id: int, within_ms: float) -> CanFrame:
        """Same as can.wait_for()."""
        return self.can.wait_for(can_id, within_ms)

    def uart_bytes(self, uart: str) -> bytes:
        """Same as uart.received()."""
        return self.uart.received(uart)

    def wait_for_uart(self, uart: str, data: bytes, within_ms: float) -> float:
        """Same as uart.wait_for()."""
        return self.uart.wait_for(uart, data, within_ms)

    def set_pin(self, port: str, pin: int, high: bool):
        """Same as gpio.set()."""
        self.gpio.set(port, pin, high)

    def rtos_task_loops(self) -> dict[str, int]:
        """Same as rtos.task_loops()."""
        return self.rtos.task_loops()


def _test_location() -> str:
    """'tel/test_boot.py:10  <source line>' of the innermost frame in a sim/tests/<board>/test_*.py file."""
    for frame in inspect.stack(context=1)[1:]:
        path = Path(frame.filename)
        if path.name.startswith("test_") and path.parent.parent == SIM_DIR / "tests":
            source = frame.code_context[0].strip() if frame.code_context else ""
            return f"{path.parent.name}/{path.name}:{frame.lineno}  {source}"
    return "(not called from a test file)"
