def test_boots_without_fault(board):
    board.run_for(ms=1000)
    assert board.symbol_at_pc() not in ("Error_Handler", "HardFault_Handler")
