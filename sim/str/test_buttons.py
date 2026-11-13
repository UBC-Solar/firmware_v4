import pytest

STR_CAN_MSG_ID = 0x580  # driver/can_driver.h


@pytest.mark.skip(reason="TODO: check NEXT_PAGE active level and which 0x580 byte holds the page")
def test_next_page_button_changes_can_output(board):
    board.run_for(ms=500)
    before = board.wait_for_can(STR_CAN_MSG_ID, within_ms=200).data
    board.set_pin("A", 2, high=False)  # press NEXT_PAGE (PA2)
    board.run_for(ms=50)
    board.set_pin("A", 2, high=True)
    after = board.wait_for_can(STR_CAN_MSG_ID, within_ms=200).data
    assert after != before
