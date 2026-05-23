#pragma once
/**
 * This SPI driver is specifically designed to interface and communicate 
 * with the 2 ADBMS1818 chips on the V4 slaveboards.
 * 
 * ADBMS1818 datasheet can be found here:
 * https://www.analog.com/media/en/technical-documentation/data-sheets/adbms1818.pdf
 */


#include <stdint.h>

#include "main.h"
#include "stm32f1xx_hal.h"

#include "mst_defs.h"
#include "mst_types.h"

typedef struct {
    uint8_t cfgra[SLAVE_NUM_DEVICES][SLAVE_REG_SIZE_BYTES]; // Record of Configuration Register Group A for each device

    uint8_t cfgrb[SLAVE_NUM_DEVICES][SLAVE_REG_SIZE_BYTES]; // Record of Configuration Register Group B for each device
} Slave_ConfigRegisters_t;

typedef struct {
    SPI_HandleTypeDef *SPI_handle;
    Slave_ConfigRegisters_t config_registers;

} Slave_Data_t;

enum Slave_Error {
    Slave_OK = 0,
    Slave_ERROR_PEC,
    Slave_ERROR_TIMEOUT,
    Slave_ERROR_SELFTEST,
    Slave_ERROR_HAL,
    Slave_ERROR_HAL_BUSY,
    Slave_ERROR_HAL_TIMEOUT
};

typedef struct {
    enum Slave_Error error;
    unsigned int device_num; // Device at which error occurred, if applicable.
    // 0 = N/A, 1 = first device in chain, 2 = second device...
    // If there is no error (error == Slave_OK), device_num should be 0.
    // The reason for the N/A option is not all operations have a means of
    // differentiating responses of different ADBMS1818 devices in the chain.
} Slave_Status_t;

#define Slave_HAL_ERROR_OFFSET (Slave_ERROR_HAL - HAL_ERROR)

// ADBMS1818 ADC mode options
// First freq applies when ADCOPT == 0, second when ADCOPT == 1
// Descriptions "fast," "normal," 'filtered" apply when ADCOPT == 0
enum Slave_MD_e {
    MD_422HZ_1KHZ  = 0x0,
    MD_27KHZ_14KHZ = 0x1,	// fast
    MD_7KHZ_3KHZ   = 0x2,	// normal
    MD_26HZ_2KHZ   = 0x3	// filtered
};

// ADBMS1818 ADC Cell Measurement Options
enum Slave_CH_e {
    CH_ALL = 0x0,
    CH_1 = 0x1, // Measure Cells 1, 7, 13
    CH_2 = 0x2, // Measure Cells 2, 8, 14
    CH_3 = 0x3, // Measure Cells 3, 9, 15
    CH_4 = 0x4, // Measure Cells 4, 10, 16
    CH_5 = 0x5, // Measure Cells 5, 11, 17
    CH_6 = 0x6  // Measure Cells 6, 12, 18
};

// ADBMS1818 GPIO selection for ADC conversion
enum Slave_CHG_e {
    CHG_ALL     = 0x0,    // GPIO 1 through 5, VREF2 and GPIO 6 through 9
    CHG_GPIO1_6 = 0x1,  // GPIO 1 and 6
    CHG_GPIO2_7 = 0x2,  // GPIO 2 and 7
    CHG_GPIO3_8 = 0x3,  // GPIO 3 and 8
    CHG_GPIO4_9 = 0x4,  // GPIO 4 and 9
    CHG_GPIO5   = 0x5,
    CHG_VREF2   = 0x6
};

// ADBMS1818 Status Group selection
enum Slave_CHST_e {
    CHST_ALL = 0x0, // Measure all 4 parameters below:
    CHST_SC  = 0x1, // Sum of all Cells
    CHST_ITMP= 0x2, // Internal Die Temperature
    CHST_VA  = 0x3, // Analog Power Supply
    CHST_VD  = 0x4  // Digital Power Supply
};

// Pull-Up/Pull-Down Current for Open Wire Conversions
enum Slave_PUP_e {
    PUP_PULLDOWN = 0x0,
    PUP_PULLUP   = 0x1
};

typedef enum {
    CS_LOW  = 0,
    CS_HIGH = 1
} CS_state_t;


typedef enum {
    NCS_HIGH = 0,
    NCS_LOW  = 1
} NCS_state_t;


typedef enum {
    CMD_WRCFGA  = 0x0001,       // Write Configuration Register Group A
    CMD_WRCFGB  = 0x0024,       // Write Configuration Register Group B
    CMD_RDCFGA  = 0x0002,       // Read Configuration Register Group A
    CMD_RDCFGB  = 0x0026,       // Read Configuration Register Group B
    CMD_RDCVA   = 0x0004,       // Read Cell Voltage Register Group A
    CMD_RDCVB   = 0x0006,       // Read Cell Voltage Register Group B
    CMD_RDCVC   = 0x0008,       // Read Cell Voltage Register Group C
    CMD_RDCVD   = 0x000A,       // Read Cell Voltage Register Group D
    CMD_RDCVE   = 0x0009,       // Read Cell Voltage Register Group E
    CMD_RDCVF   = 0x000B,       // Read Cell Voltage Register Group F
    CMD_RDAUXA  = 0x000C,       // Read Auxilliary Register Group A
    CMD_RDAUXB  = 0x000E,       // Read Auxilliary Register Group B
    CMD_RDAUXC  = 0x000D,       // Read Auxilliary Register Group C
    CMD_RDAUXD  = 0x000F,       // Read Auxilliary Register Group D
    CMD_RDSTATA = 0x0010,       // Read Status Register Group A
    CMD_RDSTATB = 0x0012,       // Read Status Register Group B
    CMD_WRSCTRL = 0x0014,       // Write S Control Register Group
    CMD_WRPWM   = 0x0020,       // Write PWM Register Group
    CMD_WRPSB   = 0x001C,       // Write PWM/S Control Register Group B
    CMD_RDSCTRL = 0x0016,       // Read S Control Register Group
    CMD_RDPWM   = 0x0022,       // Read PWM Register Group
    CMD_RDPSB   = 0x001E,       // Read PWM/S Control Register Group B
    CMD_STSCTRL = 0x0019,       // Start S Control Pulsing and Poll Status
    CMD_CLRSCTRL= 0x0018,       // Clear S Control Register Group

    // Start Cell Voltage ADC Conversion and Poll Status
    CMD_ADCV     = 0x0260 | (MD << 7) | (DCP << 4) | CH_ALL,
    CMD_ADCV_CH1 = 0x0260 | (MD << 7) | (DCP << 4) | CH_1,
    CMD_ADCV_CH2 = 0x0260 | (MD << 7) | (DCP << 4) | CH_2,
    CMD_ADCV_CH3 = 0x0260 | (MD << 7) | (DCP << 4) | CH_3,
    CMD_ADCV_CH4 = 0x0260 | (MD << 7) | (DCP << 4) | CH_4,
    CMD_ADCV_CH5 = 0x0260 | (MD << 7) | (DCP << 4) | CH_5,
    CMD_ADCV_CH6 = 0x0260 | (MD << 7) | (DCP << 4) | CH_6,

    // Start Open Wire ADC Conversion and Poll Status
    CMD_ADOW_PUP    = 0x0228 | (MD << 7) | (PUP_PULLUP << 6)   | (DCP << 4), // CH set to 0
    CMD_ADOW_PDOWN  = 0x0228 | (MD << 7) | (PUP_PULLDOWN << 6) | (DCP << 4), // CH set to 0

    // Start Self Test Cell Voltage Conversion and Poll Status
    //CMD_CVST    = 0x0207 | (MD << 7) | (ST << 5),

    // Start Overlap Measurement of Cell 7 Voltage
    CMD_ADOL    = 0x0201 | (MD << 7) | (DCP << 4),

    // Start GPIOs ADC Conversion and Poll Status
    CMD_ADAX_ALL      =  0x0460 | (MD << 7) | CHG_ALL,
    CMD_ADAX_GPIO1_6  =  0x0460 | (MD << 7) | CHG_GPIO1_6,
    CMD_ADAX_GPIO2_7  =  0x0460 | (MD << 7) | CHG_GPIO2_7,
    CMD_ADAX_GPIO3_8  =  0x0460 | (MD << 7) | CHG_GPIO3_8,
    CMD_ADAX_GPIO4_9  =  0x0460 | (MD << 7) | CHG_GPIO4_9,
    CMD_ADAX_GPIO5    =  0x0460 | (MD << 7) | CHG_GPIO5,
    CMD_ADAX_VREF2    =  0x0460 | (MD << 7) | CHG_VREF2,
    // Start GPIOs ADC Conversion With Digital Redundancy and Poll Status
    CMD_ADAXD_ALL     =  0x0400 | (MD << 7) | CHG_ALL,
    CMD_ADAXD_GPIO1_6 =  0x0400 | (MD << 7) | CHG_GPIO1_6,
    CMD_ADAXD_GPIO2_7 =  0x0400 | (MD << 7) | CHG_GPIO2_7,
    CMD_ADAXD_GPIO3_8 =  0x0400 | (MD << 7) | CHG_GPIO3_8,
    CMD_ADAXD_GPIO4_9 =  0x0400 | (MD << 7) | CHG_GPIO4_9,
    CMD_ADAXD_GPIO5   =  0x0400 | (MD << 7) | CHG_GPIO5,
    CMD_ADAXD_VREF2   =  0x0400 | (MD << 7) | CHG_VREF2,

    // Start Self Test GPIOs Conversion and Poll Status
    //CMD_AXST = 0x0407 | (MD << 7) | (ST << 5),

    // Start Status Group ADC Conversion and Poll Status
    CMD_ADSTAT_ALL  = 0x0468 | (MD << 7) | CHST_ALL,
    CMD_ADSTAT_SC   = 0x0468 | (MD << 7) | CHST_SC,
    CMD_ADSTAT_ITMP = 0x0468 | (MD << 7) | CHST_ITMP,
    CMD_ADSTAT_VA   = 0x0468 | (MD << 7) | CHST_VA,
    CMD_ADSTAT_VD   = 0x0468 | (MD << 7) | CHST_VD,
    // Start Status Group ADC Conversion With Digital Redundancy and Poll Status
    CMD_ADSTATD_ALL = 0x0408 | (MD << 7) | CHST_ALL,
    CMD_ADSTATD_SC  = 0x0408 | (MD << 7) | CHST_SC,
    CMD_ADSTATD_ITMP= 0x0408 | (MD << 7) | CHST_ITMP,
    CMD_ADSTATD_VA  = 0x0408 | (MD << 7) | CHST_VA,
    CMD_ADSTATD_VD  = 0x0408 | (MD << 7) | CHST_VD,

    // Start Self Test Status Group Conversion and Poll Status
    //CMD_STATST  = 0x040F | (MD << 7) | (ST << 5),

    // Start Combined Cell Voltage and GPIO1, GPIO2 Conversion and Poll Status
    CMD_ADCVAX  = 0x046F | (MD << 7) | (DCP << 4),
    // Start Combined Cell Voltage and SC Conversion and Poll Status
    CMD_ADCVSC  = 0x0467 | (MD << 7) | (DCP << 4),

    CMD_CLRCELL = 0x0711,       // Clear Cell Voltage Register Groups
    CMD_CLRAUX  = 0x0712,       // Clear Auxiliary Register Groups
    CMD_CLRSTAT = 0x0713,       // Clear Status Register Groups
    CMD_PLADC   = 0x0714,       // Poll ADC Conversion Status
    CMD_PLAUX   = 0x0715,       // Poll AUX Conversion Status
    CMD_DIAGN   = 0x0715,       // Diagnose MUX and Poll Status
    CMD_WRCOMM  = 0x0721,       // Write COMM Register Group
    CMD_RDCOMM  = 0x0722,       // Read COMM Register Group
    CMD_STCOMM  = 0x0723,       // Start I2C /SPI Communication
    CMD_MUTE    = 0x0028,       // Mute discharge
    CMD_UNMUTE  = 0x0029        // Unmute discharge
} Slave_Command_t;

void Slave_Init(
	SPI_HandleTypeDef *SPI_handle,
	uint8_t config_val_a[SLAVE_REG_SIZE_BYTES],
	uint8_t config_val_b[SLAVE_REG_SIZE_BYTES]);
Slave_ConfigRegisters_t* Slave_GetConfigRegisters();
void Slave_WakeUp(void);
void Slave_SendCmd(Slave_Command_t command);
Slave_Status_t Slave_SendCmdAndPoll(Slave_Command_t command);
void Slave_WriteRegisterGroup(Slave_Command_t command, uint8_t tx_data[SLAVE_NUM_DEVICES][SLAVE_REG_SIZE_BYTES]);
Slave_Status_t Slave_ReadRegisterGroup(Slave_Command_t command, uint8_t rx_data[SLAVE_NUM_DEVICES][SLAVE_REG_SIZE_BYTES]);
