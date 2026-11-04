/**
 * @file stm32f1xx_hal.h
 * @brief Test-only STM32 HAL declarations; unavailable to target builds.
 *
 * @author Tony Chen
 */

#ifndef TEST_STM32F1XX_HAL_H
#define TEST_STM32F1XX_HAL_H
#ifndef TEST
#error "The unit test HAL declarations must never be used in a target build"
#endif

/* INCLUDES */
#include <stddef.h>
#include <stdint.h>

/* DEFINES */
#define CAN_ID_STD 0x00000000U
#define CAN_RTR_DATA 0x00000000U
#define CAN_FILTER_FIFO0 0x00000000U
#define CAN_FILTERMODE_IDMASK 0x00000000U
#define CAN_FILTERSCALE_16BIT 0x00000000U
#define CAN_FILTER_ENABLE 0x00000001U
#define GPIOA (&test_gpio_a)
#define GPIOB (&test_gpio_b)
#define GPIOC (&test_gpio_c)
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

/* DATA TYPES */
/**
 * @brief Test-only HAL subset matching the vendored STM32F1 HAL declarations.
 *
 * Signatures, enums, RTC fields, and CAN filter fields match the vendored headers.
 * Handles and ports are identity tokens; no peripheral layout is simulated.
 * test_hal.c supplies the supported host operations; no hardware is accessed.
 */
typedef enum
{
    HAL_OK = 0,
    HAL_ERROR = 1,
    HAL_BUSY = 2,
    HAL_TIMEOUT = 3
} HAL_StatusTypeDef;
typedef enum
{
    GPIO_PIN_RESET = 0,
    GPIO_PIN_SET
} GPIO_PinState;
typedef struct
{
    uint32_t token;
} ADC_HandleTypeDef;
typedef struct
{
    uint32_t token;
} I2C_HandleTypeDef;
typedef struct
{
    uint32_t token;
} RTC_HandleTypeDef;
typedef struct
{
    uint32_t token;
} GPIO_TypeDef;
typedef struct
{
    uint32_t token;
} SPI_HandleTypeDef;
typedef struct
{
    uint32_t token;
} UART_HandleTypeDef;
typedef struct
{
    uint32_t token;
} CAN_HandleTypeDef;
typedef struct
{
    uint32_t FilterIdHigh, FilterIdLow, FilterMaskIdHigh, FilterMaskIdLow;
    uint32_t FilterFIFOAssignment, FilterBank, FilterMode, FilterScale;
    uint32_t FilterActivation, SlaveStartFilterBank;
} CAN_FilterTypeDef;
typedef enum
{
    DISABLE = 0,
    ENABLE = 1
} FunctionalState;
typedef struct
{
    uint32_t StdId, ExtId, IDE, RTR, DLC;
    FunctionalState TransmitGlobalTime;
} CAN_TxHeaderTypeDef;
typedef struct
{
    uint32_t StdId, ExtId, IDE, RTR, DLC, Timestamp, FilterMatchIndex;
} CAN_RxHeaderTypeDef;

typedef struct
{
    uint8_t Hours;
    uint8_t Minutes;
    uint8_t Seconds;
} RTC_TimeTypeDef;
typedef struct
{
    uint8_t WeekDay;
    uint8_t Month;
    uint8_t Date;
    uint8_t Year;
} RTC_DateTypeDef;

/* EXTERN VARIABLES */
extern GPIO_TypeDef test_gpio_a;
extern GPIO_TypeDef test_gpio_b;
extern GPIO_TypeDef test_gpio_c;

/* FUNCTION PROTOTYPES */
/**
 * @brief Return the stored tick, then advance it by tick_step.
 * @return Tick value before the configured increment.
 */
uint32_t HAL_GetTick(void);
/**
 * @brief Advance simulated time without sleeping.
 * @param Delay Delay in milliseconds.
 */
void HAL_Delay(uint32_t Delay);
/**
 * @brief Validate the ADC1 handle without accessing hardware.
 * @param hadc ADC1 handle.
 * @return HAL_OK.
 */
HAL_StatusTypeDef HAL_ADC_Start(ADC_HandleTypeDef* hadc);
/**
 * @brief Record the ADC poll timeout and return the configured result.
 * @param hadc ADC1 handle.
 * @param Timeout Requested timeout in milliseconds.
 * @return test_hal.adc_poll_status.
 */
HAL_StatusTypeDef HAL_ADC_PollForConversion(ADC_HandleTypeDef* hadc, uint32_t Timeout);
/**
 * @brief Count the ADC read and return the configured sample.
 * @param hadc ADC1 handle.
 * @return test_hal.adc_value.
 */
uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef* hadc);
/**
 * @brief Read the stored test GPIO level.
 * @param GPIOx Supported test GPIO port.
 * @param GPIO_Pin Pin mask to read.
 * @return GPIO_PIN_SET if any selected pin is high; otherwise GPIO_PIN_RESET.
 */
GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
/**
 * @brief Toggle the selected pins in stored GPIO state.
 * @param GPIOx Supported test GPIO port.
 * @param GPIO_Pin Pin mask to toggle.
 */
void HAL_GPIO_TogglePin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
/**
 * @brief Capture an I2C write, failing the test if the log or payload overflows.
 * @param hi2c I2C1 or I2C2 handle.
 * @param DevAddress Device address to record.
 * @param pData Payload bytes copied into the write log.
 * @param Size Payload size in bytes.
 * @param Timeout Requested timeout in milliseconds.
 * @return test_hal.i2c_status.
 */
HAL_StatusTypeDef HAL_I2C_Master_Transmit(
    I2C_HandleTypeDef* hi2c, uint16_t DevAddress, uint8_t* pData, uint16_t Size, uint32_t Timeout);
/**
 * @brief Set the selected pins in stored GPIO state.
 * @param GPIOx Supported test GPIO port.
 * @param GPIO_Pin Pin mask to update.
 * @param PinState Level to store.
 */
void HAL_GPIO_WritePin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState);
/**
 * @brief Store the supplied time and count the write.
 * @param hrtc Test RTC handle.
 * @param sTime Time structure used by the test RTC.
 * @param Format RTC_FORMAT_BIN; other formats fail the test.
 * @return HAL_OK.
 */
HAL_StatusTypeDef HAL_RTC_SetTime(RTC_HandleTypeDef* hrtc, RTC_TimeTypeDef* sTime, uint32_t Format);
/**
 * @brief Store the supplied date and count the write.
 * @param hrtc Test RTC handle.
 * @param sDate Date structure used by the test RTC.
 * @param Format RTC_FORMAT_BIN; other formats fail the test.
 * @return HAL_OK.
 */
HAL_StatusTypeDef HAL_RTC_SetDate(RTC_HandleTypeDef* hrtc, RTC_DateTypeDef* sDate, uint32_t Format);
/**
 * @brief Copy the stored time without advancing the RTC.
 * @param hrtc Test RTC handle.
 * @param sTime Time structure used by the test RTC.
 * @param Format RTC_FORMAT_BIN; other formats fail the test.
 * @return HAL_OK.
 */
HAL_StatusTypeDef HAL_RTC_GetTime(RTC_HandleTypeDef* hrtc, RTC_TimeTypeDef* sTime, uint32_t Format);
/**
 * @brief Copy the stored date without advancing the RTC.
 * @param hrtc Test RTC handle.
 * @param sDate Date structure used by the test RTC.
 * @param Format RTC_FORMAT_BIN; other formats fail the test.
 * @return HAL_OK.
 */
HAL_StatusTypeDef HAL_RTC_GetDate(RTC_HandleTypeDef* hrtc, RTC_DateTypeDef* sDate, uint32_t Format);
#endif
