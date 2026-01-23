/*
 * rtd.h
 *
 *  Created on: Nov 1, 2025
 *      Author: Luke
 */

#ifndef INC_RTD_H_
#define INC_RTD_H_
#include "main.h"
#include <stdbool.h>
#include <stdint.h>

typedef union {
    struct {
        bool OUV : 1;                    // bit0 = D2 (Over/undervoltage)
        bool RTDIN_Low_Voltage : 1;      // bit1 = D3
        bool REFIN_Low : 1;              // bit2 = D4
        bool REFIN_High : 1;             // bit3 = D5
        bool Low_Threshold_Violation : 1;// bit4 = D6
        bool High_Threshold_Violation : 1;// bit5 = D7
    } faults;
    uint8_t bits;
} Rtd_faults_t;

typedef enum {
	RtdStatusOk,
	RtdStatusFault,
	RtdFaultReadAttemptExceded
}Rtd_status_t;

//PUBLIC FUNCTIONS
Rtd_status_t RTD_GetTemperature(uint32_t* temperature, Rtd_faults_t* status);
void RTD_Init();
uint32_t RTD_test();


#endif /* INC_RTD_H_ */
