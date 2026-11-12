/**
 * @file gpio_runtime.c
 * @brief Levels of the input pins the FSM checks itself, built on gpio_driver.
 *
 * Only for pins the FSM reads itself (polled).
 * Pin interrupts are not handled here: they stay in gpio_driver.c (HAL_GPIO_EXTI_Callback()), which
 * calls the fault callbacks in fsm.c.
 */

#include "gpio_runtime.h"

// HAL and CubeMX
#include "main.h" // Pin names

// Drivers
#include "gpio_driver.h"

/**
 * @brief Reads every polled input pin once.
 *
 * Call once per pass and keep the result (HVC_Main() stores it in g_shared.pins), so the whole pass
 * uses the same levels, and bench overrides can change them before the FSM runs.
 *
 * @return Every polled input pin's level.
 */
PinLevels GPIO_Runtime_Read(void) {
    PinLevels pins;

    pins.estop = GPIO_Read(ESTOP_GPIO_Port, ESTOP_Pin);
    pins.imd_fault = GPIO_Read(IMD_GPIO_IN_GPIO_Port, IMD_GPIO_IN_Pin);
    pins.hv_current_alert = GPIO_Read(HV_CURRENT_ALERT_GPIO_Port, HV_CURRENT_ALERT_Pin);
    pins.mst_fault = GPIO_Read(MASTERBOARD_FAULT_GPIO_Port, MASTERBOARD_FAULT_Pin);
    pins.dcdc_active = GPIO_Read(DCDC_ACTIVE_GPIO_Port, DCDC_ACTIVE_Pin);
    pins.supp_active = GPIO_Read(SUPP_ACTIVE_GPIO_Port, SUPP_ACTIVE_Pin);

    return pins;
}
