#include "ina228_runtime.h"

#include "debug_io.h"
#include "i2c_driver.h"
#include "stm32f1xx_hal_def.h"
#include "stm32f1xx_hal_i2c.h"
#include <stdint.h>

// Raw bytes received from INA228 in big-endian format
// buf[0] = bits 23:16 (MSB), buf[1] = bits 15:8, buf[2] = bits 7:0 (LSB).
static uint8_t raw_shunt_voltage_buf[3];

// Volatile because written in ISR context (the I2C RxCplt callback) and read in main-loop context.
static volatile int32_t shunt_voltage_nV = 0;
static volatile int32_t shunt_current_mA = 0;

/**
 * @brief Reverse the byte order of a buffer in-place.
 * @param buf Pointer to the buffer to reverse.
 * @param size Number of bytes in the buffer.
 */
static inline void swap_endianness(uint8_t *buf, uint8_t size) {
    for (uint8_t i = 0; i < size / 2; i++) {
        uint8_t temp = buf[i];
        buf[i] = buf[size - 1 - i];
        buf[size - 1 - i] = temp;
    }
}

/**
 * @brief Pack an integer value into a byte buffer in little-endian order.
 * @param buf Pointer to the destination buffer (must be at least @p size bytes).
 * @param value Value to pack.
 * @param size Number of bytes to write.
 */
static inline void pack_buf(uint8_t *buf, uint32_t value, uint8_t size) {
    for (uint8_t i = 0; i < size; i++) {
        buf[i] = (value >> (8 * i)) & 0xFF;
    }
}

/**
 * @brief Initialise the INA228 — call once at startup after I2C is ready.
 *        Writes CONFIG, ADC_CONFIG, DIAG_ALRT, SOVL, and SUVL using blocking I2C.
 */
void INA228_Init(void) {
    INA228_Write_Config();
    INA228_Write_ADC_Config();
    INA228_Write_Diagnostic_Flags();
    INA228_Write_Over_Voltage();
    INA228_Write_Under_Voltage();

    HAL_I2C_StateTypeDef state = I2C_GetState();
    if (state != HAL_I2C_STATE_READY) {
        DEBUG_IO_print("Failed to initiate INA228 I2C with state 0x%02X\n\r", state);
    }
}

/**
 * @brief Write the CONFIG register (0x00) to set the ADC range.
 */
void INA228_Write_Config(void) {
    // CONFIG register (0x00), 16-bit, reset = 0x0000
    // Bit 4: ADCRANGE — set according to INA228_ADCRANGE define

    uint16_t config = (uint16_t)INA228_ADCRANGE << 4;

    uint8_t buf[2];
    pack_buf(buf, config, 2); // Convert config to little-endian byte array
    swap_endianness(buf, 2); // Swap byte order to big-endian for I2C transmission

    I2C_MemWrite(INA228_I2C_ADDRESS, INA228_REG_CONFIG, buf, 2);

    DEBUG_IO_print("Wrote CONFIG: 0x%04X\n\r", config);
}

/**
 * @brief Write the ADC_CONFIG register (0x01) to set conversion mode, times, and averaging.
 */
void INA228_Write_ADC_Config(void) {
    // ADC_CONFIG register (0x01), 16-bit, reset = 0xFB68
    //
    // Bits 15:12  MODE   = 0xF  — continuous bus + shunt + temperature
    // Bits 11:9   VBUSCT = 5    — 1052 µs bus voltage conversion time
    // Bits  8:6   VSHCT  = 5    — 1052 µs shunt voltage conversion time
    // Bits  5:3   VTCT   = 5    — 1052 µs temperature conversion time
    // Bits  2:0   AVG    = 1    — 4 samples averaged

    // Setup for Continuous shunt voltage measurement, 1052 µs conversion time for bus and shunt and temp, 4 samples averaged
    uint16_t adc_config = (0xA << 12) | (5 << 9) | (5 << 6) | (5 << 3) | (1 << 0);

    uint8_t buf[2];
    pack_buf(buf, adc_config, 2);
    swap_endianness(buf, 2);

    I2C_MemWrite(INA228_I2C_ADDRESS, INA228_REG_ADC_CONFIG, buf, 2);

    DEBUG_IO_print("Wrote ADC_CONFIG: 0x%04X\n\r", adc_config);
}

/**
 * @brief Write the DIAG_ALRT register (0x0B) to configure alert latch and slow-alert behaviour.
 */
void INA228_Write_Diagnostic_Flags(void) {
    // DIAG_ALRT register (0x0B), 16-bit, reset = 0x0001
    // Bit 15: ALATCH = 1 - Latches alert pin until read, we never read it so it's kept high until a power cycle
    // Bit 13: SLOWALERT = 1 - Internal alert is done after averaging, not before on every sample
    // Bit  0: MEMSTAT = 1 - reads 1 when trim memory is healthy (reset default).

    uint16_t diag_alert = (1 << 15) | (1 << 13) | (1 << 0);

    uint8_t buf[2];
    pack_buf(buf, diag_alert, 2);
    swap_endianness(buf, 2);

    I2C_MemWrite(INA228_I2C_ADDRESS, INA228_REG_DIAG_ALRT, buf, 2);

    DEBUG_IO_print("Wrote DIAG_ALRT: 0x%04X\n\r", diag_alert);
}

/**
 * @brief Write the SOVL register (0x0C) to set the shunt overvoltage (overcurrent) threshold.
 */
void INA228_Write_Over_Voltage(void) {
    // SOVL register (0x0C), 16-bit, reset = 7FFFh
    // Counts = INA228_OVERCURRENT_A (A) × SHUNT_RESISTANCE_NOHMS (nΩ) / fault_LSB_nV (nV/LSB)
    uint16_t sovl = (uint16_t)((uint32_t)INA228_OVERCURRENT_A * SHUNT_RESISTANCE_NOHMS / INA228_FAULT_LSB_NV);

    uint8_t buf[2];
    pack_buf(buf, sovl, 2);
    swap_endianness(buf, 2);

    I2C_MemWrite(INA228_I2C_ADDRESS, INA228_REG_SOVL, buf, 2);

    DEBUG_IO_print("Wrote SOVL: 0x%04X\n\r", sovl);
}

/**
 * @brief Write the SUVL register (0x0D) to set the shunt undervoltage (reverse-current) threshold.
 */
void INA228_Write_Under_Voltage(void) {
    // SUVL register (0x0D), 16-bit, reset = 0000h
    // Counts = (INA228_REVERSE_CURRENT_A (A) × SHUNT_RESISTANCE_NOHMS (nΩ) / fault_LSB_nV)
    int16_t suvl = -(int16_t)((uint32_t)INA228_REVERSE_CURRENT_A * SHUNT_RESISTANCE_NOHMS / INA228_FAULT_LSB_NV);

    uint8_t buf[2];
    pack_buf(buf, (uint16_t)suvl, 2);
    swap_endianness(buf, 2);

    I2C_MemWrite(INA228_I2C_ADDRESS, INA228_REG_SUVL, buf, 2);

    DEBUG_IO_print("Wrote SUVL: 0x%04X\n\r", (uint16_t)suvl);
}

/**
 * @brief Trigger a read of the VSHUNT register.
 *        INA228_Process_Shunt_Voltage() is called automatically from the I2C RxCplt callback.
 */
void INA228_Read_Shunt_Voltage(void) {
    //HAL_StatusTypeDef status = I2C_MemRead_IT(INA228_I2C_ADDRESS, INA228_REG_SHUNT_VOLTAGE, raw_shunt_voltage_buf, 3, I2C_INA228_SHUNT_VOLTAGE);
    // Test interrupt later

    I2C_MemRead(INA228_I2C_ADDRESS, INA228_REG_SHUNT_VOLTAGE, raw_shunt_voltage_buf, 3, I2C_INA228_SHUNT_VOLTAGE);
}

/**
 * @brief Convert the raw VSHUNT bytes into engineering units.
 *        Called from HAL_I2C_MemRxCpltCallback — do not call directly.
 */
void INA228_Process_Shunt_Voltage(void) {
    // Processing the buffer is documented here: https://docs.google.com/document/d/1IN_0Pcg9eEbxtUkUSTO_Nc7S2LKmDKqyU20G1GsCJE0/edit?tab=t.i0huserxy3py

    // INA228 sends bytes big-endian: buf[0]=MSB, buf[1]=MID, buf[2]=LSB. Assemble the buf into an int32_t.
    int32_t raw_value = ((int32_t)raw_shunt_voltage_buf[0] << 16)
                      | ((int32_t)raw_shunt_voltage_buf[1] << 8)
                      |  (int32_t)raw_shunt_voltage_buf[2];

    // VSHUNT bits 23:4 are the 20-bit measurement; bits 3:0 are reserved.
    // Right-shift by 4 to discard the reserved bits.
    raw_value = raw_value >> 4;

    // Sign-extend from 20-bit two's complement to 32-bit
    if (raw_value & 0x80000) {
        raw_value |= 0xFFF00000;
    }

    // Convert counts into nanovolts, multiply first to avoid truncation to zero
    shunt_voltage_nV = (int32_t)(((int64_t)raw_value * INA228_VSHUNT_LSB_NV_NUM) / INA228_VSHUNT_LSB_NV_DEN);

    // nV / nΩ = A; A * 1000 = mA
    shunt_current_mA = (int32_t)(((int64_t)shunt_voltage_nV * 1000) / SHUNT_RESISTANCE_NOHMS);

    DEBUG_IO_print("Calculated shunt voltage: %d nV\n\r", shunt_voltage_nV);
    DEBUG_IO_print("Calculated shunt current: %d mA\n\r", shunt_current_mA);
}

/**
 * @brief Return the most recently measured shunt voltage.
 * @return Shunt voltage in nanovolts. Negative values indicate reverse current.
 */
int32_t INA228_Get_Shunt_Voltage_nV(void) {
    return shunt_voltage_nV;
}

/**
 * @brief Return the most recently calculated shunt current.
 * @return Current in milliamps. Negative values indicate reverse current.
 */
int32_t INA228_Get_Shunt_Current_mA(void) {
    return shunt_current_mA;
}

bool INA228_IsOvercurrent(void)
{
    uint8_t buffer[2] = {0};

    // read DIAG_ALRT register — 2 bytes, big-endian
    I2C_MemRead(INA228_I2C_ADDRESS, INA228_REG_DIAG_ALRT, buffer, 2, I2C_IDLE);

    // reassemble big-endian bytes into 16-bit value
    uint16_t diag_alrt = ((uint16_t)buffer[0] << 8) | buffer[1];

    DEBUG_IO_print("INA228 DIAG_ALRT: 0x%04X\n\r", diag_alrt);

    // check bit 11 — SOVL flag — overcurrent
    if (diag_alrt & (1U << 11)) {
        return true;
    }
    return false;  // bit 10 — SUVL flag — undercurrent
}

