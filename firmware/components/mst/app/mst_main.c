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
    Slave_init(&hspi2);
}

void CollectBoardData() {
    GPIO_PinState balancePinState = 
        GPIO_Read(BALANCE_EN_IN_GPIO_Port, BALANCE_EN_IN_Pin);
    pack_state.bits.balancing_enable = balancePinState == GPIO_PIN_SET ? true : false;

    GPIO_PinState scrutineeringPinState = 
        GPIO_Read(SCRUTINEERING_EN_IN_GPIO_Port, SCRUTINEERING_EN_IN_Pin);
    pack_state.bits.scrutineering_enable = scrutineeringPinState == GPIO_PIN_SET ? true : false;

    LOG_DEBUG("Balance enable: %d, Scrutineering mode: %d.", balancePinState, scrutineeringPinState);
}

void CollectModuleData() {
    uint32_t voltageStart_ms = HAL_GetTick();
    StartVoltageMeasurement();
    GetVoltageMeasurement();
    uint32_t tempStart_ms = HAL_GetTick();
    StartTemperatureMeasurement();
    GetTemperatureMeasurement();
    uint32_t tempEnd_ms = HAL_GetTick();
    
    LOG_DEBUG("Voltage measurement: %lu ms, Temperature measurement: %lu ms, Total: %lu ms", 
              tempStart_ms - voltageStart_ms, 
              tempEnd_ms - tempStart_ms, 
              tempEnd_ms - voltageStart_ms);
}


void AnalyzeModuleData() {
    CheckForEmergency(pack_modules, &pack_faults, &pack_warnings);

    uint32_t balancingStart_ms = HAL_GetTick();
    DoBalancing();
    uint32_t balancingEnd_ms = HAL_GetTick();
    
    LOG_DEBUG("Balancing commands: %lu ms", balancingEnd_ms - balancingStart_ms);
}


void SendCanMMessages() {

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

    Slave_sendCmd(CMD_ADCV);

    HAL_Delay(2000);

    isoSpiCycleCount++;
}
#endif // UNIT_TEST_ISOSPI
