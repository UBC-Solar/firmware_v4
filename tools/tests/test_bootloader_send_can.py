"""Exercise the direct sender against the production C CAN transport/codec."""
from collections import deque
import ctypes
import json
import os
from pathlib import Path
import shlex
import struct
import subprocess
import sys
import tempfile
import time
import zlib
from types import SimpleNamespace
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import bootloader_send_can as ota

ROOT = Path(__file__).resolve().parents[2]


def message(**kwargs):
    return SimpleNamespace(is_remote_frame=False, is_error_frame=False, is_fd=False, **kwargs)


class NativeBus:
    """In-memory bus; the device transport and response encoder are real C."""
    def __init__(self, library, target=0x4D444920, node=0x10):
        self.library, self.target = library, target
        self.queue = deque()
        self.state, self.version, self.capabilities = 0, 1, 1
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
                reported_version = self.version if self.state == 0 else 0
                response = struct.pack(">IIIBHII", self.target, 1, reported_version,
                                       self.state, 1024, 225280, self.capabilities)
            else:
                if kind == ota.ENTER:
                    self.state = 1
                elif kind == ota.BEGIN:
                    assert len(payload) == 16
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
                    assert zlib.crc32(self.image) == struct.unpack(">I", self.manifest[12:16])[0]
                    self.state = 3
                elif kind == ota.REBOOT:
                    self.state = 0
                    self.version = struct.unpack(">I", self.manifest[4:8])[0]
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
        cls.temp = tempfile.TemporaryDirectory(prefix="can-flash-test-")
        output = Path(cls.temp.name) / "peer.so"
        boot = ROOT / "firmware/common/bootloader"
        subprocess.run(shlex.split(os.environ.get("CC", "cc")) + [
            "-std=c11", "-Wall", "-Wextra", "-Werror", "-shared", "-fPIC", f"-I{boot}",
            str(Path(__file__).with_name("can_flash_peer.c")),
            str(boot / "fw_update_protocol.c"),
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
        cls.lib.PeerSendFrame.argtypes = [byte_ptr, ctypes.c_uint16, ctypes.c_uint32]

    @classmethod
    def tearDownClass(cls):
        cls.temp.cleanup()

    def setUp(self):
        self.bus = NativeBus(self.lib)
        self.transport = ota.CanTransport(self.bus, 0x10, message)
        self.client = ota.Client(self.transport, self.bus.target)
        self.image = struct.pack("<II", 0x2000C000, 0x08008009) + bytes(range(256)) * 4 + b"odd"
        self.manifest = struct.pack(">IIII", self.bus.target, 2, len(self.image), zlib.crc32(self.image))
        self.bus.expected_image = self.image

    def run_flash(self):
        return ota.flash(self.client, self.image, self.manifest, lambda _: None)

    def test_board_transmit_capacity(self):
        encoded = (ctypes.c_uint8 * 65)(*([1] * 65))
        self.assertFalse(self.lib.PeerSendFrame(encoded, 65, self.bus.now()))
        self.assertTrue(self.lib.PeerSendFrame(encoded, 64, self.bus.now()))

    def test_prepare_image_needs_no_key(self):
        with tempfile.TemporaryDirectory() as directory:
            elf = Path(directory) / "mdi.elf"
            Path(str(elf) + ".ota.json").write_text(json.dumps({
                "schema": 2, "targetId": self.bus.target,
                "firmwareVersion": 2}))
            def objcopy(command, check):
                Path(command[-1]).write_bytes(self.image)
            with patch.object(ota.subprocess, "run", side_effect=objcopy):
                image, manifest = ota.prepare_image(elf, self.bus.target, "objcopy")
            self.assertEqual(manifest, self.manifest)
            self.assertEqual(image, self.image)

    def test_golden_hello(self):
        self.assertEqual(ota.encode_message(ota.HELLO, 0x12345678, 1).hex(),
                         '535503011234567800000001000035eed0e2')

    def test_full_flash_and_exact_retry_through_production_transport(self):
        self.bus.drop_data_ack = True
        self.assertEqual(self.run_flash()["firmware_version"], 2)
        requests = [r for r in self.bus.requests if ota.decode_message(r)[0] == ota.DATA]
        self.assertEqual(requests[0], requests[1])
        self.assertEqual(self.bus.image, self.image)

    def test_bootloader_recovery(self):
        self.bus.state = 1
        self.assertEqual(self.client.hello()["firmware_version"], 0)
        self.assertEqual(self.run_flash()["state"], 0)
        self.assertNotIn(ota.ENTER, [ota.decode_message(r)[0] for r in self.bus.requests])

    def test_preflight_rejects_closed_interlock(self):
        self.bus.capabilities = 0
        with self.assertRaisesRegex(ota.FlashError, "interlock"):
            self.run_flash()
        self.assertEqual(len(self.bus.image), 0)
        self.assertTrue(all(ota.decode_message(r)[0] == ota.HELLO for r in self.bus.requests))

    def test_same_and_older_versions_are_allowed(self):
        for installed in (2, 3):
            with self.subTest(installed=installed):
                self.bus.version = installed
                self.bus.state = 0
                self.assertEqual(self.run_flash()["firmware_version"], 2)

    def test_zero_bytes_and_trailing_zero_preserved(self):
        # Find a valid frame whose CRC ends in zero; ISO-TP must preserve it.
        for sequence in range(1024):
            frame = ota.encode_message(ota.DATA, 1, sequence, b"\0A\0")
            if frame[-1] == 0:
                break
        else:
            self.fail("No trailing-zero CRC found")
        self.assertEqual(ota.decode_message(frame)[3], b"\0A\0")
        with patch.object(self.bus, "process") as process:
            self.transport.send(frame)
        process.assert_called_once_with(frame)

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
        with self.assertRaisesRegex(ota.FlashError, "FLASH_ERROR"):
            client.request(ota.BEGIN, self.manifest)
        self.assertEqual(transport.send.call_count, 1)

    def test_prepare_rejects_wrong_target(self):
        with tempfile.TemporaryDirectory() as directory:
            elf = Path(directory) / "mdi.elf"
            Path(str(elf) + ".ota.json").write_text(json.dumps({
                "schema": 2, "targetId": ota.TARGETS["str"][0],
                "firmwareVersion": 2}))
            with self.assertRaisesRegex(ota.FlashError, "selected board"):
                ota.prepare_image(elf, self.bus.target, "objcopy")

    def test_legacy_protocol_rejected(self):
        raw = struct.pack(">2sBBIIH", b"SU", 1, ota.HELLO, 1, 1, 0)
        frame = raw + struct.pack(">I", zlib.crc32(raw))
        with self.assertRaisesRegex(ota.TransportError, "header"):
            ota.decode_message(frame)


if __name__ == "__main__":
    unittest.main()
