# emulation: how the boards are emulated

Everything that makes firmware_v4 run in [Renode](https://renode.io). Tests never import this
folder (`tests/conftest.py` refuses it): they use the `Board` API in `../api/` through the `board`
fixture, and only `api` talks to `renode.py`.

Change things here only to **fix or extend the emulator**: a missing peripheral, a new board, a
new fake device. Keep those changes in their own PR, separate from tests.

```
emulation/
  renode.py              loads Renode into Python once (pyrenode3); Monitor commands. Used by api/ only
  chip/
    stm32f103rct6.resc   chip setup shared by every board
    dma_fix.resc         DMA workaround, included after LoadELF by boards that use DMA
  peripherals/           Python stubs for peripherals Renode's F103 model lacks
    rcc.py               clocks: ready bits follow enable bits
    stub_regs.py         plain register storage (PWR, FLASH interface)
    adc.py               ADC: instant calibration, per-channel values (default 0x800)
    rtc.py               RTC: sync flags always set, counter doesn't tick
    i2c_write_ok.py      I2C master where every write succeeds (STR/HVC/MDI write-only devices)
    spi_no_device.py     SPI master with nothing on the bus (SPI1-3 on every board)
  devices/               fake external parts
    adbms1818.py         MST's 2x ADBMS1818 chain on SPI2 (cells 3.6 V, thermistors 25 degC)
  boards/
    <board>.resc         one file per board: create machine + chip + ELF + devices
                         (str tel hvc drd mdi mst dist)
```

## How a board is built

All firmware_v4 boards use the same MCU (STM32F103RCT6), so the chip is set up once and every board
reuses it. Loading `boards/str.resc` does, in order:

```
boards/str.resc
  mach create "str"                                  1. empty machine
  include $ORIGIN/../chip/stm32f103rct6.resc         2. the chip
      LoadPlatformDescription stm32f103.repl              Renode's built-in F103 model
      PyDevFromFile peripherals/rcc.py ...                + our fixes for what it lacks
      BitBanding ...
  (board devices, later)                             3. what this board talks to
  sysbus LoadELF $elf                                4. the firmware
  (include $ORIGIN/../chip/dma_fix.resc)             5. only boards that use DMA, e.g. TEL
```

- **`$ORIGIN`** is the folder of the `.resc` being run, so every path is relative to the file and
  works from any directory or machine. Don't use absolute paths.
- **`$elf ?= ...`** sets a default ELF; `?=` means a caller can override it first
  (e.g. `$elf = @/path/other.elf` before `include`).
- **Which ELF:** a board can be built into `build/Debug/`, `build/Release/` (CMake presets, VS Code)
  or `build/` itself (`make <board>` and `make <board> release` both write there, so the folder name
  doesn't say which). `find_elf()` in `../api/board.py` reads `CMAKE_BUILD_TYPE` from each folder's
  `CMakeCache.txt` and picks the newest ELF of the requested type (pytest `--build-type`, default
  Debug). `Board` sets `$elf` before including the board file; the `$elf ?=` default in each `.resc`
  (the Debug preset build) is only for running by hand.
- Peripherals must be added **before `LoadELF`**, and the emulation doesn't start until `start` (or
  `Board.wait()`). How `Board` drives Renode is in `../api/README.md`.

## The chip: what we add to Renode's F103 model

Renode's `platforms/cpus/stm32f103.repl` has the CPU, NVIC, GPIO, EXTI, USART1–5, I2C1–2, timers and
DMA, but is missing peripherals our firmware uses. Each fix goes in `chip/stm32f103rct6.resc`
(and `peripherals/` if it needs Python), so every board gets it.

| Peripheral | Address | Fix | Status |
|---|---|---|---|
| RCC | `0x40021000` | `peripherals/rcc.py`: stores writes; ready bits follow enable bits (HSI/HSE/PLL in CR, SWS←SW in CFGR, LSI in CSR) | Done |
| Bit-banding | `0x42000000` | `Miscellaneous.BitBanding` (HAL uses it) | Done |
| FLASH interface (ACR) | `0x40022000` | `stub_regs.py`: plain storage, so the latency HAL writes reads back | Done |
| PWR | `0x40007000` | `stub_regs.py` | Done |
| CAN (bxCAN) | `0x40006400` | Borrow `CAN.STMCAN` from the F4 model (same registers), IRQs 19–22; start it awake (it ignores INRQ while SLEEP is set) | Done |
| ADC1/ADC2 | `0x40012400` / `0x40012800` | `adc.py`: calibration completes instantly, SR reports end-of-conversion, DR reads the value set for the SQR3 rank-1 channel (default 0x800) | Done (single channel; no scan/DMA) |
| RTC | `0x40002800` | `rtc.py`: sync flags always set; counter doesn't tick (TEL frame timestamps are constant) | Done (no ticking) |
| IWDG | `0x40003000` | Stub; later a real countdown that resets the chip | To do |
| SPI1/SPI2/SPI3 | `0x40013000` / `0x40003800` / `0x40003C00` | `spi_no_device.py`: every byte sent instantly (TXE always set, RXNE until DR is read), MISO reads 0xFF. Boards with a real SPI device replace it (MST: `devices/adbms1818.py`) | Done |
| CRC | | MST inits it but never uses it: unmapped, writes ignored | Not needed |
| Memory sizes | | Renode maps 256 MB flash/RAM; real chip is 256 KB / 48 KB, so overflows don't fault | To do |

Renode bugs worked around (found while building the TEL sim):
- **DMA2 ch5 is wired to IRQ 60**; on the F103xE ch4 and ch5 share IRQ 59. Rewired ch5 → 59 in
  `chip/stm32f103rct6.resc`. Without it TEL's radio (UART4) TX-complete never fires.
- **DMA sets half-transfer and transfer-complete flags at the same time**, so HAL only sees the
  half-transfer and UART DMA TX stops after one frame. Fix in `chip/dma_fix.resc`: a CPU hook on
  `HAL_DMA_IRQHandler` that clears HTIE when both flags are pending. It hooks by symbol name, so it
  goes after `LoadELF`, and only boards whose firmware links DMA include it (STR doesn't).

## Board status

| Board | State in the sim |
|---|---|
| TEL | Boots into FreeRTOS, sends telemetry frames (~every 100 ms on the telemetry UART) and CAN 0x750/0x751/0x763/0x764. No GPS/IMU devices, so `gps_read_fail` gets set after ~0.5 s. |
| STR | Boots into FreeRTOS; every task loops; sends CAN 0x580 (every 100 ms), 0x581, 0x583, 0x591, 0x592. I2C1 is replaced by `i2c_write_ok.py` (see below). |
| HVC | Bare-metal FSM runs: CAN every 200 ms (0x320–0x329), heartbeat 0x322 every 1 s; reacts to CAN (TEL heartbeat → MST handshake; none → FAULT after 3 s). I2C1 is `i2c_write_ok.py` (no INA237 model: shunt current 0x320 is garbage). ADC scan+DMA never completes, so every analog value (supp voltage, LV current, DCDC temp, precharge) is computed from zeros. |
| DRD | Boots into FreeRTOS; sends 0x401 (every 25 ms), 0x403 (every 100 ms), 0x402/0x404/0x405 and the motor request 0x08F89540. LCD on SPI1 writes into `spi_no_device.py`. `defaultTask` isn't created (heap exhausted, same as TEL). |
| MDI | Bare-metal loop runs: 0x500/0x501/0x502/0x504 every 1 s. Both DAC7571s (I2C1, I2C2) are `i2c_write_ok.py`; the RTD (SPI1) reads 0xFF, so motor temperature is garbage. |
| MST | Holds FAULT for 2 s, then the main loop runs (~180 ms): 0x200-0x203, 0x210/0x220/0x230 module data. SPI2 is `devices/adbms1818.py`, so every module reads 3600 mV and 25 degC and no isoSPI errors occur. Its UART log (USART1 DMA) isn't captured yet. |
| DIST | Bare-metal FSM runs in STARTUP (waits for CAN 0x303): heartbeat 0x3A0 every 500 ms, 0x3A2, 0x3A4/0x3A5. LED driver (I2C1) and ADC use the chip defaults. |

**Renode's STM32F1 I2C doesn't complete a polled HAL write.** With no device at the address, it's
never acked; with `Mocks.DummyI2CSlave` there, BTF never gets set. Either way
`HAL_I2C_Master_Transmit(..., HAL_MAX_DELAY)` waits forever. STR's hex display task (Normal priority)
did exactly that and starved every Low task, so STR sent no CAN at all. `boards/str.resc` swaps I2C1
for `peripherals/i2c_write_ok.py`, which reports every step of a write as done. It's only right
for boards that *write* over I2C; boards that read (TEL's GPS/IMU) need a real device model.

Known TEL firmware bugs the sim exposes (tests mark them `xfail`): `defaultTask` isn't created
(`configTOTAL_HEAP_SIZE` 3072 exhausted), and the IMU driver's I2C calls use `HAL_MAX_DELAY`, so the
IMU task hangs with no IMU attached.

## Writing a peripheral stub (`peripherals/*.py`)

A stub is a Python script Renode runs on **every access** to its address range:

```
machine PyDevFromFile $ORIGIN/../peripherals/<name>.py <base address> <size> True "<name>"
```

```python
if request.IsInit:              # once, at startup: set up state and reset values
    regs = {0x00: 0x83}
elif request.IsWrite:           # CPU wrote request.Value at request.Offset
    regs[request.Offset] = request.Value
elif request.IsRead:            # set request.Value to what the CPU should read
    request.Value = regs.get(request.Offset, 0)
```

- `request.Offset` is bytes from the base address; variables created at init survive between calls.
- Plain storage is enough when HAL only writes and reads back. When HAL **waits for hardware** to set
  a bit (a ready flag, a calibration-done bit), the stub has to set it, like `rcc.py` does.
- Register offsets and bits come from the reference manual, **RM0008**.

## Finding what's missing

Run the board by hand and see where it gets stuck:

```bash
renode sim/emulation/boards/str.resc
```
```
start
pause
sysbus FindSymbolAt `cpu PC`     # where the CPU is, e.g. Error_Handler
sysbus FindSymbolAt `cpu LR`     # roughly who called it
```

Then read the log for `unimplemented register` warnings near the end: they name the peripheral and
register the firmware was using. Add the smallest stub that gets past it and run again.

## Adding a board

1. Make sure the chip file covers the peripherals the board's `.ioc` enables.
2. Create `boards/<board>.resc`:
   ```
   mach create "<board>"
   include $ORIGIN/../chip/stm32f103rct6.resc
   # board devices here
   $elf ?= $ORIGIN/../../../firmware/components/<board>/build/Debug/<board>.elf   # by-hand default
   sysbus LoadELF $elf
   # include $ORIGIN/../chip/dma_fix.resc   <- only if the firmware uses DMA
   ```
3. Add fake devices for the external parts the firmware waits on (`devices/`), or it may stall on
   I2C/SPI timeouts.
4. Create `sim/tests/<board>/` for its tests. The folder name must match the `.resc` name.

## References
- Renode docs: <https://renode.readthedocs.io>
- Renode's F103 model: `<renode install>/platforms/cpus/stm32f103.repl`
- STM32F103 reference manual: RM0008
- Original TEL prototype (Robot Framework): `renode_sims/` (outside this repo); its tests are ported
  to `sim/tests/tel/`

### ADC backdoor

`peripherals/adc.py` keeps a value per channel at offset `0x100 + 4 * channel` (channels 0–17), past
the last real register (0x4C), so the firmware never touches it. A DR read returns the value of the
channel in SQR3 bits 4:0 (rank 1, what `HAL_ADC_ConfigChannel` sets for a single conversion), or
0x800 if none was set. Scan mode (several ranks + DMA, HVC) needs the stub to step through SQR1–3
and a DMA path: not done yet.
