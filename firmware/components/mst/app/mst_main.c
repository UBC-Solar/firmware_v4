#include "mst_main.h"
#include "debug_io.h"

#include "main.h"

#include "can_driver.h"
#include "gpio_driver.h"
#include "spi_driver.h"
#include "stm32f1xx_hal_gpio.h"
#include "uart_driver.h"



void CollectPackData() {

}


void DriveOutputs() {

}


void SendCanMMessages() {

}


#ifdef UNIT_TEST_MCU
void Debug_McuTestCycle() {
    DEBUG_IO_print("Debug_McuTestCycle start (debug IO)\n");
    printf("Debug_McuTestCycle start (printf)\n");

    GPIO_Write(FAULT_OUT_GPIO_Port, FAULT_OUT_Pin, GPIO_PIN_SET);
    HAL_Delay(500);

    DEBUG_IO_print("Debug_McuTestCycle end (debug IO)\n");
    printf("Debug_McuTestCycle end (printf)\n");

    GPIO_Write(FAULT_OUT_GPIO_Port, FAULT_OUT_Pin, GPIO_PIN_RESET);
}
#endif // UNIT_TEST_MCU


#ifdef UNIT_TEST_IO
void Debug_DigitalIoTestCycle() {

}
#endif // UNIT_TEST_IO


#ifdef UNIT_TEST_CAN
void Debug_IsoSpiTestCycle() {

}
#endif // UNIT_TEST_CAN


#ifdef UNIT_TEST_ISOSPI
void Debug_CanTestCycle() {

}
#endif // UNIT_TEST_ISOSPI
