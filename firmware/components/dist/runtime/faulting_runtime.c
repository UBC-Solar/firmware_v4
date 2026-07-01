#include "faulting_runtime.h"
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

    // NOTE: the eFuses are not yet enabled at this point (CTRL_Enable_All()
    // runs later, once ACTIVATE_CTRL is entered), so their FAULT sense lines
    // have no meaningful level here — an initial-state check would just latch
    // whatever the disabled/unpowered line happens to read. EXTI is already
    // armed above and will catch the real falling edge whenever an eFuse
    // actually trips, including right at power-up.
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
    // ESTOP is level-sensitive: poll every call so a release is also reflected.
    //
    // Critical section: this read-modify-write on fault_register is not atomic,
    // and the eFuse EXTI ISR (Fault_Set) writes the same shared register. Without
    // this guard, an EXTI firing between this function's load and store gets
    // silently overwritten by the stale value this function then stores back.
    HAL_NVIC_DisableIRQ(EXTI15_10_IRQn);
    if (ESTOP_Read() == GPIO_PIN_RESET)
    {
        fault_register |= FAULT_ESTOP;
    }
    else
    {
        fault_register &= ~(FaultSource_t)FAULT_ESTOP;
    }
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
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
