"""board.gpio: drive input pins, read any pin, and see every pin change with its time."""

from . import _renode
from .results import PinEdge

PORTS = ("A", "B", "C", "D", "E")
PINS_PER_PORT = 16


class Gpio:
    def __init__(self, board):
        self._board = board
        self._ports = {}  # "A" -> gpioPortA
        self._edges = []  # every pin change on every port, oldest first
        for port in PORTS:
            gpio = board._peripheral(f"gpioPort{port}")
            if gpio is None:
                continue
            self._ports[port] = gpio
            for pin in range(PINS_PER_PORT):
                self._hook_pin(port, pin)

    def _hook_pin(self, port: str, pin: int):
        # Connections[pin] is the pin's level as the rest of the chip sees it: the output the
        # firmware drives, or the input set() drives (that's how EXTI hears it). It comes back as
        # IGPIO, which has no hook method: cast it to the real GPIO class first.
        connection = _renode.cast(self._ports[port].Connections[pin])
        board = self._board

        def on_change(high, port=port, pin=pin):
            try:
                self._edges.append(PinEdge(port, pin, bool(high), board.time_ms))
            except Exception as e:  # an exception escaping into Renode would kill the process
                board._hook_errors.append(e)

        connection.AddStateChangedHook(_renode.Action[_renode.Boolean](on_change))

    def _port(self, port: str, pin: int):
        port = port.upper()
        if port not in self._ports:
            raise ValueError(f"No GPIO port '{port}'. Known: {', '.join(self._ports)}")
        if not 0 <= pin < PINS_PER_PORT:
            raise ValueError(f"Pin {pin} out of range 0-{PINS_PER_PORT - 1}")
        return port, self._ports[port]

    def set(self, port: str, pin: int, high: bool):
        """Drive an input pin from outside, e.g. set("A", 2, False) for STR's NEXT_PAGE.

        Pins start low: Renode doesn't model pull-ups, so set an active-low input high at the
        start of a test if the firmware expects it idle-high.
        """
        port, gpio = self._port(port, pin)
        self._board._announce(f"gpio.set P{port}{pin} {'high' if high else 'low'}")
        gpio.OnGPIO(pin, high)

    def get(self, port: str, pin: int) -> bool:
        """Current level of a pin: an output the firmware drives (LED, contactor) or an input."""
        port, gpio = self._port(port, pin)
        return bool(_renode.cast(gpio.Connections[pin]).IsSet)

    def edges(self, port: str, pin: int) -> list[PinEdge]:
        """Every change of this pin since boot, oldest first (`PinEdge`: high, time_ms)."""
        port, _ = self._port(port, pin)
        return [e for e in self._edges if e.port == port and e.pin == pin]

    def wait_for(self, port: str, pin: int, high: bool, within_ms: float) -> float:
        """Run until the pin is at `high`; return the virtual time it got there.

        Returns the current time straight away if it's already there. Fails if `within_ms` of
        firmware time passes first.
        """
        port, _ = self._port(port, pin)
        level = "high" if high else "low"
        self._board._announce(f"gpio.wait_for P{port}{pin} {level} (up to {within_ms} ms)")
        if self.get(port, pin) == high:
            at = self._board.time_ms
        else:
            start = len(self._edges)

            def find():
                for edge in self._edges[start:]:
                    if edge.port == port and edge.pin == pin and edge.high == high:
                        return edge.time_ms
                return None

            at = self._board._run_until(find, within_ms, f"P{port}{pin} {level}")
        self._board._announce_done(at)
        return at
