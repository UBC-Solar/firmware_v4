from api import Board

TIME_SINCE_BOOTUP_ID = 0x500  # driver/can_driver.h, diagnostics every 1000 ms


def test_boots_without_fault(board: Board):
    board.wait(ms=1000)
    assert board.symbol_at_pc() not in ("Error_Handler", "HardFault_Handler")


def test_time_since_bootup_sent_every_second(board: Board):
    board.wait(ms=3500)
    times = [f.time_ms for f in board.can.sent(TIME_SINCE_BOOTUP_ID)]
    gaps = [b - a for a, b in zip(times, times[1:])]
    assert len(gaps) >= 2 and all(990 <= g <= 1010 for g in gaps), gaps
