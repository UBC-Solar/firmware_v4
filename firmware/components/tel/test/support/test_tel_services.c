#include "test_tel_services.h"
#include "telemetry_app.h"
#include "cmsis_os2.h"
#include "test_hal.h"
#include "unity.h"

// The real can_driver.c supplies the CAN headers and filter configuration.
CAN_HandleTypeDef hcan = {.token = 1};
TestTelServices test_tel;

void TestTelServicesReset(void)
{
    test_tel = (TestTelServices){0};
}

static void capture(CAN_comms_Tx_msg_t *messages, unsigned *count,
                    const CAN_comms_Tx_msg_t *message)
{
    TEST_ASSERT_NOT_NULL(message);
    TEST_ASSERT_TRUE_MESSAGE(message->header.DLC <= CAN_DATA_SIZE, "Invalid CAN payload length");
    TEST_ASSERT_TRUE_MESSAGE(*count < TEST_TEL_MESSAGE_CAPACITY, "TEL message log overflow");
    messages[(*count)++] = *message;
}

void CAN_comms_Add_Tx_message(CAN_comms_Tx_msg_t *message)
{
    capture(test_tel.can_messages, &test_tel.can_count, message);
}

void TelAppTransmitInternalMsg(CAN_comms_Tx_msg_t *message)
{
    capture(test_tel.telemetry_messages, &test_tel.telemetry_count, message);
}

osStatus_t osDelay(uint32_t ticks)
{
    if (ticks == 0) return osErrorParameter;
    ++test_tel.delay_count;
    test_tel.delay_ticks += ticks;
    // TEL configTICK_RATE_HZ is 1000. Advance time without a scheduler or sleep.
    test_hal.tick += ticks;
    return osOK;
}
