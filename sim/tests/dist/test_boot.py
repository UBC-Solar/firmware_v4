from api import Board

HEARTBEAT_ID = 0x3A0  # driver/can_driver.h, every 500 ms (app/fsm.c)


def test_boots_without_fault(board: Board):
    board.wait(ms=1000)
    assert board.symbol_at_pc() not in ("Error_Handler", "HardFault_Handler")


def test_heartbeat_sent_every_500ms(board: Board):
    board.wait(ms=2600)
    times = [f.time_ms for f in board.can.sent(HEARTBEAT_ID)]
    gaps = [b - a for a, b in zip(times, times[1:])]
    assert len(gaps) >= 3 and all(490 <= g <= 510 for g in gaps), gaps
