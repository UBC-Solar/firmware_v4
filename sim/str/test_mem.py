from _emulator import Board

NEXT_PAGE = ("A", 2)  # active low, EXTI on both edges (cube/Core/Src/gpio.c)
NEXT_PAGE_OFFSET = 2  # StrGpioCtx.next_page (driver/gpio_driver.h): horn_en, ptt_en, next_page, ...


def test_gpio_pin_state_is_whole_struct(board: Board):
    board.run_for(ms=100)
    assert len(board.mem.read_bytes("gpio_pin_state")) == 9  # StrGpioCtx: 9 bools


def test_next_page_press_toggles_state(board: Board):
    board.gpio.set(*NEXT_PAGE, high=True)  # idle; Renode has no pull-ups, so pins start low
    board.run_for(ms=500)
    before = board.mem.read_bytes("gpio_pin_state", offset=NEXT_PAGE_OFFSET, size=1)[0]
    board.gpio.set(*NEXT_PAGE, high=False)  # press
    board.run_for(ms=50)
    after = board.mem.read_bytes("gpio_pin_state", offset=NEXT_PAGE_OFFSET, size=1)[0]
    assert after == (not before), (before, after)


def test_write_sets_time_since_bootup(board: Board):
    board.run_for(ms=500)
    board.mem.write("g_time_since_bootup", 1000)
    board.run_for(ms=1000)  # TasksTimeSinceBootUp increments it once a second
    assert board.mem.read("g_time_since_bootup") == 1001
