"""board.uart: watch what the board writes to its UARTs, and send bytes to it."""

UARTS = ("usart1", "usart2", "usart3", "usart4", "usart5")  # names in Renode's stm32f103.repl


class Uart:
    def __init__(self, board):
        self._board = board
        # For every UART: the peripheral, all bytes the board wrote, the virtual time of each, and
        # how far wait_for() has consumed.
        self._uarts = {}
        self._bytes = {}
        self._times = {}
        self._pos = {}
        for name in UARTS:
            uart = board._peripheral(name)
            if uart is None:
                continue
            self._uarts[name] = uart
            self._bytes[name] = bytearray()
            self._times[name] = []
            self._pos[name] = 0

            def on_char(byte, name=name):  # name=name: each callback keeps its own UART name
                try:
                    self._bytes[name].append(byte)
                    self._times[name].append(board.time_ms)
                except Exception as e:
                    board._hook_errors.append(e)

            uart.CharReceived += on_char

    def _check(self, uart: str):
        if uart not in self._uarts:
            raise ValueError(f"Unknown UART '{uart}'. Known: {', '.join(self._uarts)}")

    def received(self, uart: str) -> bytes:
        """Everything the board has written to `uart` so far, e.g. received("usart2")."""
        self._check(uart)
        return bytes(self._bytes[uart])

    def wait_for(self, uart: str, data: bytes, within_ms: float) -> float:
        """Run until `data` appears on `uart`; return the virtual time its last byte was sent.

        Each wait consumes output up to the end of its match, so consecutive waits for the same
        bytes find successive occurrences. Fails if `within_ms` of firmware time passes first.
        """
        self._check(uart)

        def find():
            i = self._bytes[uart].find(data, self._pos[uart])
            if i < 0:
                return None
            end = i + len(data)
            self._pos[uart] = end
            return self._times[uart][end - 1]

        self._board._announce(f"uart.wait_for {uart} [{data.hex(' ')}] (up to {within_ms} ms)")
        found_at = self._board._run_until(find, within_ms, f"{data.hex(' ')} on {uart}")
        self._board._announce_done(found_at)
        return found_at

    def send(self, uart: str, data: bytes):
        """Send bytes *to* the board on `uart`, as if a device on that line transmitted them.

        All bytes arrive at once (no baud-rate timing): the firmware's Rx interrupt/DMA sees them
        when the emulation next runs.
        """
        self._check(uart)
        self._board._announce(f"uart.send {uart} [{bytes(data).hex(' ')}]")
        for byte in bytes(data):
            self._uarts[uart].WriteChar(byte)
