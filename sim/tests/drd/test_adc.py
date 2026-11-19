from api import Board

def test_2_adcs(board: Board):
    # Set the default adc channel (One used for accel to raw = 15)
    board.adc.set(1, channel=8, raw=15)
    # Wait to allow for debugging
    board.wait(10)
    # Set a breakpoint at readadc in accel_driver. Then uncomment MCU_HEALTH in tasks.c, REBUILD, and run this again. What happens?