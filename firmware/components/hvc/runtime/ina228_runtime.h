#pragma once

#include <stdint.h>
#include <stdbool.h>
// ---------------------------------------------------------------------------
// I2C address
// INA228 7-bit address with A1=GND, A0=GND is 0b1000000 = 0x40.
// HAL expects the 8-bit form (7-bit << 1).
// ---------------------------------------------------------------------------
#define INA228_I2C_ADDRESS (0x40 << 1)

// ---------------------------------------------------------------------------
// Register addresses (Table 7-3)
// ---------------------------------------------------------------------------
#define INA228_REG_CONFIG           0x00  // Configuration (16-bit)
#define INA228_REG_ADC_CONFIG       0x01  // ADC Configuration (16-bit)
#define INA228_REG_SHUNT_CAL        0x02  // Shunt Calibration (16-bit)
#define INA228_REG_SHUNT_VOLTAGE    0x04  // Shunt Voltage Measurement (24-bit)
#define INA228_REG_VBUS             0x05  // Bus Voltage Measurement (24-bit)
#define INA228_REG_DIETEMP          0x06  // Temperature Measurement (16-bit)
#define INA228_REG_DIAG_ALRT        0x0B  // Diagnostic Flags and Alert (16-bit)
#define INA228_REG_SOVL             0x0C  // Shunt Overvoltage Threshold  (overcurrent)
#define INA228_REG_SUVL             0x0D  // Shunt Undervoltage Threshold (reverse current)
#define INA228_REG_BOVL             0x0E  // Bus Overvoltage Threshold
#define INA228_REG_BUVL             0x0F  // Bus Undervoltage Threshold

// ADC range selection
// 0 = ±163.84 mV full scale, shunt LSB = 312.5 nV
// 1 = ±40.96 mV full scale, shunt LSB = 78.125 nV  (higher resolution, lower range)
#define INA228_ADCRANGE 1

#if INA228_ADCRANGE == 1
// Shunt voltage LSB expressed as a fraction: 625/8 nV = 78.125 nV
#define INA228_VSHUNT_LSB_NV_NUM  625 // Numerator
#define INA228_VSHUNT_LSB_NV_DEN  8 // Denominator
#else
// Shunt voltage LSB: 3125/10 nV = 312.5 nV
#define INA228_VSHUNT_LSB_NV_NUM  3125 // Numerator
#define INA228_VSHUNT_LSB_NV_DEN  10 // Denominator
#endif

// Fault threshold register LSB = 16 × ADC shunt LSB
// ADCRANGE=1: 16 × 78.125 nV = 1250 nV/LSB
// ADCRANGE=0: 16 × 312.5  nV = 5000 nV/LSB
#if INA228_ADCRANGE == 1
#define INA228_FAULT_LSB_NV         1250
#else
#define INA228_FAULT_LSB_NV         5000
#endif

#define SHUNT_RESISTANCE_NOHMS      100496   // Shunt resistor value in nano-ohms (Thx Bryan Cady of Moment Energy!)

// Fault thresholds in Amperes, unisgned in either case
#define INA228_OVERCURRENT_A        60   // Positive overcurrent limit (A)
#define INA228_REVERSE_CURRENT_A    20    // Reverse current magnitude limit (A)

/**
 * @brief Initialise the INA228 — call once at startup after I2C is ready.
 *        Writes CONFIG, ADC_CONFIG, DIAG_ALRT, SOVL, and SUVL using blocking I2C.
 */
void INA228_Init(void);

/**
 * @brief Write the CONFIG register (0x00) to set the ADC range.
 */
void INA228_Write_Config(void);

/**
 * @brief Write the ADC_CONFIG register (0x01) to set conversion mode, times, and averaging.
 */
void INA228_Write_ADC_Config(void);

/**
 * @brief Write the DIAG_ALRT register (0x0B) to configure alert latch and slow-alert behaviour.
 */
void INA228_Write_Diagnostic_Flags(void);

/**
 * @brief Write the SOVL register (0x0C) to set the shunt overvoltage (overcurrent) threshold.
 */
void INA228_Write_Over_Voltage(void);

/**
 * @brief Write the SUVL register (0x0D) to set the shunt undervoltage (reverse-current) threshold.
 */
void INA228_Write_Under_Voltage(void);

/**
 * @brief Trigger a read of the VSHUNT register.
 *        INA228_Process_Shunt_Voltage() is called automatically from the I2C RxCplt callback.
 */
void INA228_Read_Shunt_Voltage(void);

/**
 * @brief Convert the raw VSHUNT bytes into engineering units.
 *        Called from HAL_I2C_MemRxCpltCallback — do not call directly.
 */
void INA228_Process_Shunt_Voltage(void);

/**
 * @brief Return the most recently measured shunt voltage.
 * @return Shunt voltage in nanovolts. Negative values indicate reverse current.
 */
int32_t INA228_Get_Shunt_Voltage_nV(void);

/**
 * @brief Return the most recently calculated shunt current.
 * @return Current in milliamps. Negative values indicate reverse current.
 */
int32_t INA228_Get_Shunt_Current_mA(void);

/**
 * @brief Read DIAG_ALRT register and determine if overcurrent triggered the alert.
 * @return true if overcurrent, false if undercurrent.
 */
bool INA228_IsOvercurrent(void);
