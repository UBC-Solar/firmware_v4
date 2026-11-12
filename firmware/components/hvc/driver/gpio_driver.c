#include "gpio_driver.h"
#include "stm32f1xx_hal_gpio.h"
#include "uart_driver.h"
#include "main.h"
#include "fsm.h"

/**
 * @brief Read the current logic state of a GPIO pin.
 * @param port GPIO port instance (for example, DIST_CTRL_GPIO_Port).
 * @param pin GPIO pin mask (for example, DIST_CTRL_Pin).
 * @return GPIO pin state as GPIO_PIN_SET or GPIO_PIN_RESET.
 */
GPIO_PinState GPIO_Read(GPIO_TypeDef *port, uint16_t pin) {
    return HAL_GPIO_ReadPin(port, pin);
}

/**
 * @brief Write a logic state to a GPIO pin.
 * @param port GPIO port instance (for example, DIST_CTRL_GPIO_Port).
 * @param pin GPIO pin mask (for example, DIST_CTRL_Pin).
 * @param state Output state to write: GPIO_PIN_SET or GPIO_PIN_RESET.
 */
void GPIO_Write(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state) {
    HAL_GPIO_WritePin(port, pin, state);
}

/**
 * @brief Toggle the output state of a GPIO pin.
 * @param port GPIO port instance (for example, GPIOA, GPIOB).
 * @param pin GPIO pin mask (for example, GPIO_PIN_5).
 */
void GPIO_Toggle(GPIO_TypeDef *port, uint16_t pin) {
    HAL_GPIO_TogglePin(port, pin);
}

/**
 * @brief HAL callback for the fault input interrupts: calls the fault callback if the pin is high.
 *
 * Each fault input interrupts on both edges (rising and falling, set in cube gpio.c), so this
 * reads the pin to get its level now. High means fault, and the matching callback in fsm.c
 * sets the fault flag. Low does nothing: fault flags are never cleared.
 * A pin interrupt turned off by the bench overrides (TESTS_BenchBlockFakedPinInterrupts(), tests.c)
 * never reaches here.
 *
 * Side effects: may set g_fault_flags.estop, imd_fault, mst_fault, overcurrent,
 * reverse_current, or dcdc_fault (through the fsm.c callbacks).
 *
 * @param GPIO_Pin Pin that triggered the interrupt (for example, ESTOP_Pin).
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    switch (GPIO_Pin) {
        case ESTOP_Pin:
            // Emergency stop pressed: turn on ESTOP_LED and set the estop fault
            if (HAL_GPIO_ReadPin(ESTOP_GPIO_Port, ESTOP_Pin) == GPIO_PIN_SET) {
                FSM_EstopCallback();
            }
            break;

        case IMD_GPIO_IN_Pin:
            // IMD reports an insulation fault: set the IMD fault
            if (HAL_GPIO_ReadPin(IMD_GPIO_IN_GPIO_Port, IMD_GPIO_IN_Pin) == GPIO_PIN_SET) {
                FSM_ImdFaultCallback();
            }
            break;

        case MASTERBOARD_FAULT_Pin:
            // Ignored until MstHandshake() arms it: MST holds this pin high during startup
            if (HAL_GPIO_ReadPin(MASTERBOARD_FAULT_GPIO_Port, MASTERBOARD_FAULT_Pin) ==
                GPIO_PIN_SET) {
                FSM_MstFaultCallback(g_shared.mst_irq_armed);
            }
            break;

        case HV_CURRENT_ALERT_Pin:
            // Shunt monitor alert, pack current past a limit: the callback reads the shunt
            // monitor and sets the overcurrent or reverse current fault
            if (HAL_GPIO_ReadPin(HV_CURRENT_ALERT_GPIO_Port, HV_CURRENT_ALERT_Pin) ==
                GPIO_PIN_SET) {
                FSM_HvCurrentAlertCallback();
            }
            break;

        case DCDC_ACTIVE_Pin:
            // Ignored until HvConnect() arms it: DCDC_ACTIVE (active-low) is high until then
            if (HAL_GPIO_ReadPin(DCDC_ACTIVE_GPIO_Port, DCDC_ACTIVE_Pin) == GPIO_PIN_SET) {
                FSM_DcdcFaultCallback(g_shared.dcdc_irq_armed);
            }
            break;

        default:
            // Not a fault input. SUPP_ACTIVE also lands here: it is only checked by the FSM,
            // at startup and in HvConnect()
            break;
    }
}
