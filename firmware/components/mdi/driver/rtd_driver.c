/*
 * rtd.c
 *
 *  Created on: Nov 1, 2025
 *      Author: Luke Santosham & Martin Wu
 */

#include "rtd_driver.h"

#define COEFF_OF_RESISTANCE_PLAT 0.00385
#define RESISTANCE_AT_0C 1000
#define REFERENCE_RESISTANCE 3987
#define TIMEOUT_DELAY 100
#define RTD_FAULT_BIT 0x01
#define RTD_ADC_FULL_SCALE 32768.0f

/* Number of consecutive faulted reads tolerated before reporting a hard fault.
 * The over/under-voltage fault (status 0x04) is frequently a transient on noisy
 * 3-wire harnesses; we hold the last good temperature across short bursts and
 * only escalate to RtdStatusFault if the fault persists. With the 1s diagnostic
 * cadence this is ~RTD_FAULT_DEBOUNCE_COUNT seconds of continuous fault. */
#define RTD_FAULT_DEBOUNCE_COUNT 5

// Register Addresses
#define CONFIG_REG_R 0x00
#define RTD_MSB_REG_R 0x01
#define RTD_LSB_REG_R 0x02
#define FAULT_STATUS_REG_R 0x07
#define CONFIG_REG_W 0x80

// Config Register Bits
#define CONFIG_VBIAS 0x80    // V_BIAS enabled
#define CONFIG_AUTO 0x40     // Auto conversion mode
#define CONFIG_1SHOT 0x20    // 1-shot conversion
#define CONFIG_3WIRE 0x10    // 3-wire RTD
#define CONFIG_FAULTCLR 0x02 // Fault status auto-clear (D3:D2 must be 0)
#define CONFIG_FAULTCYC 0x00 // No fault cycle
#define CONFIG_FILT50HZ 0x00 // 60Hz filter

// PRIVATE FUNCTION PROTOTYPES
static bool RtdWriteRegister(uint8_t address_with_write_bit, uint8_t data);
static bool RtdReadRegister(uint8_t address_read, uint8_t* data);
static RtdStatus RtdReadResistance(uint16_t* buffer);
static void RtdResistanceToTemp(uint16_t buffer, int32_t* temp);
static void RtdClearFault(void);

extern SPI_HandleTypeDef hspi1;

/* Base configuration (VBIAS | AUTO | 3WIRE | 60Hz filter); kept so faults can
 * be cleared without disturbing the running conversion settings. */
static uint8_t s_rtd_config = CONFIG_VBIAS | CONFIG_AUTO | CONFIG_3WIRE | CONFIG_FILT50HZ;

/* Transient-fault debounce state. */
static int32_t s_last_good_temp = 0;
static bool s_has_last_good = false;
static uint32_t s_consecutive_faults = 0;

// PUBLIC FUNCTIONS
RtdStatus RtdDriverGetTemp(int32_t* temperature)
{
    uint16_t buffer = 0;
    RtdStatus status;

    status = RtdReadResistance(&buffer);
    if (status != RtdStatusOk)
    {
        return status;
    }

    if (buffer & RTD_FAULT_BIT)
    {
        /* The MAX31865 fault bit (D0) is the OR of the fault status register
         * and is LATCHED. Clear the latched fault so a transient does not
         * stick forever. */
        RtdClearFault();

        s_consecutive_faults++;

        /* Ride through short fault bursts (commonly transient over/under-voltage,
         * status 0x04) by reporting the last good temperature. */
        if (s_has_last_good && s_consecutive_faults < RTD_FAULT_DEBOUNCE_COUNT)
        {
            *temperature = s_last_good_temp;
            return RtdStatusOk;
        }

        return RtdStatusFault;
    }

    RtdResistanceToTemp(buffer, temperature);

    s_last_good_temp = *temperature;
    s_has_last_good = true;
    s_consecutive_faults = 0;

    return RtdStatusOk;
}

void RtdDriverInit(void)
{
    /* Compose config: VBIAS | AUTO | 3WIRE | filter 60Hz (CONFIG_FILT50HZ=0) */
    s_rtd_config = CONFIG_VBIAS | CONFIG_AUTO | CONFIG_3WIRE | CONFIG_FILT50HZ;
    RtdWriteRegister(CONFIG_REG_W, s_rtd_config);

    /* Clear any fault latched during power-up / VBIAS settling. */
    RtdClearFault();
}

// PRIVATE FUNCTIONS
/*
 * @brief:      Writes a single byte to an RTD register over SPI.
 * @param[in]:  address; the 7-bit register address to write to. The MSB is
 *              automatically OR’d with 0x80 to enable write mode.
 * @param[in]:  data; the 8-bit value to write to the selected register.
 * @returns:    true if an SPI error occurred, false on success.
 */
static bool RtdWriteRegister(uint8_t address_with_write_bit, uint8_t data)
{
    uint8_t tx[2];
    uint8_t rx[2];
    bool hal_err = false;

    tx[0] = address_with_write_bit; /* Has write bit (0x80) already OR'd by caller */
    tx[1] = data;

    HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin, GPIO_PIN_RESET);
    if (HAL_SPI_TransmitReceive(&hspi1, tx, rx, 2, TIMEOUT_DELAY) != HAL_OK)
    {
        hal_err = true;
    }
    HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin, GPIO_PIN_SET);

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
static bool RtdReadRegister(uint8_t address_read, uint8_t* data)
{
    uint8_t tx[2];
    uint8_t rx[2];
    bool hal_err = false;

    /* Ensure MSB cleared for read */
    tx[0] = (address_read & 0x7F);
    tx[1] = 0x00;

    HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin, GPIO_PIN_RESET);
    if (HAL_SPI_TransmitReceive(&hspi1, tx, rx, 2, TIMEOUT_DELAY) != HAL_OK)
    {
        hal_err = true;
    }
    else
    {
        *data = rx[1]; /* rx[0] is junk (MISO during address byte), rx[1] is register value */
    }
    HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin, GPIO_PIN_SET);

    return hal_err;
}

static RtdStatus RtdReadResistance(uint16_t* buffer)
{
    uint8_t msb = 0, lsb = 0;
    bool hal_err;

    hal_err = RtdReadRegister(RTD_MSB_REG_R, &msb);
    if (hal_err)
    {
        return RtdStatusHalError;
    }

    hal_err = RtdReadRegister(RTD_LSB_REG_R, &lsb);
    if (hal_err)
    {
        return RtdStatusHalError;
    }

    *buffer = ((uint16_t)msb << 8) | lsb;

    return RtdStatusOk;
}

/*
 * @brief: Clears the latched fault status by pulsing the fault-clear bit (D1)
 *         in the configuration register. D3:D2 must be 0 for the auto-clear,
 *         which the base config satisfies. The bit self-resets after the clear.
 */
static void RtdClearFault(void)
{
    RtdWriteRegister(CONFIG_REG_W, s_rtd_config | CONFIG_FAULTCLR);
}

static void RtdResistanceToTemp(uint16_t buffer, int32_t* temp)
{
    float resistance;
    float temperature;
    /* MAX31865 stores RTD data in bits 15:1; bit0 is the fault bit.
     * Convert the 16-bit register pair into the 15-bit RTD ADC code
     * by shifting right 1 before scaling.
     */
    uint16_t raw15 = (uint16_t)(buffer >> 1);

    resistance = ((float)raw15) / RTD_ADC_FULL_SCALE * (float)REFERENCE_RESISTANCE;

    temperature = (resistance - RESISTANCE_AT_0C) / (COEFF_OF_RESISTANCE_PLAT * RESISTANCE_AT_0C);
    *temp = (int32_t)temperature;
}
