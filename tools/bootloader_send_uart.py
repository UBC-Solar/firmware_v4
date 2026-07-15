#!/usr/bin/env python3
import argparse
import binascii
from pathlib import Path
import struct
import sys
import time

try:
    import serial
except ImportError:
    serial = None


ACK = 0x79
NACK = 0x1F
MAGIC = b"UBSL"
DEFAULT_BINARY = (
    Path(__file__).resolve().parents[1] /
    "firmware/components/nucleo_f103rb/build/nucleo_f103rb_app.bin"
)


def read_ack(port, timeout_s):
    deadline = time.monotonic() + timeout_s

    while time.monotonic() < deadline:
        byte = port.read(1)

        if not byte:
            continue

        value = byte[0]

        if value == ACK:
            return True

        if value == NACK:
            return False

    raise TimeoutError("timed out waiting for bootloader ACK")


def send_image(args):
    if serial is None:
        print("pyserial is required: python3 -m pip install pyserial", file=sys.stderr)
        return 2

    binary_path = Path(args.binary)

    if not binary_path.is_file():
        print(f"binary not found: {binary_path}", file=sys.stderr)
        print("build first: tools/nucleo_bootloader_test.sh build", file=sys.stderr)
        return 2

    with open(binary_path, "rb") as binary_file:
        image = binary_file.read()

    crc = binascii.crc32(image) & 0xFFFFFFFF
    header = MAGIC + struct.pack("<II", len(image), crc)

    with serial.Serial(args.port, args.baud, timeout=0.1, write_timeout=2) as port:
        print(f"port={args.port} baud={args.baud}")
        print(f"image={binary_path} size={len(image)} crc=0x{crc:08X}")
        print(f"header={header.hex(' ').upper()}")
        print("make sure the Nucleo is already in bootloader update mode")

        time.sleep(args.boot_delay)
        port.reset_input_buffer()
        port.write(header)
        port.flush()

        if not read_ack(port, args.ack_timeout):
            print("bootloader rejected image header", file=sys.stderr)
            return 1

        sent = 0
        while sent < len(image):
            chunk = image[sent:sent + args.chunk_size]
            port.write(chunk)
            port.flush()

            if not read_ack(port, args.ack_timeout):
                print(f"bootloader rejected chunk at offset {sent}", file=sys.stderr)
                return 1

            sent += len(chunk)
            print(f"\r{sent}/{len(image)} bytes", end="", flush=True)

        print()

        if not read_ack(port, args.ack_timeout):
            print("bootloader reported final CRC/programming failure", file=sys.stderr)
            return 1

    print("update complete")
    return 0


def main():
    parser = argparse.ArgumentParser(description="Send a firmware .bin to the STM32 bootloader over UART.")
    parser.add_argument("port", help="serial port, for example /dev/cu.usbmodemXXXX")
    parser.add_argument(
        "binary",
        nargs="?",
        default=str(DEFAULT_BINARY),
        help="application .bin built for 0x08008000; defaults to the Nucleo app",
    )
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--chunk-size", type=int, default=256)
    parser.add_argument("--ack-timeout", type=float, default=10.0)
    parser.add_argument("--boot-delay", type=float, default=0.5)
    return send_image(parser.parse_args())


if __name__ == "__main__":
    raise SystemExit(main())
