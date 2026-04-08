#include "ina228_runtime.h"

#include "i2c_driver.h"

volatile uint32_t shunt_voltage = 0; // nV

static uint32_t raw_shunt_voltage = 0;

void INA228_I2C_Init(void) {

}

// send read command, which is read into the shunt_voltage variable by the interrupt function
void INA228_Read_Voltage(void) {
    I2C_MemRead_IT(INA228_I2C_ADDRESS, INA228_REG_SHUNT_VOLTAGE, (uint8_t *)&raw_shunt_voltage, 3);
}

void INA228_Write_Config(void) {

}

void INA228_Write_ADC_Config(void) {

}

void INA228_Write_Diagnostic_Flags(void) {

}

void INA228_Write_Over_Voltage(void) {

}

void INA228_Write_Under_Voltage(void) {

}

void INA228_Process_Shunt_Voltage(void) {
    
}

uint32_t INA228_Get_Shunt_Voltage(void) {
    return shunt_voltage;
}

uint32_t INA228_Get_Shunt_Current(void) {
    // Current (A) = Voltage (V) / Resistance (Ohms)
    // Convert shunt voltage from nV to V and resistance from micro-ohms to ohms:
    // (Voltage * 1e-9) / (Resistance * 1e-6)
    // Voltage / Resistance * 1e-3
    // Now *1e6 so the output unit is microamps
    // 1e3 * voltage / resistance
    return 1e3 * shunt_voltage / SHUNT_RESISTANCE_UOHMS;
}
