#include "fault_handler.h"
#include "gpio_driver.h"
#include "main.h"
#include "stm32f1xx_hal.h"

/*============================================================================*/
/* PRIVATE VARIABLES */

static volatile FaultSource_t fault_register = FAULT_NONE;

/*============================================================================*/
/* PUBLIC FUNCTIONS */

void Fault_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Reconfigure eFuse FAULT sense pins as falling-edge EXTI inputs.
    // Falling edge = FAULT asserted (TPS259631 open-drain active-low output).
    // All four pins are on GPIOB (PB12-PB15) → EXTI15_10_IRQn.
    GPIO_InitStruct.Pin  = DRD_FUSE_Pin | MDI_FUSE_Pin | SPARE_CTRL_FUSE_Pin | SPARE_FUSE_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(DRD_FUSE_GPIO_Port, &GPIO_InitStruct);

    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

    // Catch faults that were already asserted before EXTI was enabled.
    // A falling edge on a pin that is already low will not re-trigger EXTI.
    if (DRD_FUSE_Read()        == GPIO_PIN_RESET) Fault_Set(FAULT_DRD_FUSE);
    if (MDI_FUSE_Read()        == GPIO_PIN_RESET) Fault_Set(FAULT_MDI_FUSE);
    if (SPARE_FUSE_Read()      == GPIO_PIN_RESET) Fault_Set(FAULT_SPARE_FUSE);
    if (SPARE_CTRL_FUSE_Read() == GPIO_PIN_RESET) Fault_Set(FAULT_SPARE_CTRL_FUSE);
}

void Fault_Set(FaultSource_t source)
{
    fault_register |= source;
}

void Fault_Clear(void)
{
    fault_register = FAULT_NONE;
}

uint8_t Fault_Any(void)
{
    return (fault_register != FAULT_NONE) ? 1U : 0U;
}

FaultSource_t Fault_Get(void)
{
    return fault_register;
}

/*============================================================================*/
/* ISR CALLBACK */

// HAL weak override. Called by HAL_GPIO_EXTI_IRQHandler for the triggering pin.
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    switch (GPIO_Pin)
    {
        case DRD_FUSE_Pin:        Fault_Set(FAULT_DRD_FUSE);        break;
        case MDI_FUSE_Pin:        Fault_Set(FAULT_MDI_FUSE);        break;
        case SPARE_FUSE_Pin:      Fault_Set(FAULT_SPARE_FUSE);      break;
        case SPARE_CTRL_FUSE_Pin: Fault_Set(FAULT_SPARE_CTRL_FUSE); break;
        default:                                                     break;
    }
}
