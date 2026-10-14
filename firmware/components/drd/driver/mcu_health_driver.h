#ifndef DRD_HEALTH_H
#define DRD_HEALTH_H

#include <stdbool.h>
#include <stdint.h>


#include "stm32f1xx_hal.h"

extern CAN_HandleTypeDef hcan;

// datasheet typical values
#define VREFINT_MV 1200     // built-in reference voltage
#define V25_MV 1430         // temp sensor voltage at 25 degC
#define MAX_TEMP_C 85       // max mcu temperature
#define MIN_VOLT_MV 2000    // minimum mcu voltage

void McuHealthDriverInit( void );
uint16_t DRD_Health_ReadVddMv( void );
uint16_t DRD_Read_Vdd( void );
int16_t DRD_Read_Temp( void );

#endif