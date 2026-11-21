/**
 * @file gpio_runtime.c
 * @brief Levels of the input pins the FSM and debug output check, built on gpio_driver.
 *
 * Only for pins read by polling.
 * Pin interrupts are not handled here: they stay in gpio_driver.c (HAL_GPIO_EXTI_Callback()), which
 * calls the eFuse fault callback in fsm.c.
 */

#include "gpio_runtime.h"

// HAL and CubeMX
#include "main.h" // Pin names

// Drivers
#include "gpio_driver.h"

/**
 * @brief Reads every polled input pin once.
 *
 * Call once per pass and keep the result (DIST_Main() stores it in g_shared.pins), so the whole
 * pass uses the same levels.
 *
 * @return Every polled input pin's level.
 */
PinLevels GPIO_Runtime_Read(void) {
    PinLevels pins;

    pins.estop = GPIO_Read(ESTOP_GPIO_GPIO_Port, ESTOP_GPIO_Pin);
    pins.mux_status = GPIO_Read(MUX_STATUS_GPIO_Port, MUX_STATUS_Pin);
    pins.drd_fuse = GPIO_Read(DRD_FUSE_GPIO_Port, DRD_FUSE_Pin);
    pins.mdi_fuse = GPIO_Read(MDI_FUSE_GPIO_Port, MDI_FUSE_Pin);
    pins.spare_fuse = GPIO_Read(SPARE_FUSE_GPIO_Port, SPARE_FUSE_Pin);
    pins.spare_ctrl_fuse = GPIO_Read(SPARE_CTRL_FUSE_GPIO_Port, SPARE_CTRL_FUSE_Pin);

    return pins;
}
