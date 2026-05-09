#include "mst_main.h"

#include <stdint.h>
#include <stdio.h>

#include "debug_io.h"
#include "main.h"

#include "mst_defs.h"
#include "mst_types.h"
#include "balancing.h"
#include "can_messages.h"
#include "emergency.h"
#include "module_data.h"
#include "spi.h"
#include "usart.h"

#include "can_driver.h"
#include "gpio_driver.h"
#include "spi_driver.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_gpio.h"
#include "uart_driver.h"

module_t pack_modules[NUM_MODULES] = {0};
faults_t pack_faults = {0};
warnings_t pack_warnings = {0};
pack_state_t pack_state = {0};

slave_t slaves[NUM_SLAVES] = {0};



void Initialize() {
    UART_Init(&huart1);
    CAN_Init(&hcan);

    // Refer to the ADBMS1818 datasheet pages 65, 68, 69 for 
    // format and content of configuration register groups A and B
    uint8_t config_val_a[SLAVE_REG_SIZE_BYTES] =
    {
        0xF8 | (REFON << 2) | ADCOPT, // GPIO 1-5 pull-downs off, REFON, ADCOPT
        (VUV & 0xFF), // VUV[7:0]
		((uint8_t) (VOV << 4)) | (((uint8_t) (VUV >> 8)) & 0x0F), // VOV[4:0] | VUV[11:8]
        (VOV >> 4), // VOV[11:4]
		0x00, // Discharge off for cells 1 through 8
        0x00  // Discharge off for cells 9 through 12, Discharge timer disabled
    };
	uint8_t config_val_b[SLAVE_REG_SIZE_BYTES] =
    {
        0x0F, // Discharge off for cells 13 through 16, GPIO 6-9 = 1
        0x00, // FDRF = 0, PS = 0, Discharge off for cells 17 and 18
        0x00,
        0x00,
        0x00,
        0x00
    };

    // Includes Slave_Init (SPI perhiperal initialization)
    Module_Init(&hspi2, slaves, config_val_a, config_val_b);
}

void CollectBoardData() {
    GPIO_PinState balancePinState = 
        GPIO_Read(BALANCE_EN_IN_GPIO_Port, BALANCE_EN_IN_Pin);
    pack_state.balancing_enable = balancePinState == GPIO_PIN_SET ? true : false;

    GPIO_PinState scrutineeringPinState = 
        GPIO_Read(SCRUTINEERING_EN_IN_GPIO_Port, SCRUTINEERING_EN_IN_Pin);
    pack_state.scrutineering_enable = scrutineeringPinState == GPIO_PIN_SET ? true : false;

    LOG_DEBUG("Balance enable: %d, Scrutineering mode: %d.", balancePinState, scrutineeringPinState);
}

void CollectModuleData() {
    uint32_t voltage_start_ms = HAL_GetTick();
    
    if (pack_state.balancing_active) {
        PauseAllBalancing(pack_modules);
    }
    RequestVoltageMeasurement();
    RetrieveVoltageMeasurement(slaves, pack_modules);
    if (pack_state.balancing_active) {
        ResumeAllBalancing(pack_modules);
    }

    uint32_t temp_start_ms = HAL_GetTick();

    RequestTemperatureMeasurement();
    RetrieveTemperatureMeasurement(slaves, pack_modules);
    
    uint32_t temp_end_ms = HAL_GetTick();
    
    LOG_DEBUG("Voltage measurement: %lu ms, Temperature measurement: %lu ms, Total: %lu ms", 
              temp_start_ms - voltage_start_ms, 
              temp_end_ms - temp_start_ms, 
              temp_end_ms - voltage_start_ms);
}


void AnalyzeModuleData() {
    CheckForEmergency(pack_modules, &pack_faults, &pack_warnings);

    ComputePackStatistics(pack_modules, &pack_state);
}

void DriveOutputs() {
    GPIO_Write(HLIM_EN_OUT_GPIO_Port, HLIM_EN_OUT_Pin, pack_state.hlim_enable);
    GPIO_Write(LLIM_EN_OUT_GPIO_Port, LLIM_EN_OUT_Pin, pack_state.llim_enable);
    
    uint32_t balancing_start_ms = HAL_GetTick();
    DoBalancing(&pack_state, pack_modules, slaves);
    uint32_t balancing_end_ms = HAL_GetTick();

    LOG_DEBUG("Balancing commands: %lu ms", balancing_end_ms - balancing_start_ms);
}


void SendCanMMessages() {
    CAN_SendMessage0x622();
    CAN_SendMessage0x623();
    CAN_SendMessage0x625();
    CAN_SendMessage0x626();
    CAN_SendMessage0x627();
    CAN_SendMessage0x628();
    CAN_SendMessage0x629();
}


#if (UNIT_TEST_MCU == RUN)
void Debug_McuTestCycle() {
    DEBUG_IO_PRINT("Debug_McuTestCycle start (debug IO)\r\n");
    UART_Transmit("Debug_McuTestCycle start (UART_Transmit)\r\n");

    GPIO_Write(LED_OUT_GPIO_Port, LED_OUT_Pin, GPIO_PIN_SET);
    HAL_Delay(1000);

    DEBUG_IO_PRINT("Debug_McuTestCycle end (debug IO)\r\n");
    UART_Transmit("Debug_McuTestCycle end (UART_Transmit)\r\n");

    GPIO_Write(LED_OUT_GPIO_Port, LED_OUT_Pin, GPIO_PIN_RESET);
    HAL_Delay(1000);
}
#endif // UNIT_TEST_MCU


#if (UNIT_TEST_IO == RUN)
void Debug_DigitalIoTestCycle() {
    DEBUG_IO_PRINT("FAULT signal HIGH. Other signals should be LOW.\r\n");
    UART_Transmit("FAULT signal HIGH. Other signals should be LOW.\r\n");
    GPIO_Write(FAULT_OUT_GPIO_Port, FAULT_OUT_Pin, GPIO_PIN_SET);
    HAL_Delay(1000);
    GPIO_Write(FAULT_OUT_GPIO_Port, FAULT_OUT_Pin, GPIO_PIN_RESET);

    DEBUG_IO_PRINT("HLIM_EN signal HIGH. Other signals should be LOW.\r\n");
    UART_Transmit("HLIM_EN signal HIGH. Other signals should be LOW.\r\n");
    GPIO_Write(HLIM_EN_OUT_GPIO_Port, HLIM_EN_OUT_Pin, GPIO_PIN_SET);
    HAL_Delay(1000);
    GPIO_Write(HLIM_EN_OUT_GPIO_Port, HLIM_EN_OUT_Pin, GPIO_PIN_RESET);

    DEBUG_IO_PRINT("LLIM_EN signal HIGH. Other signals should be LOW.\r\n");
    UART_Transmit("LLIM_EN signal HIGH. Other signals should be LOW.\r\n");
    GPIO_Write(LLIM_EN_OUT_GPIO_Port, LLIM_EN_OUT_Pin, GPIO_PIN_SET);
    HAL_Delay(1000);
    GPIO_Write(LLIM_EN_OUT_GPIO_Port, LLIM_EN_OUT_Pin, GPIO_PIN_RESET);

    DEBUG_IO_PRINT("CONTACTOR_EN signal HIGH. Other signals should be LOW.\r\n");
    UART_Transmit("CONTACTOR_EN signal HIGH. Other signals should be LOW.\r\n");
    GPIO_Write(CONTACTOR_EN_OUT_GPIO_Port, CONTACTOR_EN_OUT_Pin, GPIO_PIN_SET);
    HAL_Delay(1000);
    GPIO_Write(CONTACTOR_EN_OUT_GPIO_Port, CONTACTOR_EN_OUT_Pin, GPIO_PIN_RESET);

    GPIO_Write(LED_OUT_GPIO_Port, LED_OUT_Pin, GPIO_PIN_SET);

}
#endif // UNIT_TEST_IO


#if (UNIT_TEST_CAN == RUN)
int canCyclecCount = 1;
void Debug_CanTestCycle() {
    DEBUG_IO_PRINT("Debug_CanTestCycle round %d (debug IO)\r\n", canCyclecCount);

    CAN_SendMessgeDebug();
    
    HAL_Delay(2000);

    canCyclecCount++;
}
#endif // UNIT_TEST_CAN


#if (UNIT_TEST_ISOSPI == RUN)
int isoSpiCycleCount = 1;
void Debug_IsoSpiTestCycle() {
    DEBUG_IO_PRINT("Debug_IsoSpiTestCycle round %d (debug IO)\r\n", isoSpiCycleCount);

    Slave_SendCmd(CMD_ADCV);

    HAL_Delay(2000);

    isoSpiCycleCount++;
}
#endif // UNIT_TEST_ISOSPI
