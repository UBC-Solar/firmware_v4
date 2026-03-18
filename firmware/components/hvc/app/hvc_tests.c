#include "hvc_tests.h"

#include "main.h"
#include "stm32f1xx_hal_gpio.h"
#include "gpio_driver.h"

// Test plan: https://docs.google.com/document/d/1IN_0Pcg9eEbxtUkUSTO_Nc7S2LKmDKqyU20G1GsCJE0/edit?tab=t.ao1cu7ifcyui
void TESTS_ContactorControl() {
    GPIO_Write(POS_CTRL_GPIO_Port, POS_CTRL_Pin, GPIO_PIN_SET);
    GPIO_Write(NEG_CTRL_GPIO_Port, NEG_CTRL_Pin, GPIO_PIN_SET);
    GPIO_Write(HLIM_CTRL_GPIO_Port, HLIM_CTRL_Pin, GPIO_PIN_SET);
    GPIO_Write(LLIM_CTRL_GPIO_Port, LLIM_CTRL_Pin, GPIO_PIN_SET);
}

// Test plan: https://docs.google.com/document/d/1IN_0Pcg9eEbxtUkUSTO_Nc7S2LKmDKqyU20G1GsCJE0/edit?tab=t.jsd2m196k8n0
void TESTS_LvControl() {
    GPIO_Write(MPPT_CTRL_GPIO_Port, MPPT_CTRL_Pin, GPIO_PIN_SET);
    GPIO_Write(DIST_CTRL_GPIO_Port, DIST_CTRL_Pin, GPIO_PIN_SET);
    GPIO_Write(IMD_CTRL_GPIO_Port, IMD_CTRL_Pin, GPIO_PIN_SET);
}

void TESTS_PrechargeControl() {
    GPIO_Write(MOTOR_PC_CTRL_GPIO_Port, MOTOR_PC_CTRL_Pin, GPIO_PIN_SET);
    GPIO_Write(MPPT_PC_CTRL_GPIO_Port, MPPT_PC_CTRL_Pin, GPIO_PIN_SET);
    GPIO_Write(DISCHARGE_TOGGLE_OFF_GPIO_Port, DISCHARGE_TOGGLE_OFF_Pin, GPIO_PIN_SET);
}

void TESTS_DebugLEDs() {
    GPIO_Write(DEBUG_LED_GPIO_Port, DEBUG_LED_Pin, GPIO_PIN_SET);
    GPIO_Write(SUPP_LOW_LED_GPIO_Port, SUPP_LOW_LED_Pin, GPIO_PIN_SET);
    GPIO_Write(FAULT_LED_GPIO_Port, FAULT_LED_Pin, GPIO_PIN_SET);
    GPIO_Write(ESTOP_LED_GPIO_Port, ESTOP_LED_Pin, GPIO_PIN_SET);
}

void TESTS_AllLEDs() {
    uint16_t delay_ms = 250;
    
    while (1) {
        HAL_GPIO_TogglePin(NEG_CTRL_GPIO_Port, NEG_CTRL_Pin);
        HAL_Delay(delay_ms);
        HAL_GPIO_TogglePin(HLIM_CTRL_GPIO_Port, HLIM_CTRL_Pin);
        HAL_Delay(delay_ms);
        HAL_GPIO_TogglePin(LLIM_CTRL_GPIO_Port, LLIM_CTRL_Pin);
        HAL_Delay(delay_ms);
        HAL_GPIO_TogglePin(POS_CTRL_GPIO_Port, POS_CTRL_Pin);
        HAL_Delay(delay_ms);
        HAL_GPIO_TogglePin(DISCHARGE_TOGGLE_OFF_GPIO_Port, DISCHARGE_TOGGLE_OFF_Pin);
        HAL_Delay(delay_ms);
        HAL_GPIO_TogglePin(MPPT_PC_CTRL_GPIO_Port, MPPT_PC_CTRL_Pin);
        HAL_Delay(delay_ms);
        HAL_GPIO_TogglePin(MOTOR_PC_CTRL_GPIO_Port, MOTOR_PC_CTRL_Pin);
        HAL_Delay(delay_ms);
        HAL_GPIO_TogglePin(FAN_CTRL_GPIO_Port, FAN_CTRL_Pin);
        HAL_Delay(delay_ms);
        HAL_GPIO_TogglePin(IMD_CTRL_GPIO_Port, IMD_CTRL_Pin);
        HAL_Delay(delay_ms);
        HAL_GPIO_TogglePin(DIST_CTRL_GPIO_Port, DIST_CTRL_Pin);
        HAL_Delay(delay_ms);
        HAL_GPIO_TogglePin(MPPT_CTRL_GPIO_Port, MPPT_CTRL_Pin);
        HAL_Delay(delay_ms);
        HAL_GPIO_TogglePin(DEBUG_LED_GPIO_Port, DEBUG_LED_Pin);
        HAL_Delay(delay_ms);
        HAL_GPIO_TogglePin(SUPP_LOW_LED_GPIO_Port, SUPP_LOW_LED_Pin);
        HAL_Delay(delay_ms);
        HAL_GPIO_TogglePin(FAULT_LED_GPIO_Port, FAULT_LED_Pin);
        HAL_Delay(delay_ms);
        HAL_GPIO_TogglePin(ESTOP_LED_GPIO_Port, ESTOP_LED_Pin);
        HAL_Delay(delay_ms);
    }
}