#ifndef DRD_HEALTH_H
#define DRD_HEALTH_H

#include <stdbool.h>
#include <stdint.h>


#include "stm32f1xx_hal.h"

extern CAN_HandleTypeDef hcan;

void DRD_Health_Init( void );
void DRD_Diagnostics_Full(void);
uint16_t DRD_Health_ReadVddMv( void );
uint16_t DRD_Read_Vdd( void );
int16_t DRD_Read_Temp( void );

#endif