#include "unity.h"
#include "mdi_driver.h"
#include "main.h"
#include "test_hal.h"

void setUp(void) { TestHalReset(); }
void tearDown(void) {}

void test_motor_command_decodes_and_drives_dacs_and_direction(void)
{
    uint8_t payload[] = {0x23, 0x01, 0x01, 0x00, 0x01};
    MdiMotorCommand command = {0};
    MdiParseMotorCommand(payload, &command);
    MdiSetMotorCommand(&command);

    TEST_ASSERT_EQUAL_UINT16(0x123, command.accel_DAC_value);
    TEST_ASSERT_EQUAL_UINT16(1, command.regen_DAC_value);
    TEST_ASSERT_EQUAL_UINT(2, test_hal.i2c_count);
    TEST_ASSERT_EQUAL_PTR(&hi2c2, test_hal.i2c[0].bus);
    TEST_ASSERT_EQUAL_PTR(&hi2c1, test_hal.i2c[1].bus);
    uint8_t accel_bytes[] = {0x04, 0x8C};
    uint8_t regen_bytes[] = {0x00, 0x04};
    for (unsigned i = 0; i < 2; ++i) {
        TEST_ASSERT_EQUAL_UINT16(MDI_DAC7571_WRITE_ADDR, test_hal.i2c[i].address);
        TEST_ASSERT_EQUAL_UINT16(2, test_hal.i2c[i].size);
        TEST_ASSERT_EQUAL_UINT32(HAL_MAX_DELAY, test_hal.i2c[i].timeout);
    }
    TEST_ASSERT_EQUAL_HEX8_ARRAY(accel_bytes, test_hal.i2c[0].data, 2);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(regen_bytes, test_hal.i2c[1].data, 2);
    TEST_ASSERT_EQUAL(GPIO_PIN_SET, TestHalGetPin(DIR_GPIO_Port, DIR_Pin));
    TEST_ASSERT_EQUAL(GPIO_PIN_RESET, TestHalGetPin(ECO_MCU_GPIO_Port, ECO_MCU_Pin));
}
