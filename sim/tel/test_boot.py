import pytest

from tel.radio import wait_for_frame

TEL_FLAGS_ID = 0x751


def test_boots_without_fault(board):
    # First flags frame proves the scheduler, clocks, CAN and UART init all made it through
    wait_for_frame(board, TEL_FLAGS_ID, bytes([0x00]), within_ms=3000)
    board.run_for(ms=2000)
    assert board.symbol_at_pc() not in ("Error_Handler", "HardFault_Handler")


@pytest.mark.xfail(
    reason="Known firmware bugs: defaultTask isn't created (configTOTAL_HEAP_SIZE 3072 exhausted); "
    "the IMU task busy-waits on HAL_MAX_DELAY I2C calls with no IMU attached, which also starves "
    "TasksIWDG (osPriorityLow, below the IMU's Low1) so the watchdog is never refreshed",
    strict=False,
)
def test_every_rtos_task_runs(board):
    """Every task the firmware creates must keep looping. Catches tasks that fail to be created
    (heap), hang (e.g. HAL_MAX_DELAY on a dead bus) or starve."""
    wait_for_frame(board, TEL_FLAGS_ID, bytes([0x00]))
    # The CAN Rx task only loops when a frame arrives
    for _ in range(3):
        board.send_can(0x123, bytes([0x11, 0x22]))
        wait_for_frame(board, 0x123, bytes([0x11, 0x22]))
    # Slowest loops are TimeSinceStartup and GPS (~1 s; GPS slower with no device)
    board.run_for(ms=3000)
    loops = board.rtos_task_loops()
    assert loops, "No osThreadNew calls seen"
    stuck = {task: n for task, n in loops.items() if n < 2}
    assert not stuck, f"Tasks with fewer than 2 loops: {stuck} (all: {loops})"
