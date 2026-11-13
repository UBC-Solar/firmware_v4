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

from dataclasses import dataclass
from pathlib import Path

BOARDS_DIR = Path(__file__).parent / "boards"
GDB_PORT = 3333


@dataclass
class CanFrame:
    can_id: int
    data: bytes
    extended: bool
    time_ms: float  # virtual time the frame was sent/received


class Board:
    """One emulated board, freshly booted. `name` matches a file in boards/, e.g. "str"."""

    def __init__(self, name: str, gdb: bool = False):
        """Start Renode and load boards/<name>.resc. The emulation is not running yet.

        gdb=True: start Renode's GDB server on GDB_PORT and wait for a debugger to connect before
        any firmware runs, so boot can be debugged too.
        """
        self.name = name
        self.gdb = gdb
        self.resc = BOARDS_DIR / f"{name}.resc"
        if not self.resc.exists():
            raise FileNotFoundError(f"No emulator setup for board '{name}': {self.resc}")
        # TODO: start Renode, include self.resc, hook CAN TX capture, StartGdbServer if gdb
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

    # ---- GPIO ----------------------------------------------------------------------------

    def set_pin(self, port: str, pin: int, high: bool):
        """Drive an input pin from outside, e.g. set_pin("A", 2, False) for STR's NEXT_PAGE."""
        raise NotImplementedError

    # ---- state ---------------------------------------------------------------------------

    def symbol_at_pc(self) -> str:
        """Function the CPU is currently in, e.g. "Error_Handler"."""
        raise NotImplementedError
