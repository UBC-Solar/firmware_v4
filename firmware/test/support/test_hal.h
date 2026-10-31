/**
 * @file test_hal.h
 * @brief Shared peripheral state and helpers for host unit tests.
 */

#ifndef TEST_HAL_H
#define TEST_HAL_H
/* INCLUDES */
#include "stm32f1xx_hal.h"

/* DEFINES */
#define TEST_I2C_CAPACITY 16
#define TEST_I2C_MAX_BYTES 128

/* DATA TYPES */
/**
 * @brief Captured I2C write with a copied payload.
 */
typedef struct
{
    I2C_HandleTypeDef* bus;
    uint16_t address, size;
    uint32_t timeout;
    uint8_t data[TEST_I2C_MAX_BYTES];
} TestI2cWrite;

/**
 * @brief Peripheral inputs and captured outputs, reset before each test.
 */
typedef struct
{
    uint32_t tick;
    uint32_t tick_step;
    unsigned delay_count;
    uint32_t delay_ms;
    uint32_t adc_value;
    unsigned adc_reads;
    uint32_t adc_poll_timeout;         // Last requested ADC poll timeout in milliseconds.
    HAL_StatusTypeDef adc_poll_status; // ADC poll result; HAL_OK after reset.
    unsigned i2c_count;
    TestI2cWrite i2c[TEST_I2C_CAPACITY];
    HAL_StatusTypeDef i2c_status;
    RTC_TimeTypeDef rtc_time;
    RTC_DateTypeDef rtc_date;
    unsigned rtc_time_writes, rtc_date_writes;
} TestHalState;

/* EXTERN VARIABLES */
extern TestHalState test_hal;
extern ADC_HandleTypeDef hadc1;
extern I2C_HandleTypeDef hi2c1, hi2c2;
extern RTC_HandleTypeDef hrtc;

/* FUNCTION PROTOTYPES */
/**
 * @brief Clear peripheral state and logs; restore HAL_OK results.
 */
void TestHalReset(void);
/**
 * @brief Set a GPIO input or output level in the test state.
 * @param port Supported GPIO port (A, B, or C).
 * @param pin GPIO pin mask.
 * @param state Level to store for the selected pins.
 */
void TestHalSetPin(GPIO_TypeDef* port, uint16_t pin, GPIO_PinState state);
/**
 * @brief Read the stored GPIO level without accessing hardware.
 * @param port Supported GPIO port (A, B, or C).
 * @param pin GPIO pin mask.
 * @return GPIO_PIN_SET if any selected pin is high; otherwise GPIO_PIN_RESET.
 */
GPIO_PinState TestHalGetPin(GPIO_TypeDef* port, uint16_t pin);
#endif
