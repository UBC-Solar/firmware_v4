# RTD Temperature Sensor Library

A driver library for reading temperature from PT1000 RTD sensors using the MAX31865 chip over SPI.

## Overview

This library provides a simple interface to read temperature measurements from a PT1000 RTD connected to a MAX31865 converter chip.

Hardware:
- Sensor: PT1000 RTD
- Converter Chip: MAX31865
- Connection: 3-wire RTD
- Communication: SPI1

Diagnostics (`diagnostic.c`) call this driver once per second. Temperature is only trusted on `RtdStatusOk`. Failed reads still produce a motor-temp CAN frame (`0x502`) with `success=0` and temperature `0`; they never reuse a last-good temperature.

## Functions

### 1. RtdDriverInit()

void RtdDriverInit(void);

Description: Initializes the MAX31865 chip for continuous temperature readings.

Parameters: None

Returns: Nothing

What it does:
- Configures the chip for auto-conversion mode
- Sets up 3-wire RTD connection
- Clears debounce state and any power-up latched fault

When to call: Once during system startup, after SPI is initialized.

---

### 2. RtdDriverGetTemp()

RtdStatus RtdDriverGetTemp(int32_t* temperature);

Description: Reads the current temperature from the RTD sensor.

Parameters:
- temperature - Pointer where temperature (in °C) will be stored

Returns:
- RtdStatusOk - Temperature read successfully; value is trustworthy
- RtdStatusFault - Sensor/wiring fault bit set, or a NULL temperature pointer
- RtdStatusHalError - SPI communication error (HAL failure)

When to call: Whenever you want a temperature reading (e.g., in main loop).

If the MAX31865 data-register fault bit (D0) is set, this function reads the Fault Status register, updates debounce state, clears the latched chip fault, and returns `RtdStatusFault` without converting a temperature.

---

### 3. RtdDriverReadFaults()

RtdStatus RtdDriverReadFaults(RtdFaultFlags* faults);

Description: SPI-reads MAX31865 Fault Status register 0x07 and masks unused bits D1/D0.

Returns:
- RtdStatusOk - `*faults` holds D7 through D2
- RtdStatusFault - NULL pointer
- RtdStatusHalError - SPI failure

Named masks: `RTD_FAULT_RTD_HIGH`, `RTD_FAULT_RTD_LOW`, `RTD_FAULT_REFIN_HIGH`, `RTD_FAULT_REFIN_LOW`, `RTD_FAULT_RTDIN_LOW`, `RTD_FAULT_OVUV`.

---

### 4. RtdDriverGetFaults()

RtdFaultFlags RtdDriverGetFaults(void);

Description: Returns the **debounced** fault flags. Hardware faults must appear on 5 consecutive `RtdDriverGetTemp()` samples (~5 s at the 1 Hz diagnostic cadence) before this is non-zero. A successful temperature read clears the count and the flags.

Use this for latched diagnostic bits (`mdi_rtd_fault` on CAN `0x501`). Transient 1–4 sample glitches show up as `0x502 success=0` without latching that flag.

## Typical Usage

Step 1: Initialize all peripherals (HAL, clocks, GPIO, SPI)

Step 2: Call RtdDriverInit() once

Step 3: In your main loop, call RtdDriverGetTemp() to read temperature

Step 4: Check if return status is RtdStatusOk, then use the temperature value. On failure, use `RtdDriverGetFaults()` rather than the temperature pointer.

### Example

```c
#include "rtd_driver.h"

int main(void){
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_SPI1_Init();

    RtdDriverInit();
    
    int32_t temperature;
    
    while (1) {
        if (RtdDriverGetTemp(&temperature) == RtdStatusOk) {
            printf("Temperature: %ld°C\n", (long)temperature);
        } else {
            printf("Error reading temperature, faults=0x%02X\n",
                   (unsigned)RtdDriverGetFaults());
        }
        
        HAL_Delay(1000);
    }
}
```

## Technical Details

### Temperature Range
- Designed for PT1000 sensors (1000Ω at 0°C)
- Temperature coefficient: 0.00385 Ω/Ω/°C
- Typical operating range: -80°C to +250°C (for our specific sensor)

### Conversion Formula
The library converts the 15-bit resistance ratio from the MAX31865 into temperature:
1. Resistance = (ratio / 32768) × 3987Ω
2. Temperature = (Resistance - 1000Ω) / (0.00385 × 1000)

Example: If resistance = 1038.5Ω, then temperature = 10°C

### SPI Communication
- Chip select (CS) is controlled automatically
- Write operations: Address byte (bit 7 = 1) followed by data byte
- Read operations: Address byte (bit 7 = 0) followed by dummy byte, data received during 2nd byte

## Additional Notes

- Temperature readings are continuous in the background (using auto-conversion mode)
- Temperature is returned as an integer (no decimal places) and may be negative
- Do not use a previous good temperature across a faulted sample
- Motor-temp CAN `0x502` (DLC 6): byte 0 success, bytes 1–4 little-endian int32 °C, byte 5 debounced fault flags
- Diagnostic flags CAN `0x501` bit 3 is RTD fault, bit 4 is RTD SPI/comm error
