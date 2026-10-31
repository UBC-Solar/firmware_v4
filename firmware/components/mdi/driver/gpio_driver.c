/**
 * @file    gpio_driver.c
 * @brief   GPIO driver implementation for UBC Solar MDI board
 */

#include "gpio_driver.h"

#include "main.h"

void GpioDriverToggleDebugLed(void) { HAL_GPIO_TogglePin(DEBUG_LED_GPIO_Port, DEBUG_LED_Pin); }

void GpioDriverToggleRtdFaultLed(bool fault){
    if(fault){
		HAL_GPIO_WritePin(RTD_FAULT_LED_GPIO_PORT, RTD_FAULT_LED_GPIO_PIN, GPIO_PIN_SET);
	}
	else{
		HAL_GPIO_WritePin(RTD_FAULT_LED_GPIO_PORT, RTD_FAULT_LED_GPIO_PIN, GPIO_PIN_RESET);
	}
}

void GpioDriverToggleFwdLeds(void){
	HAL_GPIO_WritePin(REV_LED_GPIO_Port, REV_LED_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(FWD_LED_GPIO_Port, FWD_LED_Pin, GPIO_PIN_SET);
}

void GpioDriverToggleRevLeds(void){
	HAL_GPIO_WritePin(REV_LED_GPIO_Port, REV_LED_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(FWD_LED_GPIO_Port, FWD_LED_Pin, GPIO_PIN_RESET);
}

void GpioDriverEnableRegenLed(void){
	HAL_GPIO_WritePin(REGEN_LED_GPIO_Port, REGEN_LED_Pin, GPIO_PIN_SET);
}

void GpioDriverDisableRegenLed(void){
	HAL_GPIO_WritePin(REGEN_LED_GPIO_Port, REGEN_LED_Pin, GPIO_PIN_RESET);
}

