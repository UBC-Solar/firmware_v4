# Direct CAN flashing with PCAN

`bootloader_send_can.py` flashes an application directly from the laptop through
a USB-to-CAN adapter, without the Pi or TEL gateway. It supports MDI, DRD, and
STR on the existing 500 kbit/s classic CAN bus. All boards can remain connected;
`--board` selects one destination. TEL's own bootloader is currently UART-only.
HVC, MST, and the legacy Nucleo UART prototype are not supported by this tool.

This is the first of the repository's [two flashing paths](../README.md#flashing-paths).
The separate [OTA framework path](vscode-sunlite-ota/README.md) continues to use
SSH, the Pi, and TEL. Direct CAN has no runtime dependency on that framework;
it shares the board protocol, signing format, and installed version counter.
An image installed by either path requires the next update through either path
to satisfy the same bootloader version rules.

The tool signs the selected ELF using its CMake-generated `.elf.ota.json`
sidecar, asks the running application to enter its bootloader, transfers the
image with offset-checked acknowledgements, and checks the target and compiled
firmware version after reboot. It uses the same authenticated update protocol,
anti-rollback rules, application interlock, and trial confirmation as routed OTA.
It does not build the application or automatically allocate a firmware version.

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

Connect the adapter's CAN-H, CAN-L, and reference ground to the powered board
bus, with termination at the two bus ends. Do not run a concurrent Pi/TEL OTA
session: both clients use tester address `0xF1`. Use classic CAN, not CAN FD.

## Provision once, then query

Each board must already have its signed CAN bootloader installed by SWD at
`0x08000000`, using the public key corresponding to the laptop's private key.
Its application must be linked at `0x08008000` and include the CAN OTA service.
A board with blank/invalid application vectors stays in bootloader recovery and
can also be flashed directly. Stock application-only firmware cannot enter this
bootloader; provision it through SWD first.

Key creation and initial provisioning are described in the
[bootloader README](../firmware/common/bootloader/README.md#configure-and-provision).
Query each destination before flashing:

```sh
.venv/bin/python tools/bootloader_send_can.py info --board mdi
.venv/bin/python tools/bootloader_send_can.py info --board drd
.venv/bin/python tools/bootloader_send_can.py info --board str
```

The response includes state, firmware version, hardware revision, slot size,
and capabilities. Capability bit `0x4` means signature verification is enabled;
bit `0x1` means the running application allows entry into its bootloader.
`info` uses a targeted HELLO, which also confirms a pending application trial.

## Build and flash one board

Choose a numeric firmware version greater than the version returned by `info`.
For example, if MDI reports version 1, build version 2:

```sh
cmake --preset Debug -S firmware/components/mdi -B firmware/components/mdi/build \
  -DSUNLITE_OTA_PUBLIC_KEY_FILE="$HOME/.config/sunlite/firmware-ed25519.pub" \
  -DSUNLITE_OTA_FIRMWARE_VERSION=2 \
  -DSUNLITE_OTA_DISPLAY_VERSION=dev-2
cmake --build firmware/components/mdi/build --target mdi

.venv/bin/python tools/bootloader_send_can.py flash --board mdi \
  --elf firmware/components/mdi/build/mdi.elf \
  --private-key ~/.config/sunlite/firmware-ed25519-private.pem
```

`arm-none-eabi-objcopy` must be on PATH, or provide its location using
`--objcopy`. The tool reads exactly `<ELF path>.ota.json`; it rejects another
board's sidecar and invalid application vectors before opening CAN. The
bootloader independently verifies the signature before erasing. Replace `mdi`
with `drd` or `str` to build and flash that board. On Windows, provide native
paths and shell syntax for the CMake commands.

The existing remote-board Debug configuration enables the bench update fallback;
Release builds require an implemented board safety hook. `UPDATE_NOT_ALLOWED`
means the running application's interlock refused the operation. The host tool
does not override it.

Success ends with `Confirmed application firmware version <number>`. A timeout,
NACK, wrong offset, or wrong post-reboot version returns a nonzero exit code.
Interrupted image writes remain in bootloader recovery. Query the state and retry
the signed image; a valid image with a failed trial requires a newer version under
the existing anti-rollback rule. Do not interpret a partial progress log as success.

## Software verification

```sh
make ota-contract-test
make can-flash-test PYTHON=.venv/bin/python
```

The direct sender suite compiles the actual firmware C transport and wire codec
into a host test peer. It covers segmentation, sequence wrap, exact request replay
after a lost ACK, recovery, interlocks, signing vectors, and version confirmation.
Signing tests require the dependencies above; without `cryptography`, unittest
reports those tests as skipped. A host C compiler is required for this suite.

These are software tests. A powered PCAN-to-board flash, reboot, and interruption
test are still required to establish operation on the hardware.
