#pragma once

#include "mst_defs.h"
#include "mst_types.h"
#include "logging.h"
#include "stm32f1xx.h"

/*

MESSAGE: MST status
ID: 0x622
LENGTH (bytes): 4
BITS:
0	Slave Board Communication Fault
1	BMS Self Test Fault
2	Overtemperature Fault
3	Undervoltage Fault
4	Overvoltage Fault
5	Isolation Loss Fault
6	Voltage Out of Range (Short)
7	Temperature Out of Range
8	Pack Balancing Active
9	LLIM Status
10	HLIM Status
// 11	Charge Overtemperature Trip
12	Low Voltage Warning
13	High Voltage Warning
// 14	Low Temperature Warning
15	High Temperature Warning
16 balancing enable
17 scrutineering enable
Rest is unused


MESSAGE: Voltage
ID: 0x623
LENGTH: 6	
BITS:
0-15	Total pack voltage
16-23	Index of module with lowest voltage
24-31	Index of module with highest voltage
Rest is unused

MESSAGE: Temperature
ID: 0x625
LENGTH: 5
BITS: 
0-7	Average Temperature
8-15	Index of module with lowest temperature
16-23	Index of module with highest temperature
Rest is unused


MESSAGE: Module Voltages
ID: 0x626
LENGTH: 5
BITS:
0-2	Multiplexing bits: 000 - modules [1,4], 001 - modules [5, 8], 010 - modules [9, 12], 011 - modules [13, 16], 100 - modules [17, 20], 101 - modules [21, 24], 110 - modules [25, 28], 111 - modules [29, 32]
8-15	Voltage 1 (take the 8 MSB of voltage values)
16-23	Voltage 2
24-31	Voltage 3
32-39	Voltage 4

MESSAGE: Module Temperatures
ID: 0x627
LENGTH: 5
BITS: 
0-2	Multiplexing bits: 000 - modules [1,4], 001 - modules [5, 8], 010 - modules [9, 12], 011 - modules [13, 16], 100 - modules [17, 20], 101 - modules [21, 24], 110 - modules [25, 28], 111 - modules [29, 32]
8-15	Temperature 1 (convert temperature into signed integer in Celsius i.e. int8_t -127 to 127°C) (ignore TODO on missing voltage-to-temperature conversion algorithm. )
16-23	Temperature 2
24-31	Temperature 3
32-39	Temperature 4

MESSAGE: Module Statuses
ID: 0x628
LENGTH: 5
BITS:
0-2	Multiplexing bits 
8-15	Module 1 Status
16-23	Module 2 Status
24-31	Module 3 Status
32-39	Module 4 Status

MESSAGE: Balancing Status
ID: 0x629
LENGTH: 4
BITS:
0-31	Module 0-31 Status: Each bit indicates bal status (active high)

*/

void CAN_SendMessage0x622(void);
void CAN_SendMessage0x623(void);
void CAN_SendMessage0x625(void);
void CAN_SendMessage0x626(void);
void CAN_SendMessage0x627(void);
void CAN_SendMessage0x628(void);
void CAN_SendMessage0x629(void);