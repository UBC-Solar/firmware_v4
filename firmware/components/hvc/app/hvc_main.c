#include "hvc_fsm.h"
#include "hvc_main.h"

#include "stm32f1xx_hal.h"
#include <stdint.h>

#include "debug_io.h"

#include "stm32f1xx_hal_can.h"
#include "stm32f1xx_hal_gpio.h"
#include "uart_driver.h"
#include "adc_driver.h"
#include "i2c_driver.h"
#include "can_driver.h"

#include "ina228_runtime.h"


void HVC_Init(
    UART_HandleTypeDef *_huart2, 
    ADC_HandleTypeDef *_hadc1, 
    TIM_HandleTypeDef *_htim3,
    TIM_HandleTypeDef *_htim4, 
    I2C_HandleTypeDef *_hi2c1, 
    CAN_HandleTypeDef *_hcan) 
{
    UART_Init(_huart2);
    ADC_Init(_hadc1, _htim3);
    I2C_Init(_hi2c1);
    HAL_TIM_PWM_Start(_htim4, TIM_CHANNEL_3);
    
    uint16_t filter_ids[] = {
        TEL_HEARTBEAT_ID,
        LV_POWERUP_ID,
        MST_HEARTBEAT_ID,
        DIST_FAULT_ID,
        [4]=DIST_HEARTBEAT_ID
    };
    CAN_InitFilterList(_hcan, filter_ids, 5);
    CAN_Init(_hcan);

    DEBUG_IO_print("UART and ADC and I2C initialized.\n");
    // INA228_Init();
    // DEBUG_IO_print("INA228 initialized.\n");

    HVC_FSM_Init(); // sets hvc_state

    if (
        (HAL_GPIO_ReadPin(ESTOP_GPIO_Port, ESTOP_Pin) == GPIO_PIN_SET) ||
        (HAL_GPIO_ReadPin(IMD_GPIO_IN_GPIO_Port, IMD_GPIO_IN_Pin) == GPIO_PIN_SET) ||
        (HAL_GPIO_ReadPin(MASTERBOARD_FAULT_GPIO_Port, MASTERBOARD_FAULT_Pin) == GPIO_PIN_SET) ||
        (HAL_GPIO_ReadPin(HV_CURRENT_ALERT_GPIO_Port, HV_CURRENT_ALERT_Pin) == GPIO_PIN_SET))
    {
        hvc_state = FAULT; // conditionally overrides the value set by HVC_FSM_Init
    }
    DEBUG_IO_PRINT("HVC initialized\r\n");
}

void HVC_Main(void) {
    while (true) {
        HVC_FSM_Run();
    }
}

#if (UNIT_TEST_SHUNT == RUN)
static uint32_t previous_time_ms = 0;
void HVC_TestShuntCycle() {
    if (HAL_GetTick() - previous_time_ms >= 100) {
        DEBUG_IO_print("100 ms elapsed, reading shunt voltage.\n");
        previous_time_ms = HAL_GetTick();
        INA228_Read_Shunt_Voltage();
    }
}
#endif // (UNIT_TEST_SHUNT == RUN)

#if (INT_TEST_CAN == RUN)
void HVC_TestCanCycle() {
    GPIO_Write(DIST_CTRL_GPIO_Port, DIST_CTRL_Pin, GPIO_PIN_SET);
    CAN_SendMessgeDebug();
}
#endif // (INT_TEST_CAN == RUN)

