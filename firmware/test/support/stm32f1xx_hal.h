#ifndef SIL_STM32F1XX_HAL_H
#define SIL_STM32F1XX_HAL_H
#ifndef TEST
#error "The SIL HAL declarations must never be used in a target build"
#endif

#include <stddef.h>
#include <stdint.h>

// Test-only HAL subset. Signatures/enums/RTC and CAN filter fields match the
// vendored STM32F1 HAL headers. Handles and ports are identity tokens;
// no peripheral layout is simulated.
// test_hal.c supplies the supported host operations; no hardware is accessed.
typedef enum { HAL_OK = 0, HAL_ERROR = 1, HAL_BUSY = 2, HAL_TIMEOUT = 3 } HAL_StatusTypeDef;
typedef enum { GPIO_PIN_RESET = 0, GPIO_PIN_SET } GPIO_PinState;
typedef struct { uint32_t token; } ADC_HandleTypeDef;
typedef struct { uint32_t token; } I2C_HandleTypeDef;
typedef struct { uint32_t token; } RTC_HandleTypeDef;
typedef struct { uint32_t token; } GPIO_TypeDef;
typedef struct { uint32_t token; } SPI_HandleTypeDef;
typedef struct { uint32_t token; } UART_HandleTypeDef;
typedef struct { uint32_t token; } CAN_HandleTypeDef;
typedef struct {
    uint32_t FilterIdHigh, FilterIdLow, FilterMaskIdHigh, FilterMaskIdLow;
    uint32_t FilterFIFOAssignment, FilterBank, FilterMode, FilterScale;
    uint32_t FilterActivation, SlaveStartFilterBank;
} CAN_FilterTypeDef;
#define CAN_ID_STD 0x00000000U
#define CAN_RTR_DATA 0x00000000U
#define CAN_FILTER_FIFO0 0x00000000U
#define CAN_FILTERMODE_IDMASK 0x00000000U
#define CAN_FILTERSCALE_16BIT 0x00000000U
#define CAN_FILTER_ENABLE 0x00000001U
typedef enum { DISABLE = 0, ENABLE = 1 } FunctionalState;
typedef struct {
    uint32_t StdId, ExtId, IDE, RTR, DLC;
    FunctionalState TransmitGlobalTime;
} CAN_TxHeaderTypeDef;
typedef struct {
    uint32_t StdId, ExtId, IDE, RTR, DLC, Timestamp, FilterMatchIndex;
} CAN_RxHeaderTypeDef;
extern GPIO_TypeDef sil_gpio_a;
extern GPIO_TypeDef sil_gpio_b;
extern GPIO_TypeDef sil_gpio_c;
#define GPIOA (&sil_gpio_a)
#define GPIOC (&sil_gpio_c)
#define GPIOB (&sil_gpio_b)
#define GPIO_PIN_0 ((uint16_t)0x0001)
#define GPIO_PIN_1 ((uint16_t)0x0002)
#define GPIO_PIN_2 ((uint16_t)0x0004)
#define GPIO_PIN_3 ((uint16_t)0x0008)
#define GPIO_PIN_4 ((uint16_t)0x0010)
#define GPIO_PIN_5 ((uint16_t)0x0020)
#define GPIO_PIN_6 ((uint16_t)0x0040)
#define GPIO_PIN_7 ((uint16_t)0x0080)
#define GPIO_PIN_8 ((uint16_t)0x0100)
#define GPIO_PIN_9 ((uint16_t)0x0200)
#define GPIO_PIN_10 ((uint16_t)0x0400)
#define GPIO_PIN_11 ((uint16_t)0x0800)
#define GPIO_PIN_12 ((uint16_t)0x1000)
#define GPIO_PIN_13 ((uint16_t)0x2000)
#define HAL_MAX_DELAY 0xFFFFFFFFU
#define RTC_FORMAT_BIN 0x00000000U

typedef struct {
    uint8_t Hours;
    uint8_t Minutes;
    uint8_t Seconds;
} RTC_TimeTypeDef;
typedef struct {
    uint8_t WeekDay;
    uint8_t Month;
    uint8_t Date;
    uint8_t Year;
} RTC_DateTypeDef;

uint32_t HAL_GetTick(void);
void HAL_Delay(uint32_t Delay);
HAL_StatusTypeDef HAL_ADC_Start(ADC_HandleTypeDef *hadc);
HAL_StatusTypeDef HAL_ADC_PollForConversion(ADC_HandleTypeDef *hadc, uint32_t Timeout);
uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef *hadc);
GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);
void HAL_GPIO_TogglePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);
HAL_StatusTypeDef HAL_I2C_Master_Transmit(I2C_HandleTypeDef *hi2c, uint16_t DevAddress,
                                        uint8_t *pData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_I2C_Master_Receive(I2C_HandleTypeDef *hi2c, uint16_t DevAddress,
                                       uint8_t *pData, uint16_t Size, uint32_t Timeout);
void HAL_GPIO_WritePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState);
HAL_StatusTypeDef HAL_RTC_SetTime(RTC_HandleTypeDef *hrtc, RTC_TimeTypeDef *sTime, uint32_t Format);
HAL_StatusTypeDef HAL_RTC_SetDate(RTC_HandleTypeDef *hrtc, RTC_DateTypeDef *sDate, uint32_t Format);
HAL_StatusTypeDef HAL_RTC_GetTime(RTC_HandleTypeDef *hrtc, RTC_TimeTypeDef *sTime, uint32_t Format);
HAL_StatusTypeDef HAL_RTC_GetDate(RTC_HandleTypeDef *hrtc, RTC_DateTypeDef *sDate, uint32_t Format);
#endif
