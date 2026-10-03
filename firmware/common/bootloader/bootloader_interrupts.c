#include "stm32f1xx_hal.h"

typedef struct {
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr;
    uint32_t pc;
    uint32_t psr;
    uint32_t cfsr;
    uint32_t hfsr;
    uint32_t mmfar;
    uint32_t bfar;
} bootloader_fault_context_t;

volatile bootloader_fault_context_t bootloader_fault_context;

void BootloaderHardFaultCapture(uint32_t *fault_stack)
{
    bootloader_fault_context.r0 = fault_stack[0];
    bootloader_fault_context.r1 = fault_stack[1];
    bootloader_fault_context.r2 = fault_stack[2];
    bootloader_fault_context.r3 = fault_stack[3];
    bootloader_fault_context.r12 = fault_stack[4];
    bootloader_fault_context.lr = fault_stack[5];
    bootloader_fault_context.pc = fault_stack[6];
    bootloader_fault_context.psr = fault_stack[7];
    bootloader_fault_context.cfsr = SCB->CFSR;
    bootloader_fault_context.hfsr = SCB->HFSR;
    bootloader_fault_context.mmfar = SCB->MMFAR;
    bootloader_fault_context.bfar = SCB->BFAR;

    while (1) {
    }
}

void NMI_Handler(void)
{
    while (1) {
    }
}

__attribute__((naked)) void HardFault_Handler(void)
{
    __asm volatile(
        "tst lr, #4\n"
        "ite eq\n"
        "mrseq r0, msp\n"
        "mrsne r0, psp\n"
        "b BootloaderHardFaultCapture\n");
}

void MemManage_Handler(void)
{
    while (1) {
    }
}

void BusFault_Handler(void)
{
    while (1) {
    }
}

void UsageFault_Handler(void)
{
    while (1) {
    }
}

void SVC_Handler(void)
{
}

void DebugMon_Handler(void)
{
}

void PendSV_Handler(void)
{
}

void SysTick_Handler(void)
{
    HAL_IncTick();
}
