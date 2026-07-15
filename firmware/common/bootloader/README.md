# Common STM32 Bootloader

This directory contains the shared bootloader core for STM32F103 boards.

## Phase 1 Layout: Car Boards

- Bootloader flash: `0x08000000` through `0x08007FFF` (`32 KiB`)
- Application flash: `0x08008000` through `0x0803FFFF` (`224 KiB`)
- SRAM: `0x20000000` through `0x2000BFFF` (`48 KiB`)

Every board application linker script is offset to `0x08008000` so the
bootloader can validate the application vector table and jump to it.

## Nucleo-F103RB Test Layout

- Bootloader flash: `0x08000000` through `0x08007FFF` (`32 KiB`)
- Application flash: `0x08008000` through `0x0801FFFF` (`96 KiB`)
- SRAM: `0x20000000` through `0x20004FFF` (`20 KiB`)

Use `STM32F103RB_BOOTLOADER_FLASH.ld` for the bootloader and build the test
application with flash origin `0x08008000` and flash length `96K`.

## Phase 2 Entry Decision

`BootloaderRun()` enters update mode when:

- the application vector table is invalid,
- `BootloaderBoardStayInBootloader()` returns `true`, or
- `BootloaderWaitForUpdateRequest()` receives an update request during the
  startup window.

The board and transport functions are weak hooks. Keep board-specific GPIO,
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

## Nucleo UART Update Prototype

The Nucleo-F103RB bootloader update mode uses USART3 with the STM32F1 partial
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
