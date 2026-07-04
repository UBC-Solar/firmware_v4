#pragma once

#include "stm32f1xx_hal.h"

/*============================================================================*/
/* GENERIC GPIO */

GPIO_PinState GPIO_Read(GPIO_TypeDef *port, uint16_t pin);
void GPIO_Write(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state);
void GPIO_TogglePin(GPIO_TypeDef *port, uint16_t pin);

/*============================================================================*/
/* EFUSE CONTROL — drive high to enable, low to disable */

void SPARE_MUX_CTRL_Toggle(void);
void SPARE_CTRL_Toggle(void);
void MDI_CTRL_Toggle(void);
void DRD_Toggle(void);

/** @brief Assert all three eFuse CTRL pins simultaneously. */
void CTRL_Enable_All(void);

/** @brief Deassert all three eFuse CTRL pins simultaneously. */
void CTRL_Disable_All(void);

/*============================================================================*/
/* EFUSE FAULT SENSE — reads active-low open-drain FAULT output of TPS259631 */

/** @return GPIO_PIN_RESET if the DRD eFuse has tripped, GPIO_PIN_SET otherwise. */
GPIO_PinState DRD_FUSE_Read(void);

/** @return GPIO_PIN_RESET if the MDI eFuse has tripped, GPIO_PIN_SET otherwise. */
GPIO_PinState MDI_FUSE_Read(void);

/** @return GPIO_PIN_RESET if the SPARE eFuse has tripped, GPIO_PIN_SET otherwise. */
GPIO_PinState SPARE_FUSE_Read(void);

/** @return GPIO_PIN_RESET if the SPARE_CTRL eFuse has tripped, GPIO_PIN_SET otherwise. */
GPIO_PinState SPARE_CTRL_FUSE_Read(void);

/** @return Current logic state of the MUX_STATUS input pin. */
GPIO_PinState MUX_STATUS_Read(void);

/** @return GPIO_PIN_RESET if ESTOP is asserted (active-low), GPIO_PIN_SET otherwise. */
GPIO_PinState ESTOP_Read(void);

/*============================================================================*/
/* DEBUG */

void DEBUG_LED_Set(uint8_t on);
void DEBUG_LED_Toggle(void);
