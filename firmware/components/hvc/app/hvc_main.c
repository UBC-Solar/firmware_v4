#include "hvc_main.h"

#include "stm32f1xx_hal.h"
#include <stdint.h>

#include "debug_io.h"
#include "uart_driver.h"
#include "adc_driver.h"

void HVC_Init(UART_HandleTypeDef *_huart2, ADC_HandleTypeDef *_hadc1, TIM_HandleTypeDef *_htim3) {
    UART_Init(_huart2);
    ADC_Init(_hadc1, _htim3);
}

void HVC_Main(void) {
    
}
