#include "i2c_driver.h"

#include <stdbool.h>
#include <stdint.h>

#include "debug_io.h"
#include "stm32f1xx_hal_def.h"
#include "stm32f1xx_hal_i2c.h"
#include "uart_driver.h"
#include "ina228_runtime.h"

static I2C_HandleTypeDef *hi2c;

I2C_Read_State current_i2c_read_state = I2C_IDLE;

/**
 * @brief Store the HAL I2C handle and initialise the peripheral.
 * @param _hi2c Pointer to the HAL I2C handle to use for all subsequent transfers.
 */
void I2C_Init(I2C_HandleTypeDef *_hi2c) {
    hi2c = _hi2c;

    HAL_I2C_Init(hi2c);
}

/**
 * @brief Blocking master transmit — sends data and waits until the transfer completes.
 * @param dev_addr 8-bit I2C device address (7-bit address shifted left by one).
 * @param data Pointer to the byte buffer to transmit.
 * @param size Number of bytes to transmit.
 * @retval HAL_OK Transfer completed successfully.
 * @retval HAL_ERROR HAL reported a peripheral error.
 * @retval other Current HAL_I2C_StateTypeDef value if the bus was not ready.
 */
HAL_StatusTypeDef I2C_MasterTransmit(uint16_t dev_addr, uint8_t *data, uint16_t size) {
    if (HAL_I2C_GetState(hi2c) != HAL_I2C_STATE_READY) {
        DEBUG_IO_print("I2C is not ready, cannot start new transmit operation. I2C State: %02X\n\r", (uint8_t)HAL_I2C_GetState(hi2c));
        return HAL_I2C_GetState(hi2c);
    }

    return HAL_I2C_Master_Transmit(hi2c, dev_addr, data, size, HAL_MAX_DELAY);
}

/**
 * @brief Interrupt-driven master transmit — starts the transfer and returns immediately.
 * @param dev_addr 8-bit I2C device address (7-bit address shifted left by one).
 * @param data Pointer to the byte buffer to transmit.
 * @param size Number of bytes to transmit.
 * @retval HAL_OK Transfer started successfully.
 * @retval HAL_ERROR HAL reported a peripheral error.
 * @retval other Current HAL_I2C_StateTypeDef value if the bus was not ready.
 */
HAL_StatusTypeDef I2C_MasterTransmit_IT(uint16_t dev_addr, uint8_t *data, uint16_t size) {
    if (HAL_I2C_GetState(hi2c) != HAL_I2C_STATE_READY) {
        DEBUG_IO_print("I2C is not ready, cannot start new transmit operation. I2C State: %02X\n\r", (uint8_t)HAL_I2C_GetState(hi2c));
        return HAL_I2C_GetState(hi2c);
    }

    return HAL_I2C_Master_Transmit_IT(hi2c, dev_addr, data, size);
}

/**
 * @brief Blocking master receive — reads data and waits until the transfer completes.
 * @param dev_addr 8-bit I2C device address (7-bit address shifted left by one).
 * @param data Pointer to the buffer to store received bytes.
 * @param size Number of bytes to receive.
 * @retval HAL_OK Transfer completed successfully.
 * @retval HAL_ERROR HAL reported a peripheral error.
 * @retval other Current HAL_I2C_StateTypeDef value if the bus was not ready.
 */
HAL_StatusTypeDef I2C_MasterReceive(uint16_t dev_addr, uint8_t *data, uint16_t size) {
    if (HAL_I2C_GetState(hi2c) != HAL_I2C_STATE_READY) {
        DEBUG_IO_print("I2C is not ready, cannot start new receive operation. I2C State: %02X\n\r", (uint8_t)HAL_I2C_GetState(hi2c));
        return HAL_I2C_GetState(hi2c);
    }

    return HAL_I2C_Master_Receive(hi2c, dev_addr, data, size, HAL_MAX_DELAY);
}

/**
 * @brief Interrupt-driven master receive — starts the read and returns immediately.
 * @param dev_addr 8-bit I2C device address (7-bit address shifted left by one).
 * @param data Pointer to the buffer to store received bytes.
 * @param size Number of bytes to receive.
 * @retval HAL_OK Transfer started successfully.
 * @retval HAL_ERROR HAL reported a peripheral error.
 * @retval other Current HAL_I2C_StateTypeDef value if the bus was not ready.
 */
HAL_StatusTypeDef I2C_MasterReceive_IT(uint16_t dev_addr, uint8_t *data, uint16_t size) {
    if (HAL_I2C_GetState(hi2c) != HAL_I2C_STATE_READY) {
        DEBUG_IO_print("I2C is not ready, cannot start new receive operation. I2C State: %02X\n\r", (uint8_t)HAL_I2C_GetState(hi2c));
        return HAL_I2C_GetState(hi2c);
    }

    return HAL_I2C_Master_Receive_IT(hi2c, dev_addr, data, size);
}

/**
 * @brief Blocking memory write — writes bytes to a device register and waits until done.
 * @param dev_addr 8-bit I2C device address (7-bit address shifted left by one).
 * @param mem_addr Register address on the target device.
 * @param data Pointer to the byte buffer to write.
 * @param size Number of bytes to write.
 * @retval HAL_OK Write completed successfully.
 * @retval HAL_ERROR HAL reported a peripheral error.
 * @retval other Current HAL_I2C_StateTypeDef value if the bus was not ready.
 */
HAL_StatusTypeDef I2C_MemWrite(uint16_t dev_addr, uint16_t mem_addr, uint8_t *data, uint16_t size) {
    if (HAL_I2C_GetState(hi2c) != HAL_I2C_STATE_READY) {
        DEBUG_IO_print("I2C is not ready, cannot start new MemWrite operation. I2C State: %02X\n\r", (uint8_t)HAL_I2C_GetState(hi2c));
        return HAL_I2C_GetState(hi2c);
    }

    return HAL_I2C_Mem_Write(hi2c, dev_addr, mem_addr, I2C_MEMADD_SIZE_8BIT, data, size, HAL_MAX_DELAY);
}

/**
 * @brief Interrupt-driven memory write — starts the register write and returns immediately.
 * @param dev_addr 8-bit I2C device address (7-bit address shifted left by one).
 * @param mem_addr Register address on the target device.
 * @param data Pointer to the byte buffer to write.
 * @param size Number of bytes to write.
 * @retval HAL_OK Write started successfully.
 * @retval HAL_ERROR HAL reported a peripheral error.
 * @retval other Current HAL_I2C_StateTypeDef value if the bus was not ready.
 */
HAL_StatusTypeDef I2C_MemWrite_IT(uint16_t dev_addr, uint16_t mem_addr, uint8_t *data, uint16_t size) {
    if (HAL_I2C_GetState(hi2c) != HAL_I2C_STATE_READY) {
        DEBUG_IO_print("I2C not ready, cannot start new MemWrite operation. I2C State: %02X\n\r", (uint8_t)HAL_I2C_GetState(hi2c));
        return HAL_I2C_GetState(hi2c);
    }

    return HAL_I2C_Mem_Write_IT(hi2c, dev_addr, mem_addr, I2C_MEMADD_SIZE_8BIT, data, size);
}

/**
 * @brief Blocking memory read — reads bytes from a device register and immediately calls the RxCplt callback.
 * @param dev_addr 8-bit I2C device address (7-bit address shifted left by one).
 * @param mem_addr Register address on the target device.
 * @param data Pointer to the buffer to store received bytes.
 * @param size Number of bytes to read.
 * @param state Tag identifying the read operation, used to dispatch the RxCplt callback.
 * @retval HAL_OK Read completed successfully.
 * @retval HAL_ERROR HAL reported a peripheral error.
 * @retval other Current HAL_I2C_StateTypeDef value if the bus was not ready.
 */
HAL_StatusTypeDef I2C_MemRead(uint16_t dev_addr, uint16_t mem_addr, uint8_t *data, uint16_t size, I2C_Read_State state) {
    if (HAL_I2C_GetState(hi2c) != HAL_I2C_STATE_READY) {
        DEBUG_IO_print("I2C is not ready, cannot start new MemRead operation. I2C State: %02X\n\r", (uint8_t)HAL_I2C_GetState(hi2c));
        return HAL_I2C_GetState(hi2c);
    }

    current_i2c_read_state = state;

    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(hi2c, dev_addr, mem_addr, I2C_MEMADD_SIZE_8BIT, data, size, HAL_MAX_DELAY);

    HAL_I2C_MemRxCpltCallback(hi2c); // Manually call the callback since we're doing a blocking read
    
    return status;
}

/**
 * @brief Interrupt-driven memory read — starts the register read and returns immediately.
 * @param dev_addr 8-bit I2C device address (7-bit address shifted left by one).
 * @param mem_addr Register address on the target device.
 * @param data Pointer to the buffer to store received bytes.
 * @param size Number of bytes to read.
 * @param state Tag identifying the read operation, used to dispatch the RxCplt callback.
 * @retval HAL_OK Read started successfully.
 * @retval HAL_ERROR HAL reported a peripheral error.
 * @retval other Current HAL_I2C_StateTypeDef value if the bus was not ready.
 */
HAL_StatusTypeDef I2C_MemRead_IT(uint16_t dev_addr, uint16_t mem_addr, uint8_t *data, uint16_t size, I2C_Read_State state) {
    if (HAL_I2C_GetState(hi2c) != HAL_I2C_STATE_READY) {
        DEBUG_IO_print("I2C is not ready, cannot start new MemRead operation. I2C State: %02X\n\r", (uint8_t)HAL_I2C_GetState(hi2c));
        return HAL_I2C_GetState(hi2c);
    }

    current_i2c_read_state = state;

    return HAL_I2C_Mem_Read_IT(hi2c, dev_addr, mem_addr, I2C_MEMADD_SIZE_8BIT, data, size);
}

/**
 * @brief Return the current HAL I2C peripheral state.
 * @return Current state of the I2C peripheral (e.g. HAL_I2C_STATE_READY, HAL_I2C_STATE_BUSY).
 */
HAL_I2C_StateTypeDef I2C_GetState(void) {
    return HAL_I2C_GetState(hi2c);
}

/**
 * @brief Return the tag that identifies the read operation currently in flight.
 * @return Current I2C_Read_State value, or I2C_IDLE if no read is in progress.
 */
I2C_Read_State I2C_GetReadState(void) {
    return current_i2c_read_state;
}

/**
 * @brief HAL callback invoked when a memory read transfer completes.
 * @param _hi2c Pointer to the HAL I2C handle that completed the transfer.
 */
void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *_hi2c) {
    if (_hi2c == hi2c) {
        switch (current_i2c_read_state) {
            case I2C_INA228_SHUNT_VOLTAGE:
                INA228_Process_Shunt_Voltage();
                break;
            default:
                break;
        }

        current_i2c_read_state = I2C_IDLE; // Reset state after processing
    }
}

/**
 * @brief HAL callback invoked when an I2C transfer error occurs.
 * @param hi2c Pointer to the HAL I2C handle that encountered the error.
 */
void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c){
    DEBUG_IO_print("I2C Error Callback triggered. I2C State: %02X\n\r", (uint8_t)HAL_I2C_GetState(hi2c));
    current_i2c_read_state = I2C_IDLE; // Reset state on error
}
