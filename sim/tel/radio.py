"""TEL telemetry frame helpers for the tests in this folder (not a test file).

Frame (TEL_Msg_TypeDef, 24 bytes, packed):
    [8 timestamp][0x23 '#'][4 CAN ID, big-endian][8 data][1 DLC][0D 0A]
The RTC stub doesn't tick, so the timestamp is constant: helpers match from '#' onwards.
"""

import re
from pathlib import Path

from _emulator import Board

CAR_CONFIGS = Path(__file__).resolve().parents[2] / "firmware/common/car_configs/car_configs.h"


def telemetry_uart() -> str:
    """UART TEL sends frames on: CAR_CONFIG_CELLULAR 1 -> USART2 (cellular), 0 -> UART4 (radio)."""
    match = re.search(r"#define\s+CAR_CONFIG_CELLULAR\s+(\d+)", CAR_CONFIGS.read_text())
    return "usart2" if match and match.group(1) == "1" else "usart4"


def frame_bytes(can_id: int, data: bytes = b"", dlc: int | None = None) -> bytes:
    """The bytes of a frame from '#' to the end. Data is zero-padded to 8; DLC defaults to len(data)."""
    dlc = len(data) if dlc is None else dlc
    return b"#" + can_id.to_bytes(4, "big") + data.ljust(8, b"\x00") + bytes([dlc]) + b"\r\n"


def frame_header(can_id: int) -> bytes:
    """Just '#' + ID: matches a frame for this ID whatever its data."""
    return b"#" + can_id.to_bytes(4, "big")


def wait_for_frame(board: Board, can_id: int, data: bytes = b"", dlc: int | None = None,
                   within_ms: float = 3000) -> float:
    """Run until TEL sends exactly this frame on the telemetry UART."""
    return board.wait_for_uart(telemetry_uart(), frame_bytes(can_id, data, dlc), within_ms)


def wait_for_header(board: Board, can_id: int, within_ms: float = 3000) -> float:
    """Run until TEL sends any frame with this ID on the telemetry UART."""
    return board.wait_for_uart(telemetry_uart(), frame_header(can_id), within_ms)
