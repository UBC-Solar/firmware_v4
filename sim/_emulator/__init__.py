"""Test API for SIL tests. Tests use only what this module exposes.

Design rule: tests control *virtual* time. Renode's clock only moves while the emulation runs, and
the emulation only runs inside run_for() / wait_for_*(). So:
- at a Python breakpoint the firmware is frozen,
- at a firmware (GDB) breakpoint run_for() doesn't return, so the test waits,
- every captured event is timestamped in firmware time, so timing asserts are repeatable.
Never use time.sleep() or wall-clock timeouts in tests.

Status: skeleton. Every method raises NotImplementedError; the board fixture turns that into a skip.
TODO: implement on pyrenode3 (Renode in-process); fallback is Renode as a subprocess driven over
its monitor port. Tests don't change either way.
"""

import re
from dataclasses import dataclass
from pathlib import Path

BOARDS_DIR = Path(__file__).parent / "boards"
COMPONENTS_DIR = Path(__file__).resolve().parents[2] / "firmware" / "components"
GDB_PORT = 3333


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


@dataclass
class CanFrame:
    can_id: int
    data: bytes
    extended: bool
    time_ms: float  # virtual time the frame was sent/received


class Board:
    """One emulated board, freshly booted. `name` matches a file in boards/, e.g. "str"."""

    def __init__(self, name: str, gdb: bool = False, build_type: str = "Debug"):
        """Start Renode and load boards/<name>.resc. The emulation is not running yet.

        gdb=True: start Renode's GDB server on GDB_PORT and wait for a debugger to connect before
        any firmware runs, so boot can be debugged too.
        build_type: which firmware build to run ("Debug" or "Release"), see find_elf().
        """
        self.name = name
        self.gdb = gdb
        self.resc = BOARDS_DIR / f"{name}.resc"
        if not self.resc.exists():
            raise FileNotFoundError(f"No emulator setup for board '{name}': {self.resc}")
        self.elf = find_elf(name, build_type)
        # TODO: start Renode, set `$elf = @<self.elf>` BEFORE including self.resc (overrides its
        # default), hook CAN TX capture, StartGdbServer if gdb
        raise NotImplementedError("Board is not implemented yet")

    def close(self):
        """Stop the emulation and release Renode."""
        # TODO
        pass

    # ---- time ----------------------------------------------------------------------------

    def run_for(self, ms: float):
        """Run exactly `ms` of firmware (virtual) time, then pause."""
        raise NotImplementedError

    @property
    def time_ms(self) -> float:
        """Current virtual time since boot."""
        raise NotImplementedError

    # ---- CAN -----------------------------------------------------------------------------

    def send_can(self, can_id: int, data: bytes, extended: bool = False):
        """Deliver a frame to the board's CAN controller, as if another board sent it."""
        raise NotImplementedError

    def can_frames_sent(self, can_id: int | None = None) -> list[CanFrame]:
        """Frames the board has sent so far, oldest first. Optionally only one ID."""
        raise NotImplementedError

    def wait_for_can(self, can_id: int, within_ms: float) -> CanFrame:
        """Run until the board sends `can_id`. Fails if `within_ms` of firmware time passes first."""
        raise NotImplementedError

    # ---- UART ----------------------------------------------------------------------------

    def uart_bytes(self, uart: str) -> bytes:
        """Everything the board has written to `uart` so far, e.g. uart_bytes("usart2")."""
        raise NotImplementedError

    def wait_for_uart(self, uart: str, data: bytes, within_ms: float) -> float:
        """Run until `data` appears on `uart`; return the virtual time it completed.

        Each wait consumes output up to the end of its match, so consecutive waits for the same
        bytes find successive occurrences. Fails if `within_ms` of firmware time passes first.
        """
        raise NotImplementedError

    # ---- RTOS ----------------------------------------------------------------------------

    def rtos_task_loops(self) -> dict[str, int]:
        """Loop count of every task created with osThreadNew, by task function name.

        Counted by hooking osThreadNew (records each task function) and the RTOS wait functions
        (osDelay, osMessageQueueGet, osSemaphoreAcquire, ...: one call from a task = one loop).
        A task that was never created, hangs or starves shows up with a low count.
        """
        raise NotImplementedError

    # ---- GPIO ----------------------------------------------------------------------------

    def set_pin(self, port: str, pin: int, high: bool):
        """Drive an input pin from outside, e.g. set_pin("A", 2, False) for STR's NEXT_PAGE."""
        raise NotImplementedError

    # ---- state ---------------------------------------------------------------------------

    def symbol_at_pc(self) -> str:
        """Function the CPU is currently in, e.g. "Error_Handler"."""
        raise NotImplementedError
