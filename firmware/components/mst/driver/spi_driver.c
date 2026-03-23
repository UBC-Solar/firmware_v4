/**
 * Note: this SPI driver is specifically designed to interface and communicate 
 * with the 2 ADBMS1818 chips on the V4 slaveboards.
 * 
 * Please refer to the ADBMS1818 datasheet for more details on the protocol we implement here:
 * https://www.analog.com/media/en/technical-documentation/data-sheets/adbms1818.pdf
 */

#include "spi_driver.h"

Slave_Data_t slave_controller;

/**
 * Lookup table for PEC (Packet Error Code) CRC (Cyclic Redundancy Check) calculation
 */
static const uint16_t pec15Table[256] =
{
    0x0000, 0xC599, 0xCEAB, 0x0B32, 0xD8CF, 0x1D56, 0x1664, 0xD3FD, 0xF407,
    0x319E, 0x3AAC, 0xFF35, 0x2CC8, 0xE951, 0xE263, 0x27FA, 0xAD97, 0x680E,
    0x633C, 0xA6A5, 0x7558, 0xB0C1, 0xBBF3, 0x7E6A, 0x5990, 0x9C09, 0x973B,
    0x52A2, 0x815F, 0x44C6, 0x4FF4, 0x8A6D, 0x5B2E, 0x9EB7, 0x9585, 0x501C,
    0x83E1, 0x4678, 0x4D4A, 0x88D3, 0xAF29, 0x6AB0, 0x6182, 0xA41B, 0x77E6,
    0xB27F, 0xB94D, 0x7CD4, 0xF6B9, 0x3320, 0x3812, 0xFD8B, 0x2E76, 0xEBEF,
    0xE0DD, 0x2544, 0x02BE, 0xC727, 0xCC15, 0x098C, 0xDA71, 0x1FE8, 0x14DA,
    0xD143, 0xF3C5, 0x365C, 0x3D6E, 0xF8F7, 0x2B0A, 0xEE93, 0xE5A1, 0x2038,
    0x07C2, 0xC25B, 0xC969, 0x0CF0, 0xDF0D, 0x1A94, 0x11A6, 0xD43F, 0x5E52,
    0x9BCB, 0x90F9, 0x5560, 0x869D, 0x4304, 0x4836, 0x8DAF, 0xAA55, 0x6FCC,
    0x64FE, 0xA167, 0x729A, 0xB703, 0xBC31, 0x79A8, 0xA8EB, 0x6D72, 0x6640,
    0xA3D9, 0x7024, 0xB5BD, 0xBE8F, 0x7B16, 0x5CEC, 0x9975, 0x9247, 0x57DE,
    0x8423, 0x41BA, 0x4A88, 0x8F11, 0x057C, 0xC0E5, 0xCBD7, 0x0E4E, 0xDDB3,
    0x182A, 0x1318, 0xD681, 0xF17B, 0x34E2, 0x3FD0, 0xFA49, 0x29B4, 0xEC2D,
    0xE71F, 0x2286, 0xA213, 0x678A, 0x6CB8, 0xA921, 0x7ADC, 0xBF45, 0xB477,
    0x71EE, 0x5614, 0x938D, 0x98BF, 0x5D26, 0x8EDB, 0x4B42, 0x4070, 0x85E9,
    0x0F84, 0xCA1D, 0xC12F, 0x04B6, 0xD74B, 0x12D2, 0x19E0, 0xDC79, 0xFB83,
    0x3E1A, 0x3528, 0xF0B1, 0x234C, 0xE6D5, 0xEDE7, 0x287E, 0xF93D, 0x3CA4,
    0x3796, 0xF20F, 0x21F2, 0xE46B, 0xEF59, 0x2AC0, 0x0D3A, 0xC8A3, 0xC391,
    0x0608, 0xD5F5, 0x106C, 0x1B5E, 0xDEC7, 0x54AA, 0x9133, 0x9A01, 0x5F98,
    0x8C65, 0x49FC, 0x42CE, 0x8757, 0xA0AD, 0x6534, 0x6E06, 0xAB9F, 0x7862,
    0xBDFB, 0xB6C9, 0x7350, 0x51D6, 0x944F, 0x9F7D, 0x5AE4, 0x8919, 0x4C80,
    0x47B2, 0x822B, 0xA5D1, 0x6048, 0x6B7A, 0xAEE3, 0x7D1E, 0xB887, 0xB3B5,
    0x762C, 0xFC41, 0x39D8, 0x32EA, 0xF773, 0x248E, 0xE117, 0xEA25, 0x2FBC,
    0x0846, 0xCDDF, 0xC6ED, 0x0374, 0xD089, 0x1510, 0x1E22, 0xDBBB, 0x0AF8,
    0xCF61, 0xC453, 0x01CA, 0xD237, 0x17AE, 0x1C9C, 0xD905, 0xFEFF, 0x3B66,
    0x3054, 0xF5CD, 0x2630, 0xE3A9, 0xE89B, 0x2D02, 0xA76F, 0x62F6, 0x69C4,
    0xAC5D, 0x7FA0, 0xBA39, 0xB10B, 0x7492, 0x5368, 0x96F1, 0x9DC3, 0x585A,
    0x8BA7, 0x4E3E, 0x450C, 0x8095
};

/**
 * @brief Calculates the PEC for "len" bytes of data (as a group).
 * Function adapted from code on pg. 76 of LTC6811 Datasheet.
 *
 * @param data The bytes to calculate a PEC for
 * @param len The number of bytes of data to calculate the PEC for
 * @return Returns the 2-byte CRC PEC generated
 */
uint16_t calculatePec15(uint8_t *data, uint32_t len)
{
	uint16_t remainder, address;
	remainder = 16; // initial value for PEC computation
	for (int i = 0; i < len; i++)
	{
		address = ((remainder >> 7) ^ data[i]) & 0xff; // lookup table address
		remainder = (remainder << 8) ^ pec15Table[address];
	}
	return (remainder << 1); // The CRC15 has a 0 in the LSB so the final value
							 // must be leftshifted 1 bit
}

/**
 * @brief sends a 2-byte command followed by the 2-byte PEC for that command
 * over SPI.
 * @attention The caller must ensure the CS line is pulled low prior to calling
 * this function.
 *
 * @param command The 2-byte command to send
 */
void sendCommand(Slave_Command_t command)
{
	uint16_t pecValue;
	uint8_t tx_message[4];

	tx_message[0] = (uint8_t) (command >> 8);
	tx_message[1] = (uint8_t) command;
	pecValue = calculatePec15(tx_message, 2);
	tx_message[2] = (uint8_t) (pecValue >> 8);
	tx_message[3] = (uint8_t) pecValue;

	// size parameter is number of bytes to transmit - here it's 4 8bit frames
    HAL_SPI_Transmit(slave_controller.SPI_handle, tx_message, 4, SLAVE_TIMEOUT_VAL);
}

/**
 * @brief Toggles the SPI Chip Select (CS) pin
 *
 * @param cs_state The state (CS_HIGH or CS_LOW) to write to the CS pin
 */
void writeCS(CS_state_t cs_state)
{
    HAL_GPIO_WritePin(SPI_ADBMS_NSS_GPIO_Port, SPI_ADBMS_NSS_Pin, cs_state);
}

// Helper function to translate a HAL error into a Slave error
Slave_Status_t processHALStatus(HAL_StatusTypeDef status_HAL, unsigned int device_num)
{
    Slave_Status_t status_Slave;
    status_Slave.error = Slave_OK;
    status_Slave.device_num = 0;

    if (status_HAL != HAL_OK) {
        status_Slave.error = status_HAL + Slave_HAL_ERROR_OFFSET;
        status_Slave.device_num = device_num;
    }

    return status_Slave;
}


/*============================================================================*/
/* PUBLIC FUNCTION DEFINITIONS */

/**
 * @brief Toggles the CS line to wake up the entire chain of ADBMS1818 devices.
 *
 * Wakes up the daisy chain as per method described on pg. 52 of datasheet
 * (method 2)
 */
void Slave_wakeup(void)
{
    // Using HAL_Delay() for this is not particularly ideal, since the
    // minimum delay is 1ms and the delays required are 300us and
    // 10us -ish (shorter than 1 ms)
    // If it doesn't, add another faster timer for more precise delays
	writeCS(CS_HIGH);
	HAL_Delay(1); // wait 1ms
    for (int i = 0; i < SLAVE_NUM_DEVICES; i++)
	{
		writeCS(CS_LOW);
		HAL_Delay(1); // wait 1ms
		writeCS(CS_HIGH);
		// Then delay at least 10us
		HAL_Delay(1); // wait 1ms - the minimum with this timer setup
	}
}

/**
 * @brief Initializes the ADBMS1818 devices and driver data
 *
 * @param SPI_handle HAL SPI handle for the SPI peripheral used for communication to battery monitoring hardware
 */
void Slave_init(SPI_HandleTypeDef *SPI_handle)
{
    // Refer to the ADBMS1818 datasheet pages 60 and 65 for format and content of config_val_a
    uint8_t config_val_a[SLAVE_REG_GROUP_SIZE] =
    {
        0xF8 | (REFON << 2) | ADCOPT, // GPIO 1-5 pull-downs off, REFON, ADCOPT
        (VUV & 0xFF), // VUV[7:0]
		((uint8_t) (VOV << 4)) | (((uint8_t) (VUV >> 8)) & 0x0F), // VOV[4:0] | VUV[11:8]
        (VOV >> 4), // VOV[11:4]
		0x00, // Discharge off for cells 1 through 8
        0x00, // Discharge off for cells 9 through 12, Discharge timer disabled
    };
	uint8_t config_val_b[SLAVE_REG_GROUP_SIZE] =
    {
        0x0F, // Discharge off for cells 13 through 16, GPIO 6-9 = 1
        0x00, // FDRF = 0, PS = 0, Discharge off for cells 17 and 18
        0x00,
        0x00,
        0x00,
        0x00
    };

        slave_controller.SPI_handle = SPI_handle;

        for(int ic_num = 0; ic_num < SLAVE_NUM_DEVICES; ic_num++)
    {
		for(int reg_num = 0; reg_num < SLAVE_REG_GROUP_SIZE; reg_num++)
        {
			slave_controller.cfgra[ic_num][reg_num] = config_val_a[reg_num];
			slave_controller.cfgrb[ic_num][reg_num] = config_val_b[reg_num];
        }
    }

	HAL_Delay(2250); // Let the ADBMS1818 watchdog time out (max 2.2sec) to start IC config from a clean slate
        Slave_wakeup(); // Wake up all ADBMS1818 devices in the chain
        Slave_writeRegisterGroup(CMD_WRCFGA, slave_controller.cfgra); // Write to Config. Reg. Group A
        Slave_writeRegisterGroup(CMD_WRCFGB, slave_controller.cfgrb); // Write to Config. Reg. Group B
}


/**
 * @brief sends a 2-byte command followed by the 2-byte PEC for that command
 * over SPI.
 *
 * @param command The 2-byte command to send
 */
void Slave_sendCmd(Slave_Command_t command)
{
    writeCS(CS_LOW);
    sendCommand(command);
    writeCS(CS_HIGH);
}

/**
 * @brief sends a polling-type command (eg. ADCV) and then polls the ADBMS1818
 * This function is blocking. It will wait for the ADBMS1818 to signal it is
 * finished; however, there is a timeout feature in case something goes wrong.
 * The timeout threshold is SLAVE_TIMEOUT_VAL.
 *
 * @param command The 2-byte (polling) command to send
 * @return 	Returns Slave_OK once ADBMS1818 devices have completed their conversions,
 *          or Slave_ERROR_TIMEOUT upon timeout.
 */
Slave_Status_t Slave_sendCmdAndPoll(Slave_Command_t command)
{
    uint8_t rx_buffer = 0;
	uint32_t start_tick;
	HAL_StatusTypeDef status_HAL = HAL_OK;
    Slave_Status_t status_Slave = {Slave_OK, 0};

	writeCS(CS_LOW);

	sendCommand(command);

	start_tick = HAL_GetTick(); // Start timeout timer

	// Poll for conversion completion; see "Polling Methods" in datasheet pg. 55-57)
	// Make sure MISO goes low...
	do
	{
        if (HAL_GetTick() - start_tick > SLAVE_TIMEOUT_VAL)
		{
		    writeCS(CS_HIGH);
            status_Slave.error = Slave_ERROR_TIMEOUT; // ADBMS1818 didn't respond before timeout
            return status_Slave;
		}

		rx_buffer = 0;
        status_HAL = HAL_SPI_Receive(slave_controller.SPI_handle, &rx_buffer, 1, SLAVE_TIMEOUT_VAL);
        status_Slave = processHALStatus(status_HAL, 0);
        if (status_Slave.error != Slave_OK) return status_Slave;

	} while (rx_buffer == 0xFF);

    // ... then wait for MISO to go high;
    // this signifies that the ADBMS1818 devices are done reading their ADCs.

    // Must send at least SLAVE_NUM_DEVICES clock pulses before response is valid
	// That's why there's an extra read initially - it's slight overkill but that's ok
	rx_buffer = 0;
    status_HAL = HAL_SPI_Receive(slave_controller.SPI_handle, &rx_buffer, 1, SLAVE_TIMEOUT_VAL);
    status_Slave = processHALStatus(status_HAL, 0);
    if (status_Slave.error != Slave_OK) {
        return status_Slave;
    }
	do
	{
	    if (HAL_GetTick() - start_tick > SLAVE_TIMEOUT_VAL)
        {
            writeCS(CS_HIGH);
            status_Slave.error = Slave_ERROR_TIMEOUT; // ADBMS1818 didn't respond before timeout
            return status_Slave;
        }

		rx_buffer = 0;
		status_HAL = HAL_SPI_Receive(slave_controller.SPI_handle, &rx_buffer, 1, SLAVE_TIMEOUT_VAL);
		status_Slave = processHALStatus(status_HAL, 0);
		if (status_Slave.error != Slave_OK)
            return status_Slave;

	} while (rx_buffer == 0);

	writeCS(CS_HIGH);

    return status_Slave;
}


/**
 * @brief Writes the 6 bytes of a configuration register group in the ADBMS1818.
 *
 * @param command A write command to specify which register group to write.
 *                Write commands start with "WR".
 * @param tx_data Pointer to a 2-dimensional array of size
 *                SLAVE_NUM_DEVICES x SLAVE_REG_GROUP_SIZE containing the data to write.
 */
void Slave_writeRegisterGroup(Slave_Command_t command, uint8_t tx_data[SLAVE_NUM_DEVICES][SLAVE_REG_GROUP_SIZE])
{
	uint16_t pecValue = 0;
	uint8_t tx_message[8];

	writeCS(CS_LOW);
	sendCommand(command);

	for (int i = 0; i < SLAVE_NUM_DEVICES; i++)
	{
		for (int j = 0; j < SLAVE_REG_GROUP_SIZE; j++)
		{
			// ADBMS1818 register group writes' data are ordered with data for the last device in the chain first.
			// This is the opposite of a register group read's device ordering
			tx_message[j] = tx_data[(SLAVE_NUM_DEVICES - 1) - i][j];
		}
		pecValue = calculatePec15(tx_message, SLAVE_REG_GROUP_SIZE);
		tx_message[6] = (uint8_t) (pecValue >> 8);
		tx_message[7] = (uint8_t) pecValue;
		HAL_SPI_Transmit(slave_controller.SPI_handle, tx_message, 8, SLAVE_TIMEOUT_VAL);
	}

	writeCS(CS_HIGH);
}

/**
 * @brief Reads the 6 bytes of a configuration register group in the ADBMS1818.
 *
 * The data received will only be written to rx_data if the PEC matches.
 *
 * @param command A read command to specify which register group to read.
 *                Read commands start with "RD".
 * @param rx_data Pointer to a 2-dimensional array of size
 *                SLAVE_NUM_DEVICES x SLAVE_REG_GROUP_SIZE to copy received data to.
 * @return Returns Slave_OK if the received PEC is valid, or Slave_ERROR_PEC if
 *         a full set of valid data could not be obtained after
 *         SLAVE_MAX_READ_ATTEMPTS tries.
 */
Slave_Status_t Slave_readRegisterGroup(Slave_Command_t command, uint8_t rx_data[SLAVE_NUM_DEVICES][SLAVE_REG_GROUP_SIZE])
{
	uint16_t pecValue = 0;
	Slave_Status_t status = {Slave_OK, 0};
	HAL_StatusTypeDef status_HAL = HAL_OK;
	// Initialize rx_message before using it, or the garbage it contains will be
	// sent as dummy data - see definition of HAL_SPI_Receive
	uint8_t rx_message[8] = {0};
	int ic_num = 0;
	int error_counter = 0;

	// Try a maximum of SLAVE_MAX_READ_ATTEMPTS times to read register group
	do
	{
		// Send command to read register group
		writeCS(CS_LOW);
		sendCommand(command);

		// Read back the data, but stop between device data groups on error
		// This will indicate to caller which ADBMS1818 is having problems, if problems are encountered
		ic_num = 0;
		// reset status before a new try
		status.error = Slave_OK;
		status.device_num = 0;
		while ((ic_num < SLAVE_NUM_DEVICES) && (status.error == Slave_OK))
		{
			// 6 data bytes + 2 PEC bytes = 8 bytes
		    status_HAL = HAL_SPI_Receive(slave_controller.SPI_handle, rx_message, 8, SLAVE_TIMEOUT_VAL);
		    status = processHALStatus(status_HAL, ic_num + 1);
		    if (status.error != Slave_OK) return status;

			pecValue = calculatePec15(rx_message, 8); // 0 if transfer was clean
			if (pecValue)
			{
				status.error = Slave_ERROR_PEC;
				status.device_num = ic_num + 1;
				for(int i = 0; i < 8; i++)
				{
					rx_message[i] = 0; // Clear buffer for next try
				}
			}
			else
			{
				for (int j = 0; j < 8; j++)
				{
					if (j < SLAVE_REG_GROUP_SIZE)
					{
						rx_data[ic_num][j] = rx_message[j]; // Copy the data (no PEC)
					}
					rx_message[j] = 0; // Clear rx_message for next loop
				}
			}
			ic_num++;
		}

		writeCS(CS_HIGH);
		error_counter++;
	} while ((status.error != Slave_OK) && (error_counter < SLAVE_MAX_READ_ATTEMPTS));

	return status;
}