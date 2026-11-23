#!/usr/bin/env bash
# Set up the sim on Linux, once per machine (safe to re-run, e.g. after pulling):
#   1. the pinned Renode build in sim/.renode/
#   2. the Python environment in sim/.venv/ (pytest, pyrenode3 ... from sim/requirements.txt)
# Both are gitignored. Windows: setup.ps1 next to this file does the same.
#
# Needs curl, git (pyrenode3 comes from GitHub) and Python 3.11-3.15 with venv
# (Ubuntu: apt install python3-venv). PYTHON=python3.12 sim/setup/setup.sh picks the interpreter.
#
# Why a pinned Renode, not "whatever Renode you have": the chip model
# (emulation/chip/stm32f103rct6.repl) uses peripheral classes that only exist in newer Renodes
# (the tagged 1.16.1 release has no F103 DMA or STM32F1_I2C). pytest and the VS Code "Renode:"
# tasks use sim/.renode/renode. Set PYRENODE_PATH to use another Renode folder (at your own risk).
#
# To move to a newer Renode, in BOTH setup.sh and setup.ps1: change RENODE_BUILD and the checksum
# (sha256sum of the Linux .tar.gz here, of the Windows .zip there). Re-run, copy
# sim/.renode/platforms/cpus/stm32f103.repl over sim/emulation/chip/stm32f103rct6.repl (never edit
# that copy; our fixes live in sim/emulation/chip/stm32f103rct6.resc), run the tests.
#
# Usage: sim/setup/setup.sh   (from anywhere)

set -euo pipefail

RENODE_BUILD="renode-1.16.1+20260623git3f5a91013"
RENODE_SHA256="75b7552761991f8a2e2ca1a31275f75eba4a5e2d61990258e377d0439f0e3d24"
URL="https://builds.renode.io/${RENODE_BUILD}.linux-portable-dotnet.tar.gz"

# Python versions pythonnet (in requirements.txt) supports: >=3.11, <3.16
PY_MIN_MINOR=11
PY_MAX_MINOR=15

SIM_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DEST="$SIM_DIR/.renode"
STAMP="$DEST/.build"
VENV="$SIM_DIR/.venv"
PYTHON="${PYTHON:-python3}"

TMP_DIR=""
trap '[[ -n "$TMP_DIR" ]] && rm -rf "$TMP_DIR"' EXIT

die() {
    echo "ERROR: $*" >&2
    exit 1
}

install_renode() {
    if [[ -f "$STAMP" && "$(cat "$STAMP")" == "$RENODE_BUILD" && -x "$DEST/renode" ]]; then
        echo "Renode $RENODE_BUILD already installed in $DEST"
        return
    fi
    command -v curl >/dev/null || die "curl not found (apt install curl)"

    TMP_DIR="$(mktemp -d)"
    echo "Downloading $URL (~100 MB)"
    curl -fL --progress-bar -o "$TMP_DIR/renode.tar.gz" "$URL"

    echo "$RENODE_SHA256  $TMP_DIR/renode.tar.gz" | sha256sum --check --quiet \
        || die "checksum mismatch on the Renode download: not installing"

    rm -rf "$DEST"
    mkdir -p "$DEST"
    # The tarball has one top folder (renode_<version>-portable/); put its contents straight in .renode/
    tar -xzf "$TMP_DIR/renode.tar.gz" -C "$DEST" --strip-components=1
    echo "$RENODE_BUILD" > "$STAMP"
    echo "Installed: $("$DEST/renode" --version | head -1)"
}

# Fails unless $1 is a Python the requirements support
check_python_version() {
    local py="$1" minor
    minor="$("$py" -c 'import sys; print(sys.version_info[1] if sys.version_info[0] == 3 else -1)')" \
        || die "can't run $py"
    if (( minor < PY_MIN_MINOR || minor > PY_MAX_MINOR )); then
        die "$py is Python $("$py" -c 'import platform; print(platform.python_version())'); need" \
            "3.$PY_MIN_MINOR-3.$PY_MAX_MINOR (pythonnet). Pick one with PYTHON=python3.X, and delete" \
            "$VENV first if it was made with the wrong one."
    fi
}

install_python_env() {
    command -v git >/dev/null || die "git not found (pip needs it to fetch pyrenode3)"
    if [[ ! -x "$VENV/bin/python" ]]; then
        command -v "$PYTHON" >/dev/null || die "$PYTHON not found (set PYTHON=...)"
        check_python_version "$PYTHON"
        echo "Creating Python environment in $VENV"
        "$PYTHON" -m venv "$VENV" || die "venv failed (Ubuntu: apt install python3-venv)"
    fi
    check_python_version "$VENV/bin/python"
    # Run every time: a no-op when requirements.txt hasn't changed, an update when it has
    echo "Installing Python packages (requirements.txt)"
    "$VENV/bin/python" -m pip install --quiet --disable-pip-version-check -r "$SIM_DIR/requirements.txt"
}

echo "== Renode"
install_renode
echo "== Python"
install_python_env

echo
echo "Done. Run the tests from the repo root:  sim/.venv/bin/pytest sim"
