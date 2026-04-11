#include "mst_main.h"

#include <stdio.h>

#include "debug_io.h"
#include "main.h"

#include "spi.h"
#include "usart.h"

#include "can_driver.h"
#include "gpio_driver.h"
#include "spi_driver.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_gpio.h"
#include "uart_driver.h"

void Initialize() {
    UART_Init(&huart1);

#ifdef UNIT_TEST_CAN
    CAN_Init(&hcan);
#endif // UNIT_TEST_CAN

#ifdef UNIT_TEST_ISOSPI
    Slave_init(&hspi2);
#endif // UNIT_TEST_ISOSPI
}


#ifdef UNIT_TEST_MCU
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


#ifdef UNIT_TEST_IO
void Debug_DigitalIoTestCycle() {
    DEBUG_IO_PRINT("Debug_DigitalIoTestCycle start (debug IO)\r\n");
    UART_Transmit("Debug_DigitalIoTestCycle start (UART_Transmit)\r\n");

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

    DEBUG_IO_PRINT("Debug_DigitalIoTestCycle end (debug IO)\r\n");
    UART_Transmit("Debug_DigitalIoTestCycle end (UART_Transmit)\r\n");
}
#endif // UNIT_TEST_IO


#ifdef UNIT_TEST_CAN
void Debug_CanTestCycle() {
    DEBUG_IO_PRINT("Debug_DigitalIoTestCycle start (debug IO)\r\n");
    UART_Transmit("Debug_DigitalIoTestCycle start (UART_Transmit)\r\n");

    CAN_SendMessgeDebug();

    DEBUG_IO_PRINT("Debug_DigitalIoTestCycle end (debug IO)\r\n");
    UART_Transmit("Debug_DigitalIoTestCycle end (UART_Transmit)\r\n");
    
    HAL_Delay(2000);
}
#endif // UNIT_TEST_CAN


#ifdef UNIT_TEST_ISOSPI
void Debug_IsoSpiTestCycle() {
    DEBUG_IO_PRINT("Debug_IsoSpiTestCycle start (debug IO)\r\n");
    UART_Transmit("Debug_IsoSpiTestCycle start (UART_Transmit)\r\n");

    Slave_sendCmd(CMD_ADCV);

    DEBUG_IO_PRINT("Debug_IsoSpiTestCycle end (debug IO)\r\n");
    UART_Transmit("Debug_IsoSpiTestCycle end (UART_Transmit)\r\n");
    
    HAL_Delay(2000);
}
#endif // UNIT_TEST_ISOSPI
