#include "unity.h"
#include "imu_app.h"
#include "imu_driver.h"
#include "can_driver.h"
#include "main.h"
#include "test_hal.h"
#include "test_tel_services.h"

void setUp(void)
{
    TestHalReset();
    TestTelServicesReset();
    TestHalSetPin(I_INTN_GPIO_Port, I_INTN_Pin, GPIO_PIN_RESET);
}

void tearDown(void) { TestHalVerifyI2cReads(); }

void test_imu_packet_is_decoded_and_published_to_can_and_telemetry(void)
{
    // The sensor returns the SHTP header, then the complete packet on a reread.
    const uint8_t boot[] = {4, 0, 0, 0};
    TestHalQueueI2cRead(&hi2c1, IMU_ADDRESS, boot, sizeof(boot), HAL_OK);
    TestHalQueueI2cRead(&hi2c1, IMU_ADDRESS, boot, sizeof(boot), HAL_OK);
    ImuAppInit();

    TEST_ASSERT_EQUAL(GPIO_PIN_SET, TestHalGetPin(I_BOOTN_GPIO_Port, I_BOOTN_Pin));
    TEST_ASSERT_EQUAL(GPIO_PIN_SET, TestHalGetPin(I_NRST_GPIO_Port, I_NRST_Pin));
    TEST_ASSERT_EQUAL_UINT(1, test_hal.delay_count);
    TEST_ASSERT_EQUAL_UINT32(10, test_hal.delay_ms);
    TEST_ASSERT_EQUAL_UINT(3, test_hal.i2c_count);
    const uint8_t reports[] = {SH2_REPORT_ACCEL, SH2_REPORT_GYRO, SH2_REPORT_MAG};
    for (unsigned i = 0; i < 3; ++i) {
        TEST_ASSERT_EQUAL_PTR(&hi2c1, test_hal.i2c[i].bus);
        TEST_ASSERT_EQUAL_UINT16(IMU_ADDRESS, test_hal.i2c[i].address);
        TEST_ASSERT_EQUAL_UINT16(21, test_hal.i2c[i].size);
        TEST_ASSERT_EQUAL_HEX8(SH2_REPORT_SET_FEAATURE_CMD, test_hal.i2c[i].data[4]);
        TEST_ASSERT_EQUAL_HEX8(reports[i], test_hal.i2c[i].data[5]);
    }

    // One report for each sensor: x = 1, y = -2, z = 0.5 in its output units.
    // Fixed-point input formats: acceleration Q8, gyro Q9, magnetometer Q4.
    const uint8_t packet[] = {
        34, 0, SHTP_CHANNEL_REPORTS, 0,
        SH2_REPORT_ACCEL, 0, 3, 0, 0x00, 0x01, 0x00, 0xFE, 0x80, 0x00,
        SH2_REPORT_GYRO,  0, 3, 0, 0x00, 0x02, 0x00, 0xFC, 0x00, 0x01,
        SH2_REPORT_MAG,   0, 3, 0, 0x10, 0x00, 0xE0, 0xFF, 0x08, 0x00,
    };
    TestHalQueueI2cRead(&hi2c1, IMU_ADDRESS, packet, 4, HAL_OK);
    TestHalQueueI2cRead(&hi2c1, IMU_ADDRESS, packet, sizeof(packet), HAL_OK);
    ImuAppTask(); // One real task iteration, not the infinite RTOS wrapper.

    TEST_ASSERT_EQUAL_UINT(4, test_hal.i2c_read_count);
    TEST_ASSERT_EQUAL_UINT(6, test_tel.can_count);
    TEST_ASSERT_EQUAL_UINT(6, test_tel.telemetry_count);
    TEST_ASSERT_EQUAL_UINT(12, test_tel.delay_count);
    TEST_ASSERT_EQUAL_UINT32(24, test_tel.delay_ticks);
    const uint8_t values[][4] = {
        {0x00, 0x00, 0x80, 0x3F}, // IEEE754 little-endian 1.0
        {0x00, 0x00, 0x00, 0xC0}, // -2.0
        {0x00, 0x00, 0x00, 0x3F}, // 0.5
    };
    for (unsigned i = 0; i < 6; ++i) {
        const CAN_comms_Tx_msg_t *can = &test_tel.can_messages[i];
        const CAN_comms_Tx_msg_t *telemetry = &test_tel.telemetry_messages[i];
        // Capture the existing firmware headers; physical CAN validity is separate.
        TEST_ASSERT_EQUAL_HEX32(0x800 + i, can->header.StdId);
        TEST_ASSERT_EQUAL_UINT32(CAN_ID_STD, can->header.IDE);
        TEST_ASSERT_EQUAL_UINT32(CAN_RTR_DATA, can->header.RTR);
        TEST_ASSERT_EQUAL_UINT32(i < 3 ? 8 : 4, can->header.DLC);
        TEST_ASSERT_EQUAL_HEX8_ARRAY(values[i % 3], can->data, 4);
        if (i < 3) TEST_ASSERT_EQUAL_HEX8_ARRAY(values[i], can->data + 4, 4);
        TEST_ASSERT_EQUAL_UINT32(can->header.StdId, telemetry->header.StdId);
        TEST_ASSERT_EQUAL_UINT32(can->header.IDE, telemetry->header.IDE);
        TEST_ASSERT_EQUAL_UINT32(can->header.DLC, telemetry->header.DLC);
        TEST_ASSERT_EQUAL_HEX8_ARRAY(can->data, telemetry->data, can->header.DLC);
    }
}
