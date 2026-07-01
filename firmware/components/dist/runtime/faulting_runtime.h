#pragma once

#include <stdint.h>

/**
 * Bitmask of all fault sources. Multiple faults can be active simultaneously.
 * ISRs (interrupts) write to this register; the FSM reads and acts on it.
 */
typedef enum {
    FAULT_NONE             = 0,
    FAULT_DRD_FUSE         = (1U << 0),
    FAULT_MDI_FUSE         = (1U << 1),
    FAULT_SPARE_FUSE       = (1U << 2),
    FAULT_SPARE_CTRL_FUSE  = (1U << 3),
    FAULT_ESTOP            = (1U << 4),
} FaultSource_t;

/**
 * @brief Configure EXTI falling-edge interrupts on all eFuse FAULT pins and
 *        check their initial state to catch faults that asserted before init.
 *        Must be called after MX_GPIO_Init().
 */
void Fault_Init(void);

/**
 * @brief Set one or more fault bits. Safe to call from ISR context.
 * @param source Bitmask of FaultSource_t values to assert.
 */
void Fault_Set(FaultSource_t source);

/**
 * @brief Clear all fault bits. Use only from a deliberate recovery path.
 */
void Fault_Clear(void);

/** @return Non-zero if any fault bit is set. */
uint8_t Fault_Any(void);

/** @return Current fault register bitmask. */
FaultSource_t Fault_Get(void);
