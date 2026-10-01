#ifndef SIL_TEST_HAL_H
#define SIL_TEST_HAL_H
#include "stm32f1xx_hal.h"

// Small, inspectable host peripheral state, reset before every test.
#define TEST_I2C_CAPACITY 16
#define TEST_I2C_MAX_BYTES 128
typedef struct {
    I2C_HandleTypeDef *bus;
    uint16_t address, size;
    uint32_t timeout;
    uint8_t data[TEST_I2C_MAX_BYTES];
} TestI2cWrite;

typedef struct {
    TestI2cWrite transfer;
    HAL_StatusTypeDef status;
} TestI2cRead;

typedef struct {
    uint32_t tick;
    uint32_t tick_step;
    unsigned delay_count;
    uint32_t delay_ms;
    uint32_t adc_value;
    unsigned adc_reads;
    unsigned i2c_count;
    TestI2cWrite i2c[TEST_I2C_CAPACITY];
    HAL_StatusTypeDef i2c_status;
    unsigned i2c_read_count, i2c_read_queued;
    TestI2cRead i2c_reads[TEST_I2C_CAPACITY];
    RTC_TimeTypeDef rtc_time;
    RTC_DateTypeDef rtc_date;
    unsigned rtc_time_writes, rtc_date_writes;
} TestHalState;

extern TestHalState test_hal;
extern ADC_HandleTypeDef hadc1;
extern I2C_HandleTypeDef hi2c1, hi2c2;
extern RTC_HandleTypeDef hrtc;

void TestHalReset(void);
void TestHalSetPin(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state);
GPIO_PinState TestHalGetPin(GPIO_TypeDef *port, uint16_t pin);
// Queued responses are copied. A failed response may have NULL data.
void TestHalQueueI2cRead(I2C_HandleTypeDef *bus, uint16_t address,
                       const uint8_t *data, uint16_t size, HAL_StatusTypeDef status);
void TestHalVerifyI2cReads(void);
#endif
