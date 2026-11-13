import pytest

from _emulator import Board


def pytest_addoption(parser):
    parser.addoption(
        "--gdb",
        action="store_true",
        help="Start Renode's GDB server and wait for a debugger before running the firmware",
    )
    parser.addoption(
        "--build-type",
        default="Debug",
        choices=["Debug", "Release"],
        help="Which firmware build to test (read from each build folder's CMakeCache.txt)",
    )


@pytest.fixture
def board(request):
    """Fresh emulated board. The test's folder picks which: sim/str/test_x.py -> "str"."""
    name = request.path.parent.name
    try:
        b = Board(
            name,
            gdb=request.config.getoption("--gdb"),
            build_type=request.config.getoption("--build-type"),
        )
    except NotImplementedError as e:
        pytest.skip(f"Emulator not ready: {e}")
    yield b
    b.close()
