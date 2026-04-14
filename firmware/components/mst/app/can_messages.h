#pragma once

#include "mst_defs.h"
#include "mst_types.h"
#include "logging.h"
#include "stm32f1xx.h"

/*

MST status
0	Slave Board Communication Fault
1	BMS Self Test Fault
2	Overtemperature Fault
3	Undervoltage Fault
4	Overvoltage Fault
5	Isolation Loss Fault
// 6	Discharge or Charge Overcurrent Fault
7	Voltage Out of Range (Short)
8	Temperature Out of Range
9	Pack Balancing Active
10	LLIM Status
11	HLIM Status
12	Charge Overtemperature Trip
13	Low Voltage Warning
14	High Voltage Warning
15	Low Temperature Warning
16	High Temperature Warning
// 17	Request Regen Turned Off
// 18	No ECU Current Message Received Warning
new balancing enable
new scrutineering enable
19-55	Reserved

Pack summary
    Voltage 	0x623	10	TRUE	6	0-15	Total pack voltage
                                        // 16-23	Voltage of least charged module
                                        24-31	Index of module with lowest voltage
                                        // 32-39	Voltage of highest charged module
                                        40-47	Index of module with highest voltage
    Temperature 	0x625	10	FALSE	5	0-7	Average Temperature
                                            // 8-15	Min Temperature
                                            16-23	Index of module with lowest temperature
                                            // 24-31	Max Temperature 
                                            32-39	Index of module with highest temperature
// Pack Health	0x624	10	FALSE	7	0-7	SOC
//                                     8-23	Depth Of Discharge
//                                     24-39	Capacity
//                                     40-47	0x00
//                                     48-55	SOH
Module Voltages	0x626	80	TRUE	5	0-2	Multiplexing bits 
                                        8-15	Voltage 1
                                        16-23	Voltage 2
                                        24-31	Voltage 3
                                        32-39	Voltage 4
Module Temperatures	0x627	80	TRUE	5	0-2	Multiplexing bits 
                                            8-15	Temperature 1
                                            16-23	Temperature 2
                                            24-31	Temperature 3
                                            32-39	Temperature 4
Module Statuses	0x628	80	FALSE	5	0-2	Multiplexing bits 
					8-15	Module 1 Status
					16-23	Module 2 Status
					24-31	Module 3 Status
					32-39	Module 4 Status
Balancing Status	0x629	10	FALSE	4	0-31	Module 0-31 Status

*/