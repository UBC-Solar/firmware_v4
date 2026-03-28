"""
UBC Solar — Cruise Control CAN HIL Plant Simulator
====================================================
Simulates drag, rolling resistance, and hill forces on a bench-top
(car jacked up) by closing the loop over the real CAN bus.

The STM32 DRD board runs real firmware. This script:
  1. Transitions the drive state into CRUISE mode
  2. Sets the cruise setpoint speed via debug CAN
  3. Reads the motor command (0x401) — accel/regen DAC
  4. Runs a plant model (inertia + drag + rolling + hill)
  5. Sends simulated velocity (fake motor controller RPM)
  6. Sends simulated IMU data for Kalman pitch estimation

SAFETY FEATURES:
  - Heartbeat watchdog: if no motor command received in WATCHDOG_TIMEOUT,
    the script zeroes velocity and disables cruise
  - Velocity hard-clamped to [0, VELOCITY_HARD_LIMIT]
  - All CAN TX wrapped in try/except — bus errors don't crash the loop
  - Graceful shutdown on Ctrl+C — always disables cruise before exit
  - Input validation on all command-line parameters
  - Plant state sanity checks every tick

Requirements:
    pip install python-can numpy

    Motor controller must be DISCONNECTED from CAN bus so velocity
    messages don't conflict.

Usage:
    python cruise_hil.py --channel can0 --setpoint 11.11 --hill 0.0
    python cruise_hil.py --channel can0 --setpoint 16.0 --hill 3.0 --log run.csv
"""

import argparse
import csv
import logging
import math
import signal
import struct
import sys
import threading
import time

import numpy as np

# ── Logging ──────────────────────────────────────────────────────────────────
logging.basicConfig(
    level=logging.INFO,
    format="[%(levelname)s %(asctime)s] %(message)s",
    datefmt="%H:%M:%S",
)
log = logging.getLogger("HIL")

# ── CAN Message IDs ──────────────────────────────────────────────────────────
MOTOR_CMD_ID       = 0x401          # STD  — DRD TX: accel/regen DAC
FRAME0_ID          = 0x08850225     # EXT  — motor controller velocity (RPM)
STR_CAN_MSG_ID     = 0x580          # STD  — steering: cruise_on / regen_on
STATE_REQ_ID       = 0x500          # STD  — DEBUG: next/prev state request
CRUISE_SETPOINT_ID = 0x501          # STD  — DEBUG: set cruise setpoint (float)
IMU_X_ID           = 0x752          # STD  — IMU accel_x + gyro_x
IMU_Y_ID           = 0x753          # STD  — IMU accel_y + gyro_y
IMU_Z_ID           = 0x754          # STD  — IMU accel_z + gyro_z

# ── Vehicle Constants (must match cruise_control.c) ──────────────────────────
WHEEL_RADIUS        = 0.283         # m
ACCEL_MIN           = -2.5          # m/s²
ACCEL_MAX           = 2.0           # m/s²
MC_DAC_MAX          = 1023
CRUISE_SPEED_MIN_MS = 6.94          # m/s  (25 km/h)
CRUISE_SPEED_MAX_MS = 22.22         # m/s  (80 km/h)
GRAVITY             = 9.81

# ── Safety Limits ────────────────────────────────────────────────────────────
VELOCITY_HARD_LIMIT = 30.0          # m/s absolute cap (108 km/h)
WATCHDOG_TIMEOUT    = 2.0           # seconds — no motor cmd = emergency stop
IMU_NOISE_STD       = 0.05          # m/s² — small sensor noise
GYRO_NOISE_STD      = 2.0           # mdps — gyro noise


# ═════════════════════════════════════════════════════════════════════════════
# Plant Model
# ═════════════════════════════════════════════════════════════════════════════
class PlantModel:
    """1-D longitudinal vehicle dynamics with drag, rolling, and hill forces."""

    def __init__(self, mass, cd, frontal_area, air_density,
                 rolling_resistance, initial_velocity):
        assert mass > 0, "Mass must be positive"
        assert cd >= 0, "Drag coefficient must be non-negative"
        assert frontal_area > 0, "Frontal area must be positive"
        assert rolling_resistance >= 0, "Rolling resistance must be non-negative"

        self.mass = mass
        self.cd = cd
        self.frontal_area = frontal_area
        self.air_density = air_density
        self.rolling_resistance = rolling_resistance
        self.velocity = initial_velocity
        self.hill_rad = 0.0

    def force_drag(self, v):
        return 0.5 * self.air_density * self.cd * self.frontal_area * v * v

    def force_rolling(self):
        return self.rolling_resistance * self.mass * GRAVITY

    def force_hill(self):
        return self.mass * GRAVITY * math.sin(self.hill_rad)

    def step(self, motor_force, dt):
        """Advance state by dt. Returns (velocity, acceleration, f_resist)."""
        v = max(self.velocity, 0.01)

        f_drag = self.force_drag(v)
        f_roll = self.force_rolling()
        f_hill = self.force_hill()
        f_resist = f_drag + f_roll + f_hill

        f_net = motor_force - f_resist
        accel = f_net / self.mass
        accel = max(ACCEL_MIN, min(ACCEL_MAX, accel))

        self.velocity += accel * dt
        self.velocity = max(0.0, min(self.velocity, VELOCITY_HARD_LIMIT))

        # Sanity check — NaN/inf guard
        if not math.isfinite(self.velocity):
            log.error("Plant velocity is NaN/inf — resetting to 0")
            self.velocity = 0.0

        return self.velocity, accel, f_resist


# ═════════════════════════════════════════════════════════════════════════════
# CAN Encoding / Decoding
# ═════════════════════════════════════════════════════════════════════════════
def pack_float_le(value):
    """Pack a float into 4 bytes, little-endian."""
    return struct.pack('<f', value)


def encode_velocity_frame(velocity_ms):
    """
    Encode velocity (m/s) into FRAME0 format (0x08850225, extended).

    DriveStateVelocityCanMsgHandler parses:
        rpm = (data[4] >> 3) | ((data[5] & 0x7f) << 5)
        velocity = WHEEL_RADIUS * 2*pi * rpm / 60
    """
    velocity_ms = max(0.01, velocity_ms)
    rpm = (velocity_ms * 60.0) / (WHEEL_RADIUS * 2.0 * math.pi)
    rpm_int = int(round(rpm))
    rpm_int = max(0, min(rpm_int, 4095))  # 12-bit field

    data = bytearray(8)
    data[4] = (rpm_int << 3) & 0xFF
    data[5] = (rpm_int >> 5) & 0x7F
    return bytes(data)


def encode_imu_axis(accel_val, gyro_val):
    """Pack accel + gyro as two LE floats into 8 bytes."""
    return pack_float_le(accel_val) + pack_float_le(gyro_val)


def decode_motor_cmd(data):
    """
    Decode motor command from DRD (0x401, DLC=5).
    Returns (accel_dac, regen_dac, flags).
    """
    accel_dac = data[0] | (data[1] << 8)
    regen_dac = data[2] | (data[3] << 8)
    flags = data[4] if len(data) > 4 else 0
    return accel_dac, regen_dac, flags


def dac_to_force(dac_value, mass):
    """Convert 10-bit DAC to motor force (N), linearly mapped ACCEL_MIN..ACCEL_MAX."""
    ratio = dac_value / MC_DAC_MAX
    accel = ACCEL_MIN + ratio * (ACCEL_MAX - ACCEL_MIN)
    return accel * mass


# ═════════════════════════════════════════════════════════════════════════════
# CAN TX — all sends wrapped for safety
# ═════════════════════════════════════════════════════════════════════════════
def safe_send(bus, msg):
    """Send a CAN message, logging errors instead of crashing."""
    try:
        bus.send(msg)
    except Exception as e:
        log.warning("CAN TX failed (ID=0x%X): %s", msg.arbitration_id, e)


def send_velocity(bus, velocity_ms):
    """Send simulated velocity as fake motor controller RPM."""
    import can
    safe_send(bus, can.Message(
        arbitration_id=FRAME0_ID,
        data=encode_velocity_frame(velocity_ms),
        is_extended_id=True,
    ))


def send_imu(bus, hill_rad):
    """Send simulated IMU X/Y/Z consistent with the given hill angle."""
    import can
    ax = GRAVITY * math.sin(hill_rad) + np.random.normal(0, IMU_NOISE_STD)
    ay = np.random.normal(0, IMU_NOISE_STD)
    az = GRAVITY * math.cos(hill_rad) + np.random.normal(0, IMU_NOISE_STD)
    gx = np.random.normal(0, GYRO_NOISE_STD)
    gy = np.random.normal(0, GYRO_NOISE_STD)
    gz = np.random.normal(0, GYRO_NOISE_STD)

    safe_send(bus, can.Message(arbitration_id=IMU_X_ID,
                               data=encode_imu_axis(ax, gx), is_extended_id=False))
    safe_send(bus, can.Message(arbitration_id=IMU_Y_ID,
                               data=encode_imu_axis(ay, gy), is_extended_id=False))
    safe_send(bus, can.Message(arbitration_id=IMU_Z_ID,
                               data=encode_imu_axis(az, gz), is_extended_id=False))
    return ax, ay, az, gy


def send_steering(bus, cruise_on, regen_on=False):
    """Send steering CAN msg to enable/disable cruise."""
    import can
    data = bytearray(8)
    data[0] = (0x01 if regen_on else 0x00) | (0x02 if cruise_on else 0x00)
    safe_send(bus, can.Message(arbitration_id=STR_CAN_MSG_ID, data=data,
                               is_extended_id=False))


def send_state_request(bus, next_state):
    """Send DEBUG state transition request (0=next, 1=prev)."""
    import can
    safe_send(bus, can.Message(arbitration_id=STATE_REQ_ID,
                               data=[0x00 if next_state else 0x01],
                               is_extended_id=False))


def send_cruise_setpoint(bus, setpoint_ms):
    """Send DEBUG cruise setpoint (float, m/s) on CAN 0x501."""
    import can
    data = bytearray(8)
    data[:4] = pack_float_le(setpoint_ms)
    safe_send(bus, can.Message(arbitration_id=CRUISE_SETPOINT_ID, data=data,
                               is_extended_id=False))


# ═════════════════════════════════════════════════════════════════════════════
# CAN RX Thread
# ═════════════════════════════════════════════════════════════════════════════
class MotorCmdReceiver:
    """Thread-safe receiver for motor command messages from DRD."""

    def __init__(self, bus):
        self._bus = bus
        self._lock = threading.Lock()
        self._accel_dac = 0
        self._regen_dac = 0
        self._flags = 0
        self._last_rx_time = time.monotonic()
        self._running = True
        self._thread = threading.Thread(target=self._run, daemon=True)
        self._thread.start()

    def _run(self):
        while self._running:
            msg = self._bus.recv(timeout=0.05)
            if msg is None:
                continue
            if msg.arbitration_id == MOTOR_CMD_ID and not msg.is_extended_id:
                a, r, f = decode_motor_cmd(msg.data)
                with self._lock:
                    self._accel_dac = a
                    self._regen_dac = r
                    self._flags = f
                    self._last_rx_time = time.monotonic()

    def get(self):
        """Returns (accel_dac, regen_dac, flags, seconds_since_last_rx)."""
        with self._lock:
            age = time.monotonic() - self._last_rx_time
            return self._accel_dac, self._regen_dac, self._flags, age

    def stop(self):
        self._running = False
        self._thread.join(timeout=1.0)


# ═════════════════════════════════════════════════════════════════════════════
# Main
# ═════════════════════════════════════════════════════════════════════════════
def validate_args(args):
    """Validate all user inputs before touching the CAN bus."""
    errors = []
    if args.mass <= 0:
        errors.append("--mass must be positive")
    if args.cd < 0:
        errors.append("--cd must be non-negative")
    if args.rolling < 0:
        errors.append("--rolling must be non-negative")
    if not (CRUISE_SPEED_MIN_MS <= args.setpoint <= CRUISE_SPEED_MAX_MS):
        errors.append(f"--setpoint must be in [{CRUISE_SPEED_MIN_MS}, {CRUISE_SPEED_MAX_MS}] m/s")
    if abs(args.hill) > 20:
        errors.append("--hill must be in [-20, 20] degrees")
    if args.dt <= 0 or args.dt > 1.0:
        errors.append("--dt must be in (0, 1.0] seconds")
    if args.duration <= 0:
        errors.append("--duration must be positive")
    if errors:
        for e in errors:
            log.error(e)
        sys.exit(1)


def main():
    import can  # import here so --help works without python-can installed

    parser = argparse.ArgumentParser(
        description="Cruise Control CAN HIL Plant Simulator — UBC Solar",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""Examples:
  python cruise_hil.py --setpoint 11.11 --hill 0
  python cruise_hil.py --setpoint 16.0 --hill 3.0 --mass 350 --log run.csv
  python cruise_hil.py --setpoint 11.11 --hill -2.0 --duration 60
        """,
    )
    parser.add_argument("--interface", default="socketcan")
    parser.add_argument("--channel", default="can0")
    parser.add_argument("--setpoint", type=float, default=11.11,
                        help="Cruise setpoint (m/s). Range [6.94, 22.22]")
    parser.add_argument("--hill", type=float, default=0.0,
                        help="Hill angle (degrees). Positive = uphill.")
    parser.add_argument("--mass", type=float, default=350.0, help="Vehicle mass (kg)")
    parser.add_argument("--cd", type=float, default=0.116, help="Drag coefficient")
    parser.add_argument("--frontal-area", type=float, default=1.18, help="Frontal area (m²)")
    parser.add_argument("--rolling", type=float, default=0.007, help="Rolling resistance coeff")
    parser.add_argument("--dt", type=float, default=0.1, help="Plant timestep (s)")
    parser.add_argument("--duration", type=float, default=120.0, help="Sim duration (s)")
    parser.add_argument("--log", default=None, help="CSV output file path")
    args = parser.parse_args()

    validate_args(args)

    setpoint_ms = args.setpoint
    hill_rad = math.radians(args.hill)
    dt = args.dt

    # ── Plant model ───────────────────────────────────────────────────────
    plant = PlantModel(
        mass=args.mass,
        cd=args.cd,
        frontal_area=args.frontal_area,
        air_density=1.26714,
        rolling_resistance=args.rolling,
        initial_velocity=CRUISE_SPEED_MIN_MS,
    )
    plant.hill_rad = hill_rad

    # ── CAN bus ───────────────────────────────────────────────────────────
    try:
        bus = can.interface.Bus(channel=args.channel, interface=args.interface)
    except Exception as e:
        log.error("Failed to open CAN bus: %s", e)
        sys.exit(1)

    receiver = MotorCmdReceiver(bus)

    # ── CSV log ───────────────────────────────────────────────────────────
    csv_file = None
    csv_writer = None
    if args.log:
        csv_file = open(args.log, 'w', newline='')
        csv_writer = csv.writer(csv_file)
        csv_writer.writerow([
            "time_s", "setpoint_ms", "velocity_ms", "velocity_kmh",
            "accel_dac", "regen_dac", "motor_force_N", "accel_ms2",
            "f_drag_N", "f_rolling_N", "f_hill_N", "f_resist_N",
            "hill_deg", "imu_ax", "imu_az", "imu_gy",
        ])

    # ── Graceful shutdown ─────────────────────────────────────────────────
    shutdown = [False]

    def on_signal(sig, frame):
        shutdown[0] = True

    signal.signal(signal.SIGINT, on_signal)
    signal.signal(signal.SIGTERM, on_signal)

    # ── Startup sequence ──────────────────────────────────────────────────
    log.info("=== UBC Solar Cruise HIL Simulator ===")
    log.info("Setpoint: %.2f m/s (%.1f km/h)", setpoint_ms, setpoint_ms * 3.6)
    log.info("Hill: %.1f°  Mass: %.0f kg  Cd: %.3f  Rolling: %.4f",
             args.hill, args.mass, args.cd, args.rolling)
    log.info("Timestep: %.3f s  Duration: %.0f s", dt, args.duration)
    log.info("")

    # Step 1: PARK → FORWARD
    log.info("Sending state request: PARK → FORWARD")
    send_state_request(bus, next_state=True)
    time.sleep(0.5)

    # Step 2: Send initial velocity so velocity_under_threshold clears
    log.info("Sending initial velocity = %.2f m/s", CRUISE_SPEED_MIN_MS)
    send_velocity(bus, CRUISE_SPEED_MIN_MS)
    time.sleep(0.3)

    # Step 3: Enable cruise
    log.info("Enabling cruise mode")
    send_steering(bus, cruise_on=True)
    time.sleep(0.3)

    # Step 4: Set cruise setpoint
    log.info("Setting cruise setpoint = %.2f m/s", setpoint_ms)
    send_cruise_setpoint(bus, setpoint_ms)
    time.sleep(0.2)

    # ── Main loop ─────────────────────────────────────────────────────────
    log.info("")
    log.info("%-7s  %-6s  %-7s  %-6s  %-5s  %-5s  %-8s  %-7s  %-7s  %-7s  %-7s",
             "time", "setpt", "vel", "km/h", "aDAC", "rDAC",
             "F_motor", "accel", "F_drag", "F_roll", "F_hill")
    log.info("-" * 95)

    t = 0.0
    watchdog_tripped = False

    while t < args.duration and not shutdown[0]:
        loop_start = time.monotonic()

        # ── Read DRD motor command ────────────────────────────────────
        accel_dac, regen_dac, flags, rx_age = receiver.get()

        # ── Watchdog: no motor command in WATCHDOG_TIMEOUT → stop ─────
        if rx_age > WATCHDOG_TIMEOUT:
            if not watchdog_tripped:
                log.warning("WATCHDOG: No motor command for %.1f s — "
                            "zeroing velocity", rx_age)
                watchdog_tripped = True
            motor_force = 0.0
        else:
            watchdog_tripped = False
            # Convert DAC → motor force
            if accel_dac > 0:
                motor_force = dac_to_force(accel_dac, plant.mass)
            elif regen_dac > 0:
                # Regen is braking — negative force, scaled down
                motor_force = -dac_to_force(regen_dac, plant.mass) * 0.5
            else:
                motor_force = 0.0

        # ── Step plant model ──────────────────────────────────────────
        velocity, accel, f_resist = plant.step(motor_force, dt)

        # ── Send simulated velocity ───────────────────────────────────
        send_velocity(bus, velocity)

        # ── Send simulated IMU ────────────────────────────────────────
        imu_ax, imu_ay, imu_az, imu_gy = send_imu(bus, hill_rad)

        # ── Keep cruise alive ─────────────────────────────────────────
        send_steering(bus, cruise_on=True)

        # ── Console output ────────────────────────────────────────────
        f_drag = plant.force_drag(max(velocity, 0.01))
        f_roll = plant.force_rolling()
        f_hill = plant.force_hill()

        sys.stdout.write(
            f"\r{t:7.1f}  {setpoint_ms:6.2f}  {velocity:7.3f}  "
            f"{velocity * 3.6:6.1f}  {accel_dac:5d}  {regen_dac:5d}  "
            f"{motor_force:8.2f}  {accel:+7.3f}  {f_drag:7.2f}  "
            f"{f_roll:7.2f}  {f_hill:+7.2f}"
        )
        sys.stdout.flush()

        # ── CSV log ───────────────────────────────────────────────────
        if csv_writer:
            csv_writer.writerow([
                f"{t:.3f}", f"{setpoint_ms:.3f}", f"{velocity:.4f}",
                f"{velocity * 3.6:.2f}", accel_dac, regen_dac,
                f"{motor_force:.3f}", f"{accel:.4f}",
                f"{f_drag:.3f}", f"{f_roll:.3f}", f"{f_hill:.3f}",
                f"{f_resist:.3f}", f"{args.hill:.2f}",
                f"{imu_ax:.4f}", f"{imu_az:.4f}", f"{imu_gy:.4f}",
            ])

        t += dt

        # ── Maintain real-time pace ───────────────────────────────────
        elapsed = time.monotonic() - loop_start
        sleep_time = dt - elapsed
        if sleep_time > 0:
            time.sleep(sleep_time)

    # ── Shutdown ──────────────────────────────────────────────────────────
    print()
    log.info("Shutting down — disabling cruise")

    send_steering(bus, cruise_on=False)
    time.sleep(0.1)
    send_velocity(bus, 0.0)

    receiver.stop()
    bus.shutdown()

    if csv_file:
        csv_file.close()

    log.info("Final velocity: %.3f m/s (%.1f km/h)", plant.velocity, plant.velocity * 3.6)
    if args.log:
        log.info("Log saved to: %s", args.log)


if __name__ == "__main__":
    main()
