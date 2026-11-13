# _emulator: how the boards are emulated

Everything that makes firmware_v4 run in [Renode](https://renode.io). Tests never touch this
folder directly: they use the `Board` API in `__init__.py` through the `board` fixture.

Change things here only to **fix or extend the emulator**: a missing peripheral, a new board, a
new fake device, a new `Board` method. Keep those changes in their own PR, separate from tests.

```
_emulator/
  __init__.py            Board + CanFrame: the API tests use
  chip/
    stm32f103rct6.resc   chip setup shared by every board
  peripherals/
    rcc.py               Python stubs for peripherals Renode's F103 model lacks
  devices/               fake external parts (GPS, IMU, displays ...), none yet
  boards/
    str.resc             one file per board: create machine + chip + ELF + devices
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
```

- **`$ORIGIN`** is the folder of the `.resc` being run, so every path is relative to the file and
  works from any directory or machine. Don't use absolute paths.
- **`$elf ?= ...`** sets a default ELF; `?=` means a caller can override it first
  (e.g. `$elf = @/path/other.elf` before `include`).
- Peripherals must be added **before `LoadELF`**, and the emulation doesn't start until `start` (or
  `Board.run_for()`).

## The chip: what we add to Renode's F103 model

Renode's `platforms/cpus/stm32f103.repl` has the CPU, NVIC, GPIO, EXTI, USART1–5, I2C1–2, timers and
DMA, but is missing peripherals our firmware uses. Each fix goes in `chip/stm32f103rct6.resc`
(and `peripherals/` if it needs Python), so every board gets it.

| Peripheral | Address | Fix | Status |
|---|---|---|---|
| RCC | `0x40021000` | `peripherals/rcc.py`: stores writes; ready bits follow enable bits (HSI/HSE/PLL in CR, SWS←SW in CFGR, LSI in CSR) | Done |
| Bit-banding | `0x42000000` | `Miscellaneous.BitBanding` (HAL uses it) | Done |
| FLASH interface (ACR) | `0x40022000` | Plain register storage, so the latency HAL writes reads back | **Next**: STR stops here |
| PWR | `0x40007000` | Plain register storage | To do |
| CAN (bxCAN) | `0x40006400` | Borrow `CAN.STMCAN` from the F4 model (same registers), IRQs 19–22; start it awake (it ignores INRQ while SLEEP is set) | To do |
| ADC1/ADC2 | `0x40012400` / `0x40012800` | Stub: calibration completes, conversions return a set value | To do |
| IWDG | `0x40003000` | Stub; later a real countdown that resets the chip | To do |
| RTC | `0x40002800` | Stub (TEL only) | To do |
| SPI1/SPI2 | | Needed by drd, mdi, mst | To do |
| CRC | | Needed by mst | To do |
| Memory sizes | | Renode maps 256 MB flash/RAM; real chip is 256 KB / 48 KB, so overflows don't fault | To do |

Renode bugs to work around (seen in the TEL prototype, `renode_sims/tel.resc`):
- **DMA2 ch5 is wired to IRQ 60**; on the F103xE ch4 and ch5 share IRQ 59. Rewire ch5 → 59.
- **DMA sets half-transfer and transfer-complete flags at the same time**, so HAL only sees the
  half-transfer and UART DMA TX stops after one frame. Fix: a CPU hook on `HAL_DMA_IRQHandler` that
  clears HTIE when both flags are pending.

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
renode sim/_emulator/boards/str.resc
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
   $elf ?= $ORIGIN/../../../firmware/components/<board>/build/Debug/<board>.elf
   sysbus LoadELF $elf
   ```
3. Add fake devices for the external parts the firmware waits on (`devices/`), or it may stall on
   I2C/SPI timeouts.
4. Create `sim/<board>/` for its tests. The folder name must match the `.resc` name.

## The Board API (`__init__.py`)

Status: **skeleton**. Every method raises `NotImplementedError`, which the `board` fixture turns
into a skipped test.

Rules any implementation must keep:
- **Tests control time.** The emulation only runs inside `run_for()` and `wait_for_*()`; otherwise
  it is paused. A Python breakpoint then freezes the firmware, and a firmware breakpoint blocks the
  test.
- **Timeouts are in virtual time** (`within_ms`), never wall clock.
- **Every captured event is timestamped in virtual time** (`CanFrame.time_ms`).
- **`gdb=True`** starts Renode's GDB server on `GDB_PORT` (3333) and waits for a debugger before any
  firmware runs.
- **One fresh machine per `Board`**, fully released in `close()`.

How to drive Renode, in order of preference:
1. **pyrenode3**: Renode inside the Python process, same objects as the `.resc` (`machine`,
   `sysbus`, CAN controller events). Not yet tested with our portable .NET Renode.
2. **Subprocess**: run `renode --port <n>` and send Monitor commands over that port.

Open question: does Renode's virtual time stop while the CPU is halted at a GDB breakpoint? If not,
`Board` must pause the emulation when the CPU halts.

## References
- Renode docs: <https://renode.readthedocs.io>
- Renode's F103 model: `<renode install>/platforms/cpus/stm32f103.repl`
- STM32F103 reference manual: RM0008
- Working TEL prototype with all the fixes above: `renode_sims/tel.resc` (outside this repo)
