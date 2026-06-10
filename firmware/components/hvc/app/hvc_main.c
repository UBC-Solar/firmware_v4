#include "hvc_fsm.h"
#include "hvc_main.h"

#include "stm32f1xx_hal.h"
#include <stdint.h>

#include "debug_io.h"

#include "uart_driver.h"
#include "adc_driver.h"
#include "i2c_driver.h"

#include "ina228_runtime.h"

uint32_t previous_time_ms = 0;

void HVC_Init(UART_HandleTypeDef *_huart2, ADC_HandleTypeDef *_hadc1, TIM_HandleTypeDef *_htim3, I2C_HandleTypeDef *_hi2c1) {
    UART_Init(_huart2);
    ADC_Init(_hadc1, _htim3);
    I2C_Init(_hi2c1);

    DEBUG_IO_print("UART and ADC and I2C initialized.\n");
    INA228_Init();
    DEBUG_IO_print("INA228 initialized.\n");
}

void HVC_Main(void) {
#if (UNIT_TEST_SHUNT == RUN)
    if (HAL_GetTick() - previous_time_ms >= 100) {
        DEBUG_IO_print("100 ms elapsed, reading shunt voltage.\n");
        previous_time_ms = HAL_GetTick();
        INA228_Read_Shunt_Voltage();
    }
#endif // (UNIT_TEST_SHUNT == RUN)

    while (true) {
        HVC_FSM_Run();
    }
}
