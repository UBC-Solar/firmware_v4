import can
import time
import threading
import struct
import platform
import os
import glob
import sys

"""
MDI LED / output bench test.

Simulates DRD (0x401 motor command) and STR (0x580 steering control) and walks
through each MDI output, waiting for the tester to confirm it by eye or multimeter.

0x401 (DRD -> MDI), 5 bytes, little endian:
    [0:1] accel DAC value   (kept at 0 by this script)
    [2:3] regen DAC value   (kept at 0 by this script)
    [4]   bit0 = direction (1 = REV, 0 = FWD), bit1 = eco (1 = ECO, 0 = POWER)

0x580 (STR -> DRD/MDI), byte 0:
    bit0 = regen_en (REGEN LED), bit1 cruise, bit2 next_page, ...

MDI -> bus:
    0x500 time since bootup (u32), 0x501 diagnostic flags, 0x502 RTD temp

Run with the motor controller DISCONNECTED or the wheels off the ground.
"""

BITRATE = 500000

DRD_MOTOR_COMMAND_CAN_ID = 0x401
STR_CONTROL_CAN_ID = 0x580
MDI_TIME_SINCE_BOOTUP_CAN_ID = 0x500
MDI_DIAGNOSTIC_FLAGS_CAN_ID = 0x501
MDI_MOTOR_TEMP_CAN_ID = 0x502

MOTOR_COMMAND_PERIOD_S = 0.05   # must stay well under MDI_MAX_TIMEOUT_VALUE (100 ms)

DIAG_FLAG_NAMES = [
    "crash_iwdg",
    "voltage_over_threshold",
    "motor_over_temp",
    "rtd_fault",
    "rtd_comm_error",
]


# Checks system and tries to find a CAN interface (same as drd/test/test_scripts/lcd_test.py)
def _try_bus(channel, interface):
    try:
        bus = can.interface.Bus(channel=channel, interface=interface, bitrate=BITRATE)
        print(f"Using CAN interface={interface} channel={channel}")
        return bus
    except Exception as e:
        print(f"  skip {interface}/{channel}: {e}")
        return None

def select_can_interface():
    env_iface = os.environ.get("CAN_INTERFACE")
    env_channel = os.environ.get("CAN_CHANNEL")
    if env_iface and env_channel:
        bus = _try_bus(env_channel, env_iface)
        if bus is not None:
            return bus
        raise RuntimeError(f"Failed to open CAN_INTERFACE={env_iface} CAN_CHANNEL={env_channel}")

    system = platform.system()

    if system == "Windows":
        candidates = [
            ("PCAN_USBBUS1", "pcan"),
            ("COM3", "slcan"),
        ]
    elif system == "Darwin":  # for mac
        candidates = [
            ("PCAN_USBBUS1", "pcan"),
        ]
        for port in sorted(glob.glob("/dev/cu.usbmodem*") + glob.glob("/dev/cu.usbserial*")):
            candidates.append((port, "slcan"))
    else:  # Linux
        candidates = [
            ("can0", "socketcan"),
            ("PCAN_USBBUS1", "pcan"),
            ("/dev/ttyACM0", "slcan"),
        ]

    for channel, interface in candidates:
        bus = _try_bus(channel, interface)
        if bus is not None:
            return bus

    raise RuntimeError(
        "No CAN adapter found. Plug in your adapter, or set:\n"
        "  CAN_INTERFACE=pcan CAN_CHANNEL=PCAN_USBBUS1\n"
        "  CAN_INTERFACE=slcan CAN_CHANNEL=/dev/cu.usbmodemXXXX\n"
        "  CAN_INTERFACE=socketcan CAN_CHANNEL=can0"
    )


def send_message(bus, can_id, data, isextended_id=False):
    message = can.Message(arbitration_id=can_id, data=data, is_extended_id=isextended_id)
    try:
        bus.send(message)
    except can.CanError as e:
        print(f"Message NOT sent {e}")


def build_motor_command(accel=0, regen=0, reverse=False, eco=True):
    flags = (1 if reverse else 0) | ((1 if eco else 0) << 1)
    return [
        accel & 0xFF, (accel >> 8) & 0xFF,
        regen & 0xFF, (regen >> 8) & 0xFF,
        flags,
    ]


class MotorCommandSender(threading.Thread):
    """Sends 0x401 periodically like the DRD does, so the MDI timeout doesn't trip."""

    def __init__(self, bus):
        super().__init__(daemon=True)
        self.bus = bus
        self.lock = threading.Lock()
        self.payload = build_motor_command()
        self.enabled = threading.Event()
        self.stopped = threading.Event()

    def set_state(self, reverse, eco):
        with self.lock:
            self.payload = build_motor_command(reverse=reverse, eco=eco)

    def run(self):
        while not self.stopped.is_set():
            if self.enabled.is_set():
                with self.lock:
                    payload = list(self.payload)
                send_message(self.bus, DRD_MOTOR_COMMAND_CAN_ID, payload)
            time.sleep(MOTOR_COMMAND_PERIOD_S)


class MdiListener(can.Listener):
    """Keeps the latest MDI diagnostic frames so the test can report them."""

    def __init__(self):
        self.uptime = None
        self.flags = None
        self.rtd = None
        self.last_rx = None

    def on_message_received(self, msg):
        if msg.is_extended_id:
            return
        if msg.arbitration_id == MDI_TIME_SINCE_BOOTUP_CAN_ID and len(msg.data) >= 4:
            self.uptime = struct.unpack_from("<I", msg.data, 0)[0]
            self.last_rx = time.time()
        elif msg.arbitration_id == MDI_DIAGNOSTIC_FLAGS_CAN_ID and len(msg.data) >= 1:
            self.flags = msg.data[0]
            self.last_rx = time.time()
        elif msg.arbitration_id == MDI_MOTOR_TEMP_CAN_ID and len(msg.data) >= 6:
            ok = msg.data[0]
            temp_c = struct.unpack_from("<i", msg.data, 1)[0]
            self.rtd = (ok, temp_c, msg.data[5])
            self.last_rx = time.time()


class MDITest:
    def __init__(self, bus):
        self.bus = bus
        self.sender = MotorCommandSender(bus)
        self.listener = MdiListener()
        self.notifier = can.Notifier(bus, [self.listener])
        self.results = []

    def confirm(self, name, expected):
        print(f"\n[{name}]")
        for line in expected:
            print(f"  expect: {line}")
        answer = input("  pass? [Y/n] ").strip().lower()
        passed = answer in ("", "y", "yes")
        self.results.append((name, passed))

    def send_str(self, regen_en):
        send_message(self.bus, STR_CONTROL_CAN_ID, [0x01 if regen_en else 0x00, 0, 0, 0, 0, 0, 0, 0])

    def run_test(self):
        self.sender.start()
        self.sender.set_state(reverse=False, eco=True)
        self.sender.enabled.set()
        time.sleep(0.3)

        self.test_heartbeat_and_diagnostics()
        self.test_direction_leds()
        self.test_eco_power()
        self.test_regen_led()
        self.test_timeout()

        self.sender.stopped.set()
        self.notifier.stop()
        self.print_summary()

    def test_heartbeat_and_diagnostics(self):
        print("\n=== Heartbeat / diagnostics ===")
        print("Listening for MDI diagnostics for 2.5 s...")
        time.sleep(2.5)
        if self.listener.last_rx is None:
            print("  No 0x500/0x501/0x502 frames received from MDI!")
        else:
            print(f"  0x500 uptime   : {self.listener.uptime}")
            if self.listener.flags is not None:
                active = [n for i, n in enumerate(DIAG_FLAG_NAMES) if self.listener.flags & (1 << i)]
                print(f"  0x501 flags    : 0x{self.listener.flags:02X} {active if active else '(none)'}")
            if self.listener.rtd is not None:
                ok, temp_c, faults = self.listener.rtd
                print(f"  0x502 RTD      : read_ok={ok} temp={temp_c} C faults=0x{faults:02X}")
        self.confirm("Heartbeat", [
            "DEBUG_LED (PC12) blinks at ~1 Hz",
            "MDI diagnostics printed above (uptime increasing)",
            "RTD_LED (PD2) ON only if RTD faults != 0",
        ])

    def test_direction_leds(self):
        print("\n=== Direction LEDs ===")
        self.sender.set_state(reverse=False, eco=True)
        time.sleep(0.3)
        self.confirm("Forward", ["FWD_LED ON", "REV_LED OFF"])

        self.sender.set_state(reverse=True, eco=True)
        time.sleep(0.3)
        self.confirm("Reverse", ["REV_LED ON", "FWD_LED OFF"])

        print("\nToggling FWD/REV every 0.5 s for 4 s...")
        for i in range(8):
            self.sender.set_state(reverse=(i % 2 == 0), eco=True)
            time.sleep(0.5)
        self.sender.set_state(reverse=False, eco=True)
        time.sleep(0.3)
        self.confirm("Direction toggle", [
            "FWD/REV alternated cleanly, never both on, never both off",
            "Now ends on FWD_LED ON",
        ])

    def test_eco_power(self):
        print("\n=== ECO / POWER (opto, no LED) ===")
        print("Put a multimeter in continuity mode across PIN_5 / PIN_6")
        print("(or measure across R5.2: ~2 V = opto on, 0 V = off)")
        input("Press Enter when ready...")

        self.sender.set_state(reverse=False, eco=True)
        time.sleep(0.3)
        self.confirm("ECO", ["PIN_5/6 OPEN (no continuity), ~0 V across R5.2"])

        self.sender.set_state(reverse=False, eco=False)
        time.sleep(0.3)
        self.confirm("POWER", ["PIN_5/6 CLOSED (continuity), ~2 V across R5.2"])

        self.sender.set_state(reverse=False, eco=True)
        time.sleep(0.3)
        self.confirm("Back to ECO", ["PIN_5/6 OPEN again"])

    def test_regen_led(self):
        print("\n=== REGEN LED (STR 0x580 bit0) ===")
        print("Make sure the real STR is NOT on the bus, or it will override these frames.")
        input("Press Enter when ready...")

        self.send_str(regen_en=True)
        time.sleep(0.3)
        self.confirm("Regen on", [
            "REGEN_LED ON",
            "FWD_LED still ON (STR frame must not touch motor state)",
        ])

        self.send_str(regen_en=False)
        time.sleep(0.3)
        self.confirm("Regen off", ["REGEN_LED OFF"])

        print("\nSending STR frames with junk in bytes 1-3 while DRD commands accel=0...")
        print("Measure the ACCEL and REGEN DAC outputs (DAC7571 VOUT) while this runs.")
        input("Press Enter to start (3 s)...")
        end = time.time() + 3
        while time.time() < end:
            send_message(self.bus, STR_CONTROL_CAN_ID, [0x01, 0xFF, 0xFF, 0xFF, 0, 0, 0, 0])
            time.sleep(0.02)
        self.send_str(regen_en=False)
        self.confirm("STR doesn't drive DACs", [
            "Both DAC outputs stayed at ~0 V the whole time",
        ])

    def test_timeout(self):
        print("\n=== 100 ms command timeout ===")
        self.sender.set_state(reverse=True, eco=False)
        time.sleep(0.3)
        print("State is now REV + POWER (PIN_5/6 closed, REV_LED on).")
        input("Press Enter to STOP sending 0x401...")
        self.sender.enabled.clear()
        time.sleep(0.5)
        self.confirm("Timeout -> stop", [
            "PIN_5/6 OPEN (MdiStopMotor forces ECO)",
            "DIR output LOW (forward)",
            "NOTE: FWD/REV LEDs only update on 0x401 frames, so REV_LED may still be ON",
        ])

        self.sender.set_state(reverse=False, eco=True)
        self.sender.enabled.set()
        time.sleep(0.3)
        self.confirm("Recovery", ["FWD_LED ON, REV_LED OFF, PIN_5/6 OPEN"])

    def print_summary(self):
        print("\n=== Summary ===")
        for name, passed in self.results:
            print(f"  {'PASS' if passed else 'FAIL'}  {name}")
        failed = sum(1 for _, p in self.results if not p)
        print(f"\n{len(self.results) - failed}/{len(self.results)} passed")
        return failed


if(__name__ == "__main__"):
    with select_can_interface() as bus:
        print("CAN bus initialized")
        mdi_test = MDITest(bus)
        try:
            mdi_test.run_test()
        except KeyboardInterrupt:
            print("\nStopped by user")
            mdi_test.sender.stopped.set()
            mdi_test.notifier.stop()
            sys.exit(1)
        print("Test completed")
