#ifndef DRD_HEALTH_H
#define DRD_HEALTH_H

#include <stdbool.h>
#include <stdint.h>


#include "stm32f1xx_hal.h"

extern CAN_HandleTypeDef hcan;

// datasheet typical values
#define VREFINT_MV 1200     // built-in reference voltage (DS Table 12)
#define V25_MV 1430         // temp sensor voltage at 25 degC (DS Table 51)
#define MAX_TEMP_C 125       // max mcu temperature in C 
#define MIN_VOLT_MV 2000    // minimum mcu voltage in mV
#define ADC_FULL_SCALE 4095      // 12-bit ADC (2^12 - 1)
#define AVG_SLOPE_UV_PER_C 4300 // temp sensor slope in uV/C (DS Table 51)
#define TS_REF_TEMP_C 25       // temp sensor reference temperature in C (DS Table 51)


void McuHealthDriverInit( void );
uint16_t DRD_Health_ReadVddMv( void );
uint16_t DRD_Read_Vdd( void );
int16_t DRD_Read_Temp( void );

#endif