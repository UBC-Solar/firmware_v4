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

## Complete routed OTA path

The same version-1 `SU` protocol is carried across every hop:

```text
active CMake application target in VS Code
  -> ELF to BIN + Ed25519-signed manifest
  -> SSH/SCP to the Sunlite Raspberry Pi
  -> Pi verifies and atomically stages the bundle
  -> dedicated UART5, PC12/PD2, 230400 baud, 8-N-1
  -> running TEL application
       -> TEL target: reset into tel_bootloader and continue on UART
       -> other target: proxy the request over 500 kbit/s CAN
  -> selected board's signed CAN bootloader
  -> SHA-256 check, metadata commit, vector-last commit, reboot
  -> one trial application launch
  -> targeted BOARD_INFO confirms the compiled firmware version and trial
```

TEL is the only UART gateway. UART5 is dedicated to OTA so USART2 telemetry can
continue during routed MDI, DRD, and STR updates. The accepted remote paths use classic CAN with
29-bit normal-fixed identifiers and a small flow-controlled segmentation layer.
Each destination application answers `HELLO`, checks its update-safety hook,
acknowledges `ENTER_BOOTLOADER`, stores a reset token in backup register DR10,
and resets. Its bootloader then owns CAN until the signed image is complete.

| Board | Target ID | CAN node | Bootloader CAN pins |
| --- | ---: | ---: | --- |
| TEL | `0x54454C45` (`TELE`) | gateway/tester `0xF1` | PA11/PA12 only for proxy traffic |
| MDI | `0x4D444920` (`MDI `) | `0x10` | PB8/PB9 |
| DRD | `0x44524420` (`DRD `) | `0x11` | PB8/PB9 |
| STR | `0x53545220` (`STR `) | `0x12` | PB8/PB9 |
| HVC | `0x48564320` (`HVC `) | `0x13` | outside current acceptance pass |
| MST | `0x4D535420` (`MST `) | `0x14` | outside current acceptance pass |

Every request is COBS framed with CRC32 and carries a random session ID plus a
sequence number. Exact duplicate requests return the cached response, making
Pi, UART, and CAN retries idempotent. The selected target is bound to the TEL
session, checked again by the Pi, and independently checked in the destination
bootloader's signed manifest before flash erase.

The bootloader verifies Ed25519 before erasing, streams and verifies SHA-256,
rehashes the programmed flash before committing it,
enforces the target, hardware range, minimum bootloader version, slot size, and
monotonic firmware version, and stores the installed version in alternating
generation-counted, CRC-protected metadata pages.

After committing authenticated metadata, the bootloader arms a redundant trial
marker in backup registers DR8/DR9 before it writes the application's vector
prefix. The central application jump changes that marker from `armed` to
`launched`; a second launch is refused until the running application completes
a `BOARD_INFO` response to a four-byte `HELLO` explicitly naming that board.
An empty discovery `HELLO` never confirms a trial. TEL clears the marker only
after its blocking UART transmit succeeds. The accepted MDI, DRD, and STR paths clear it only
after the segmented CAN response finishes without a transport error and all CAN
mailboxes drain. A reset during the trial, or a torn/corrupt marker pair, leaves
the board in bootloader recovery and disables the 30-second idle jump.

This confirmation proves that the application reached and serviced its OTA
path; it is not a complete board self-test. The response is sent before the
marker is cleared so a failed transmit remains fail-closed. A reset or backup
write failure in that short interval can therefore let the Pi receive
`BOARD_INFO` while the next reset still enters recovery. Include that boundary
in powered fault-injection testing. Recovery does not roll back to the prior
image, and anti-rollback means repairing a valid-vector failed trial requires a
newer signed firmware version (or wired service access).

DR8/DR9 survive ordinary resets and a VDD power interruption only while the
STM32 backup domain remains powered through VBAT. The car-board hardware must
provide that retention for the trial rule to cover a full power cut; otherwise
the marker resets with the backup domain. Deliberately test this on each board.
True recovery across loss of both VDD and VBAT would require a flash-backed
trial record or external nonvolatile state.

DR8 and DR9 are reserved for the trial marker and DR10 for bootloader entry;
application RTC code must not use those registers or reset the backup domain.
The current TEL RTC implementation reads/writes the RTC calendar only and does
not write backup data registers. Its generated clock setup also avoids a
backup-domain reset while the retained RTC source remains LSI. Changing that
source would clear these markers and must be treated as an OTA migration.

## Configure and provision

Generate the private key on the laptop and keep it there. Configure every board
with the corresponding public PEM and a monotonically increasing numeric
version. Replace `tel` with the selected board name:

```sh
cmake --preset Debug \
  -S firmware/components/tel \
  -B firmware/components/tel/build \
  -DSUNLITE_OTA_PUBLIC_KEY_FILE="$HOME/.config/sunlite/firmware-ed25519.pub" \
  -DSUNLITE_OTA_FIRMWARE_VERSION=2 \
  -DSUNLITE_OTA_DISPLAY_VERSION=2.0.0 \
  -DTEL_OTA_ENABLE_TEST_BYPASS=ON
cmake --build firmware/components/tel/build
```

Each application build generates `<board>.elf.ota.json` beside its ELF. The VS
Code extension uses that exact sidecar to bind the active CMake target, target
ID, hardware revision, and compiled firmware version into the signed manifest.

Install each bootloader once at `0x08000000` with SWD/J-Link or ST-Link, then
install an application linked at `0x08008000`. Install the same public key on
the Pi as `/etc/sunlite/firmware-ed25519.pub`. If the CMake public-key option is
omitted, the application and bootloader omit `SIGNATURE_VERIFICATION`, and the
Pi refuses to flash. This is an intentional fail-closed build.

The STM32F103RC integration is single-slot: it verifies the signed
manifest before erasing, writes the vector table last, and remains recoverable
in the bootloader after interrupted programming. It does not advertise resume
or rollback. Do not enable unattended vehicle updates until this recovery path
has passed power-interruption testing; automatic rollback requires external
staging flash or a different memory layout.

TEL defaults `TEL_OTA_ENABLE_TEST_BYPASS` to `OFF`, including in
Debug builds. Its `SunliteOtaBoardUpdateAllowed()` implementation now requires
the following DBC-backed CAN state to remain valid for 500 ms: DRD state is
PARK with the brake pressed, the MDU reports stopped with zero speed and zero
accelerator/output target, DRD commands zero throttle and regeneration, and
the relevant DRD timeout/fault flags are clear. All four source frames must be
no more than 1000 ms old. TEL applies this interlock to its own update and
before forwarding `ENTER_BOOTLOADER` to a remote board.

For a controlled bench only, explicitly configure a Debug TEL build with
`-DTEL_OTA_ENABLE_TEST_BYPASS=ON`. The
`TelOtaSafetyTestBypassEnabled()` function contains this test-only decision;
CMake forces it off in non-Debug builds. Remote-board Debug configurations
still use their own bench fallback until their weak
`SunliteOtaCanBoardUpdateAllowed()` hooks are replaced with local interlocks.
Configure flash option-byte write protection for the bootloader after
provisioning.

MST is outside this acceptance pass. Its existing recovery application and
bootloader integration were not validated here and must not be selected by the
VS Code OTA extension.

Both UART and CAN bootloaders return to a valid application after 30 seconds of
inactivity, including the first launch of an armed trial image. If an
interrupted update has invalidated the vector table, or an unconfirmed trial
has already launched, they remain in recovery mode instead.

Run both host protocol suites with:

```sh
make ota-contract-test
```

These tests and cross-builds establish the software contracts; they do not
replace a powered Pi-to-TEL UART-to-CAN bench test, CAN termination/bitrate
validation, or deliberate power-cut recovery testing.

The repository does not contain the TEL PCB or schematic. Confirm that the TEL
debug connector really exposes 3.3 V UART5 PC12 (TX), PD2 (RX), and ground
before connecting a second USB-UART adapter. A TEL self-update necessarily
pauses TEL application telemetry while TEL is running its bootloader; cellular
networking and the Pi logger process remain up, and routed-board updates do not
take ownership of the telemetry UART.

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

The separate Nucleo-F103RB prototype predates the signed Sunlite protocol. Its
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
