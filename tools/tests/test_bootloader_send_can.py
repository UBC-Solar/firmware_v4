"""Exercise the direct sender against the production C CAN transport/codec."""
from collections import deque
import ctypes
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import shlex
import struct
import subprocess
import sys
import tempfile
import time
from types import SimpleNamespace
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import bootloader_send_can as ota

ROOT = Path(__file__).resolve().parents[2]
HAS_CRYPTO = importlib.util.find_spec("cryptography") is not None


def message(**kwargs):
    return SimpleNamespace(is_remote_frame=False, is_error_frame=False, is_fd=False, **kwargs)


class NativeBus:
    """In-memory bus; the device transport and response encoder are real C."""
    def __init__(self, library, target=0x4D444920, node=0x10):
        self.library, self.target = library, target
        self.queue = deque()
        self.state, self.version, self.capabilities = 0, 1, 5
        self.image = bytearray()
        self.expected_image = b""
        self.cached = None
        self.requests = []
        self.drop_data_ack = False
        self.bad_ack_offset = False
        self.bad_confirmation = False
        self.callback = self.library.callback_type(self.emit)
        self.library.PeerInit(self.callback, node)

    def now(self):
        return int(time.monotonic() * 1000) & 0xFFFFFFFF

    def emit(self, user, can_id, data, length):
        self.queue.append(message(arbitration_id=can_id, is_extended_id=True,
                                  data=bytes(data[:length])))
        return True

    def send(self, msg, timeout):
        data = (ctypes.c_uint8 * len(msg.data)).from_buffer_copy(msg.data)
        result = self.library.PeerInput(msg.arbitration_id, data, len(data), self.now())
        if result == 3:
            raise AssertionError("Production C transport rejected Python frame")
        encoded = (ctypes.c_uint8 * ota.MAX_FRAME)()
        size = self.library.PeerTake(encoded)
        if size < 0:
            raise AssertionError("Production C codec rejected Python request")
        if size:
            self.process(bytes(encoded[:size]))

    def recv(self, timeout):
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            self.library.PeerPoll(self.now())
            if self.queue:
                return self.queue.popleft()
            # An intentionally lost reply need not slow the test by 5 seconds.
            if self.dropped:
                self.dropped = False
                return None
            time.sleep(0.001)
        return None

    dropped = False

    def process(self, encoded):
        kind, session, sequence, payload = ota.decode_message(encoded)
        self.requests.append(encoded)
        if self.cached and self.cached[0] == encoded:
            reply, response = self.cached[1:]
        else:
            reply = ota.ACK
            offset = len(self.image)
            status = 0
            if kind == ota.HELLO:
                assert payload == struct.pack(">I", self.target)
                reply = ota.BOARD_INFO
                response = struct.pack(">IH I I BB H I I I", self.target, 1, 1,
                                       self.version, 0, self.state, 1024, 225280, 0,
                                       self.capabilities)
            else:
                if kind == ota.ENTER:
                    self.state = 1
                elif kind == ota.BEGIN:
                    assert len(payload) == 116
                    self.manifest = payload
                    self.image.clear()
                    self.state = 2
                    offset = 0
                elif kind == ota.DATA:
                    assert struct.unpack(">I", payload[:4])[0] == len(self.image)
                    self.image.extend(payload[4:])
                    offset = len(self.image) + int(self.bad_ack_offset)
                elif kind == ota.END:
                    assert bytes(self.image) == self.expected_image
                    assert hashlib.sha256(self.image).digest() == self.manifest[20:52]
                    self.state = 3
                elif kind == ota.REBOOT:
                    self.state = 0
                    self.version = struct.unpack(">I", self.manifest[8:12])[0]
                    self.version += int(self.bad_confirmation)
                response = bytes([kind, status]) + struct.pack(">I", offset)
            self.cached = encoded, reply, response
            if kind == ota.DATA and self.drop_data_ack:
                self.drop_data_ack = False
                self.dropped = True
                return
        data = (ctypes.c_uint8 * len(response)).from_buffer_copy(response)
        assert self.library.PeerReply(reply, session, sequence, data, len(response), self.now())


class DirectCanTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory(prefix="sunlite-can-test-")
        output = Path(cls.temp.name) / "peer.so"
        boot = ROOT / "firmware/common/bootloader"
        subprocess.run(shlex.split(os.environ.get("CC", "cc")) + [
            "-std=c11", "-Wall", "-Wextra", "-Werror", "-shared", "-fPIC", f"-I{boot}",
            str(Path(__file__).with_name("can_flash_peer.c")),
            str(boot / "sunlite_ota_can_transport.c"), str(boot / "sunlite_ota_protocol.c"),
            "-o", str(output)], check=True)
        cls.lib = ctypes.CDLL(str(output))
        byte_ptr = ctypes.POINTER(ctypes.c_uint8)
        cls.lib.callback_type = ctypes.CFUNCTYPE(ctypes.c_bool, ctypes.c_void_p,
                                                 ctypes.c_uint32, byte_ptr, ctypes.c_uint8)
        cls.lib.PeerInit.argtypes = [cls.lib.callback_type, ctypes.c_uint8]
        cls.lib.PeerInput.argtypes = [ctypes.c_uint32, byte_ptr, ctypes.c_uint8, ctypes.c_uint32]
        cls.lib.PeerTake.argtypes = [byte_ptr]
        cls.lib.PeerPoll.argtypes = [ctypes.c_uint32]
        cls.lib.PeerReply.argtypes = [ctypes.c_uint8, ctypes.c_uint32, ctypes.c_uint32,
                                     byte_ptr, ctypes.c_uint16, ctypes.c_uint32]

    @classmethod
    def tearDownClass(cls):
        cls.temp.cleanup()

    def setUp(self):
        self.bus = NativeBus(self.lib)
        self.transport = ota.CanTransport(self.bus, 0x10, message)
        self.client = ota.Client(self.transport, self.bus.target)
        self.image = struct.pack("<II", 0x2000C000, 0x08008009) + bytes(range(256)) * 4 + b"odd"
        self.manifest = struct.pack(">IHHIII", self.bus.target, 1, 1, 2, len(self.image), 1)
        self.manifest += hashlib.sha256(self.image).digest() + bytes(64)
        self.bus.expected_image = self.image

    def run_flash(self):
        return ota.flash(self.client, self.image, self.manifest, lambda _: None)

    def test_golden_hello(self):
        self.assertEqual(ota.encode_message(ota.HELLO, 0x12345678, 1).hex(),
                         "095355010112345678010102010105d9d54e7d")

    def test_full_flash_and_exact_retry_through_production_transport(self):
        self.bus.drop_data_ack = True
        self.assertEqual(self.run_flash()["firmware_version"], 2)
        requests = [r for r in self.bus.requests if ota.decode_message(r)[0] == ota.DATA]
        self.assertEqual(requests[0], requests[1])
        self.assertEqual(self.bus.image, self.image)

    def test_bootloader_recovery(self):
        self.bus.state, self.bus.version = 1, 0
        self.assertEqual(self.run_flash()["state"], 0)
        self.assertNotIn(ota.ENTER, [ota.decode_message(r)[0] for r in self.bus.requests])

    def test_preflight_rejects_closed_interlock_unsigned_and_old_version(self):
        for caps, version, expected in [(4, 1, "interlock"), (1, 1, "signature"), (5, 2, "greater")]:
            with self.subTest(caps=caps, version=version):
                self.bus.capabilities, self.bus.version = caps, version
                with self.assertRaisesRegex(ota.FlashError, expected):
                    self.run_flash()
                self.assertEqual(len(self.bus.image), 0)
        self.assertTrue(all(ota.decode_message(r)[0] == ota.HELLO for r in self.bus.requests))

    def test_bad_data_offset_stops_before_end(self):
        self.bus.bad_ack_offset = True
        with self.assertRaisesRegex(ota.FlashError, "offset"):
            self.run_flash()
        self.assertNotIn(ota.END, [ota.decode_message(r)[0] for r in self.bus.requests])

    def test_wrong_application_version_is_not_success(self):
        self.bus.bad_confirmation = True
        with self.assertRaisesRegex(ota.FlashError, "version differs"):
            self.run_flash()

    def test_crc_corruption(self):
        encoded = bytearray(ota.encode_message(ota.HELLO, 1, 1))
        encoded[-1] ^= 1
        with self.assertRaises(ota.TransportError):
            ota.decode_message(encoded)

    def test_wrong_can_id_and_standard_frames_are_ignored(self):
        self.bus.queue.extend([
            message(arbitration_id=self.transport.rx_id ^ 1, is_extended_id=True, data=b"\x01A"),
            message(arbitration_id=self.transport.rx_id, is_extended_id=False, data=b"\x01B"),
            message(arbitration_id=self.transport.rx_id, is_extended_id=True, data=b"\x01C"),
        ])
        self.assertEqual(self.transport.receive(time.monotonic() + 1), b"C")

    def test_malformed_response_sequence_and_flow_control(self):
        with patch.object(self.transport, "frame", side_effect=[b"\x10\x08abcdef", b"\x22gh"]), \
                patch.object(self.transport, "emit"):
            with self.assertRaisesRegex(ota.TransportError, "sequence"):
                self.transport.receive(time.monotonic() + 1)
        for frame, expected in [(b"\x32\0\0", "rejected"), (b"\x31\0\0", "WAIT")]:
            with patch.object(self.transport, "frame", return_value=frame):
                with self.assertRaisesRegex(ota.TransportError, expected):
                    self.transport.flow_control()

    def test_nack_and_stale_response(self):
        transport = unittest.mock.Mock()
        client = ota.Client(transport, self.bus.target)
        transport.receive.side_effect = [
            ota.encode_message(ota.ACK, client.session ^ 1, 1, b"\x04\0\0\0\0\0"),
            ota.encode_message(ota.NACK, client.session, 1, b"\x04\x05\0\0\0\0"),
        ]
        with self.assertRaisesRegex(ota.FlashError, "BAD_SIGNATURE"):
            client.request(ota.BEGIN, self.manifest)
        self.assertEqual(transport.send.call_count, 1)

    @unittest.skipUnless(HAS_CRYPTO, "install requirements-can-flash.txt for signing tests")
    def test_signing_matches_firmware_golden_vector(self):
        from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey
        key = Ed25519PrivateKey.from_private_bytes(bytes(range(32)))
        fields = struct.pack(">IHHIII", 0x54454C45, 1, 2, 17, 8, 1)
        fields += bytes.fromhex("c3bf47ea1f4a4a605470313cacb3a44f4a461f68c6faeab07e737610cb5ac835")
        self.assertEqual(key.sign(ota.SIGNING_DOMAIN + fields).hex(),
                         "70f32f85b8611856690571c2be2c99d898ea24a27cd788ea689392a91361aa31"
                         "0ae3071174e919222ae64b7ca7ab8a71b8f11f19ece6e064dd7c58a4cddc4403")

    @unittest.skipUnless(HAS_CRYPTO, "install requirements-can-flash.txt for signing tests")
    def test_prepare_elf_sidecar_and_signature(self):
        from cryptography.hazmat.primitives import serialization
        from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey
        with tempfile.TemporaryDirectory() as directory:
            elf = Path(directory) / "mdi.elf"
            key_path = Path(directory) / "key.pem"
            key = Ed25519PrivateKey.generate()
            key_path.write_bytes(key.private_bytes(serialization.Encoding.PEM,
                                  serialization.PrivateFormat.PKCS8, serialization.NoEncryption()))
            metadata = {"schema": 1, "targetId": "0x4D444920", "hardwareRevisionMin": 1,
                        "hardwareRevisionMax": 1, "firmwareVersion": 2, "minimumBootloaderVersion": 1}
            sidecar = Path(str(elf) + ".ota.json")
            sidecar.write_text(json.dumps(metadata))
            def objcopy(command, check):
                Path(command[-1]).write_bytes(self.image)
            with patch.object(ota.subprocess, "run", side_effect=objcopy):
                image, manifest = ota.prepare_image(elf, self.bus.target, key_path, "objcopy")
                self.assertEqual(image, self.image)
                key.public_key().verify(manifest[52:], ota.SIGNING_DOMAIN + manifest[:52])
                metadata["targetId"] = ota.TARGETS["str"][0]
                sidecar.write_text(json.dumps(metadata))
                with self.assertRaisesRegex(ota.FlashError, "selected board"):
                    ota.prepare_image(elf, self.bus.target, key_path, "objcopy")


if __name__ == "__main__":
    unittest.main()
