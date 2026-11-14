from _emulator import Board


def test_boots_without_fault(board: Board):
    board.run_for(ms=1000)
    assert board.symbol_at_pc() not in ("Error_Handler", "HardFault_Handler")


def test_every_rtos_task_runs(board: Board):
    # The CAN Rx task only loops when a frame arrives
    for _ in range(3):
        board.send_can(0x123, bytes([0x11, 0x22]))
        board.run_for(ms=50)
    # Slowest loop is TimeSinceBootUp (1 s)
    board.run_for(ms=3000)
    loops = board.rtos_task_loops()
    assert loops, "No osThreadNew calls seen"
    stuck = {task: n for task, n in loops.items() if n < 2}
    assert not stuck, f"Tasks with fewer than 2 loops: {stuck} (all: {loops})"
