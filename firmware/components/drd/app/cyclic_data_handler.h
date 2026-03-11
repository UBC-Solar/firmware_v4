/**
 * @file    cyclic_data_handler.h
 * @brief   Cyclic Data Handler for the DRD Module
 *
 * This header file declares the function prototypes for the getters and setter of the Cyclic datatypes. 
 *
 * @author  Gregory Bian
 * @date    Feb 4 2026
 */


#ifndef CYCLIC_DATA_HANDLER_H
#define CYCLIC_DATA_HANDLER_H

#include <stdint.h>
#include <stdbool.h>

// User Defines
#define MAX_CYCLE_TIME 1000 // Maximum cycle time in milliseconds

/* CYCLIC DATA DRIVE DATA SETTERS */
void CyclicDataSetSpeed(uint32_t speed);
void CyclicDataSetPackCurrent(int16_t current);
void CyclicDataSetPackVoltage(uint16_t voltage);
void CyclicDataSetDriveState(uint8_t state);
void CyclicDataSetSoc(uint8_t soc);

/* CYCLIC DATA TEMPERATURE DATA SETTERS */
void CyclicDataSetMpptATemperature(uint8_t temperature);
void CyclicDataSetMpptBTemperature(uint8_t temperature);
void CyclicDataSetMpptCTemperature(uint8_t temperature);
void CyclicDataSetMpptDTemperature(uint8_t temperature);
void CyclicDataSetBatteryMinTemperature(uint8_t temperature);
void CyclicDataSetBatteryMaxTemperature(uint8_t temperature);
void CyclicDataSetMtrContTemperature(uint8_t temperature);
void CyclicDataSetMtrThermTemperature(uint8_t temperature);

/* CYCLIC DATA BATTERY FAULT DATA SETTERS */
void CyclicDataSetBatteryFault(bool flag);
void CyclicDataSetBatterySupplyLow(bool flag);
void CyclicDataSetBatteryVoltageHigh(bool flag);
void CyclicDataSetBatteryVoltageLow(bool flag);
void CyclicDataSetBatteryOvertemp(bool flag);
void CyclicDataSetBatterySlaveBoardCommFault(bool flag);
void CyclicDataSetBatteryOvervoltFault(bool flag);
void CyclicDataSetBatteryUndervoltFault(bool flag);
void CyclicDataSetBatteryChargeOvercurrentFault(bool flag);
void CyclicDataSetBatteryDischargeOvercurrentFault(bool flag);
void CyclicDataSetBatteryResetFromWatchdogFault(bool flag);

/* CYCLIC DATA MOTOR FAULT DATA SETTERS */
void CyclicDataSetMotorSystemFault(bool flag);
void CyclicDataSetMotorOvercurrentFault(bool flag);
void CyclicDataSetMotorOvervoltageFault(bool flag);
void CyclicDataSetMotorFetThermistorError(bool flag);
void CyclicDataSetMotorCommFault(bool flag);
void CyclicDataSetMotorThrottleAdcOutOfRange(bool flag);
void CyclicDataSetMotorThrottleAdcMismatch(bool flag);

/* CYCLIC DATA WARNING DATA SETTERS */
void CyclicDataSetLowVoltWarning(bool flag);
void CyclicDataSetHighVoltWarning(bool flag);
void CyclicDataSetLowTempWarning(bool flag);
void CyclicDataSetHighTempWarning(bool flag);
void CyclicDataSetNoEcuMessageWarning(bool flag);
void CyclicDataSetPackOverdischargeWarning(bool flag);
void CyclicDataSetPackOverchargeWarning(bool flag);


/* CYCLIC DATA DRIVE DATA GETTERS */
uint32_t* CyclicDataGetSpeed(void);
int16_t* CyclicDataGetPackCurrent(void);
uint16_t* CyclicDataGetPackVoltage(void);
uint8_t* CyclicDataGetDriveState(void);
uint8_t* CyclicDataGetSoc(void);

/* CYCLIC DATA TEMPERATURE DATA GETTERS */
uint8_t* CyclicDataGetMpptATemperature(void);
uint8_t* CyclicDataGetMpptBTemperature(void);
uint8_t* CyclicDataGetMpptCTemperature(void);
uint8_t* CyclicDataGetMpptDTemperature(void);
uint8_t* CyclicDataGetBatteryMinTemperature(void);
uint8_t* CyclicDataGetBatteryMaxTemperature(void);
uint8_t* CyclicDataGetMtrContTemperature(void);
uint8_t* CyclicDataGetMtrThermTemperature(void);

/* CYCLIC DATA BATTERY FAULT DATA GETTERS */
bool* CyclicDataGetBatteryFault(void);
bool* CyclicDataGetBatterySupplyLow(void);
bool* CyclicDataGetBatteryVoltageHigh(void);
bool* CyclicDataGetBatteryVoltageLow(void);
bool* CyclicDataGetBatteryOvertemp(void);
bool* CyclicDataGetBatterySlaveBoardCommFault(void);
bool* CyclicDataGetBatteryOvervoltFault(void);
bool* CyclicDataGetBatteryUndervoltFault(void);
bool* CyclicDataGetBatteryChargeOvercurrentFault(void);
bool* CyclicDataGetBatteryDischargeOvercurrentFault(void);
bool* CyclicDataGetBatteryResetFromWatchdogFault(void);

/* CYCLIC DATA MOTOR FAULT DATA GETTERS */
bool* CyclicDataGetMotorSystemFault(void);
bool* CyclicDataGetMotorOvercurrentFault(void);
bool* CyclicDataGetMotorOvervoltageFault(void);
bool* CyclicDataGetMotorFetThermistorError(void);
bool* CyclicDataGetMotorCommFault(void);
bool* CyclicDataGetMotorThrottleAdcOutOfRange(void);
bool* CyclicDataGetMotorThrottleAdcMismatch(void);

/* CYCLIC DATA WARNING DATA GETTERS */
bool* CyclicDataGetLowVoltWarning(void);
bool* CyclicDataGetHighVoltWarning(void);
bool* CyclicDataGetLowTempWarning(void);
bool* CyclicDataGetHighTempWarning(void);
bool* CyclicDataGetNoEcuMessageWarning(void);
bool* CyclicDataGetPackOverdischargeWarning(void);
bool* CyclicDataGetPackOverchargeWarning(void);


#endif // CYCLIC_DATA_HANDLER_H
