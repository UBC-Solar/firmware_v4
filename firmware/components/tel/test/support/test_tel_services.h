#ifndef TEST_TEL_SERVICES_H
#define TEST_TEL_SERVICES_H
#include "CAN_comms.h"

#define TEST_TEL_MESSAGE_CAPACITY 16
typedef struct {
    CAN_comms_Tx_msg_t can_messages[TEST_TEL_MESSAGE_CAPACITY];
    CAN_comms_Tx_msg_t telemetry_messages[TEST_TEL_MESSAGE_CAPACITY];
    unsigned can_count, telemetry_count;
    unsigned delay_count;
    uint32_t delay_ticks;
} TestTelServices;

extern TestTelServices test_tel;
void TestTelServicesReset(void);
#endif
