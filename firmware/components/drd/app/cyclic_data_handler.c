/**
 * @file    cyclic_data_handler.c
 * @brief   Cyclic Data Handler for the DRD Module
 *
 * This file declares cyclic data for revlevant data relating to the DRD. It defines getter and 
 * setter functions for each datatype for easy handling and processing. 
 *
 * @author  Gregory Bian
 * @date    Feb 4 2026
 */

#include "cyclic_data_handler.h"
#include "cyclic_data.h"
#include <stdint.h>
#include <stdbool.h>
#include "diagnostic.h"
// #include "drive_state.h"

/*--------------------------------------------------------------------------
  CYCLIC DATA DEFINITIONS
--------------------------------------------------------------------------*/
       
/* CYCLIC DATA DRIVE DATA DEFINITIONS */
CYCLIC_DATA(uint32_t, cyclic_speed, MAX_CYCLE_TIME);            // Vehicle speed (km/h)
CYCLIC_DATA(int16_t, cyclic_pack_current, MAX_CYCLE_TIME);      // Battery pack current
CYCLIC_DATA(uint16_t, cyclic_pack_voltage, MAX_CYCLE_TIME);     // Battery pack voltage
CYCLIC_DATA(uint8_t, cyclic_drive_state, MAX_CYCLE_TIME);       // Drive state (ie. PARK, FORWARD)
CYCLIC_DATA(uint8_t, cyclic_soc, MAX_CYCLE_TIME);               // State of Charge (SOC %)

/* CYCLIC DATA TEMPERATURE DATA DEFINITIONS */
CYCLIC_DATA(uint8_t, cyclic_mppta_temperature, MAX_CYCLE_TIME); // MPPTA Temperature (°C)
CYCLIC_DATA(uint8_t, cyclic_mpptb_temperature, MAX_CYCLE_TIME); // MPPTB Temperature (°C)
CYCLIC_DATA(uint8_t, cyclic_mpptc_temperature, MAX_CYCLE_TIME); // MPPTC Temperature (°C)
CYCLIC_DATA(uint8_t, cyclic_mpptd_temperature, MAX_CYCLE_TIME); // MPPTD Temperature (°C)
CYCLIC_DATA(uint8_t, cyclic_batt_min_temperature, MAX_CYCLE_TIME);  // Battery Minimum Temp (°C)
CYCLIC_DATA(uint8_t, cyclic_batt_max_temperature, MAX_CYCLE_TIME);  // Battery Maximum Temp(°C)
CYCLIC_DATA(uint8_t, cyclic_mtr_cont_temperature, MAX_CYCLE_TIME);  // Motor Controller Temp (°C)
CYCLIC_DATA(uint8_t, cyclic_mtr_therm_temperature, MAX_CYCLE_TIME); // Motor Thermistor Temp (°C)

/* CYCLIC DATA BATTERY FAULT DATA DEFINITIONS */
CYCLIC_DATA(bool, cyclic_battery_fault, MAX_CYCLE_TIME); // Battery fault flag
CYCLIC_DATA(bool, cyclic_battery_supply_low, MAX_CYCLE_TIME);   // Motor fault flag
CYCLIC_DATA(bool, cyclic_battery_voltage_high, MAX_CYCLE_TIME); // High temperature warning flag
CYCLIC_DATA(bool, cyclic_battery_voltage_low, MAX_CYCLE_TIME);  // Low temperature warning flag
CYCLIC_DATA(bool, cyclic_battery_overtemp, MAX_CYCLE_TIME);     // Overtemperature warning flag
CYCLIC_DATA(bool, cyclic_battery_slave_board_comm_fault, MAX_CYCLE_TIME); // Slave board communication fault
CYCLIC_DATA(bool, cyclic_battery_overvolt_fault, MAX_CYCLE_TIME); // Overvoltage fault
CYCLIC_DATA(bool, cyclic_battery_undervolt_fault, MAX_CYCLE_TIME); // Undervoltage fault
CYCLIC_DATA(bool, cyclic_battery_charge_overcurrent_fault, MAX_CYCLE_TIME); // Charge overcurrent fault
CYCLIC_DATA(bool, cyclic_battery_discharge_overcurrent_fault, MAX_CYCLE_TIME); // Discharge overcurrent fault
CYCLIC_DATA(bool, cyclic_battery_reset_from_watchdog, MAX_CYCLE_TIME); // Reset-from-watchdog fault

/* CYCLIC DATA MOTOR FAULT DATA DEFINITIONS */
CYCLIC_DATA(bool, cyclic_motor_system_fault, MAX_CYCLE_TIME); // Motor system fault flag
CYCLIC_DATA(bool, cyclic_motor_overcurrent_fault, MAX_CYCLE_TIME); // Motor overcurrent fault flag
CYCLIC_DATA(bool, cyclic_motor_overvoltage_fault, MAX_CYCLE_TIME); // Motor overvoltage fault flag
CYCLIC_DATA(bool, cyclic_motor_fet_thermistor_error, MAX_CYCLE_TIME); // Motor FET thermistor error flag
CYCLIC_DATA(bool, cyclic_motor_comm_fault, MAX_CYCLE_TIME); // Motor communication fault flag
CYCLIC_DATA(bool, cyclic_motor_throttle_adc_out_of_range, MAX_CYCLE_TIME); // Motor throttle ADC out of range flag
CYCLIC_DATA(bool, cyclic_motor_throttle_adc_mismatch, MAX_CYCLE_TIME); // Motor throttle ADC mismatch flag

/* CYCLIC DATA WARNING DATA DEFINITIONS */
CYCLIC_DATA(bool, cyclic_low_volt_warning, MAX_CYCLE_TIME);       // Low voltage warning
CYCLIC_DATA(bool, cyclic_high_volt_warning, MAX_CYCLE_TIME);      // High voltage warning
CYCLIC_DATA(bool, cyclic_low_temp_warning, MAX_CYCLE_TIME);       // Low temperature warning
CYCLIC_DATA(bool, cyclic_high_temp_warning, MAX_CYCLE_TIME);      // High temperature warning
CYCLIC_DATA(bool, cyclic_no_ecu_message_warning, MAX_CYCLE_TIME); // No ECU message warning
CYCLIC_DATA(bool, cyclic_pack_overdischarge_warning, MAX_CYCLE_TIME); // Pack overdischarge warning
CYCLIC_DATA(bool, cyclic_pack_overcharge_warning, MAX_CYCLE_TIME); // Pack overcharge warning

/*--------------------------------------------------------------------------
  CYCLIC DATA SETTERS
--------------------------------------------------------------------------*/

/* CYCLIC DATA DRIVE DATA SETTERS */
void CyclicDataSetSpeed(uint32_t speed) { CYCLIC_DATA_SET(cyclic_speed, speed); }
void CyclicDataSetPackCurrent(int16_t current) { CYCLIC_DATA_SET(cyclic_pack_current, current); }
void CyclicDataSetPackVoltage(uint16_t voltage) { CYCLIC_DATA_SET(cyclic_pack_voltage, voltage); }
void CyclicDataSetDriveState(uint8_t state) { CYCLIC_DATA_SET(cyclic_drive_state, state); }
void CyclicDataSetSoc(uint8_t soc) { CYCLIC_DATA_SET(cyclic_soc, soc); }

/* CYCLIC DATA TEMPERATURE DATA SETTERS */
void CyclicDataSetMpptATemperature(uint8_t temperature) { CYCLIC_DATA_SET(cyclic_mppta_temperature, temperature); }
void CyclicDataSetMpptBTemperature(uint8_t temperature) { CYCLIC_DATA_SET(cyclic_mpptb_temperature, temperature); }
void CyclicDataSetMpptCTemperature(uint8_t temperature) { CYCLIC_DATA_SET(cyclic_mpptc_temperature, temperature); }
void CyclicDataSetMpptDTemperature(uint8_t temperature) { CYCLIC_DATA_SET(cyclic_mpptd_temperature, temperature); }
void CyclicDataSetBatteryMinTemperature(uint8_t temperature) { CYCLIC_DATA_SET(cyclic_batt_min_temperature, temperature); }
void CyclicDataSetBatteryMaxTemperature(uint8_t temperature) { CYCLIC_DATA_SET(cyclic_batt_max_temperature, temperature); }
void CyclicDataSetMtrContTemperature(uint8_t temperature) { CYCLIC_DATA_SET(cyclic_mtr_cont_temperature, temperature); }
void CyclicDataSetMtrThermTemperature(uint8_t temperature) { CYCLIC_DATA_SET(cyclic_mtr_therm_temperature, temperature); }

/* CYCLIC DATA BATTERY FAULT DATA SETTERS */
void CyclicDataSetBatteryFault(bool flag) { CYCLIC_DATA_SET(cyclic_battery_fault, flag); }
void CyclicDataSetBatterySupplyLow(bool flag) { CYCLIC_DATA_SET(cyclic_battery_supply_low, flag); }
void CyclicDataSetBatteryVoltageHigh(bool flag) { CYCLIC_DATA_SET(cyclic_battery_voltage_high, flag); }
void CyclicDataSetBatteryVoltageLow(bool flag) { CYCLIC_DATA_SET(cyclic_battery_voltage_low, flag); }
void CyclicDataSetBatteryOvertemp(bool flag) { CYCLIC_DATA_SET(cyclic_battery_overtemp, flag); }
void CyclicDataSetBatterySlaveBoardCommFault(bool flag) { CYCLIC_DATA_SET(cyclic_battery_slave_board_comm_fault, flag); }
void CyclicDataSetBatteryOvervoltFault(bool flag) { CYCLIC_DATA_SET(cyclic_battery_overvolt_fault, flag); }
void CyclicDataSetBatteryUndervoltFault(bool flag) { CYCLIC_DATA_SET(cyclic_battery_undervolt_fault, flag); }
void CyclicDataSetBatteryChargeOvercurrentFault(bool flag) { CYCLIC_DATA_SET(cyclic_battery_charge_overcurrent_fault, flag); }
void CyclicDataSetBatteryDischargeOvercurrentFault(bool flag) { CYCLIC_DATA_SET(cyclic_battery_discharge_overcurrent_fault, flag); }
void CyclicDataSetBatteryResetFromWatchdogFault(bool flag) { CYCLIC_DATA_SET(cyclic_battery_reset_from_watchdog, flag); }

/* CYCLIC DATA MOTOR FAULT DATA SETTERS */
void CyclicDataSetMotorSystemFault(bool flag) { CYCLIC_DATA_SET(cyclic_motor_system_fault, flag); }
void CyclicDataSetMotorOvercurrentFault(bool flag) { CYCLIC_DATA_SET(cyclic_motor_overcurrent_fault, flag); }
void CyclicDataSetMotorOvervoltageFault(bool flag) { CYCLIC_DATA_SET(cyclic_motor_overvoltage_fault, flag); }
void CyclicDataSetMotorFetThermistorError(bool flag) { CYCLIC_DATA_SET(cyclic_motor_fet_thermistor_error, flag); }
void CyclicDataSetMotorCommFault(bool flag) { CYCLIC_DATA_SET(cyclic_motor_comm_fault, flag); }
void CyclicDataSetMotorThrottleAdcOutOfRange(bool flag) { CYCLIC_DATA_SET(cyclic_motor_throttle_adc_out_of_range, flag); }
void CyclicDataSetMotorThrottleAdcMismatch(bool flag) { CYCLIC_DATA_SET(cyclic_motor_throttle_adc_mismatch, flag); }

/* CYCLIC DATA WARNING DATA SETTERS */
void CyclicDataSetLowVoltWarning(bool flag) { CYCLIC_DATA_SET(cyclic_low_volt_warning, flag); }
void CyclicDataSetHighVoltWarning(bool flag) { CYCLIC_DATA_SET(cyclic_high_volt_warning, flag); }
void CyclicDataSetLowTempWarning(bool flag) { CYCLIC_DATA_SET(cyclic_low_temp_warning, flag); }
void CyclicDataSetHighTempWarning(bool flag) { CYCLIC_DATA_SET(cyclic_high_temp_warning, flag); }
void CyclicDataSetNoEcuMessageWarning(bool flag) { CYCLIC_DATA_SET(cyclic_no_ecu_message_warning, flag); }
void CyclicDataSetPackOverdischargeWarning(bool flag) { CYCLIC_DATA_SET(cyclic_pack_overdischarge_warning, flag); }
void CyclicDataSetPackOverchargeWarning(bool flag) { CYCLIC_DATA_SET(cyclic_pack_overcharge_warning, flag); }

/*--------------------------------------------------------------------------
  CYCLIC DATA GETTERS
--------------------------------------------------------------------------*/

/* CYCLIC DATA DRIVE DATA GETTERS */
uint32_t* CyclicDataGetSpeed(void) { 
    DiagnosticSetSpeedTimeout(CYCLIC_DATA_GET(cyclic_speed) == NULL ? true : false);
    return CYCLIC_DATA_GET(cyclic_speed); 
} 
int16_t* CyclicDataGetPackCurrent(void) { 
    DiagnosticSetCurrentTimeout(CYCLIC_DATA_GET(cyclic_pack_current) == NULL ? true : false);
    return CYCLIC_DATA_GET(cyclic_pack_current); 
}
uint16_t* CyclicDataGetPackVoltage(void) { 
    DiagnosticSetVoltageTimeout(CYCLIC_DATA_GET(cyclic_pack_voltage) == NULL ? true : false);
    return CYCLIC_DATA_GET(cyclic_pack_voltage); 
}
uint8_t* CyclicDataGetDriveState(void)
{
    if (CyclicDataGetSpeed() == NULL)
    {
        DiagnosticSetDriveStateTimeout(true);
        return NULL; // Stale data for drive state
    }
    else
    {
        DiagnosticSetDriveStateTimeout(CYCLIC_DATA_GET(cyclic_drive_state) == NULL ? true : false);
        return CYCLIC_DATA_GET(cyclic_drive_state);
    }
}
uint8_t* CyclicDataGetSoc(void)
{
    if ((CyclicDataGetPackVoltage() == NULL) || (CyclicDataGetPackCurrent() == NULL))
    {
        DiagnosticSetSocTimeout(true);
        return NULL;
    }
    else
    {
        DiagnosticSetSocTimeout(CYCLIC_DATA_GET(cyclic_soc) == NULL ? true : false);
        return CYCLIC_DATA_GET(cyclic_soc);
    }
}

/* CYCLIC DATA TEMPERATURE DATA GETTERS */
uint8_t* CyclicDataGetMpptATemperature(void) { return CYCLIC_DATA_GET(cyclic_mppta_temperature); }
uint8_t* CyclicDataGetMpptBTemperature(void) { return CYCLIC_DATA_GET(cyclic_mpptb_temperature); }
uint8_t* CyclicDataGetMpptCTemperature(void) { return CYCLIC_DATA_GET(cyclic_mpptc_temperature); }
uint8_t* CyclicDataGetMpptDTemperature(void) { return CYCLIC_DATA_GET(cyclic_mpptd_temperature); }
uint8_t* CyclicDataGetBatteryMinTemperature(void) { return CYCLIC_DATA_GET(cyclic_batt_min_temperature); }
uint8_t* CyclicDataGetBatteryMaxTemperature(void) { return CYCLIC_DATA_GET(cyclic_batt_max_temperature); }
uint8_t* CyclicDataGetMtrContTemperature(void) { return CYCLIC_DATA_GET(cyclic_mtr_cont_temperature); }
uint8_t* CyclicDataGetMtrThermTemperature(void) { return CYCLIC_DATA_GET(cyclic_mtr_therm_temperature); }

/* CYCLIC DATA BATTERY FAULT DATA GETTERS */
bool* CyclicDataGetBatteryFault(void) { return CYCLIC_DATA_GET(cyclic_battery_fault); }
bool* CyclicDataGetBatterySupplyLow(void) { return CYCLIC_DATA_GET(cyclic_battery_supply_low); }
bool* CyclicDataGetBatteryVoltageHigh(void) { return CYCLIC_DATA_GET(cyclic_battery_voltage_high); }
bool* CyclicDataGetBatteryVoltageLow(void) { return CYCLIC_DATA_GET(cyclic_battery_voltage_low); }
bool* CyclicDataGetBatteryOvertemp(void) { return CYCLIC_DATA_GET(cyclic_battery_overtemp); }
bool* CyclicDataGetBatterySlaveBoardCommFault(void) { return CYCLIC_DATA_GET(cyclic_battery_slave_board_comm_fault); }
bool* CyclicDataGetBatteryOvervoltFault(void) { return CYCLIC_DATA_GET(cyclic_battery_overvolt_fault); }
bool* CyclicDataGetBatteryUndervoltFault(void) { return CYCLIC_DATA_GET(cyclic_battery_undervolt_fault); }
bool* CyclicDataGetBatteryChargeOvercurrentFault(void) { return CYCLIC_DATA_GET(cyclic_battery_charge_overcurrent_fault); }
bool* CyclicDataGetBatteryDischargeOvercurrentFault(void) { return CYCLIC_DATA_GET(cyclic_battery_discharge_overcurrent_fault); }
bool* CyclicDataGetBatteryResetFromWatchdogFault(void) { return CYCLIC_DATA_GET(cyclic_battery_reset_from_watchdog); }

/* CYCLIC DATA MOTOR FAULT DATA GETTERS */
bool* CyclicDataGetMotorSystemFault(void) { return CYCLIC_DATA_GET(cyclic_motor_system_fault); }
bool* CyclicDataGetMotorOvercurrentFault(void) { return CYCLIC_DATA_GET(cyclic_motor_overcurrent_fault); }
bool* CyclicDataGetMotorOvervoltageFault(void) { return CYCLIC_DATA_GET(cyclic_motor_overvoltage_fault); }
bool* CyclicDataGetMotorFetThermistorError(void) { return CYCLIC_DATA_GET(cyclic_motor_fet_thermistor_error); }
bool* CyclicDataGetMotorCommFault(void) { return CYCLIC_DATA_GET(cyclic_motor_comm_fault); }
bool* CyclicDataGetMotorThrottleAdcOutOfRange(void) { return CYCLIC_DATA_GET(cyclic_motor_throttle_adc_out_of_range); }
bool* CyclicDataGetMotorThrottleAdcMismatch(void) { return CYCLIC_DATA_GET(cyclic_motor_throttle_adc_mismatch); }  

/* CYCLIC DATA WARNING DATA GETTERS */
bool* CyclicDataGetLowVoltWarning(void) { return CYCLIC_DATA_GET(cyclic_low_volt_warning); }
bool* CyclicDataGetHighVoltWarning(void) { return CYCLIC_DATA_GET(cyclic_high_volt_warning); }
bool* CyclicDataGetLowTempWarning(void) { return CYCLIC_DATA_GET(cyclic_low_temp_warning); }
bool* CyclicDataGetHighTempWarning(void) { return CYCLIC_DATA_GET(cyclic_high_temp_warning); }
bool* CyclicDataGetNoEcuMessageWarning(void) { return CYCLIC_DATA_GET(cyclic_no_ecu_message_warning); }
bool* CyclicDataGetPackOverdischargeWarning(void) { return CYCLIC_DATA_GET(cyclic_pack_overdischarge_warning); }
bool* CyclicDataGetPackOverchargeWarning(void) { return CYCLIC_DATA_GET(cyclic_pack_overcharge_warning); }

