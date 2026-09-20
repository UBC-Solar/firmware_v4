#!/usr/bin/env python3
"""Direct, signed Sunlite application flashing over classic CAN (PCAN by default)."""

import argparse
import hashlib
import json
from pathlib import Path
import secrets
import struct
import subprocess
import sys
import tempfile
import time
import zlib

TARGETS = {"mdi": (0x4D444920, 0x10), "drd": (0x44524420, 0x11),
           "str": (0x53545220, 0x12)}
HELLO, BOARD_INFO, ENTER, BEGIN, DATA, END, ACK, NACK, STATUS, REBOOT, ABORT = range(1, 12)
STATES = ("application", "bootloader", "update_in_progress", "pending_image")
STATUSES = ("OK", "BAD_STATE", "BAD_TARGET", "BAD_HARDWARE_REVISION",
            "BAD_VERSION", "BAD_SIGNATURE", "BAD_HASH", "BAD_OFFSET",
            "FLASH_ERROR", "IMAGE_TOO_LARGE", "UNSUPPORTED",
            "UPDATE_NOT_ALLOWED", "INTERNAL_ERROR")
SIGNING_DOMAIN = b"SUNLITE-OTA-MANIFEST-V1\0"
MAX_FRAME = 1051

class FlashError(Exception):
    pass

class TransportError(FlashError):
    pass

def cobs_encode(raw):
    encoded = bytearray([0])
    code_index, code = 0, 1
    for value in raw:
        if value == 0:
            encoded[code_index] = code
            code_index, code = len(encoded), 1
            encoded.append(0)
        else:
            encoded.append(value)
            code += 1
            if code == 255:
                encoded[code_index] = code
                code_index, code = len(encoded), 1
                encoded.append(0)
    encoded[code_index] = code
    return bytes(encoded)

def cobs_decode(encoded):
    raw = bytearray()
    index = 0
    while index < len(encoded):
        code = encoded[index]
        index += 1
        if code == 0 or index + code - 1 > len(encoded):
            raise TransportError("Malformed COBS response")
        raw.extend(encoded[index:index + code - 1])
        index += code - 1
        if code < 255 and index < len(encoded):
            raw.append(0)
    return bytes(raw)

def encode_message(kind, session, sequence, payload=b""):
    raw = struct.pack(">2sBBIIH", b"SU", 1, kind, session, sequence, len(payload)) + payload
    return cobs_encode(raw + struct.pack(">I", zlib.crc32(raw)))

def decode_message(encoded):
    if not encoded or len(encoded) > MAX_FRAME or 0 in encoded:
        raise TransportError("Invalid encoded response")
    raw = cobs_decode(encoded)
    if len(raw) < 18 or zlib.crc32(raw[:-4]) != struct.unpack(">I", raw[-4:])[0]:
        raise TransportError("Response CRC mismatch")
    magic, version, kind, session, sequence, length = struct.unpack(">2sBBIIH", raw[:14])
    if magic != b"SU" or version != 1 or not 1 <= kind <= 11 or length != len(raw) - 18:
        raise TransportError("Invalid Sunlite response header")
    return kind, session, sequence, raw[14:-4]

class CanTransport:
    """Synchronous ISO-TP subset matching sunlite_ota_can_transport.c.

    Only one request is outstanding. The bus must not have another receiver
    consuming these identifiers (including a concurrent TEL OTA session).
    """

    def __init__(self, bus, node, message_factory=None):
        if message_factory is None:
            import can
            message_factory = can.Message
        self.bus = bus
        self.message_factory = message_factory
        self.tx_id = 0x18DA0000 | node << 8 | 0xF1
        self.rx_id = 0x18DAF100 | node

    def emit(self, data):
        self.bus.send(self.message_factory(arbitration_id=self.tx_id,
                      is_extended_id=True, data=data.ljust(8, b"\xaa")), timeout=0.25)

    def frame(self, deadline):
        while time.monotonic() < deadline:
            msg = self.bus.recv(max(0, deadline - time.monotonic()))
            if msg is None:
                break
            if (msg.arbitration_id == self.rx_id and msg.is_extended_id
                    and not msg.is_remote_frame and not msg.is_error_frame and not msg.is_fd):
                if not 1 <= len(msg.data) <= 8:
                    raise TransportError("Invalid classic CAN frame length")
                return bytes(msg.data)
        raise TransportError("CAN response timed out; check power, termination, bitrate and bootloader provisioning")

    def flow_control(self):
        for _ in range(4):
            data = self.frame(time.monotonic() + 0.25)
            if len(data) < 3 or data[0] >> 4 != 3:
                raise TransportError("Expected CAN flow control")
            if data[0] == 0x30:
                stmin = data[2]
                if stmin <= 0x7F:
                    delay = stmin / 1000
                elif 0xF1 <= stmin <= 0xF9:
                    delay = (stmin - 0xF0) / 10000
                else:
                    raise TransportError("Reserved CAN separation time")
                return data[1], delay
            if data[0] != 0x31:
                raise TransportError("CAN receiver rejected transfer")
        raise TransportError("CAN flow-control WAIT limit exceeded")

    def send(self, encoded):
        if not 1 <= len(encoded) <= MAX_FRAME or 0 in encoded:
            raise FlashError("Invalid outgoing COBS frame")
        if len(encoded) <= 7:
            self.emit(bytes([len(encoded)]) + encoded)
            return
        self.emit(bytes([0x10 | len(encoded) >> 8, len(encoded) & 255]) + encoded[:6])
        offset, sequence = 6, 1
        while offset < len(encoded):
            block_size, delay = self.flow_control()
            count = 0
            while offset < len(encoded) and (block_size == 0 or count < block_size):
                time.sleep(delay)
                self.emit(bytes([0x20 | sequence]) + encoded[offset:offset + 7])
                offset += 7
                sequence = (sequence + 1) & 15
                count += 1

    def receive(self, deadline):
        data = self.frame(deadline)
        if data[0] >> 4 == 0:
            size = data[0] & 15
            if not 1 <= size <= 7 or len(data) < size + 1:
                raise TransportError("Malformed CAN single frame")
            return data[1:size + 1]
        if data[0] >> 4 != 1 or len(data) != 8:
            raise TransportError("Expected CAN first frame")
        size = (data[0] & 15) << 8 | data[1]
        if not 7 < size <= MAX_FRAME:
            self.emit(b"\x32\0\0")
            raise TransportError("CAN response exceeds supported size")
        encoded = bytearray(data[2:])
        sequence, count = 1, 0
        self.emit(b"\x30\x03\x02")
        while len(encoded) < size:
            data = self.frame(min(deadline, time.monotonic() + 0.25))
            take = min(7, size - len(encoded))
            if data[0] != 0x20 | sequence or len(data) < take + 1:
                raise TransportError("CAN consecutive-frame sequence/length mismatch")
            encoded.extend(data[1:take + 1])
            sequence, count = (sequence + 1) & 15, count + 1
            if count == 3 and len(encoded) < size:
                self.emit(b"\x30\x03\x02")
                count = 0
        return bytes(encoded)

class Client:
    def __init__(self, transport, target, timeout=5, retries=3):
        self.transport, self.target = transport, target
        self.timeout, self.retries = timeout, retries
        self.session, self.sequence = secrets.randbits(32), 0

    def request(self, kind, payload=b"", expected_offset=None):
        self.sequence += 1
        encoded = encode_message(kind, self.session, self.sequence, payload)
        last_error = None
        for attempt in range(self.retries):
            try:
                self.transport.send(encoded)
                deadline = time.monotonic() + self.timeout
                while True:
                    response, session, sequence, data = decode_message(self.transport.receive(deadline))
                    if (session, sequence) != (self.session, self.sequence):
                        continue
                    if response in (ACK, NACK):
                        if len(data) != 6 or data[0] != kind:
                            raise FlashError("ACK does not match request")
                        status, offset = data[1], struct.unpack(">I", data[2:])[0]
                        if response == NACK or status:
                            label = STATUSES[status] if status < len(STATUSES) else str(status)
                            raise FlashError(f"Board rejected message {kind}: {label}")
                        if kind == HELLO:
                            raise FlashError("HELLO returned ACK instead of BOARD_INFO")
                        if expected_offset is not None and offset != expected_offset:
                            raise FlashError(f"Board acknowledged offset {offset}, expected {expected_offset}")
                        return data
                    if kind == HELLO and response == BOARD_INFO:
                        return data
                    raise FlashError("Unexpected response type")
            except TransportError as error:
                last_error = error
                if attempt + 1 < self.retries:
                    # Let the board's partial transfer expire before exact replay.
                    time.sleep(0.3)
        raise TransportError(f"Request {kind} failed after {self.retries} attempts: {last_error}")

    def hello(self):
        payload = self.request(HELLO, struct.pack(">I", self.target))
        if len(payload) != 30:
            raise FlashError("Invalid BOARD_INFO length")
        values = struct.unpack(">IH I I BB H I I I", payload)
        board = dict(zip(("target_id", "hardware_revision", "bootloader_version",
                         "firmware_version", "active_slot", "state", "max_chunk",
                         "slot_size", "resume_offset", "capabilities"), values))
        if board["target_id"] != self.target or board["state"] >= len(STATES):
            raise FlashError("BOARD_INFO target/state mismatch")
        return board

    def wait_state(self, state, firmware_version=None, timeout=20):
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            try:
                board = self.hello()
                if board["state"] == state:
                    if firmware_version is not None and board["firmware_version"] != firmware_version:
                        raise FlashError("Running application version differs from signed image")
                    return board
            except TransportError:
                pass
            time.sleep(0.1)
        raise FlashError(f"Board did not reach {STATES[state]} state")

def integer(metadata, name, maximum=0xFFFFFFFF):
    value = metadata.get(name)
    if isinstance(value, str):
        value = int(value, 16 if value.lower().startswith("0x") else 10)
    if type(value) is not int or not 0 <= value <= maximum:
        raise FlashError(f"Invalid metadata field {name}")
    return value

def prepare_image(elf, target, private_key, objcopy):
    """Use the ELF's generated sidecar; never invent a compiled version."""
    from cryptography.hazmat.primitives import serialization
    from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey

    metadata = json.loads(Path(str(elf) + ".ota.json").read_text())
    if not isinstance(metadata, dict) or integer(metadata, "schema") != 1:
        raise FlashError("Expected schema-1 ELF OTA metadata")
    target_id = integer(metadata, "targetId")
    if target_id != target:
        raise FlashError("ELF metadata does not match selected board")
    hw_min = integer(metadata, "hardwareRevisionMin", 65535)
    hw_max = integer(metadata, "hardwareRevisionMax", 65535)
    version = integer(metadata, "firmwareVersion")
    minimum_bootloader = integer(metadata, "minimumBootloaderVersion")
    if hw_min > hw_max:
        raise FlashError("Metadata hardware revision range is reversed")
    with tempfile.TemporaryDirectory(prefix="sunlite-can-") as directory:
        binary = Path(directory) / "firmware.bin"
        subprocess.run([objcopy, "-O", "binary", str(elf), str(binary)], check=True)
        image = binary.read_bytes()
    if not 8 <= len(image) <= 225280:
        raise FlashError("Image does not fit the application slot")
    stack, reset = struct.unpack("<II", image[:8])
    if (not 0x20000000 <= stack <= 0x2000C000 or stack & 7 or not reset & 1
            or not 0x08008000 <= (reset & ~1) < 0x08008000 + len(image)):
        raise FlashError("ELF must be an application linked at 0x08008000 with valid vectors")
    fields = struct.pack(">IHHIII", target, hw_min, hw_max, version, len(image), minimum_bootloader)
    fields += hashlib.sha256(image).digest()
    key = serialization.load_pem_private_key(Path(private_key).expanduser().read_bytes(), password=None)
    if not isinstance(key, Ed25519PrivateKey):
        raise FlashError("Signing key must be Ed25519")
    return image, fields + key.sign(SIGNING_DOMAIN + fields)

def validate_board(board, manifest, image_size):
    target, hw_min, hw_max, version, size, minimum = struct.unpack(">IHHIII", manifest[:20])
    if board["target_id"] != target or not hw_min <= board["hardware_revision"] <= hw_max:
        raise FlashError("Board target/hardware revision does not match image")
    if not board["capabilities"] & 4:
        raise FlashError("Board lacks a provisioned signature-verification key")
    if board["bootloader_version"] < minimum:
        raise FlashError("Image requires a newer bootloader")
    if size != image_size or size > board["slot_size"]:
        raise FlashError("Image exceeds board application slot")
    if board["max_chunk"] < 2:
        raise FlashError("Board advertised an invalid chunk size")
    installed = board["firmware_version"]
    # Recovery advertises zero when vectors are invalid; the bootloader still
    # enforces the retained metadata version before erase.
    if installed and version <= installed:
        raise FlashError(f"Build a firmware version greater than installed version {installed}")

def flash(client, image, manifest, progress=print):
    board = client.hello()
    validate_board(board, manifest, len(image))
    if board["state"] == 0:
        if not board["capabilities"] & 1:
            raise FlashError("Board update interlock is closed (UPDATE_NOT_ALLOWED)")
        progress("Entering CAN bootloader...")
        client.request(ENTER)
        time.sleep(0.3)
        board = client.wait_state(1)
        validate_board(board, manifest, len(image))
    elif board["state"] != 1:
        raise FlashError("Board already has an update in progress; recover/reset it before a new flash")
    progress("Verifying signature and erasing application...")
    client.request(BEGIN, manifest, expected_offset=0)
    chunk_size = min(1024, board["max_chunk"]) & ~1
    for offset in range(0, len(image), chunk_size):
        chunk = image[offset:offset + chunk_size]
        client.request(DATA, struct.pack(">I", offset) + chunk, expected_offset=offset + len(chunk))
        progress(f"Written {offset + len(chunk)}/{len(image)} bytes")
    client.request(END, expected_offset=len(image))
    # The final ACK can be lost while the application starts. Success still
    # requires a targeted application HELLO with the exact signed version.
    try:
        client.request(REBOOT)
    except TransportError:
        pass
    version = struct.unpack(">I", manifest[8:12])[0]
    confirmed = client.wait_state(0, version)
    progress(f"Confirmed application firmware version {version}")
    return confirmed

def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", choices=("info", "flash"))
    parser.add_argument("--board", required=True, choices=TARGETS)
    parser.add_argument("--interface", default="pcan", help="python-can backend (default: pcan)")
    parser.add_argument("--channel", default="PCAN_USBBUS1")
    parser.add_argument("--elf", type=Path, help="Application ELF with adjacent .ota.json")
    parser.add_argument("--private-key", default="~/.config/sunlite/firmware-ed25519-private.pem")
    parser.add_argument("--objcopy", default="arm-none-eabi-objcopy")
    args = parser.parse_args(argv)
    if args.command == "flash" and args.elf is None:
        parser.error("flash requires --elf")
    try:
        import can
    except ImportError:
        parser.exit(1, "Install dependencies: python -m pip install -r tools/requirements-can-flash.txt\n")
    try:
        target, node = TARGETS[args.board]
        if args.command == "flash":
            image, manifest = prepare_image(args.elf, target, args.private_key, args.objcopy)
        with can.Bus(interface=args.interface, channel=args.channel, bitrate=500000,
                     ignore_config=True, receive_own_messages=False,
                     can_filters=[{"can_id": 0x18DAF100 | node, "can_mask": 0x1FFFFFFF,
                                   "extended": True}]) as bus:
            client = Client(CanTransport(bus, node), target)
            if args.command == "info":
                board = client.hello()
                board["state"] = STATES[board["state"]]
                print(json.dumps(board, indent=2))
            else:
                flash(client, image, manifest)
    except (FlashError, can.CanError, OSError, ValueError, subprocess.CalledProcessError, ImportError) as error:
        print(f"CAN flash failed: {error}", file=sys.stderr)
        return 1
    except KeyboardInterrupt:
        print("Interrupted. Rerun info to check board recovery state.", file=sys.stderr)
        return 130
    return 0

if __name__ == "__main__":
    sys.exit(main())
