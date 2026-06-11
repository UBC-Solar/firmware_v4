#include "gpio_driver.h"
#include "stm32f1xx_hal_gpio.h"
#include "main.h"

/*============================================================================*/
/* GENERIC GPIO */

GPIO_PinState GPIO_Read(GPIO_TypeDef *port, uint16_t pin) {
    return HAL_GPIO_ReadPin(port, pin);
}

void GPIO_Write(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state) {
    HAL_GPIO_WritePin(port, pin, state);
}

void GPIO_TogglePin(GPIO_TypeDef *port, uint16_t pin) {
    HAL_GPIO_TogglePin(port, pin);
}

/*============================================================================*/
/* EFUSE CONTROL
 *
 * Each CTRL signal drives the EN pin of a TPS259631 eFuse. Driving high
 * enables the eFuse (output on); driving low disables it (output off).
 */

void SPARE_CTRL_Toggle(void) {
    GPIO_TogglePin(SPARE_CTRL_GPIO_Port, SPARE_CTRL_Pin);
}

void DRD_Toggle(void) {
    GPIO_TogglePin(DRD_CTRL_GPIO_Port, DRD_CTRL_Pin);
}

void MDI_CTRL_Toggle(void) {
    GPIO_TogglePin(MDI_CTRL_GPIO_Port, MDI_CTRL_Pin);
}

void CTRL_Enable_All(void) {
    GPIO_Write(SPARE_CTRL_GPIO_Port, SPARE_CTRL_Pin, GPIO_PIN_SET);
    GPIO_Write(MDI_CTRL_GPIO_Port,   MDI_CTRL_Pin,   GPIO_PIN_SET);
    GPIO_Write(DRD_CTRL_GPIO_Port,   DRD_CTRL_Pin,   GPIO_PIN_SET);
}

void CTRL_Disable_All(void) {
    GPIO_Write(SPARE_CTRL_GPIO_Port, SPARE_CTRL_Pin, GPIO_PIN_RESET);
    GPIO_Write(MDI_CTRL_GPIO_Port,   MDI_CTRL_Pin,   GPIO_PIN_RESET);
    GPIO_Write(DRD_CTRL_GPIO_Port,   DRD_CTRL_Pin,   GPIO_PIN_RESET);
}

/*============================================================================*/
/* EFUSE FAULT SENSE
 *
 * The TPS259631 FAULT pin is open-drain active-low. It asserts (goes low)
 * when the eFuse shuts off due to overcurrent or thermal protection.
 * Pins are configured as GPIO_NOPULL — bias is provided externally.
 */

GPIO_PinState DRD_FUSE_Read(void) {
    return GPIO_Read(DRD_FUSE_GPIO_Port, DRD_FUSE_Pin);
}

GPIO_PinState MDI_FUSE_Read(void) {
    return GPIO_Read(MDI_FUSE_GPIO_Port, MDI_FUSE_Pin);
}

GPIO_PinState SPARE_FUSE_Read(void) {
    return GPIO_Read(SPARE_FUSE_GPIO_Port, SPARE_FUSE_Pin);
}

GPIO_PinState SPARE_CTRL_FUSE_Read(void) {
    return GPIO_Read(SPARE_CTRL_FUSE_GPIO_Port, SPARE_CTRL_FUSE_Pin);
}

uint8_t Any_Fuse_Fault(void) {
    return (DRD_FUSE_Read()        == GPIO_PIN_RESET ||
            SPARE_FUSE_Read()      == GPIO_PIN_RESET ||
            SPARE_CTRL_FUSE_Read() == GPIO_PIN_RESET) ? 1 : 0;
}

/*============================================================================*/
/* DEBUG */

void DEBUG_LED_Set(uint8_t on) {
    GPIO_Write(DEBUG_GPIO_Port, DEBUG_Pin, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void DEBUG_LED_Toggle(void) {
    GPIO_TogglePin(DEBUG_GPIO_Port, DEBUG_Pin);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    switch (GPIO_Pin) {
        default:
            break;
    }
}
