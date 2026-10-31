#include "test_hal.h"
#include "unity.h"
#include <string.h>

TestHalState test_hal;
GPIO_TypeDef test_gpio_a = {.token = 1};
GPIO_TypeDef test_gpio_b = {.token = 2};
GPIO_TypeDef test_gpio_c = {.token = 3};
ADC_HandleTypeDef hadc1 = {.token = 1};
I2C_HandleTypeDef hi2c1 = {.token = 1}, hi2c2 = {.token = 2};
RTC_HandleTypeDef hrtc = {.token = 1};
static uint16_t pins[3];

static unsigned TestHalPortIndex(GPIO_TypeDef* port)
{
    if (port == GPIOA)
        return 0;
    if (port == GPIOB)
        return 1;
    if (port == GPIOC)
        return 2;
    TEST_FAIL_MESSAGE("Unsupported GPIO port in unit test support");
    return 0;
}

void TestHalReset(void)
{
    test_hal = (TestHalState){.i2c_status = HAL_OK};
    memset(pins, 0, sizeof(pins));
}

void TestHalSetPin(GPIO_TypeDef* port, uint16_t pin, GPIO_PinState state)
{
    unsigned index = TestHalPortIndex(port);
    if (state == GPIO_PIN_SET)
        pins[index] |= pin;
    else
        pins[index] &= (uint16_t)~pin;
}

GPIO_PinState TestHalGetPin(GPIO_TypeDef* port, uint16_t pin)
{
    return (pins[TestHalPortIndex(port)] & pin) ? GPIO_PIN_SET : GPIO_PIN_RESET;
}

uint32_t HAL_GetTick(void)
{
    uint32_t now = test_hal.tick;
    test_hal.tick += test_hal.tick_step;
    return now;
}
void HAL_Delay(uint32_t delay)
{
    test_hal.tick += delay;
    test_hal.delay_ms += delay;
    ++test_hal.delay_count;
}
GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef* port, uint16_t pin)
{
    return TestHalGetPin(port, pin);
}
void HAL_GPIO_WritePin(GPIO_TypeDef* port, uint16_t pin, GPIO_PinState state)
{
    TestHalSetPin(port, pin, state);
}
void HAL_GPIO_TogglePin(GPIO_TypeDef* port, uint16_t pin) { pins[TestHalPortIndex(port)] ^= pin; }

HAL_StatusTypeDef HAL_ADC_Start(ADC_HandleTypeDef* adc)
{
    TEST_ASSERT_EQUAL_PTR(&hadc1, adc);
    return HAL_OK;
}
HAL_StatusTypeDef HAL_ADC_PollForConversion(ADC_HandleTypeDef* adc, uint32_t timeout)
{
    TEST_ASSERT_EQUAL_PTR(&hadc1, adc);
    test_hal.adc_poll_timeout = timeout;
    return test_hal.adc_poll_status;
}
uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef* adc)
{
    TEST_ASSERT_EQUAL_PTR(&hadc1, adc);
    ++test_hal.adc_reads;
    return test_hal.adc_value;
}

HAL_StatusTypeDef HAL_I2C_Master_Transmit(
    I2C_HandleTypeDef* bus, uint16_t address, uint8_t* data, uint16_t size, uint32_t timeout)
{
    TEST_ASSERT_TRUE(bus == &hi2c1 || bus == &hi2c2);
    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_TRUE_MESSAGE(size <= TEST_I2C_MAX_BYTES, "Extend test I2C payload capacity");
    TEST_ASSERT_TRUE_MESSAGE(test_hal.i2c_count < TEST_I2C_CAPACITY, "Test I2C log overflow");
    TestI2cWrite* write = &test_hal.i2c[test_hal.i2c_count++];
    *write = (TestI2cWrite){.bus = bus, .address = address, .size = size, .timeout = timeout};
    memcpy(write->data, data, size);
    return test_hal.i2c_status;
}

// RTC state is explicitly controlled; it does not advance with the test tick.
HAL_StatusTypeDef HAL_RTC_SetTime(RTC_HandleTypeDef* rtc, RTC_TimeTypeDef* time, uint32_t format)
{
    TEST_ASSERT_EQUAL_PTR(&hrtc, rtc);
    TEST_ASSERT_EQUAL_UINT32(RTC_FORMAT_BIN, format);
    TEST_ASSERT_NOT_NULL(time);
    test_hal.rtc_time = *time;
    ++test_hal.rtc_time_writes;
    return HAL_OK;
}
HAL_StatusTypeDef HAL_RTC_SetDate(RTC_HandleTypeDef* rtc, RTC_DateTypeDef* date, uint32_t format)
{
    TEST_ASSERT_EQUAL_PTR(&hrtc, rtc);
    TEST_ASSERT_EQUAL_UINT32(RTC_FORMAT_BIN, format);
    TEST_ASSERT_NOT_NULL(date);
    test_hal.rtc_date = *date;
    ++test_hal.rtc_date_writes;
    return HAL_OK;
}
HAL_StatusTypeDef HAL_RTC_GetTime(RTC_HandleTypeDef* rtc, RTC_TimeTypeDef* time, uint32_t format)
{
    TEST_ASSERT_EQUAL_PTR(&hrtc, rtc);
    TEST_ASSERT_EQUAL_UINT32(RTC_FORMAT_BIN, format);
    TEST_ASSERT_NOT_NULL(time);
    *time = test_hal.rtc_time;
    return HAL_OK;
}
HAL_StatusTypeDef HAL_RTC_GetDate(RTC_HandleTypeDef* rtc, RTC_DateTypeDef* date, uint32_t format)
{
    TEST_ASSERT_EQUAL_PTR(&hrtc, rtc);
    TEST_ASSERT_EQUAL_UINT32(RTC_FORMAT_BIN, format);
    TEST_ASSERT_NOT_NULL(date);
    *date = test_hal.rtc_date;
    return HAL_OK;
}
