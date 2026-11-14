from _emulator import Board

DEBUG_LED = ("C", 3)  # cube/Core/Inc/main.h, toggled by DiagnosticTimeSinceBootup()
TIME_SINCE_BOOTUP_DELAY_MS = 1000  # runtime/tasks.h


def test_debug_led_toggles_every_second(board: Board):
    board.run_for(ms=3500)
    edges = board.gpio.edges(*DEBUG_LED)
    assert len(edges) >= 3, edges
    gaps = [b.time_ms - a.time_ms for a, b in zip(edges, edges[1:])]
    assert all(900 <= g <= 1100 for g in gaps), gaps
    assert all(a.high != b.high for a, b in zip(edges, edges[1:])), edges


def test_debug_led_turns_off_within_a_period(board: Board):
    board.gpio.wait_for(*DEBUG_LED, high=True, within_ms=100)  # first toggle right after boot
    off_at = board.gpio.wait_for(*DEBUG_LED, high=False, within_ms=1500)
    assert not board.gpio.get(*DEBUG_LED)
    assert 900 <= off_at <= 1100, off_at
