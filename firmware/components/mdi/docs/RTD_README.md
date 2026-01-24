# RTD Temperature Sensor Library

A driver library for reading temperature from PT1000 RTD sensors using the MAX31865 chip over SPI.

## Overview

This library provides a simple interface to read temperature measurements from a PT1000 RTD connected to a MAX31865 converter chip.

Hardware:
- Sensor: PT1000 RTD
- Converter Chip: MAX31865
- Connection: 3-wire RTD
- Communication: SPI1

## Functions

### 1. RtdDriverInit()

void RtdDriverInit(void);

Description: Initializes the MAX31865 chip for continuous temperature readings.

Parameters: None

Returns: Nothing

What it does:
- Configures the chip for auto-conversion mode
- Sets up 3-wire RTD connection

When to call: Once during system startup, after SPI is initialized.

---

### 2. RtdDriverGetTemp()

Rtd_status_t RtdDriverGetTemp(uint32_t* temperature);

Description: Reads the current temperature from the RTD sensor.

Parameters:
- temperature - Pointer where temperature (in °C) will be stored

Returns:
- RtdStatusOk - Temperature read successfully
- RtdStatusFault - Temperature read with fault bit set (indicates sensor issue)

When to call: Whenever you want a temperature reading (e.g., in main loop).

Note: The fault detection is based on the simple fault bit (bit 0) in the RTD data register. If the fault bit is set, the temperature reading should not be trusted.

## Typical Usage

Step 1: Initialize all peripherals (HAL, clocks, GPIO, SPI)

Step 2: Call RtdDriverInit() once

Step 3: In your main loop, call RtdDriverGetTemp() to read temperature

Step 4: Check if return status is RtdStatusOk, then use the temperature value

### Example

```c
#include "rtd.h"

int main(void){
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_SPI1_Init();

    RtdDriverInit();
    
    uint32_t temperature;
    
    while (1) {
        if (RtdDriverGetTemp(&temperature) == RtdStatusOk) {
            printf("Temperature: %lu°C\n", temperature);
        } else {
            printf("Error reading temperature\n");
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
1. Resistance = (ratio / 32768) × 4300Ω
2. Temperature = (Resistance - 1000Ω) / 3.85

Example: If resistance = 1038.5Ω, then temperature = 10°C

### SPI Communication
- Chip select (CS) is controlled automatically
- Write operations: Address byte (bit 7 = 1) followed by data byte
- Read operations: Address byte (bit 7 = 0) followed by dummy byte, data received during 2nd byte

## Additional Notes

- Temperature readings are continuous in the background (using auto-conversion mode)
- Temperature is returned as an integer (no decimal places)
- Fault detection uses the simple fault bit from the RTD data register (bit 0), which indicates basic sensor faults