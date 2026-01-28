/*
 * rtd.c
 *
 *  Created on: Nov 1, 2025
 *      Author: Luke Santosham & Martin Wu
 */

#include "rtd.h"

#define COEFF_OF_RESISTANCE_PLAT 0.00385
#define RESISTANCE_AT_0C 1000
#define REFERENCE_RESISTANCE 4300
#define TIMEOUT_DELAY 100
#define INIT_FAULT_READ 0x84
#define MAX_ATTEMPTS 400
// 15 BIT NUMBERS, discard MSB
#define MAX_FAULT_THRESHOLD 0xFFFF
#define MIN_FAULT_THRESHOLD 0x0000

// Register Addresses
#define CONFIG_REG_R 0x00
#define RTD_MSB_REG_R 0x01
#define RTD_LSB_REG_R 0x02
#define MAX_FAULT_THRESHOLD_MSB_R 0x03
#define MAX_FAULT_THRESHOLD_LSB_R 0x04
#define MIN_FAULT_THRESHOLD_MSB_R 0x05
#define MIN_FAULT_THRESHOLD_LSB_R 0x06
#define FAULT_STATUS_R 0x07
#define CONFIG_REG_W 0x80
#define MAX_FAULT_THRESHOLD_MSB_W 0x83
#define MAX_FAULT_THRESHOLD_LSB_W 0x84
#define MIN_FAULT_THRESHOLD_MSB_W 0x85
#define MIN_FAULT_THRESHOLD_LSB_W 0x86

// Config Register Bits
#define CONFIG_VBIAS 0x80    // V_BIAS enabled
#define CONFIG_AUTO 0x40     // Auto conversion mode
#define CONFIG_1SHOT 0x20    // 1-shot conversion
#define CONFIG_3WIRE 0x10    // 3-wire RTD
#define CONFIG_FAULTCYC 0x00 // No fault cycle
#define CONFIG_FILT50HZ 0x00 // 60Hz filter

// PRIVATE FUNCTION PROTOTYPES
static bool RTDWriteRegister(uint8_t address_with_write_bit, uint8_t data);
static bool RTDReadRegister(uint8_t address_read, uint8_t* data);
static RtdStatus RTDReadResistance(uint16_t* buffer);
static void RTDResistanceToTemp(uint16_t buffer, uint32_t* temp);

// PUBLIC FUNCTIONS
RtdStatus RtdDriverGetTemp(uint32_t* temperature)
{
    uint16_t buffer = 0;
    RtdStatus status;

    status = RTDReadResistance(&buffer);
    if (status != RtdStatusOk)
    {
        return status;
    }

    if (buffer & 0x01)
    {
        return RtdStatusFault;
    }

    RTDResistanceToTemp(buffer, temperature);

    return RtdStatusOk;
}

void RtdDriverInit(void)
{
    /* Compose config: VBIAS | AUTO | 3WIRE | filter 50Hz (CONFIG_FILT50HZ=0) */
    uint8_t config = CONFIG_VBIAS | CONFIG_AUTO | CONFIG_3WIRE | CONFIG_FILT50HZ;
    RTDWriteRegister(CONFIG_REG_W, config);
}

// PRIVATE FUNCTIONS
/*
 * @brief:      Writes a single byte to an RTD register over SPI.
 * @param[in]:  address; the 7-bit register address to write to. The MSB is
 *              automatically OR’d with 0x80 to enable write mode.
 * @param[in]:  data; the 8-bit value to write to the selected register.
 * @returns:    true if an SPI error occurred, false on success.
 */
static bool RTDWriteRegister(uint8_t address_with_write_bit, uint8_t data)
{
    uint8_t tx[2];
    uint8_t rx[2];
    bool hal_err = false;

    tx[0] = address_with_write_bit; /* Has write bit (0x80) already OR'd by caller */
    tx[1] = data;

    HAL_GPIO_WritePin(SPI_CS_GPIO_Port, SPI_CS_Pin, GPIO_PIN_RESET);
    if (HAL_SPI_TransmitReceive(&hspi1, tx, rx, 2, TIMEOUT_DELAY) != HAL_OK)
    {
        hal_err = true;
    }
    HAL_GPIO_WritePin(SPI_CS_GPIO_Port, SPI_CS_Pin, GPIO_PIN_SET);

    return hal_err;
}
/*
 * @brief:      Reads a single byte from an RTD register over SPI.
 * @param[in]:  address; the 7-bit register address to read from. The MSB is cleared
 *              to ensure read mode.
 * @param[out]: data; a pointer to store the received register value.
 * @returns:    true if an SPI error occurred during transmit or receive,
 *              false on success.
 */
static bool RTDReadRegister(uint8_t address_read, uint8_t* data)
{
    uint8_t tx[2];
    uint8_t rx[2];
    bool hal_err = false;

    /* Ensure MSB cleared for read */
    tx[0] = (address_read & 0x7F);
    tx[1] = 0x00;

    HAL_GPIO_WritePin(SPI_CS_GPIO_Port, SPI_CS_Pin, GPIO_PIN_RESET);
    if (HAL_SPI_TransmitReceive(&hspi1, tx, rx, 2, TIMEOUT_DELAY) != HAL_OK)
    {
        hal_err = true;
    }
    else
    {
        *data = rx[1]; /* rx[0] is junk (MISO during address byte), rx[1] is register value */
    }
    HAL_GPIO_WritePin(SPI_CS_GPIO_Port, SPI_CS_Pin, GPIO_PIN_SET);

    return hal_err;
}
static RtdStatus RTDReadResistance(uint16_t* buffer)
{
    uint8_t msb = 0, lsb = 0;
    bool hal_err;

    hal_err = RTDReadRegister(RTD_MSB_REG_R, &msb);
    if (hal_err)
    {
        return RtdStatusHalError;
    }

    hal_err = RTDReadRegister(RTD_LSB_REG_R, &lsb);
    if (hal_err)
    {
        return RtdStatusHalError;
    }

    *buffer = ((uint16_t)msb << 8) | lsb;

    return RtdStatusOk;
}

static void RTDResistanceToTemp(uint16_t buffer, uint32_t* temp)
{
    uint32_t resistance, temperature;

    resistance = (buffer >> 1);
    resistance = resistance / 32768 * REFERENCE_RESISTANCE;

    temperature =
        (uint32_t)((resistance - RESISTANCE_AT_0C) / (COEFF_OF_RESISTANCE_PLAT * RESISTANCE_AT_0C));
    *temp = temperature;
}
