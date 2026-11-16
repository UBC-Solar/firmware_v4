"""board.can: inject CAN frames and watch the ones the board sends."""

from emulation import renode
from .results import CanFrame


class Can:
    def __init__(self, board):
        self._board = board
        self._can = board._peripheral("can1")
        self._frames = []  # every frame the board sent, oldest first
        self._seen = 0  # wait_for() looks at frames from this index on
        if self._can is None:
            return

        def on_frame_sent(frame):
            try:
                self._frames.append(
                    CanFrame(int(frame.Id), bytes(frame.Data), bool(frame.ExtendedFormat),
                             board.time_ms)
                )
            except Exception as e:  # an exception escaping into Renode would kill the process
                board._hook_errors.append(e)

        self._can.FrameSent += on_frame_sent

    def send(self, can_id: int, data: bytes, extended: bool = False):
        """Deliver a frame to the board's CAN controller, as if another board sent it."""
        self._board._announce(f"can.send 0x{can_id:X} [{bytes(data).hex(' ')}]")
        if self._can is None:
            raise ValueError(f"Board '{self._board.name}' has no CAN controller (can1)")
        r = renode
        frame = r.CANMessageFrame(can_id, r.Array[r.Byte](bytes(data)), extended, False, False, False)
        self._can.OnFrameReceived(frame)

    def sent(self, can_id: int | None = None) -> list[CanFrame]:
        """Frames the board has sent so far, oldest first. Optionally only one ID."""
        return [f for f in self._frames if can_id is None or f.can_id == can_id]

    def wait_for(self, can_id: int, within_ms: float) -> CanFrame:
        """Run until the board sends `can_id` (counting only frames sent from now on, or after the
        previous wait_for). Fails if `within_ms` of firmware time passes first."""

        def find():
            for i in range(self._seen, len(self._frames)):
                if self._frames[i].can_id == can_id:
                    self._seen = i + 1
                    return self._frames[i]
            return None

        self._seen = max(self._seen, len(self._frames))  # only new frames count
        self._board._announce(f"can.wait_for 0x{can_id:X} (up to {within_ms} ms)")
        frame = self._board._run_until(find, within_ms, f"CAN 0x{can_id:X}")
        self._board._announce_done(frame.time_ms)
        return frame
