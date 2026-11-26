"""Brake input (BRK_IN) handling in app/drive_state.c.

The brake is polled by TasksDriveState every DRIVE_STATE_FSM_DELAY (25 ms), and that task alone
sends the motor command 0x401. BRK_IN (PC9, active high) is still wired to a falling-edge EXTI, but
the ISR must not touch CAN: it used to queue a zero-torque 0x401 on every edge, so a noisy or
floating brake line flooded the bus (and, being a falling edge on an active-high input, it fired on
brake *release*).

0x401 (MotorCommandPackAndSend() in app/can_app.c): data[0:2] accel DAC, data[2:4] regen DAC,
data[4] flags (bit 0 forward/park).
"""

from api import Board, CanFrame

MOTOR_COMMAND_ID = 0x401  # driver/can_driver.h MOTOR_DRIVE_CONTROL_ADDRESS
DIAGNOSTIC_ID = 0x403  # sent every 100 ms by TasksDiagnostic
FSM_PERIOD_MS = 25  # app/drive_state.h DRIVE_STATE_FSM_DELAY
MDI_TIMEOUT_MS = 100  # mdi/driver/mdi_driver.h MDI_MAX_TIMEOUT_VALUE: motor stops after this
REGEN_DAC_ON = 1023  # app/drive_state.h
FORWARD_BIT = 0x01

# The drive task is Low priority: in the sim a gap occasionally stretches to ~37 ms (see
# test_drive_state.py). Two periods is the bound for "the next command".
NEXT_COMMAND_MS = 2 * FSM_PERIOD_MS
# Closest two task-sent commands can be. An ISR-sent command lands between two task ticks.
MIN_GAP_MS = 15

# cube/Core/Inc/main.h
BRK_IN = ("C", 9)  # active high, falling-edge EXTI
BRK_OUT = ("B", 13)
NEXT = ("A", 10)  # DRIVE_STATE_NEXT, active low (pulled up)
PREV = ("A", 11)  # DRIVE_STATE_PREV


def idle(board: Board):
    """Release the shift buttons (Renode doesn't model the pull-ups) and let the board boot."""
    board.gpio.set(*NEXT, high=True)
    board.gpio.set(*PREV, high=True)
    board.wait(ms=300)


def go_forward(board: Board):
    """Brake + NEXT: PARK -> FORWARD, then release the brake."""
    board.gpio.set(*BRK_IN, high=True)
    board.wait(ms=50)
    board.gpio.set(*NEXT, high=False)
    board.wait(ms=50)
    board.gpio.set(*NEXT, high=True)
    board.gpio.set(*BRK_IN, high=False)
    board.wait(ms=100)
    frame = board.can.wait_for(MOTOR_COMMAND_ID, within_ms=NEXT_COMMAND_MS).data
    assert frame[4] & FORWARD_BIT and regen(frame) == REGEN_DAC_ON, frame.hex(" ")


def chatter(board: Board, duration_ms: float, half_period_ms: float = 1.0) -> tuple[float, float]:
    """Toggle BRK_IN like a bouncing or floating line, ending low. Returns (start, end) times.

    The default gives one falling edge every 2 ms: 250 edges in 500 ms.
    """
    start = board.time_ms
    high = True
    while board.time_ms - start < duration_ms:
        board.gpio.set(*BRK_IN, high=high)
        board.wait(ms=half_period_ms)
        high = not high
    board.gpio.set(*BRK_IN, high=False)
    return start, board.time_ms


def commands_between(board: Board, start_ms: float, end_ms: float) -> list[CanFrame]:
    return [f for f in board.can.sent(MOTOR_COMMAND_ID) if start_ms <= f.time_ms <= end_ms]


def gaps(frames: list[CanFrame]) -> list[float]:
    times = [f.time_ms for f in frames]
    return [b - a for a, b in zip(times, times[1:])]


def regen(frame: bytes) -> int:
    return int.from_bytes(frame[2:4], "little")


def zero_torque(frame: bytes) -> bool:
    return frame[0:4] == bytes(4)


# ---- the bug: a chattering brake line must not flood the bus ---------------------------------


def test_brake_chatter_does_not_flood_can_bus(board: Board):
    idle(board)
    start, end = chatter(board, duration_ms=500)
    frames = commands_between(board, start, end)
    edges = [e for e in board.gpio.edges(*BRK_IN) if start <= e.time_ms <= end and not e.high]
    # One command per FSM tick (~20 in 500 ms), not one per falling edge (~250)
    budget = (end - start) / FSM_PERIOD_MS + 2
    assert len(edges) > 200, f"chatter only produced {len(edges)} falling edges"
    assert len(frames) <= budget, f"{len(frames)} motor commands for {len(edges)} brake edges"


def test_motor_command_cadence_unchanged_during_brake_chatter(board: Board):
    idle(board)
    start, end = chatter(board, duration_ms=500)
    g = gaps(commands_between(board, start, end))
    assert len(g) >= 10, g
    assert min(g) >= MIN_GAP_MS, f"extra commands squeezed between FSM ticks: {sorted(g)[:5]}"
    assert max(g) < MDI_TIMEOUT_MS / 2, f"motor command starved, MDI would stop the motor: {g}"


def test_other_tasks_keep_running_during_brake_chatter(board: Board):
    idle(board)
    start, end = chatter(board, duration_ms=600)
    times = [f.time_ms for f in board.can.sent(DIAGNOSTIC_ID) if start <= f.time_ms <= end]
    g = [b - a for a, b in zip(times, times[1:])]
    assert len(g) >= 4 and all(g_ms <= 150 for g_ms in g), g  # normally every 100 ms


# ---- the brake still works, through the drive task alone ---------------------------------------


def test_brake_press_cuts_drive_within_one_fsm_tick(board: Board):
    idle(board)
    go_forward(board)
    pressed = board.time_ms
    board.gpio.set(*BRK_IN, high=True)
    board.wait(ms=200)
    after = commands_between(board, pressed, board.time_ms)
    first_zero = next(f for f in after if zero_torque(f.data))
    assert first_zero.time_ms - pressed <= NEXT_COMMAND_MS, (first_zero.time_ms - pressed)
    # From then on, every command is zero torque while the brake is held
    held = [f for f in after if f.time_ms >= first_zero.time_ms]
    assert all(zero_torque(f.data) for f in held), [f.data.hex(" ") for f in held]


def test_brake_release_sends_no_extra_command(board: Board):
    # The falling edge on release used to fire the ISR, which sent a zero-torque command even
    # though the brake had just been let go.
    idle(board)
    go_forward(board)
    board.gpio.set(*BRK_IN, high=True)
    board.wait(ms=200)
    released = board.time_ms
    board.gpio.set(*BRK_IN, high=False)
    board.wait(ms=200)
    after = commands_between(board, released, board.time_ms)
    assert after and all(regen(f.data) == REGEN_DAC_ON for f in after), [
        f"{f.time_ms - released:.2f} ms: {f.data.hex(' ')}" for f in after
    ]
    around = commands_between(board, released - 100, board.time_ms)
    assert min(gaps(around)) >= MIN_GAP_MS, gaps(around)


# ---- hardware-fault edge cases ------------------------------------------------------------------


def test_stuck_brake_holds_zero_torque_at_normal_rate(board: Board):
    # A brake line stuck high (shorted switch) is the fail-safe direction: no drive, but MDI keeps
    # getting commands on time and nothing floods the bus.
    idle(board)
    go_forward(board)
    board.gpio.set(*BRK_IN, high=True)
    board.wait(ms=NEXT_COMMAND_MS)  # let the drive task see it
    start = board.time_ms
    board.wait(ms=1000)
    frames = commands_between(board, start, board.time_ms)
    assert all(zero_torque(f.data) for f in frames), [f.data.hex(" ") for f in frames]
    g = gaps(frames)
    assert len(frames) <= 1000 / FSM_PERIOD_MS + 2, len(frames)
    assert min(g) >= MIN_GAP_MS and max(g) < MDI_TIMEOUT_MS / 2, g
    assert board.gpio.get(*BRK_OUT)


def test_brake_chatter_does_not_change_drive_state(board: Board):
    # Without a NEXT/PREV press, a bouncing brake must leave the car in FORWARD, and once the line
    # settles low the brake must not stay latched on.
    idle(board)
    go_forward(board)
    chatter(board, duration_ms=300)
    board.wait(ms=100)
    frame = board.can.wait_for(MOTOR_COMMAND_ID, within_ms=NEXT_COMMAND_MS).data
    assert frame[4] & FORWARD_BIT and regen(frame) == REGEN_DAC_ON, frame.hex(" ")
    assert not board.gpio.get(*BRK_OUT)
