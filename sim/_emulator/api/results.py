"""Small result types the Board API returns. Tests import them from `_emulator`."""

from dataclasses import dataclass


class SimTimeout(AssertionError):
    """An expected event didn't happen within the virtual-time limit. Subclasses AssertionError so
    pytest reports it as a test failure, not a crash."""


@dataclass
class CanFrame:
    can_id: int
    data: bytes
    extended: bool
    time_ms: float  # virtual time the frame was sent/received


@dataclass
class PinEdge:
    port: str  # "A".."E"
    pin: int
    high: bool  # level after the edge
    time_ms: float  # virtual time of the edge
