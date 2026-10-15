#!/usr/bin/env bash
#
# Bring up a SocketCAN interface (e.g. PEAK PCAN-USB) at the car's bitrate
# and run candump on it.
#
# Usage: tools/candump.sh [-i iface] [-b bitrate] [-l] [filter ...]
#   -i iface    CAN interface (default: can0)
#   -b bitrate  Bus bitrate in bit/s (default: 500000, matches all v4 boards)
#   -l          Also log to can_log_<timestamp>.log in the current directory
#   filter      Optional candump filters, e.g. 750:7FF 751:7FF
#
# Examples:
#   tools/candump.sh                  # dump everything on can0
#   tools/candump.sh -l               # dump and log to file
#   tools/candump.sh 750:7FF          # only TEL time-since-bootup

set -e  # Exit immediately if a command fails

IFACE="can0"
BITRATE=500000
LOG=0

usage() {
    sed -n '3,15p' "$0" | sed 's/^# \{0,1\}//'
    exit "${1:-0}"
}

while getopts "i:b:lh" opt; do
    case $opt in
        i) IFACE="$OPTARG" ;;
        b) BITRATE="$OPTARG" ;;
        l) LOG=1 ;;
        h) usage 0 ;;
        *) usage 1 ;;
    esac
done
shift $((OPTIND - 1))

if ! command -v candump >/dev/null; then
    echo "candump not found. Install it with: sudo apt install can-utils"
    exit 1
fi

if ! ip link show "$IFACE" >/dev/null 2>&1; then
    echo "Interface $IFACE not found. Is the CAN adapter plugged in?"
    exit 1
fi

# Reconfigure only if the interface is down or at the wrong bitrate
CURRENT_BITRATE=$(ip -details link show "$IFACE" | awk '/bitrate/ {print $2; exit}')
if ! ip link show "$IFACE" | grep -q "state UP" || [ "$CURRENT_BITRATE" != "$BITRATE" ]; then
    echo "Configuring $IFACE at $BITRATE bit/s (needs sudo)..."
    echo "Tip: run 'sudo tools/setup_can_autoconfig.sh' once to make this automatic."
    sudo ip link set "$IFACE" down
    sudo ip link set "$IFACE" type can bitrate "$BITRATE" restart-ms 100
    sudo ip link set "$IFACE" up
fi

ip -details link show "$IFACE" | grep -E "can state|bitrate"

# Build the candump interface argument, e.g. can0,750:7FF,751:7FF
DUMP_ARG="$IFACE"
for f in "$@"; do
    DUMP_ARG="$DUMP_ARG,$f"
done

echo "Listening on $DUMP_ARG (Ctrl+C to stop)"
echo ""

if [ "$LOG" -eq 1 ]; then
    LOGFILE="can_log_$(date +%Y%m%d_%H%M%S).log"
    echo "Logging to $LOGFILE"
    # Log file uses candump's replayable format (canplayer -I <file>)
    candump -L "$DUMP_ARG" > "$LOGFILE" &
    LOGGER_PID=$!
    trap 'kill $LOGGER_PID 2>/dev/null' EXIT
fi

candump -td -x "$DUMP_ARG"
