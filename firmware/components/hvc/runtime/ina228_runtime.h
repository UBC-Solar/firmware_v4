#pragma once

#define INA228_I2C_ADDRESS 0x80 << 1 // Shifted left by 1 bit, more documentation for this later

#define INA228_REG_CONFIG 0x00
#define INA228_REG_ADC_CONFIG 0x01
#define INA228_REG_SHUNT_VOLTAGE 0x04
#define INA228_REG_DIAGNOSTIC_FLAGS 0x0B
#define INA228_REG_OVER_VOLTAGE_LIMIT 0x0E
#define INA228_REG_UNDER_VOLTAGE_LIMIT 0x0F

#define SHUNT_RESISTANCE_UOHMS 100 // 100 micro-ohms
#define CURRENT_LSB_UA 1 // 1 microamp per bit

void INA228_I2C_Init(void);

void INA228_Read_Voltage(void);

void INA228_Write_Config(void);
void INA228_Write_ADC_Config(void);
void INA228_Write_Diagnostic_Flags(void);
void INA228_Write_Over_Voltage(void);
void INA228_Write_Under_Voltage(void);

void INA228_Process_Shunt_Voltage(void);

uint32_t INA228_Get_Shunt_Voltage(void);
uint32_t INA228_Get_Shunt_Current(void);
