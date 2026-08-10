#pragma once

#include "mst_defs.h"
#include "mst_types.h"

void SelfCheck_Init(ADC_HandleTypeDef *hadc);
Slave_Status_t SelfCheck_Comms(void);
Slave_Status_t SelfCheck_DieTemp(void);
Slave_Status_t SelfCheck_VREF2(void);
Slave_Status_t SelfCheck_OpenWire(void);
Slave_Status_t SelfCheck_OverlapVoltage(void);

