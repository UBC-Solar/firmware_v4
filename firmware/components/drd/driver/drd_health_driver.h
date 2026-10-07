
#include <stdbool.h>
#include <stdint.h>

#include "stm32f1xx_hal.h"

extern CAN_HandleTypeDef hcan;

static uint16_t ReadADC( uint32_t channel );

