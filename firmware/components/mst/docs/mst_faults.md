# MST (BMS) fault list

Every condition in the MST firmware that raises a fault, warning or error, and what the board does
about each one. Based on `firmware/components/mst` on branch `user/gurmankhella/hvc/firmware-refactor`
(commit `73873f5`).

## How the MST reports a problem

| Output | Pin / ID | Meaning when active |
|---|---|---|
| `FAULT_OUT` | GPIO → HVC `MASTERBOARD_FAULT` | High = fault. Also held high for 2 s at startup as the HVC handshake (`INIT_FAULT_HOLD_DURATION_MS`). |
| `CONTACTOR_DIS_OUT` | GPIO | High = contactors disabled |
| `HLIM_DIS_OUT` | GPIO | High = charging (HLIM) disabled |
| `LLIM_DIS_OUT` | GPIO | High = discharging (LLIM) disabled |
| `LED_OUT` | GPIO | Solid on while any `*_DIS_OUT` is high. Blinks in the two error handlers (see below). |
| Status / heartbeat | CAN `0x200`, every 1 s | Bytes 4–5 = status bits (table below). Bytes 6–7 = total isoSPI comm failures (saturates at 0xFFFF). |
| Module status | CAN `0x230`–`0x233` | One byte per module, 8 modules per frame: bits 7–4 = module faults, bits 2–0 = module warnings |

`0x200` status bits (`CAN_SendHeartbeatMessage()`, `can_messages.c`):

| Bit | Byte.bit | Field | Kind |
|---|---|---|---|
| 0 | 4.0 | `error_comm_fail` | Error |
| 1 | 4.1 | `error_self_test` | Error (never set, see below) |
| 2 | 4.2 | `fault_over_voltage` | Fault |
| 3 | 4.3 | `fault_under_voltage` | Fault |
| 4 | 4.4 | `fault_over_temperature` | Fault |
| 5 | 4.5 | `fault_under_temperature` | Fault |
| 6 | 4.6 | `warn_high_voltage` | Warning |
| 7 | 4.7 | `warn_low_voltage` | Warning |
| 8 | 5.0 | `warn_high_temperature` | Warning |
| 9–13 | 5.1–5.5 | `llim_enable`, `hlim_enable`, `balancing_enable`, `balancing_active`, `scrutineering_enable` | Status, not faults |

## 1. Cell faults (latched)

Checked per module, every main loop, by `CheckForEmergency()` (`analysis.c`). The check starts on main loop 5:
loops 1–4 are skipped (`NUM_INIT_MAINLOOPS`) because not every module has been measured yet. Limits are in
`mst_defs.h` (Molicel M35A).

| Fault | Condition (per module) | Limit |
|---|---|---|
| `fault_over_voltage` | `voltage_mv >= MAX_VOLTAGE_mV` | 4200 mV |
| `fault_under_voltage` | `voltage_mv <= MIN_VOLTAGE_mV` | 2750 mV |
| `fault_over_temperature` | `temperature_mC >= MAX_TEMP_degC × 1000` | 60 °C |
| `fault_under_temperature` | `temperature_mC <= MIN_TEMP_degC × 1000` | 0 °C |

When any module has a fault:
- **`FAULT_OUT` goes high** (`AnalyzeModuleData()`), and no code ever drives it low again.
- **`CONTACTOR_DIS_OUT` goes high** (`DriveOutputs()`: `contactor_enable = !pack_faults.raw`).
- **Balancing stops** for every module (`is_safe_to_balance` in `balancing.c`).
- The `0x200` and `0x230+` bits are set.

**They latch.** `CheckForEmergency()` ORs each module's faults into `pack_faults` and only clears
`pack_warnings`. So the pack-level fault bits, `FAULT_OUT` and `CONTACTOR_DIS_OUT` stay set until the MST
resets. Each module's own `faults` byte is recomputed every loop, so a module's `0x230+` status can clear while
the pack bit in `0x200` stays set.

Thermistor failures appear as temperature faults. `ThermistorVoltToTemp_()` clamps readings outside its lookup
table to the end values, so an open thermistor reads −30 °C (under-temperature fault) and a shorted one reads
200 °C (over-temperature fault). There is no separate sensor fault.

## 2. Cell warnings (not latched)

Set by the same `CheckForEmergency()` pass. `pack_warnings` is cleared at the start of every pass, so a warning
clears as soon as the condition goes away.

| Warning | Condition (per module) | Limit | What the MST does |
|---|---|---|---|
| `warn_high_voltage` | `voltage_mv >= WARN_HIGH_VOLTAGE_mV` | 4100 mV | `HLIM_DIS_OUT` high (stops charging) |
| `warn_low_voltage` | `voltage_mv <= WARN_LOW_VOLTAGE_mV` | 2850 mV | `LLIM_DIS_OUT` high (stops discharging); stops balancing |
| `warn_high_temperature` | `temperature_mC >= WARN_HIGH_TEMP_degC × 1000` | 55 °C | Stops balancing only |

`LED_OUT` is on while HLIM, LLIM or the contactors are disabled.

## 3. isoSPI communication errors

### `error_comm_fail`: single read failure
Set by `IncrementIsoSpiError_()` (`mst_main.c`) when a voltage or temperature read from the ADBMS1818 chain
returns anything but `Slave_OK`:

| `Slave_Error_t` | Cause |
|---|---|
| `Slave_ERROR_PEC` | Received PEC (CRC-15) does not match the data |
| `Slave_ERROR_TIMEOUT` | ADC conversion not done within `SLAVE_TIMEOUT_MS` (100 ms) |
| `Slave_ERROR_HAL` / `_HAL_BUSY` / `_HAL_TIMEOUT` | HAL SPI call failed |

A single failure sets bit 0 of `0x200` and increments the failure counter (bytes 6–7). The next successful read
clears the bit. A single failure does not change any output.

### Too many comm failures → `IsoSpiErrorHandler_()` (terminal)
Triggered by 3 failures (`NUM_CONSECUTIVE_COMM_ERR`) within a 2000 ms window (`CONSECUTIVE_TIMEFRAME_MS`).
The window starts at the first failure and resets at the first failure more than 2 s later.

What happens:
- `FAULT_OUT`, `CONTACTOR_DIS_OUT`, `HLIM_DIS_OUT` and `LLIM_DIS_OUT` go high.
- The MST loops forever. Every 1 s it sends the heartbeat (`0x200`) and commit hash (`0x204`) and toggles
  `LED_OUT`.
- Interrupts stay on, so CAN keeps working. Only a reset gets out of this loop.

Disabled entirely when `ISOSPI_CONNECTED` is `false`.

### `error_self_test`: declared but never set
The field exists, and it is sent as bit 1 of `0x200`, but nothing writes it. `Slave_ERROR_SELFTEST` is also never
returned. The bit is always 0.

## 4. `Error_Handler()` (terminal, all interrupts off)

`Error_Handler()` (`cube/Core/Src/main.c`) disables interrupts, sets `FAULT_OUT` and all three `*_DIS_OUT`
high, then toggles `LED_OUT` about every 125 ms forever. CAN stops, so the HVC also sees the MST heartbeat
time out. Only a reset gets out.

| Where | Trigger |
|---|---|
| `main.c` `SystemClock_Config()` | `HAL_RCC_OscConfig()` or `HAL_RCC_ClockConfig()` fails |
| `can.c`, `crc.c`, `spi.c`, `usart.c` | `HAL_CAN_Init`, `HAL_CRC_Init`, `HAL_SPI_Init`, `HAL_UART_Init` or USART1 TX `HAL_DMA_Init` fails |
| `can_driver.c` `CAN_Init()` | `HAL_CAN_ActivateNotification()` fails |
| `can_driver.c` `CAN_InitFilterList()` | Bad arguments, or `HAL_CAN_ConfigFilter()` fails |
| `can_driver.c` `CAN_QueueTxMessage()` | **Software TX queue full (32 frames).** Auto-retransmission is on, so if no other node ACKs (CAN unplugged, bus-off) the 3 mailboxes never empty and the queue fills. In normal running (~6 frames per loop) that takes about a second. Inside `IsoSpiErrorHandler_()` (2 frames/s) it takes about 17 s. |
| `can_driver.c` `tryTransmitFromQueue()` | `HAL_CAN_AddTxMessage()` fails |
| `can_driver.c` RX callback | `HAL_CAN_GetRxMessage()` fails |
| `module_data.c` | Voltage or temperature register index out of range (programming assert) |
| `balancing.c` | A module's voltage is below the pack minimum (programming assert) |

## 5. Gaps worth knowing

- **CPU faults do not trip anything.** `HardFault_Handler`, `MemManage_Handler`, `BusFault_Handler` and
  `UsageFault_Handler` are CubeMX defaults that just spin, so all outputs freeze where they were. `FAULT_OUT`
  stays low and the contactors stay enabled. The MST has no watchdog (no IWDG). The only thing that catches
  it is the HVC's 3 s MST heartbeat timeout.
- **The ADBMS1818's own UV/OV flags are configured but never read.** `VUV`/`VOV` (2.7 V / 4.2 V) are written to
  the config register, but only the firmware limits in section 1 are checked.
- **No cell checks for the first 4 main loops** after the 2 s startup hold. During that time `FAULT_OUT` is low
  and only the heartbeat and commit hash are sent.
- **Bench switches that hide faults** (`mst_defs.h`), all currently at their safe values:
  `IGNORE_TEMPEREATURE_VALUES` (false), `ISOSPI_CONNECTED` (true), `GENERATE_FAKE_BATTERY_DATA` (false),
  `CAN_CONNECTED` (true), `RUN_MAIN_LOOP` (true), and every `UNIT_TEST_*` / `INT_TEST_*` set to `SKIP`.

## 6. How the HVC reads these

- **`MASTERBOARD_FAULT` pin** (MST `FAULT_OUT`): after the startup handshake, a rising edge sets the HVC's
  `mst_fault`.
- **`0x200` status:** the HVC sets `mst_healthy = (data[4] == 0) && ((data[5] & 1) == 0)`
  (`hvc/app/can_messages.c`). That covers status bits 0–8: both errors, all four faults **and all three
  warnings**. After `STATE_MST_STATUS_CHECK`, any of them sets the HVC's latched `mst_unhealthy` fault and
  opens every contactor.

  This is stricter than the MST itself. Two examples:
  - At 4100 mV (`warn_high_voltage`, a normal near-full-charge level) the MST only wants HLIM opened, but the
    HVC opens every contactor.
  - A single isoSPI read error (bit 0), which the MST tolerates up to twice in 2 s, faults the HVC if it
    happens to be set when the 1 s heartbeat goes out.

  Worth confirming with the team that this is intended.
