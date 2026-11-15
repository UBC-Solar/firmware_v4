from _emulator import Board

STATE_ID = 0x324  # app/can_messages.h, data[0] = g_state, every 200 ms
FAULT_ID = 0x321  # 3 bytes of fault flags (SendFaults() in app/can_messages.c)
TEL_HEARTBEAT_ID = 0x750
STATE_TEL_STARTUP, STATE_MST_HANDSHAKE, STATE_FAULT = 1, 2, 13  # app/hvc_types.h
TEL_STARTUP_TIMEOUT = 0x10  # 0x321 byte 1 bit 4


def test_boots_without_fault(board: Board):
    board.wait(ms=1000)
    assert board.symbol_at_pc() not in ("Error_Handler", "HardFault_Handler")


def test_waits_for_tel_heartbeat(board: Board):
    frame = board.can.wait_for(STATE_ID, within_ms=300)
    assert frame.data[0] == STATE_TEL_STARTUP, frame


def test_no_tel_heartbeat_faults_after_3s(board: Board):
    board.wait(ms=2900)
    assert board.can.wait_for(STATE_ID, within_ms=300).data[0] == STATE_TEL_STARTUP
    board.wait(ms=300)  # TEL_STARTUP_TIMEOUT_MS = 3000
    assert board.can.wait_for(STATE_ID, within_ms=300).data[0] == STATE_FAULT
    faults = board.can.wait_for(FAULT_ID, within_ms=300).data
    assert faults[1] & TEL_STARTUP_TIMEOUT, faults.hex(" ")


def test_tel_heartbeat_starts_mst_handshake(board: Board):
    board.wait(ms=500)
    board.can.send(TEL_HEARTBEAT_ID, bytes(4))
    board.wait(ms=100)
    assert board.can.wait_for(STATE_ID, within_ms=300).data[0] == STATE_MST_HANDSHAKE
