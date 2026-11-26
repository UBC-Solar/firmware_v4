import re

import pytest

from api import Board, build_firmware


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
        "--no-build",
        action="store_true",
        help="Don't build the boards under test first (run whatever ELF is already there)",
    )
    parser.addoption(
        "--sim-trace",
        action="store_true",
        help="Print each test step (file:line, action, virtual time) to the terminal; on with --gdb",
    )


def pytest_collectstart(collector):
    """Tests use only `api`. `emulation` is the backend behind it: importing it from a test (or a
    helper next to the tests) would tie the test to Renode, so it's refused at collection."""
    path = getattr(collector, "path", None)
    if path is None or path.suffix != ".py" or path.name == "conftest.py":
        return
    source = path.read_text()
    if re.search(r"^\s*(from|import)\s+emulation\b", source, re.MULTILINE):
        raise pytest.UsageError(
            f"{path}: tests import from `api` only, not `emulation`. "
            "If the test needs something api doesn't have yet, add it to api/."
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
    build_boards(config, items)


def build_boards(config, items):
    """Build every board that has a selected test, so a test never runs stale firmware.
    The test's folder names the board: sim/tests/str/test_x.py -> "str"."""
    if config.getoption("--no-build") or config.getoption("--collect-only"):
        return
    build_type = config.getoption("--build-type")
    reporter = config.pluginmanager.getplugin("terminalreporter")
    for board in sorted({item.path.parent.name for item in items}):
        if reporter is not None:
            reporter.write_line(f"Building {board} ({build_type}) ...")
        try:
            build_firmware(board, build_type)
        except RuntimeError as err:
            raise pytest.UsageError(str(err))


@pytest.fixture
def board(request):
    """Fresh emulated board. The test's folder picks which: sim/tests/str/test_x.py -> "str"."""
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
