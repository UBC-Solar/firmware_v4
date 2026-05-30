#pragma once

#include "stm32f1xx_hal.h"

GPIO_PinState GPIO_Read(GPIO_TypeDef *port, uint16_t pin);
void GPIO_Write(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state);
void GPIO_TogglePin(GPIO_TypeDef *port, uint16_t pin);

void SPARE_MUX_CTRL_Toggle(void);
void SPARE_CTRL_Toggle(void);
void DRD_Toggle(void);

void MDI_CTRL_Toggle(void);

GPIO_PinState DRD_FUSE_Read(void);
GPIO_PinState MDI_FUSE_Read(void);
GPIO_PinState SPARE_FUSE_Read(void);
GPIO_PinState SPARE_CTRL_FUSE_Read(void);