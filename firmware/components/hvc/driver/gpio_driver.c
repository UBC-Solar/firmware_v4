#include "gpio_driver.h"
#include "uart_driver.h"
#include "main.h"
#include "hvc_fsm.h"  

/**
 * @brief Read the current logic state of a GPIO pin.
 * @param port GPIO port instance (for example, GPIOA, GPIOB).
 * @param pin GPIO pin mask (for example, GPIO_PIN_5).
 * @return GPIO pin state as GPIO_PIN_SET or GPIO_PIN_RESET.
 */
GPIO_PinState GPIO_Read(GPIO_TypeDef *port, uint16_t pin) {
    return HAL_GPIO_ReadPin(port, pin);
}

/**
 * @brief Write a logic state to a GPIO pin.
 * @param port GPIO port instance (for example, GPIOA, GPIOB).
 * @param pin GPIO pin mask (for example, GPIO_PIN_5).
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
 * @brief HAL callback invoked on external interrupt line events.
 * @param GPIO_Pin The pin number that triggered the interrupt (for example, GPIO_PIN_5).
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    GPIO_PinState level;

    // Interrupts are triggered on rising or falling edges, so we read the pin state to determine the current level

    switch (GPIO_Pin) {
        case ESTOP_Pin:
            level = HAL_GPIO_ReadPin(ESTOP_GPIO_Port, ESTOP_Pin);
            if (level == GPIO_PIN_SET) {
                // ESTOP is high (rising edge or already high when sampled)
                ESTOPCallback();
            } else {
                // ESTOP is low (falling edge or already low when sampled)
            }
            break;

        case IMD_GPIO_IN_Pin:
            level = HAL_GPIO_ReadPin(IMD_GPIO_IN_GPIO_Port, IMD_GPIO_IN_Pin);
            if (level == GPIO_PIN_SET) {
                UART_Printf("High!\n\r");
                IMDFaultCallback();// IMD_GPIO_IN is high
            } else {
                UART_Printf("Low!\n\r");
                // IMD_GPIO_IN is low
            }
            break;

        case MASTERBOARD_FAULT_Pin:
            level = HAL_GPIO_ReadPin(MASTERBOARD_FAULT_GPIO_Port, MASTERBOARD_FAULT_Pin);
            if (level == GPIO_PIN_SET) {
                MasterboardFaultCallback(true);// MASTERBOARD_FAULT is high
            } else {
                MasterboardFaultCallback(false);// MASTERBOARD_FAULT is low
            }
            break;

        case DCDC_ACTIVE_Pin:
            level = HAL_GPIO_ReadPin(DCDC_ACTIVE_GPIO_Port, DCDC_ACTIVE_Pin);
            if (level == GPIO_PIN_SET) {
                // DCDC_ACTIVE is high
            } else {
                DCDCFaultCallback(); // DCDC_ACTIVE is low
            }
            break;

        //case SUPP_ACTIVE_Pin:
            //level = HAL_GPIO_ReadPin(SUPP_ACTIVE_GPIO_Port, SUPP_ACTIVE_Pin);
            //if (level == GPIO_PIN_SET) {
                // SUPP_ACTIVE is high
            //} else {
                // SUPP_ACTIVE is low
            //}
            //break;

        case HV_CURRENT_ALERT_Pin:
            level = HAL_GPIO_ReadPin(HV_CURRENT_ALERT_GPIO_Port, HV_CURRENT_ALERT_Pin);
            if (level == GPIO_PIN_SET) {
                HVCurrentAlertCallback();// HV_CURRENT_ALERT is high
            } else {
                // HV_CURRENT_ALERT is low
            }
            break;

        default:
            break;
    }
}