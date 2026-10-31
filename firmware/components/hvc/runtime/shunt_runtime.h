#pragma once

#include <stdint.h>
#include <stdbool.h>

// ---------------------------------------------------------------------------
// Shunt monitor chip selection
// The INA237 and INA228 share a register map; only the VSHUNT register
// (0x04) differs: INA228 is 24-bit (20-bit data), INA237 is 16-bit.
// ---------------------------------------------------------------------------
#define SHUNT_MONITOR_INA228 1
#define SHUNT_MONITOR_INA237 2
#define SHUNT_MONITOR SHUNT_MONITOR_INA237

#if !defined(SHUNT_MONITOR) || (SHUNT_MONITOR != SHUNT_MONITOR_INA228 && SHUNT_MONITOR != SHUNT_MONITOR_INA237)
#error "SHUNT_MONITOR must be SHUNT_MONITOR_INA228 or SHUNT_MONITOR_INA237"
#endif

// ---------------------------------------------------------------------------
// I2C address
// INA228/INA237 7-bit address with A1=GND, A0=GND is 0b1000000 = 0x40.
// HAL expects the 8-bit form (7-bit << 1).
// ---------------------------------------------------------------------------
#define SHUNT_I2C_ADDRESS (0x40 << 1)

// ---------------------------------------------------------------------------
// Register addresses (Table 7-3)
// ---------------------------------------------------------------------------
#define SHUNT_REG_CONFIG       0x00  // Configuration (16-bit)
#define SHUNT_REG_ADC_CONFIG   0x01  // ADC Configuration (16-bit)
#define SHUNT_REG_SHUNT_CAL    0x02  // Shunt Calibration (16-bit)
#define SHUNT_REG_SHUNT_VOLTAGE 0x04  // Shunt Voltage Measurement (INA228: 24-bit, INA237: 16-bit)
#define SHUNT_REG_VBUS         0x05  // Bus Voltage Measurement (24-bit)
#define SHUNT_REG_DIETEMP      0x06  // Temperature Measurement (16-bit)
#define SHUNT_REG_DIAG_ALRT    0x0B  // Diagnostic Flags and Alert (16-bit)
#define SHUNT_REG_SOVL         0x0C  // Shunt Overvoltage Threshold  (overcurrent)
#define SHUNT_REG_SUVL         0x0D  // Shunt Undervoltage Threshold (reverse current)
#define SHUNT_REG_BOVL         0x0E  // Bus Overvoltage Threshold
#define SHUNT_REG_BUVL         0x0F  // Bus Undervoltage Threshold

// ADC range selection
// 0 = ±163.84 mV full scale, shunt LSB = 312.5 nV (INA228) / 5 µV (INA237)
// 1 = ±40.96 mV full scale, shunt LSB = 78.125 nV (INA228) / 1.25 µV (INA237)  (higher resolution, lower range)
#define SHUNT_ADCRANGE 1

#if SHUNT_MONITOR == SHUNT_MONITOR_INA228
// VSHUNT is 24-bit; bits 23:4 hold the 20-bit measurement
#define SHUNT_VSHUNT_READ_BYTES  3
#if SHUNT_ADCRANGE == 1
// Shunt voltage LSB expressed as a fraction: 625/8 nV = 78.125 nV
#define SHUNT_VSHUNT_LSB_NV_NUM  625 // Numerator
#define SHUNT_VSHUNT_LSB_NV_DEN  8 // Denominator
#else
// Shunt voltage LSB: 3125/10 nV = 312.5 nV
#define SHUNT_VSHUNT_LSB_NV_NUM  3125 // Numerator
#define SHUNT_VSHUNT_LSB_NV_DEN  10 // Denominator
#endif
#elif SHUNT_MONITOR == SHUNT_MONITOR_INA237
// VSHUNT is 16-bit; all bits hold the measurement
#define SHUNT_VSHUNT_READ_BYTES  2
#if SHUNT_ADCRANGE == 1
// Shunt voltage LSB: 1250 nV = 1.25 µV
#define SHUNT_VSHUNT_LSB_NV_NUM  1250 // Numerator
#define SHUNT_VSHUNT_LSB_NV_DEN  1 // Denominator
#else
// Shunt voltage LSB: 5000 nV = 5 µV
#define SHUNT_VSHUNT_LSB_NV_NUM  5000 // Numerator
#define SHUNT_VSHUNT_LSB_NV_DEN  1 // Denominator
#endif
#endif

// Fault thresholds for SOVL/SUVL threshold register LSB
// ADCRANGE=1: 1250 nV/LSB
// ADCRANGE=0: 5000 nV/LSB
#if SHUNT_ADCRANGE == 1
#define SHUNT_FAULT_LSB_NV     1250
#else
#define SHUNT_FAULT_LSB_NV     5000
#endif

#define SHUNT_RESISTANCE_NOHMS  100496   // Shunt resistor value in nano-ohms (Thx Bryan Cady of Moment Energy!)

// Fault thresholds in Amperes, unisgned in either case
#define SHUNT_OVERCURRENT_A     60   // Positive overcurrent limit (A)
#define SHUNT_REVERSE_CURRENT_A 20    // Reverse current magnitude limit (A)

/**
 * @brief Initialise the shunt monitor — call once at startup after I2C is ready.
 *        Writes CONFIG, ADC_CONFIG, DIAG_ALRT, SOVL, and SUVL using blocking I2C.
 */
void Shunt_Init(void);

/**
 * @brief Write the CONFIG register (0x00) to set the ADC range.
 */
void Shunt_Write_Config(void);

/**
 * @brief Write the ADC_CONFIG register (0x01) to set conversion mode, times, and averaging.
 */
void Shunt_Write_ADC_Config(void);

/**
 * @brief Write the DIAG_ALRT register (0x0B) to configure alert latch and slow-alert behaviour.
 */
void Shunt_Write_Diagnostic_Flags(void);

/**
 * @brief Write the SOVL register (0x0C) to set the shunt overvoltage (overcurrent) threshold.
 */
void Shunt_Write_Over_Voltage(void);

/**
 * @brief Write the SUVL register (0x0D) to set the shunt undervoltage (reverse-current) threshold.
 */
void Shunt_Write_Under_Voltage(void);

/**
 * @brief Trigger a read of the VSHUNT register.
 *        Shunt_Process_Shunt_Voltage() is called automatically from the I2C RxCplt callback.
 */
void Shunt_Read_Shunt_Voltage(void);

/**
 * @brief Convert the raw VSHUNT bytes into engineering units.
 *        Called from HAL_I2C_MemRxCpltCallback — do not call directly.
 */
void Shunt_Process_Shunt_Voltage(void);

/**
 * @brief Return the most recently measured shunt voltage.
 * @return Shunt voltage in nanovolts. Negative values indicate reverse current.
 */
int32_t Shunt_Get_Shunt_Voltage_nV(void);

/**
 * @brief Return the most recently calculated shunt current.
 * @return Current in milliamps. Negative values indicate reverse current.
 */
int32_t Shunt_Get_Shunt_Current_mA(void);

/**
 * @brief Read DIAG_ALRT register and determine if overcurrent triggered the alert.
 * @return true if overcurrent, false if undercurrent.
 */
bool Shunt_IsOvercurrent(void);
