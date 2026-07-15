#!/usr/bin/env python3
import argparse
import datetime
import serial


def format_ascii(data):
    return "".join(chr(byte) if 32 <= byte <= 126 else "." for byte in data)


parser = argparse.ArgumentParser()
parser.add_argument("port", help="example: /dev/cu.usbserialXXXX")
parser.add_argument("--baud", type=int, default=115200)
parser.add_argument("--text-only", action="store_true")
args = parser.parse_args()

with serial.Serial(args.port, args.baud, timeout=1) as ser:
    print(f"Listening on {args.port} at {args.baud} baud...")
    while True:
        data = ser.read(128)
        if data:
            if args.text_only:
                print(data.decode(errors="replace"), end="", flush=True)
            else:
                timestamp = datetime.datetime.now().strftime("%H:%M:%S.%f")[:-3]
                hex_bytes = " ".join(f"{byte:02X}" for byte in data)
                ascii_text = format_ascii(data)
                decoded_text = data.decode(errors="replace")

                print(f"[{timestamp}] {len(data)} byte(s)")
                print(f"  hex:   {hex_bytes}")
                print(f"  ascii: {ascii_text}")

                if decoded_text.strip():
                    print(f"  text:  {decoded_text!r}")
