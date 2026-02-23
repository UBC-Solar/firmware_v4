/**
 * @file    cyclic_data_handler.h
 * @brief   Cyclic Date Hnadler for the DRD Module
 *
 * This header file declares the function prototypes for the getters and setter of the Cyclic datatypes. 
 *
 * @author  Gregory Bian
 * @date    Feb 4 2026
 */


#ifndef CYCLIC_DATA_HANDLER_H
#define CYCLIC_DATA_HANDLER_H

#include <stdint.h>

// User Defines
#define MAX_CYCLE_TIME 1000 // Maximum cycle time in milliseconds

// Set functions for cyclic data
void CyclicDataSetSpeed(uint32_t speed);
void CyclicDataSetPackCurrent(int16_t current);
void CyclicDataSetPackVoltage(uint16_t voltage);
void CyclicDataSetDriveState(uint8_t state);
void CyclicDataSetSoc(uint8_t soc);
void CyclicDataSetMpptATemperature(uint8_t temperature);
void CyclicDataSetMpptBTemperature(uint8_t temperature);
void CyclicDataSetMpptCTemperature(uint8_t temperature);
void CyclicDataSetMpptDTemperature(uint8_t temperature);
void CyclicDataSetBatteryMinTemperature(uint8_t temperature);
void CyclicDataSetBatteryMaxTemperature(uint8_t temperature);
void CyclicDataSetMtrContTemperature(uint8_t temperature);
void CyclicDataSetMtrThermTemperature(uint8_t temperature);

// Get functions for cyclic data
uint32_t* CyclicDataGetSpeed(void);
int16_t* CyclicDataGetPackCurrent(void);
uint16_t* CyclicDataGetPackVoltage(void);
uint8_t* CyclicDataGetDriveState(void);
uint8_t* CyclicDataGetSoc(void);
uint8_t* CyclicDataGetMpptATemperature(void);
uint8_t* CyclicDataGetMpptBTemperature(void);
uint8_t* CyclicDataGetMpptCTemperature(void);
uint8_t* CyclicDataGetMpptDTemperature(void);
uint8_t* CyclicDataGetBatteryMinTemperature(void);
uint8_t* CyclicDataGetBatteryMaxTemperature(void);
uint8_t* CyclicDataGetMtrContTemperature(void);
uint8_t* CyclicDataGetMtrThermTemperature(void);
#endif // CYCLIC_DATA_HANDLER_H
