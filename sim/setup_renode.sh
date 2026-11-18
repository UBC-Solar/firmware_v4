#!/usr/bin/env bash
# Install the Renode build the sim is tested with into sim/.renode/ (gitignored).
#
# The sim needs this exact build, not "whatever Renode you have": the chip model
# (emulation/chip/stm32f103rct6.repl) uses peripheral classes that only exist in newer Renodes
# (the tagged 1.16.1 release has no F103 DMA or STM32F1_I2C).
#
# pytest and the VS Code "Renode:" tasks use sim/.renode/renode. Set PYRENODE_PATH to use another
# Renode folder instead (at your own risk).
#
# To move to a newer Renode: change RENODE_BUILD and RENODE_SHA256 (sha256sum of the tarball), re-run
# this script, copy .renode/platforms/cpus/stm32f103.repl over emulation/chip/stm32f103rct6.repl
# (never edit that copy; our fixes live in emulation/chip/stm32f103rct6.resc), run the tests.
#
# Usage: sim/setup_renode.sh   (from anywhere; safe to re-run)

set -euo pipefail

RENODE_BUILD="renode-1.16.1+20260623git3f5a91013"
RENODE_SHA256="75b7552761991f8a2e2ca1a31275f75eba4a5e2d61990258e377d0439f0e3d24"
URL="https://builds.renode.io/${RENODE_BUILD}.linux-portable-dotnet.tar.gz"

SIM_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEST="$SIM_DIR/.renode"
STAMP="$DEST/.build"

if [[ -f "$STAMP" && "$(cat "$STAMP")" == "$RENODE_BUILD" && -x "$DEST/renode" ]]; then
    echo "Renode $RENODE_BUILD already installed in $DEST"
    exit 0
fi

tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT

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
echo "Location:  $DEST"
