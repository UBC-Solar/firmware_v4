#ifndef BMP390_H
#define BMP390_H

#include "stm32f1xx_hal.h"
#include <stdint.h>
#include "stm32f1xx_hal_i2c.h"

typedef struct {
    I2C_HandleTypeDef *hi2c;
    uint8_t addr;
} BMP390_t;

HAL_StatusTypeDef BMP390_Init(void);
HAL_StatusTypeDef BMP390_Read(float *temp_c, float *press_pa);
float BMP390_ReadAltitude(float pressure_pa);


#endif