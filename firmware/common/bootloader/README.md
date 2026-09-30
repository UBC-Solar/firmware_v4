# Common STM32 Bootloader

This directory contains the shared bootloader core for STM32F103 boards.

## Car-board layout

All six STM32F103RC application/bootloader pairs use the same protected layout:

- bootloader: `0x08000000` through `0x08007FFF` (`32 KiB`)
- application: `0x08008000` through `0x0803EFFF` (`220 KiB`)
- primary metadata page: `0x0803F000` through `0x0803F7FF`
- backup metadata page: `0x0803F800` through `0x0803FFFF`
- SRAM: `0x20000000` through `0x2000BFFF` (`48 KiB`)

The application linker scripts start at `0x08008000` and reserve both final
flash pages. The bootloader validates the application's stack pointer and reset
vector before jumping and writes the first eight vector bytes last during an
update. An interrupted write therefore returns to the bootloader instead of
booting a partial image.

The current software acceptance pass covers TEL, MDI, DRD, and STR. HVC and MST
remain in the shared target registry, but their board integrations are outside
this pass and must not be inferred from the results below.

## Direct CAN flashing

For a laptop connected directly to CAN through PCAN, use
[`tools/bootloader_send_can.py`](../../../tools/bootloader_send_can.py).
It queries and flashes MDI, DRD, or STR individually using the existing signed
protocol. The Pi and TEL gateway are not required. All boards can remain on the
bus, but TEL self-updates still require UART. See the
[PCAN setup and flash instructions](../../../tools/README-can-flash.md).

## Direct CAN updates

The supported host workflow is laptop → PCAN → MDI, DRD, or STR.
See [setup, provisioning, and flash commands](../../../tools/README-can-flash.md).
The Pi/SSH VS Code extension has been removed. TEL's legacy UART implementation
is retained for existing firmware compatibility; it is not a direct CAN target.

| Board | Target ID | CAN node | Bootloader pins |
| --- | --- | --- | --- |
| MDI | `0x4D444920` | `0x10` | PB8/PB9 |
| DRD | `0x44524420` | `0x11` | PB8/PB9 |
| STR | `0x53545220` | `0x12` | PB8/PB9 |

Classic CAN runs at 500 kbit/s with extended identifiers
`0x18DA<destination><source>`. The laptop uses tester address `0xF1`.
The transport segments COBS/CRC32 frames and supports idempotent retries.
The `fw_update_*` protocol and engine files remain required by direct CAN.

## Configure and provision

For a controlled bench, configure the selected board with
`-DFW_UPDATE_ALLOW_UNSIGNED_BENCH=ON` in Debug. No keys are required.
The sender must also receive `--unsigned-bench`. Release configurations reject
this option. Otherwise configure `FW_UPDATE_PUBLIC_KEY_FILE` with an Ed25519
public PEM and sign with the corresponding private key on the laptop.

Install `<board>_bootloader.bin` at `0x08000000` through SWD once. Applications
are linked at `0x08008000`; the generated `.elf.ota.json` records their target,
hardware revision and compiled version. Increment `FW_UPDATE_FIRMWARE_VERSION`
for later updates. Migrating old application-only firmware requires clearing the
old application area so it cannot be mistaken for a valid relocated image.

The engine checks target, hardware, minimum bootloader version, slot size,
monotonic version and SHA-256. Signed mode verifies Ed25519 before erasing.
Unsigned bench mode omits authentication but retains the other checks.
The single-slot updater commits application vectors last and uses redundant,
CRC-protected metadata pages. There is no rollback or resumable transfer.

After programming, one trial launch is permitted. A targeted HELLO serviced by
the application confirms it. A failed trial stays in bootloader recovery and
requires a newer version if valid vectors remain. Backup registers DR8/DR9 hold
the trial marker and DR10 holds the application-approved boot entry token.
These registers need VBAT retention across loss of VDD; losing both supplies
loses the markers. Test power-interruption recovery on each board.

The application must approve bootloader entry through its safety hook. Debug
bench safety fallback is separate from the unsigned-firmware option.
Bootloaders return to a valid application after 30 seconds idle unless trial
recovery blocks launch. Blank/invalid vectors keep the bootloader available.

Run the protocol, transport, recovery and sender tests with:

```sh
make fw-update-test
make can-flash-test PYTHON=.venv/bin/python
```

## Nucleo-F103RB Test Layout

- Bootloader flash: `0x08000000` through `0x08007FFF` (`32 KiB`)
- Application flash: `0x08008000` through `0x0801FFFF` (`96 KiB`)
- SRAM: `0x20000000` through `0x20004FFF` (`20 KiB`)

Use `STM32F103RB_BOOTLOADER_FLASH.ld` for the bootloader and build the test
application with flash origin `0x08008000` and flash length `96K`.

## Boot entry decision

`BootloaderRun()` enters update mode when:

- the application vector table is invalid, so interrupted/blank-image recovery
  remains possible, or
- the one permitted trial launch was not confirmed (or its redundant marker is
  inconsistent), or
- `BootloaderBoardStayInBootloader()` returns `true` after consuming the
  application-approved reset token (or checking an explicit physical recovery
  condition).

A valid application never enters the bootloader merely because UART/CAN
traffic arrives during an ordinary reset. This prevents the startup path from
bypassing the application's update-safety interlock.

The DR10 entry token requires `SFTRSTF` and rejects watchdog, low-power, and
power-reset flags. STM32F103 also sets `PINRSTF` as a generic companion to a
software reset, so `SFTRSTF | PINRSTF` is accepted while `PINRSTF` alone is
not. The application clears accumulated RCC flags immediately before arming
the token; the bootloader always consumes a present-but-rejected token so it
cannot authorize a later reset. Ordinary boots preserve RCC flags for the
application's reset-cause diagnostics.

The board entry and transport functions are hooks. Keep board-specific GPIO,
CAN, UART, or gateway behavior outside the common bootloader core.

## Build Target

Each car-board CMake project includes a `${board}_bootloader` target through
`bootloader_target.cmake`. The target links against
`STM32F103RC_BOOTLOADER_FLASH.ld`, calls `BootloaderRun()` from
`bootloader_main.c`, and emits a `${board}_bootloader.bin` image.

Example:

```sh
cmake --build firmware/components/mdi/build --target mdi_bootloader
```

For a Nucleo-F103RB target, pass the RB linker script and apply the RB compile
definitions:

```cmake
include(path/to/bootloader_target.cmake)
add_stm32f103_bootloader_target(nucleo_bootloader ${CMAKE_CURRENT_SOURCE_DIR}
    path/to/STM32F103RB_BOOTLOADER_FLASH.ld)
configure_stm32f103rb_bootloader_target(nucleo_bootloader)
```

The standalone Nucleo smoke-test target lives in
`firmware/components/nucleo_f103rb`:

```sh
make nucleo debug
```

It emits:

- `firmware/components/nucleo_f103rb/build/nucleo_f103rb_bootloader.bin`
- `firmware/components/nucleo_f103rb/build/nucleo_f103rb_app.bin`

## Nucleo legacy UART prototype

The separate Nucleo-F103RB prototype predates the signed update protocol. Its
simple `UBSL` sender is retained only for Nucleo bring-up. It uses USART3 with
the STM32F1 partial
remap onto the PC10/PC11 pins:

- PC10: USART3 TX
- PC11: USART3 RX
- baud: `115200`
- protocol magic: `UBSL`
- header: `UBSL` + little-endian `uint32_t image_size` + little-endian
  `uint32_t crc32`
- payload: raw application `.bin` bytes built for `0x08008000`

The bootloader erases only the application region needed by the incoming image,
streams the payload into flash, CRC-checks the full image, and writes the first
8 bytes of the application vector table last. This keeps an interrupted update
from looking like a valid application.

Prototype flow:

```sh
make nucleo debug

STM32_Programmer_CLI -c port=SWD \
  -w firmware/components/nucleo_f103rb/build/nucleo_f103rb_bootloader.bin \
  0x08000000
STM32_Programmer_CLI -c port=SWD -rst
```

Hold the blue button during reset to enter update mode, then send the app image
over a 3.3 V USB-UART adapter wired to PC10/PC11:

```sh
python3 tools/bootloader_send_uart.py \
  /dev/cu.usbserialXXXX \
  firmware/components/nucleo_f103rb/build/nucleo_f103rb_app.bin
```

After a successful update, the bootloader jumps to the new app.

## Naming and compatibility

Shared update modules use `fw_update_*`, C symbols use `FirmwareUpdate*`, and
build options use `FW_UPDATE_*`. Reconfigure existing builds using the new
option names; old `SUNLITE_OTA_*` cache entries are no longer used.
The wire header, legacy signature domain, and `.elf.ota.json` sidecar suffix
remain unchanged so installed bootloaders and existing artifacts stay compatible.
