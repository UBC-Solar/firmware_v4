from tel.radio import wait_for_frame, wait_for_header

TIME_SINCE_BOOTUP_ID = 0x750  # DLC 4, little-endian seconds counter, every 1 s
TEL_FLAGS_ID = 0x751  # DLC 1, every ~100 ms
CAN_BUS_LOAD_ID = 0x763
GPS_READ_FAIL = 0x08  # 0x751 bit 3


def test_flags_sent_every_100ms(board):
    # Ten frames ~100 ms apart; would stall after one if the UART DMA TX-complete path broke
    times = [wait_for_header(board, TEL_FLAGS_ID, within_ms=1000)]  # first one includes boot
    times += [wait_for_header(board, TEL_FLAGS_ID, within_ms=200) for _ in range(9)]
    gaps = [b - a for a, b in zip(times, times[1:])]
    assert all(80 <= g <= 130 for g in gaps), gaps


def test_flags_gps_read_failure_with_no_gps(board):
    # Nothing answers on I2C2 (u-blox at 0x42) in the sim -> gps_read_fail set in 0x751
    wait_for_frame(board, TEL_FLAGS_ID, bytes([GPS_READ_FAIL]), within_ms=3000)
    sent = [f.data for f in board.can_frames_sent(can_id=TEL_FLAGS_ID)]
    assert bytes([GPS_READ_FAIL]) in sent, sent


def test_time_since_bootup_increments(board):
    for seconds in (1, 2, 3):
        wait_for_frame(board, TIME_SINCE_BOOTUP_ID, seconds.to_bytes(4, "little"), within_ms=1500)


def test_diagnostics_sent_on_can(board):
    wait_for_frame(board, TIME_SINCE_BOOTUP_ID, (1).to_bytes(4, "little"), within_ms=1500)
    sent = board.can_frames_sent()
    ids = {f.can_id for f in sent}
    assert any(f.can_id == TIME_SINCE_BOOTUP_ID and f.data == bytes([1, 0, 0, 0]) for f in sent)
    assert any(f.can_id == TEL_FLAGS_ID and f.data == bytes([0x00]) for f in sent)
    assert CAN_BUS_LOAD_ID in ids, sorted(hex(i) for i in ids)
