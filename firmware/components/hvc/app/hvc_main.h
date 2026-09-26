#pragma once

#include "stm32f1xx_hal.h"

/**
 * Tests. 
 */
//These hold no meaning outside the context of hardware unit tests and integration.
#define RUN 1
#define SKIP 0

// Hardware unit tests (UNIT_...) to verify that the hardware works properly.
// Integration tests (INT_...) to verify functionality with rest of MST.
// A test is either set to RUN or SKIP.
#define UNIT_TEST_SHUNT SKIP
#define INT_TEST_CAN SKIP
// Integration test June 11th, 2026: TEL and MST will not be connected.
// relevant TEL and MST input data must be faked
#define INT_TEST_JUNE_11TH SKIP
#define INT_TEST_PRECHARGE SKIP



void HVC_Init(
    UART_HandleTypeDef *_huart2, 
    ADC_HandleTypeDef *_hadc1, 
    TIM_HandleTypeDef *_htim3, 
    TIM_HandleTypeDef *_htim4, 
    I2C_HandleTypeDef *_hi2c1, 
    CAN_HandleTypeDef *_hcan);
void HVC_Main(void);

#if (UNIT_TEST_SHUNT == RUN)
void HVC_TestShuntCycle();
#endif // (UNIT_TEST_SHUNT == RUN)

#if (INT_TEST_CAN == RUN)
void HVC_TestCanCycle();
#endif // (INT_TEST_CAN == RUN)