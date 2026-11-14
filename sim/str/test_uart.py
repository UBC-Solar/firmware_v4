import pytest

from _emulator import Board

# Nothing in STR's firmware reads a UART yet, so this only checks uart.send() works. Real input
# tests come with the GPS device model (TEL).


def test_uart_send_reaches_emulation(board: Board):
    board.run_for(ms=100)
    board.uart.send("usart5", b"$GPGGA\r\n")
    board.run_for(ms=100)
    assert board.symbol_at_pc() not in ("Error_Handler", "HardFault_Handler")


def test_unknown_uart_is_rejected(board: Board):
    with pytest.raises(ValueError, match="Unknown UART 'usart9'"):
        board.uart.send("usart9", b"x")
