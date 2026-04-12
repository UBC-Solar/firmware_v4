#pragma once

#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_i2c.h"

/**
 * @brief Identifies which read operation is in flight, used to dispatch the RxCplt callback.
 */
typedef enum {
    I2C_IDLE,                 /**< No read in progress. */
    I2C_INA228_SHUNT_VOLTAGE, /**< Reading the INA228 shunt voltage register. */
} I2C_Read_State;

/**
 * @brief Store the HAL I2C handle and initialise the peripheral.
 * @param _hi2c Pointer to the HAL I2C handle to use for all subsequent transfers.
 */
void I2C_Init(I2C_HandleTypeDef *_hi2c);

/**
 * @brief Blocking master transmit — sends data and waits until the transfer completes.
 * @param dev_addr 8-bit I2C device address (7-bit address shifted left by one).
 * @param data Pointer to the byte buffer to transmit.
 * @param size Number of bytes to transmit.
 * @retval HAL_OK Transfer completed successfully.
 * @retval HAL_ERROR HAL reported a peripheral error.
 * @retval other Current HAL_I2C_StateTypeDef value if the bus was not ready.
 */
HAL_StatusTypeDef I2C_MasterTransmit(uint16_t dev_addr, uint8_t *data, uint16_t size);

/**
 * @brief Interrupt-driven master transmit — starts the transfer and returns immediately.
 * @param dev_addr 8-bit I2C device address (7-bit address shifted left by one).
 * @param data Pointer to the byte buffer to transmit.
 * @param size Number of bytes to transmit.
 * @retval HAL_OK Transfer started successfully.
 * @retval HAL_ERROR HAL reported a peripheral error.
 * @retval other Current HAL_I2C_StateTypeDef value if the bus was not ready.
 */
HAL_StatusTypeDef I2C_MasterTransmit_IT(uint16_t dev_addr, uint8_t *data, uint16_t size);

/**
 * @brief Blocking master receive — reads data and waits until the transfer completes.
 * @param dev_addr 8-bit I2C device address (7-bit address shifted left by one).
 * @param data Pointer to the buffer to store received bytes.
 * @param size Number of bytes to receive.
 * @retval HAL_OK Transfer completed successfully.
 * @retval HAL_ERROR HAL reported a peripheral error.
 * @retval other Current HAL_I2C_StateTypeDef value if the bus was not ready.
 */
HAL_StatusTypeDef I2C_MasterReceive(uint16_t dev_addr, uint8_t *data, uint16_t size);

/**
 * @brief Interrupt-driven master receive — starts the read and returns immediately.
 * @param dev_addr 8-bit I2C device address (7-bit address shifted left by one).
 * @param data Pointer to the buffer to store received bytes.
 * @param size Number of bytes to receive.
 * @retval HAL_OK Transfer started successfully.
 * @retval HAL_ERROR HAL reported a peripheral error.
 * @retval other Current HAL_I2C_StateTypeDef value if the bus was not ready.
 */
HAL_StatusTypeDef I2C_MasterReceive_IT(uint16_t dev_addr, uint8_t *data, uint16_t size);

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
HAL_StatusTypeDef I2C_MemWrite(uint16_t dev_addr, uint16_t mem_addr, uint8_t *data, uint16_t size);

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
HAL_StatusTypeDef I2C_MemWrite_IT(uint16_t dev_addr, uint16_t mem_addr, uint8_t *data, uint16_t size);

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
HAL_StatusTypeDef I2C_MemRead(uint16_t dev_addr, uint16_t mem_addr, uint8_t *data, uint16_t size, I2C_Read_State state);

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
HAL_StatusTypeDef I2C_MemRead_IT(uint16_t dev_addr, uint16_t mem_addr, uint8_t *data, uint16_t size, I2C_Read_State state);

/**
 * @brief Return the current HAL I2C peripheral state.
 * @return Current state of the I2C peripheral (e.g. HAL_I2C_STATE_READY, HAL_I2C_STATE_BUSY).
 */
HAL_I2C_StateTypeDef I2C_GetState(void);

/**
 * @brief Return the tag that identifies the read operation currently in flight.
 * @return Current I2C_Read_State value, or I2C_IDLE if no read is in progress.
 */
I2C_Read_State I2C_GetReadState(void);
