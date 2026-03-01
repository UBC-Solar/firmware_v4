#include "hvc_main.h"
#include "debug_io.h"
#include "gpio_driver.h"
#include "main.h"
#include "stdio.h"
#include "stm32f1xx_hal_gpio.h"

void hvcMain(void)
{
    //DEBUG_IO_PRINT("Hello from hvcMain!\n");
    //printf("Hello from printf!\n");

    GPIO_Read(MASTERBOARD_FAULT_GPIO_Port, MASTERBOARD_FAULT_Pin);
    HAL_GPIO_ReadPin(MASTERBOARD_FAULT_GPIO_Port, MASTERBOARD_FAULT_Pin);
}
