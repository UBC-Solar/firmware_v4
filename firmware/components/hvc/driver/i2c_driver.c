#include "i2c_driver.h"

#include <stdbool.h>
#include <stdint.h>

#include "debug_io.h"
#include "stm32f1xx_hal_i2c.h"
#include "uart_driver.h"
#include "ina228_runtime.h"

static I2C_HandleTypeDef *hi2c;

I2C_State current_i2c_state = I2C_IDLE;

void I2C_Init(I2C_HandleTypeDef *_hi2c) {
    hi2c = _hi2c;

    HAL_I2C_Init(hi2c);
}

HAL_StatusTypeDef I2C_MasterTransmit_IT(uint16_t dev_addr, uint8_t *data, uint16_t size) {
    if (HAL_I2C_GetState(hi2c) != HAL_I2C_STATE_READY) {
        DEBUG_IO_print("I2C is busy, cannot start new transmit operation. I2C State: %02X\r\n", (uint8_t)HAL_I2C_GetState(hi2c));
        return HAL_BUSY;
    }

    return HAL_I2C_Master_Transmit_IT(hi2c, dev_addr, data, size);
}

HAL_StatusTypeDef I2C_MasterReceive_IT(uint16_t dev_addr, uint8_t *data, uint16_t size) {
    if (HAL_I2C_GetState(hi2c) != HAL_I2C_STATE_READY) {
        DEBUG_IO_print("I2C is busy, cannot start new receive operation. I2C State: %02X\r\n", (uint8_t)HAL_I2C_GetState(hi2c));
        return HAL_BUSY;
    }

    return HAL_I2C_Master_Receive_IT(hi2c, dev_addr, data, size);
}

HAL_StatusTypeDef I2C_MemWrite(uint16_t dev_addr, uint16_t mem_addr, uint8_t *data, uint16_t size) {
    if (HAL_I2C_GetState(hi2c) != HAL_I2C_STATE_READY) {
        DEBUG_IO_print("I2C is busy, cannot start new MemWrite operation. I2C State: %02X\r\n", (uint8_t)HAL_I2C_GetState(hi2c));
        return HAL_BUSY;
    }

    return HAL_I2C_Mem_Write(hi2c, dev_addr, mem_addr, I2C_MEMADD_SIZE_8BIT, data, size, HAL_MAX_DELAY);
}

HAL_StatusTypeDef I2C_MemWrite_IT(uint16_t dev_addr, uint16_t mem_addr, uint8_t *data, uint16_t size) {
    if (HAL_I2C_GetState(hi2c) != HAL_I2C_STATE_READY) {
        DEBUG_IO_print("I2C is busy, cannot start new MemWrite operation. I2C State: %02X\r\n", (uint8_t)HAL_I2C_GetState(hi2c));
        return HAL_BUSY;
    }

    return HAL_I2C_Mem_Write_IT(hi2c, dev_addr, mem_addr, I2C_MEMADD_SIZE_8BIT, data, size);
}

HAL_StatusTypeDef I2C_MemRead_IT(uint16_t dev_addr, uint16_t mem_addr, uint8_t *data, uint16_t size, I2C_State state) {
    if (HAL_I2C_GetState(hi2c) != HAL_I2C_STATE_READY) {
        DEBUG_IO_print("I2C is busy, cannot start new MemRead operation. I2C State: %02X\r\n", (uint8_t)HAL_I2C_GetState(hi2c));
        return HAL_BUSY;
    }

    current_i2c_state = state;

    return HAL_I2C_Mem_Read_IT(hi2c, dev_addr, mem_addr, I2C_MEMADD_SIZE_8BIT, data, size);
}

HAL_I2C_StateTypeDef I2C_GetState(void) {
    return HAL_I2C_GetState(hi2c);
}

void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *_hi2c) {
    DEBUG_IO_print("HAL_I2C_MemRxCpltCallback triggered. I2C State: %02X\r\n", (uint8_t)HAL_I2C_GetState(_hi2c));
    if (_hi2c == hi2c) {
        DEBUG_IO_print("I2C Mem Read Complete Callback triggered with state: %d, time ms: %lu\r\n", current_i2c_state, (unsigned long)HAL_GetTick());

        switch (current_i2c_state) {
            case I2C_INA228_SHUNT_VOLTAGE:
                INA228_Process_Shunt_Voltage();
                break;
            default:
                break;
        }

        current_i2c_state = I2C_IDLE; // Reset state after processing
    }
}
