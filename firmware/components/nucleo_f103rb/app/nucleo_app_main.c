#include "stm32f1xx.h"

#define NUCLEO_LD2_PIN GPIO_ODR_ODR5

static void BusyWait(uint32_t count)
{
    for (volatile uint32_t delay = 0; delay < count; delay++) {
    }
}

static void InitLd2(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;

    GPIOA->CRL &= ~(GPIO_CRL_MODE5 | GPIO_CRL_CNF5);
    GPIOA->CRL |= GPIO_CRL_MODE5_1;
    GPIOA->BRR = NUCLEO_LD2_PIN;
}

int main(void)
{
    InitLd2();

    for (;;) {
        GPIOA->ODR ^= NUCLEO_LD2_PIN;
        BusyWait(500000U);
    }
}
