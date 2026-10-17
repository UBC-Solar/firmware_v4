/**
 * @file    gpio_driver.c
 * @brief   GPIO driver implementation for UBC Solar MDI board
 */

#include "gpio_driver.h"

#include "main.h"

void GpioDriverToggleDebugLed(void) { HAL_GPIO_TogglePin(DEBUG_LED_GPIO_Port, DEBUG_LED_Pin); }

void GpioDriverToggleRtdFaultLed(bool fault){
    if(faults){
		HAL_GPIO_WritePin(RTD_FAULT_LED_GPIO_PORT, RTD_FAULT_LED_GPIO_PIN, GPIO_PIN_SET);
	}
	else{
		HAL_GPIO_WritePin(RTD_FAULT_LED_GPIO_PORT, RTD_FAULT_LED_GPIO_PIN, GPIO_PIN_RESET);
	}
}
