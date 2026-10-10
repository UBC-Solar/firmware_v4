# Direct CAN flashing with PCAN

`bootloader_send_can.py` flashes an application directly from the laptop through
a USB-to-CAN adapter, without the Pi or TEL gateway. It supports MDI, DRD, and
STR on the existing 500 kbit/s classic CAN bus. All boards can remain connected;
`--board` selects one destination. TEL, HVC, and MST are not supported by this tool.

The tool uses the selected ELF's CMake-generated `.elf.ota.json` sidecar,
asks the running application to enter its bootloader, transfers the image with
offset-checked acknowledgements, and confirms the firmware version after reboot.
It preserves CRC32 integrity checks, board identification, and application
interlocks. It does not build firmware or allocate a firmware version.

## Basic update format

Protocol version 3 sends raw messages inside ISO-TP, without COBS or delimiters.
The 16-byte BEGIN payload contains the board ID, firmware version, image size,
and image CRC32. Packet CRC32, whole-image readback CRC32, offset checks, and
vector-last programming detect accidental corruption and interrupted writes.

Identical versions and downgrades are allowed. Hardware revision ranges, minimum
bootloader requirements, SHA-256, signatures, and trial-boot confirmation are
not part of this basic updater. The sender still checks the application version
after reboot to report whether the requested image is running.

Protocol 1/2 bootloaders and applications are incompatible with this sender.
Install the protocol-3 bootloader and rebuilt application through SWD once.
Both must use the new update service; replacing only one leaves CAN flashing
incompatible. Rebuild to generate the schema-2 `.elf.ota.json` sidecar.

## Host setup

Install into a Python virtual environment (Python 3.9 or newer):

```sh
python3 -m venv .venv
.venv/bin/python -m pip install -r tools/requirements-can-flash.txt
```

On Windows, use `py -m venv .venv`, then `.venv\Scripts\python.exe` in place of
`.venv/bin/python` in the commands below. Install the PEAK PCAN driver and
PCAN-Basic library. The default channel is `PCAN_USBBUS1`; use `--channel
PCAN_USBBUS2` for a second channel.

On macOS, python-can's PCAN backend needs the
[MacCAN PCBUSB library](https://github.com/mac-can/PCBUSB-Library), including a
library build matching the Python process architecture. A `PCBUSB library not
found` error means the host library must be installed before querying a board.
See the [python-can PCAN backend](https://github.com/hardbyte/python-can/blob/main/can/interfaces/pcan/basic.py)
for the library loading path.

On Linux, PCAN can also use the kernel SocketCAN driver. Configure `can0` for
500000 bit/s, bring it up, and pass `--interface socketcan --channel can0`.
See [python-can's PCAN documentation](https://python-can.readthedocs.io/en/stable/interfaces/pcan.html).

For a Linux adapter exposed as `can0`:

```sh
sudo ip link set can0 down
sudo ip link set can0 type can bitrate 500000
sudo ip link set can0 up
```

Then add `--interface socketcan --channel can0` to the query and flash commands.


Connect the adapter's CAN-H, CAN-L, and reference ground to the powered board
bus, with termination at the two bus ends. Run only one flashing client at a time; the client uses tester address `0xF1`. Use classic CAN, not CAN FD.

## Provision once, then query

Each board must have its CAN bootloader installed by SWD at `0x08000000`.
Its application must be linked at `0x08008000` and include the CAN update service.
A board with blank/invalid application vectors stays in bootloader recovery and
can also be flashed directly. Stock application-only firmware cannot enter this
bootloader; provision it through SWD first.

Build options and initial provisioning are described in the
[bootloader README](../firmware/common/bootloader/README.md#configure-and-provision).
Query each destination before flashing:

```sh
.venv/bin/python tools/bootloader_send_can.py info --board mdi
.venv/bin/python tools/bootloader_send_can.py info --board drd
.venv/bin/python tools/bootloader_send_can.py info --board str
```

The response includes state, firmware and bootloader versions, slot size,
and capabilities. Bit `0x1` means the running application allows entry into its
bootloader.
`info` queries the selected board with a targeted HELLO.

## Build and flash one board

Choose a numeric firmware version to identify the build. It may be the same
as or older than the installed version. For example, build version 2:

```sh
cmake --preset Debug -S firmware/components/mdi -B firmware/components/mdi/build \
  -DFW_UPDATE_FIRMWARE_VERSION=2
cmake --build firmware/components/mdi/build --target mdi

.venv/bin/python tools/bootloader_send_can.py flash --board mdi \
  --elf firmware/components/mdi/build/mdi.elf
```

`arm-none-eabi-objcopy` must be on PATH, or provide its location using
`--objcopy`. The tool reads exactly `<ELF path>.ota.json`; it rejects another
board's sidecar and invalid application vectors before opening CAN. The
bootloader checks image metadata before erasing and verifies the image CRC32
before committing its vectors. Replace `mdi`
with `drd` or `str` to build and flash that board. On Windows, provide native
paths and shell syntax for the CMake commands.

The existing remote-board Debug configuration enables the bench update fallback;
Release builds require an implemented board safety hook. `UPDATE_NOT_ALLOWED`
means the running application's interlock refused the operation. The host tool
does not override it.

Success ends with `Confirmed application firmware version <number>`. A timeout,
NACK, wrong offset, or wrong post-reboot version returns a nonzero exit code.
Interrupted image writes remain in bootloader recovery. Query the state and retry
the image, including the same version. Trial recovery is removed: if a fully
programmed application crashes before it can service CAN, SWD may be needed
to recover it. Do not interpret a partial progress log as success.

## Software verification

```sh
make fw-update-test
make can-flash-test PYTHON=.venv/bin/python
```

The direct sender suite compiles the actual firmware C transport and wire codec
into a host test peer. It covers segmentation, sequence wrap, exact request replay
after a lost ACK, recovery, interlocks, corruption detection, protocol-version
rejection, reflashes/downgrades, preservation of zero bytes, and version confirmation. A host C compiler is required for this suite.

These are software tests. A powered PCAN-to-board flash, reboot, and interruption
test are still required to establish operation on the hardware.
