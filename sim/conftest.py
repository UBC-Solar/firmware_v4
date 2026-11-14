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
    parser.addoption(
        "--sim-trace",
        action="store_true",
        help="Print each test step (file:line, action, virtual time) to the terminal; on with --gdb",
    )


@pytest.fixture
def board(request):
    """Fresh emulated board. The test's folder picks which: sim/str/test_x.py -> "str"."""
    name = request.path.parent.name
    capture = request.config.pluginmanager.getplugin("capturemanager")

    def say(line):
        # Briefly switch off pytest's output capture so the line reaches the terminal live
        with capture.global_and_fixture_disabled():
            print(line, flush=True)

    b = Board(
        name,
        gdb=request.config.getoption("--gdb"),
        build_type=request.config.getoption("--build-type"),
        trace=request.config.getoption("--sim-trace"),
        say=say,
    )
    yield b
    b.close()
