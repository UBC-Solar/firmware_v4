import pytest

@pytest.fixture
def demo():
    print("SETUP")
    yield 42
    print("TEARDOWN")

def test_fail(demo):
    assert demo == 0

def test_time(board):
    print("before:", board.time_ms, "ms,", len(board.can.sent()), "frames")
    board.wait(ms=500)
    print("after: ", board.time_ms, "ms,", len(board.can.sent()), "frames")