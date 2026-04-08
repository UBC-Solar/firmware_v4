#pragma once

#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_i2c.h"

void I2C_Init(I2C_HandleTypeDef *_hi2c);

HAL_StatusTypeDef I2C_MasterTransmit_IT(uint16_t dev_addr, uint8_t *data, uint16_t size);
HAL_StatusTypeDef I2C_MasterReceive_IT(uint16_t dev_addr, uint8_t *data, uint16_t size);
HAL_StatusTypeDef I2C_MemWrite_IT(uint16_t dev_addr, uint16_t mem_addr, uint8_t *data, uint16_t size);
HAL_StatusTypeDef I2C_MemRead_IT(uint16_t dev_addr, uint16_t mem_addr, uint8_t *data, uint16_t size);

HAL_I2C_StateTypeDef I2C_GetState(void);

// enum defines what we're reading
typedef enum {
    I2C_IDLE,
    I2C_INA228_VOLTAGE
} I2C_State;