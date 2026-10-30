#!/usr/bin/env bash
#
# One-time setup: install a udev rule so any PEAK PCAN-USB adapter is brought
# up at the car's bitrate automatically (at boot and on plug-in). After this,
# tools/candump.sh never needs sudo.
#
# Usage: sudo tools/setup_can_autoconfig.sh [bitrate]   (default: 500000)
#        sudo tools/setup_can_autoconfig.sh --uninstall

set -e  # Exit immediately if a command fails

RULE_FILE="/etc/udev/rules.d/80-ubc-solar-can.rules"
BITRATE="${1:-500000}"

if [ "$EUID" -ne 0 ]; then
    echo "Run with sudo: sudo $0 $*"
    exit 1
fi

if [ "$1" = "--uninstall" ]; then
    rm -f "$RULE_FILE"
    udevadm control --reload-rules
    echo "Removed $RULE_FILE"
    exit 0
fi

IP_BIN="$(command -v ip)"

echo "Installing $RULE_FILE (bitrate $BITRATE)..."
cat > "$RULE_FILE" <<EOF
# UBC Solar: auto-configure PEAK PCAN-USB adapters for the car's CAN bus.
# Installed by tools/setup_can_autoconfig.sh
ACTION=="add", SUBSYSTEM=="net", KERNEL=="can*", DRIVERS=="peak_usb", \\
    RUN+="$IP_BIN link set \$name down", \\
    RUN+="$IP_BIN link set \$name type can bitrate $BITRATE restart-ms 100", \\
    RUN+="$IP_BIN link set \$name up"
EOF

udevadm control --reload-rules

# Apply to any adapter that is already plugged in
udevadm trigger --action=add --subsystem-match=net --attr-match=type=280
udevadm settle

echo ""
for dev in /sys/class/net/can*; do
    [ -e "$dev" ] || continue
    ip -details link show "$(basename "$dev")" | grep -E "state|bitrate"
done

echo ""
echo "======================================"
echo " Done. CAN adapters will now come up"
echo " at $BITRATE bit/s automatically."
echo "======================================"
