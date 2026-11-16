"""TEL telemetry frame helpers for the tests in this folder (not a test file).

Frame (TEL_Msg_TypeDef, 24 bytes, packed):
    [8 timestamp][0x23 '#'][4 CAN ID, big-endian][8 data][1 DLC][0D 0A]
The RTC stub doesn't tick, so the timestamp is constant: helpers match from '#' onwards.
"""

import re
from pathlib import Path

from api import Board, SimTimeout

CAR_CONFIGS = Path(__file__).resolve().parents[3] / "firmware/common/car_configs/car_configs.h"


def telemetry_uart() -> str:
    """UART TEL sends frames on: CAR_CONFIG_CELLULAR 1 -> USART2 (cellular), 0 -> UART4 (radio)."""
    match = re.search(r"#define\s+CAR_CONFIG_CELLULAR\s+(\d+)", CAR_CONFIGS.read_text())
    return "usart2" if match and match.group(1) == "1" else "usart4"


def _wait_on_telemetry_uart(board: Board, data: bytes, within_ms: float) -> float:
    """wait_for_uart on the UART car_configs.h names. If that times out while TEL sent on the other
    UART, the ELF was built with a different CAR_CONFIG_CELLULAR than the header now says (header
    edited without rebuilding, or an ELF from another branch): say so instead of a bare timeout."""
    uart = telemetry_uart()
    try:
        return board.wait_for_uart(uart, data, within_ms)
    except SimTimeout as timeout:
        other = "usart4" if uart == "usart2" else "usart2"
        if board.uart.received(other) and not board.uart.received(uart):
            names = {"usart2": "usart2 (cellular)", "usart4": "usart4 (radio)"}
            raise SimTimeout(
                f"TEL is sending on {names[other]}, but car_configs.h says CAR_CONFIG_CELLULAR "
                f"{1 if uart == 'usart2' else 0}, so {names[uart]}. The ELF doesn't match the "
                "header: rebuild TEL."
            ) from timeout
        raise


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
    return _wait_on_telemetry_uart(board, frame_bytes(can_id, data, dlc), within_ms)


def wait_for_header(board: Board, can_id: int, within_ms: float = 3000) -> float:
    """Run until TEL sends any frame with this ID on the telemetry UART."""
    return _wait_on_telemetry_uart(board, frame_header(can_id), within_ms)
