#ifndef STM32F1XX_HAL_H
#define STM32F1XX_HAL_H

#include <stdint.h>

typedef struct {
    volatile uint32_t DR1;
    volatile uint32_t DR2;
    volatile uint32_t DR3;
    volatile uint32_t DR4;
    volatile uint32_t DR5;
    volatile uint32_t DR6;
    volatile uint32_t DR7;
    volatile uint32_t DR8;
    volatile uint32_t DR9;
    volatile uint32_t DR10;
} BKP_TypeDef;

typedef struct {
    volatile uint32_t CSR;
} RCC_TypeDef;

extern BKP_TypeDef test_bkp_registers;
extern RCC_TypeDef test_rcc_registers;

#define BKP (&test_bkp_registers)
#define RCC (&test_rcc_registers)

#define RCC_CSR_RMVF      (1UL << 24U)
#define RCC_CSR_PINRSTF   (1UL << 26U)
#define RCC_CSR_PORRSTF   (1UL << 27U)
#define RCC_CSR_SFTRSTF   (1UL << 28U)
#define RCC_CSR_IWDGRSTF  (1UL << 29U)
#define RCC_CSR_WWDGRSTF  (1UL << 30U)
#define RCC_CSR_LPWRRSTF  (1UL << 31U)

void TestHalClearResetFlags(void);
void TestHalBackupWrite(volatile uint32_t *destination, uint16_t value);
void HAL_PWR_EnableBkUpAccess(void);
void HAL_Delay(uint32_t delay_ms);

#define __HAL_RCC_PWR_CLK_ENABLE() ((void)0)
#define __HAL_RCC_BKP_CLK_ENABLE() ((void)0)
#define __HAL_RCC_CLEAR_RESET_FLAGS() TestHalClearResetFlags()
#define __DSB() ((void)0)
#define SUNLITE_OTA_BKP_WRITE(destination, value) \
    TestHalBackupWrite((destination), (uint16_t)(value))

#endif /* STM32F1XX_HAL_H */
