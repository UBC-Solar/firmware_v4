from api import Board

def test_neutral_signal_init(board: Board):
    # Initialize as both high because passive high
    board.gpio.set("A", 0, True)
    board.gpio.set("A", 1, True) 
    board.wait(ms=300)

    frame = board.can.wait_for(0x580, within_ms=200)   # STRDRDControl
    left  = bool(frame.data[0] & (1 << 3))             # STRLeftTurnSignalEnabled
    right = bool(frame.data[0] & (1 << 4))             # STRRightTurnSignalEnabled
    assert (left, right) == (False, False)

    # Turn on left turn
    board.gpio.set("A", 0, False) 
    board.gpio.set("A", 1, True) 
    board.wait(ms=300)
    frame = board.can.wait_for(0x580, within_ms=200)   # STRDRDControl
    left  = bool(frame.data[0] & (1 << 3))             # STRLeftTurnSignalEnabled
    right = bool(frame.data[0] & (1 << 4))             # STRRightTurnSignalEnabled
    assert (left, right) == (True, False)

    # Set back to neutral
    board.gpio.set("A", 0, True)
    board.gpio.set("A", 1, True) 
    board.wait(ms=300)

    # Set to Right Turn
    board.gpio.set("A", 0, True)
    board.gpio.set("A", 1, False) 
    board.wait(ms=300)

    frame = board.can.wait_for(0x580, within_ms=200)   # STRDRDControl
    left  = bool(frame.data[0] & (1 << 3))             # STRLeftTurnSignalEnabled
    right = bool(frame.data[0] & (1 << 4))             # STRRightTurnSignalEnabled
    assert (left, right) == (False, True)


def test_lts_signal_init(board: Board):
    # Initialize as Left Turn Signal on
    board.gpio.set("A", 0, False) # Left Turn false for active low 
    board.gpio.set("A", 1, True) # Right Turn True for passive high
    board.wait(ms=300)

    frame = board.can.wait_for(0x580, within_ms=200)   # STRDRDControl
    left  = bool(frame.data[0] & (1 << 3))             # STRLeftTurnSignalEnabled
    right = bool(frame.data[0] & (1 << 4))             # STRRightTurnSignalEnabled
    assert (left, right) == (True, False)

    # Set to Neutral 
    board.gpio.set("A", 0, True) # Left Turn True for passive High
    board.gpio.set("A", 1, True) # Right Turn True for passive High
    board.wait(ms=300)
    frame = board.can.wait_for(0x580, within_ms=200)   # STRDRDControl
    left  = bool(frame.data[0] & (1 << 3))             # STRLeftTurnSignalEnabled
    right = bool(frame.data[0] & (1 << 4))             # STRRightTurnSignalEnabled
    assert (left, right) == (False, False)
