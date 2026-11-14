"""CAR_CONFIG_CAN_MSG_ALL 1: TEL forwards every received CAN frame to telemetry."""

from _emulator import Board
from tel.radio import wait_for_frame

TEL_FLAGS_ID = 0x751


def booted(board: Board):
    wait_for_frame(board, TEL_FLAGS_ID, bytes([0x00]))


def test_forwards_standard_frame(board: Board):
    booted(board)
    data = bytes([0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88])
    board.send_can(0x123, data)
    wait_for_frame(board, 0x123, data, within_ms=500)


def test_forwards_short_frame_with_its_dlc(board: Board):
    booted(board)
    board.send_can(0x401, bytes([0xAB, 0xCD]))
    wait_for_frame(board, 0x401, bytes([0xAB, 0xCD]), dlc=2, within_ms=500)


def test_forwards_extended_frame(board: Board):
    booted(board)
    data = bytes([0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08])
    board.send_can(0x1ABCDEF0, data, extended=True)
    wait_for_frame(board, 0x1ABCDEF0, data, within_ms=500)
