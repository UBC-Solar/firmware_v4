#include "stdint.h"
#include "core_cm4.h"

__attribute__((naked)) void HardFault_Entry(void)
{
    __asm volatile(
        "tst lr, #4      \n"
        "ite eq          \n"
        "mrseq r0, msp   \n"
        "mrsne r0, psp   \n"
        "b HardFault_C   \n"
    );
}

void HardFault_C(uint32_t *stack)
{
    volatile uint32_t pc = stack[6];
    volatile uint32_t lr = stack[5];
    volatile uint32_t cfsr = SCB->CFSR;
    volatile uint32_t hfsr = SCB->HFSR;
    volatile uint32_t bfar = SCB->BFAR;
    volatile uint32_t mmfar = SCB->MMFAR;
    __asm("BKPT #0");
    while (1) {}
}