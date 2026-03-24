/**
 * @file system_fault_handler_driver.h
 * @brief HardFault handler interface for Cortex-M microcontrollers.
 */

#ifndef SYSTEM_FAULT_HANDLER_DRIVER_H
#define SYSTEM_FAULT_HANDLER_DRIVER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Entry point for the HardFault exception handler (naked function).
 *
 * This function is marked as naked to allow custom assembly for stack frame extraction.
 * It determines the correct stack pointer and passes it to HardFault_C().
 */
__attribute__((naked)) void HardFault_Entry(void);

/**
 * @brief C handler for HardFault exceptions.
 *
 * This function receives a pointer to the stack at the time of the fault and can be used
 * to extract register values and debug the cause of the fault.
 *
 * @param stack Pointer to the stack frame at the time of the HardFault.
 */
void HardFault_C(uint32_t *stack);

#ifdef __cplusplus
}
#endif

#endif // SYSTEM_FAULT_HANDLER_DRIVER_H
