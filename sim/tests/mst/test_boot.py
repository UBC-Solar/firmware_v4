from api import Board

STATUS_ID = 0x200       # app/mst_defs.h, every main loop
MODULE_VOLT_ID = 0x210  # first group: 4 modules x uint16 mV
MODULE_TEMP_ID = 0x220  # first group: 4 modules x uint16 degC
# The emulated ADBMS1818 chain (emulation/devices/adbms1818.py) reads every cell as 3.6 V and every
# thermistor as 25 degC.
CELL_MV, TEMP_DEGC = 3600, 25


def test_boots_without_fault(board: Board):
    board.wait(ms=1000)
    assert board.symbol_at_pc() not in ("Error_Handler", "HardFault_Handler", "IsoSpiErrorHandler_")


def test_status_sent_after_init(board: Board):
    board.wait(ms=2000)  # INIT_FAULT_HOLD_DURATION_MS
    board.can.wait_for(STATUS_ID, within_ms=1000)


def test_reports_module_voltages_and_temperatures(board: Board):
    board.wait(ms=2000)  # init hold, then NUM_INIT_MAINLOOPS before module data is sent
    volts = board.can.wait_for(MODULE_VOLT_ID, within_ms=3000).data
    temps = board.can.wait_for(MODULE_TEMP_ID, within_ms=1000).data
    assert [int.from_bytes(volts[i:i + 2], "little") for i in range(0, 8, 2)] == [CELL_MV] * 4
    assert [int.from_bytes(temps[i:i + 2], "little") for i in range(0, 8, 2)] == [TEMP_DEGC] * 4
    assert board.symbol_at_pc() != "IsoSpiErrorHandler_"
