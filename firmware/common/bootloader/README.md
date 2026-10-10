# Common STM32 CAN bootloader

The MDI, DRD, and STR boards share a basic updater for direct laptop → PCAN →
CAN flashing. TEL, the Pi, and a gateway are not required. See the
[PCAN setup and flashing guide](../../../tools/README-can-flash.md).

## Memory layout

All three STM32F103RC boards use the same layout:

| Region | Address range | Size |
| --- | --- | --- |
| Bootloader | `0x08000000`–`0x08007FFF` | 32 KiB |
| Application | `0x08008000`–`0x0803EFFF` | 220 KiB |
| Unused reserved flash | `0x0803F000`–`0x0803FFFF` | 4 KiB |
| SRAM | `0x20000000`–`0x2000BFFF` | 48 KiB |

The bootloader checks the application's stack/reset vectors before jumping.
During updates it keeps these first eight bytes in RAM and writes them last,
after verifying the complete image. It verifies the reset vector before committing
the stack pointer, keeping failed reset-vector writes unbootable.
No persistent firmware records are stored. The final two flash pages remain
unused to keep the existing application linker layout compatible.

## CAN addresses

Classic CAN runs at 500 kbit/s with extended identifiers
`0x18DA<destination><source>`. The laptop uses tester address `0xF1`.

| Board | Target ID | Node address | Bootloader pins |
| --- | --- | --- | --- |
| MDI | `0x4D444920` | `0x10` | PB8/PB9 |
| DRD | `0x44524420` | `0x11` | PB8/PB9 |
| STR | `0x53545220` | `0x12` | PB8/PB9 |

ISO-TP provides message boundaries, segmentation, flow control, and timeouts.
Raw protocol messages may contain zero bytes; no escaping or delimiter is used.

## Basic protocol

Protocol version 3 uses a 14-byte header containing magic, protocol version,
command, session, sequence, and payload length, followed by payload and CRC32.
The maximum DATA chunk is 1024 bytes. BEGIN is just four big-endian 32-bit values:

| Offset | Value |
| --- | --- |
| 0 | Board target ID |
| 4 | Firmware version |
| 8 | Image size in bytes |
| 12 | Whole-image CRC32 |

The version identifies the build; identical versions and downgrades are allowed.
There are no hardware revision ranges, minimum bootloader requirements, SHA-256,
signatures, rollback, resumable transfer, or trial-boot confirmation.
CRC32 checks accidental corruption and does not authenticate firmware.

BOARD_INFO reports the target ID, bootloader version, firmware version, state,
maximum chunk size, slot capacity, and update-allowed capability. Its payload is
23 bytes. The application reports its compiled version; bootloader mode reports
`0` (unknown) for the installed version. The sender checks the application version
after reboot.

## Configure and provision

Each board CMake project includes `bootloader_target.cmake`, which creates
`<board>_bootloader`, links `STM32F103RC_BOOTLOADER_FLASH.ld`, and emits a binary.
Release bootloaders use `-Os` and link-time optimization. Configure the application
version with `FW_UPDATE_FIRMWARE_VERSION`; CMake generates a schema-2
`.elf.ota.json` containing the target ID and compiled firmware version.

Install the bootloader binary at `0x08000000` through SWD and the rebuilt
application at `0x08008000`. Migrating application-only firmware also requires
clearing the old application area so it cannot resemble a relocated image.

Protocol 1/2 firmware and senders are incompatible with protocol 3. Install both
the updated bootloader and application via SWD once, then use the updated sender.
No signing keys or cryptography dependencies are needed.

Example build:

```sh
cmake --preset Debug -S firmware/components/mdi -B firmware/components/mdi/build \
  -DFW_UPDATE_FIRMWARE_VERSION=2
cmake --build firmware/components/mdi/build --target mdi mdi_bootloader
```

## Boot entry and recovery

Invalid/blank application vectors keep the bootloader in update mode. A valid
application starts normally unless it approved a one-shot update request in
backup register DR10 before a software reset. The token requires `SFTRSTF` and
rejects watchdog, low-power, and power-reset flags. Rejected tokens are consumed;
ordinary boots preserve reset-cause flags for application diagnostics.

The application safety hook remains board-specific. Debug builds can enable the
bench fallback; Release builds require the board's safe-state check. Traffic on
CAN during normal startup does not bypass this check. The bootloader services
an inherited watchdog and returns to a valid application after 30 seconds idle.

Trial recovery is removed. A verified image boots without laptop confirmation.
If that image crashes before it can answer CAN requests, recovery may require
SWD. Interrupted image writes remain recoverable because vectors are written last.

## Following an update in the code

Start at `BootloaderRun()` in `bootloader.c`. It launches the app or enters
`BootloaderEnterUpdateMode()` to poll CAN. Complete messages reach the engine:

1. `ValidateBeginUpdate()` checks board ID and image size, then erases image pages.
2. `WriteData()` writes chunks in order, keeping the stack/reset vectors in RAM.
3. `FinishUpdate()` checks the image CRC32 using flash plus the withheld vectors,
   then writes the vectors last.
4. `REBOOT` launches the image after its ACK is sent. The sender queries the
   running application to check its board ID and firmware version.

Retries with the same session, sequence, command, length, and payload CRC32 reuse
the cached response without repeating flash writes. Changed retries are rejected.
An update failure locks flash. ABORT resets the transfer so a fresh BEGIN can retry.

The production code uses six C files and three headers:

| Module | Responsibility |
| --- | --- |
| `bootloader.c/.h` | Startup, memory layout, CAN setup, watchdog, handoff, and fault handlers |
| `bootloader_boot_request.c` | Application-approved boot-entry token; API in `bootloader.h` |
| `bootloader_storage.c` | Flash erase, programming, and readback verification; API in `bootloader.h` |
| `fw_update_bootloader_engine.c` | Image validation, installation, and retries; API in `bootloader.h` |
| `fw_update_can.c/.h` | STM32 HAL adapter and application discovery/reset service |
| `fw_update_protocol.c/.h` | Raw message codec, CRC32, and ISO-TP transport |

The code follows the root `.clang-format` settings and documents each function.
`BOOTLOADER_BUILD` excludes the application service from bootloader builds.
The startup/transport/flash grouping is consistent with
[Ferenc Nemeth's STM32 bootloader](https://github.com/ferenc-nemeth/stm32-bootloader);
no external source was copied.

## Verification

```sh
make fw-update-test
make can-flash-test PYTHON=.venv/bin/python
```

Host tests cover C/Python wire vectors, CRC32, zero-containing messages, ISO-TP
segmentation and timeouts, exact retries, changed retries, board checks, reflashes
and downgrades, flash readback corruption, split vectors, boot tokens, and
application write failures and slot boundaries. The combined engine/flash test
also programs a full 220 KiB slot, an odd-length image across a chunk boundary,
and fails each launch-vector halfword in turn. It also corrupts the reset vector
into another address inside the slot to check that the image remains unbootable.
It can additionally test built
application binaries passed to `tests/run_can_transport_tests.sh`.
Hardware flashing, reboot,
and interrupted-power tests still need to be performed on each board.
