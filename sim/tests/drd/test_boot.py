import pytest

from api import Board

DIAGNOSTIC_ID = 0x403  # driver/can_driver.h, every 100 ms


def test_boots_without_fault(board: Board):
    board.wait(ms=1000)
    assert board.symbol_at_pc() not in ("Error_Handler", "HardFault_Handler")


def test_diagnostic_sent_every_100ms(board: Board):
    board.wait(ms=1000)
    times = [f.time_ms for f in board.can.sent(DIAGNOSTIC_ID)]
    gaps = [b - a for a, b in zip(times, times[1:])]
    assert len(gaps) >= 8 and all(90 <= g <= 110 for g in gaps), gaps


@pytest.mark.xfail(
    reason="Known firmware bug: defaultTask isn't created (configTOTAL_HEAP_SIZE 3072 exhausted, "
    "64 bytes left after CAN_comms)",
    strict=False,
)
@pytest.mark.rtos_trace
def test_every_rtos_task_runs(board: Board):
    # The CAN Rx task only loops when a frame arrives
    for _ in range(3):
        board.can.send(0x123, bytes([0x11, 0x22]))
        board.wait(ms=50)
    board.wait(ms=3000)  # slowest loop is TasksTimeSinceStartup (1 s)
    loops = board.rtos.task_loops()
    assert loops, "No osThreadNew calls seen"
    stuck = {task: n for task, n in loops.items() if n < 2}
    assert not stuck, f"Tasks with fewer than 2 loops: {stuck} (all: {loops})"
