#!/usr/bin/env bash
# Set up the sim, once per machine (safe to re-run, e.g. after pulling):
#   1. the pinned Renode build in sim/.renode/
#   2. the Python environment in sim/.venv/ (pytest, pyrenode3 ... from requirements.txt)
# Both are gitignored. Needs curl, git (pyrenode3 comes from GitHub) and python3 with venv
# (Ubuntu: apt install python3-venv). PYTHON=python3.12 sim/setup.sh picks another interpreter.
#
# Why a pinned Renode, not "whatever Renode you have": the chip model
# (emulation/chip/stm32f103rct6.repl) uses peripheral classes that only exist in newer Renodes
# (the tagged 1.16.1 release has no F103 DMA or STM32F1_I2C). pytest and the VS Code "Renode:"
# tasks use sim/.renode/renode. Set PYRENODE_PATH to use another Renode folder (at your own risk).
#
# To move to a newer Renode: change RENODE_BUILD and RENODE_SHA256 (sha256sum of the tarball), re-run
# this script, copy .renode/platforms/cpus/stm32f103.repl over emulation/chip/stm32f103rct6.repl
# (never edit that copy; our fixes live in emulation/chip/stm32f103rct6.resc), run the tests.
#
# Usage: sim/setup.sh   (from anywhere)

set -euo pipefail

RENODE_BUILD="renode-1.16.1+20260623git3f5a91013"
RENODE_SHA256="75b7552761991f8a2e2ca1a31275f75eba4a5e2d61990258e377d0439f0e3d24"
URL="https://builds.renode.io/${RENODE_BUILD}.linux-portable-dotnet.tar.gz"

SIM_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEST="$SIM_DIR/.renode"
STAMP="$DEST/.build"

PYTHON="${PYTHON:-python3}"

install_renode() {
    if [[ -f "$STAMP" && "$(cat "$STAMP")" == "$RENODE_BUILD" && -x "$DEST/renode" ]]; then
        echo "Renode $RENODE_BUILD already installed in $DEST"
        return
    fi

    local tmp
    tmp="$(mktemp -d)"
    trap 'rm -rf "$tmp"' RETURN

    echo "Downloading $URL"
    curl -fL --progress-bar -o "$tmp/renode.tar.gz" "$URL"

    echo "$RENODE_SHA256  $tmp/renode.tar.gz" | sha256sum --check --quiet || {
        echo "Checksum mismatch: not installing" >&2
        exit 1
    }

    rm -rf "$DEST"
    mkdir -p "$DEST"
    # The tarball has one top folder (renode_<version>-portable/); put its contents straight in .renode/
    tar -xzf "$tmp/renode.tar.gz" -C "$DEST" --strip-components=1
    echo "$RENODE_BUILD" > "$STAMP"
    echo "Installed: $("$DEST/renode" --version | head -1)"
}

install_python_env() {
    local venv="$SIM_DIR/.venv"
    if [[ ! -x "$venv/bin/python" ]]; then
        echo "Creating Python environment in $venv"
        "$PYTHON" -m venv "$venv"
    fi
    # Re-run every time: a no-op when requirements.txt hasn't changed, an update when it has
    echo "Installing Python packages (requirements.txt)"
    "$venv/bin/pip" install --quiet --disable-pip-version-check -r "$SIM_DIR/requirements.txt"
}

echo "== Renode"
install_renode
echo "== Python"
install_python_env

echo
echo "Done. Run the tests from the repo root:  sim/.venv/bin/pytest sim"
