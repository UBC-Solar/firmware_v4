#include "hvc_main.h"

#include "main.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_gpio.h"
#include <stdint.h>

#include "debug_io.h"
#include "uart_driver.h"
#include "adc_driver.h"
#include "gpio_driver.h"
#include "hvc_tests.h"

void HVC_Init(UART_HandleTypeDef *_huart2, ADC_HandleTypeDef *_hadc1, TIM_HandleTypeDef *_htim3) {
    UART_Init(_huart2);
    ADC_Init(_hadc1, _htim3);

    //TESTS_ContactorControl();
    //TESTS_LvControl();
    //TESTS_PrechargeControl();
    //TESTS_DebugLEDs();
    TESTS_AllLEDs();
}

void HVC_Main(void) {
    return;
    
    UART_Printf("HVC Main Loop Running\n\r");
    
    GPIO_Write(DEBUG_LED_GPIO_Port, DEBUG_LED_Pin, GPIO_PIN_RESET);
    GPIO_Write(SUPP_LOW_LED_GPIO_Port, SUPP_LOW_LED_Pin, GPIO_PIN_SET);
    HAL_Delay(500);
    GPIO_Write(DEBUG_LED_GPIO_Port, DEBUG_LED_Pin, GPIO_PIN_SET);
    GPIO_Write(SUPP_LOW_LED_GPIO_Port, SUPP_LOW_LED_Pin, GPIO_PIN_RESET);
    HAL_Delay(500);
}
