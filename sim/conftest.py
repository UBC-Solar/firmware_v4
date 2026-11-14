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


@pytest.hookimpl(trylast=True)  # after -k / -m have removed the deselected tests
def pytest_collection_modifyitems(config, items):
    """--gdb debugs one test at a time: every test boots a fresh machine with its own GDB server,
    and the debugger's connection ends with the test, so a second test would wait forever."""
    if config.getoption("--gdb") and len(items) > 1:
        tests = "\n  ".join(item.nodeid for item in items)
        raise pytest.UsageError(
            f"--gdb debugs one test at a time, but {len(items)} are selected:\n  {tests}\n"
            "Pick one: pytest <file>::<test_name> --gdb  (or -k <part of the name>).\n"
            "In VS Code: type the test's name when F5 asks for it."
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
