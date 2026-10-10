#include "fw_update_protocol.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#define BUS_CAPACITY 512U

typedef struct
{
    uint32_t id;
    uint8_t data[8];
    uint8_t dlc;
} QueuedCanFrame;

typedef struct
{
    QueuedCanFrame frames[BUS_CAPACITY];
    size_t head;
    size_t tail;
    size_t count;
    size_t transmitted_count;
    size_t flow_control_count;
} TestBus;

typedef struct
{
    TestBus* bus;
    unsigned reject_count;
} TestSender;

typedef struct
{
    uint32_t id;
    uint8_t data[8];
    uint8_t dlc;
    size_t count;
} Capture;

static bool QueueCanFrame(void* user, uint32_t extended_id, const uint8_t data[8], uint8_t dlc)
{
    TestSender* sender = user;
    if (sender->reject_count > 0U)
    {
        sender->reject_count--;
        return false;
    }
    TestBus* bus = sender->bus;
    assert(bus->count < BUS_CAPACITY);
    QueuedCanFrame* frame = &bus->frames[bus->tail];
    frame->id = extended_id;
    frame->dlc = dlc;
    memcpy(frame->data, data, sizeof(frame->data));
    bus->tail = (bus->tail + 1U) % BUS_CAPACITY;
    bus->count++;
    bus->transmitted_count++;
    if ((data[0] & 0xF0U) == 0x30U)
    {
        bus->flow_control_count++;
    }
    return true;
}

static bool CaptureCanFrame(void* user, uint32_t extended_id, const uint8_t data[8], uint8_t dlc)
{
    Capture* capture = user;
    capture->id = extended_id;
    capture->dlc = dlc;
    memcpy(capture->data, data, sizeof(capture->data));
    capture->count++;
    return true;
}

static bool RejectCanFrame(void* user, uint32_t extended_id, const uint8_t data[8], uint8_t dlc)
{
    (void)user;
    (void)extended_id;
    (void)data;
    (void)dlc;
    return false;
}

static bool PopCanFrame(TestBus* bus, QueuedCanFrame* frame)
{
    if (bus->count == 0U)
    {
        return false;
    }
    *frame = bus->frames[bus->head];
    bus->head = (bus->head + 1U) % BUS_CAPACITY;
    bus->count--;
    return true;
}

static uint32_t DrivePair(FirmwareUpdateCanTransport* first,
                          FirmwareUpdateCanTransport* second,
                          TestBus* bus,
                          uint32_t now_ms,
                          uint32_t maximum_iterations)
{
    for (uint32_t iteration = 0U; iteration < maximum_iterations; iteration++, now_ms++)
    {
        FirmwareUpdateCanPoll(first, now_ms);
        FirmwareUpdateCanPoll(second, now_ms);

        QueuedCanFrame frame;
        if (!PopCanFrame(bus, &frame))
        {
            continue;
        }
        if (frame.id == first->receive_id)
        {
            (void)FirmwareUpdateCanOnFrame(first, frame.id, frame.data, frame.dlc, now_ms);
        }
        else if (frame.id == second->receive_id)
        {
            (void)FirmwareUpdateCanOnFrame(second, frame.id, frame.data, frame.dlc, now_ms);
        }
        else
        {
            assert(!"test bus emitted an unaddressed frame");
        }
    }
    return now_ms;
}

/**
 * @brief Builds a maximum-size DATA message containing all byte values.
 */
static size_t BuildMaximumDataFrame(uint8_t* encoded, size_t encoded_capacity)
{
    uint8_t payload[FW_UPDATE_MAX_RX_PAYLOAD];
    FirmwareUpdateWriteBe32(payload, 0x00020000U);
    for (size_t index = FW_UPDATE_DATA_OFFSET_SIZE; index < sizeof(payload); index++)
    {
        payload[index] = (uint8_t)((index * 37U) & 0xFFU);
    }
    size_t length = FirmwareUpdateFrameEncode(FW_UPDATE_MESSAGE_DATA,
                                              0x12345678U,
                                              0x89ABCDEFU,
                                              payload,
                                              sizeof(payload),
                                              encoded,
                                              encoded_capacity);
    assert(length == FW_UPDATE_MAX_FRAME_SIZE);
    return length;
}

static void AssertReceivedFrame(FirmwareUpdateCanTransport* transport,
                                const uint8_t* expected,
                                size_t expected_length)
{
    const uint8_t* received = NULL;
    size_t received_length = 0U;
    assert(FirmwareUpdateCanPeekFrame(transport, &received, &received_length));
    assert(expected_length > 0U);
    assert(received_length == expected_length);
    assert(memcmp(received, expected, expected_length) == 0);
}

static void TestAddressing(void)
{
    assert(FirmwareUpdateCanAddressedId(FW_UPDATE_CAN_NODE_MDI, FW_UPDATE_CAN_TESTER_ADDRESS) ==
           0x18DA10F1U);
    assert(FirmwareUpdateCanAddressedId(FW_UPDATE_CAN_TESTER_ADDRESS, FW_UPDATE_CAN_NODE_MDI) ==
           0x18DAF110U);
}

static void TestGoldenHelloRoundTrip(void)
{
    static const uint8_t expected[] = {
        0x53,
        0x55,
        0x03,
        0x01,
        0x12,
        0x34,
        0x56,
        0x78,
        0x00,
        0x00,
        0x00,
        0x01,
        0x00,
        0x00,
        0x35,
        0xEE,
        0xD0,
        0xE2,
    };
    TestBus bus = {0};
    TestSender tester_sender = {.bus = &bus};
    TestSender target_sender = {.bus = &bus};
    FirmwareUpdateCanTransport tester;
    FirmwareUpdateCanTransport target;
    FirmwareUpdateCanInit(&tester,
                          FW_UPDATE_CAN_TESTER_ADDRESS,
                          FW_UPDATE_CAN_NODE_MDI,
                          QueueCanFrame,
                          &tester_sender,
                          250U);
    FirmwareUpdateCanInit(&target,
                          FW_UPDATE_CAN_NODE_MDI,
                          FW_UPDATE_CAN_TESTER_ADDRESS,
                          QueueCanFrame,
                          &target_sender,
                          250U);

    assert(FirmwareUpdateCanSendFrame(&tester, expected, sizeof(expected), 0U));
    (void)DrivePair(&tester, &target, &bus, 0U, 100U);
    assert(!FirmwareUpdateCanTxBusy(&tester));
    AssertReceivedFrame(&target, expected, sizeof(expected));

    const uint8_t* received = NULL;
    size_t received_length = 0U;
    FirmwareUpdateMessage message;
    assert(FirmwareUpdateCanPeekFrame(&target, &received, &received_length));
    assert(FirmwareUpdateFrameDecode(received, received_length, &message));
    assert(message.type == FW_UPDATE_MESSAGE_HELLO);
    assert(message.session_id == 0x12345678U);
    FirmwareUpdateCanConsumeFrame(&target);
    assert(target.receive_state == FW_UPDATE_CAN_RX_IDLE);
}

static void TestMaximumDataRoundTrip(void)
{
    uint8_t expected[FW_UPDATE_MAX_FRAME_SIZE];
    size_t expected_length = BuildMaximumDataFrame(expected, sizeof(expected));
    assert(expected_length == FW_UPDATE_MAX_FRAME_SIZE);

    TestBus bus = {0};
    TestSender tester_sender = {.bus = &bus};
    TestSender target_sender = {.bus = &bus};
    FirmwareUpdateCanTransport tester;
    FirmwareUpdateCanTransport target;
    FirmwareUpdateCanInit(&tester,
                          FW_UPDATE_CAN_TESTER_ADDRESS,
                          FW_UPDATE_CAN_NODE_DRD,
                          QueueCanFrame,
                          &tester_sender,
                          250U);
    FirmwareUpdateCanInit(&target,
                          FW_UPDATE_CAN_NODE_DRD,
                          FW_UPDATE_CAN_TESTER_ADDRESS,
                          QueueCanFrame,
                          &target_sender,
                          250U);
    assert(FirmwareUpdateCanSendFrame(&tester, expected, expected_length, 1000U));
    (void)DrivePair(&tester, &target, &bus, 1000U, 1000U);
    assert(!FirmwareUpdateCanTxBusy(&tester));
    assert(bus.flow_control_count > 1U);
    AssertReceivedFrame(&target, expected, expected_length);

    const uint8_t* received = NULL;
    size_t received_length = 0U;
    FirmwareUpdateMessage message;
    assert(FirmwareUpdateCanPeekFrame(&target, &received, &received_length));
    assert(FirmwareUpdateFrameDecode(received, received_length, &message));
    assert(message.type == FW_UPDATE_MESSAGE_DATA);
    assert(message.payload_length == FW_UPDATE_MAX_RX_PAYLOAD);
    assert(FirmwareUpdateReadBe32(message.payload) == 0x00020000U);
}

static void TestBackpressure(void)
{
    static const uint8_t frame[] = {1U, 2U, 3U};
    TestBus bus = {0};
    TestSender sender = {.bus = &bus, .reject_count = 2U};
    FirmwareUpdateCanTransport transport;
    FirmwareUpdateCanInit(&transport,
                          FW_UPDATE_CAN_TESTER_ADDRESS,
                          FW_UPDATE_CAN_NODE_STR,
                          QueueCanFrame,
                          &sender,
                          20U);
    assert(FirmwareUpdateCanSendFrame(&transport, frame, sizeof(frame), 0U));
    assert(FirmwareUpdateCanTxBusy(&transport));
    assert(!FirmwareUpdateCanSendFrame(&transport, frame, sizeof(frame), 0U));
    assert(transport.last_error == FW_UPDATE_CAN_ERROR_BUSY);
    FirmwareUpdateCanPoll(&transport, 1U);
    assert(FirmwareUpdateCanTxBusy(&transport));
    FirmwareUpdateCanPoll(&transport, 2U);
    assert(!FirmwareUpdateCanTxBusy(&transport));
    assert(bus.count == 1U);
}

static void TestWrongIdAndSequenceFailure(void)
{
    Capture capture = {0};
    FirmwareUpdateCanTransport target;
    FirmwareUpdateCanInit(&target,
                          FW_UPDATE_CAN_NODE_MDI,
                          FW_UPDATE_CAN_TESTER_ADDRESS,
                          CaptureCanFrame,
                          &capture,
                          20U);

    uint8_t first[8] = {0x10U, 0x08U, 1U, 2U, 3U, 4U, 5U, 6U};
    assert(FirmwareUpdateCanOnFrame(&target, 0x18DA11F1U, first, sizeof(first), 0U) ==
           FW_UPDATE_CAN_RX_IGNORED);
    assert(target.receive_state == FW_UPDATE_CAN_RX_IDLE);
    assert(FirmwareUpdateCanOnFrame(&target, target.receive_id, first, sizeof(first), 0U) ==
           FW_UPDATE_CAN_RX_ACCEPTED);
    assert(capture.count == 1U);
    assert((capture.data[0] & 0x0FU) == 0U);

    uint8_t wrong_sequence[8] = {0x22U, 7U, 8U, 0U, 0U, 0U, 0U, 0U};
    assert(FirmwareUpdateCanOnFrame(
               &target, target.receive_id, wrong_sequence, sizeof(wrong_sequence), 1U) ==
           FW_UPDATE_CAN_RX_ERROR);
    assert(target.last_error == FW_UPDATE_CAN_ERROR_SEQUENCE);
    assert(target.receive_state == FW_UPDATE_CAN_RX_IDLE);
}

static void TestTimeouts(void)
{
    Capture capture = {0};
    FirmwareUpdateCanTransport sender;
    FirmwareUpdateCanInit(&sender,
                          FW_UPDATE_CAN_TESTER_ADDRESS,
                          FW_UPDATE_CAN_NODE_MDI,
                          CaptureCanFrame,
                          &capture,
                          10U);
    uint8_t frame[16];
    memset(frame, 0x55, sizeof(frame));
    assert(FirmwareUpdateCanSendFrame(&sender, frame, sizeof(frame), 0xFFFFFFF8U));
    assert(sender.transmit_state == FW_UPDATE_CAN_TX_WAIT_FLOW_CONTROL);
    FirmwareUpdateCanPoll(&sender, 2U);
    assert(!FirmwareUpdateCanTxBusy(&sender));
    assert(sender.last_error == FW_UPDATE_CAN_ERROR_TIMEOUT);

    FirmwareUpdateCanTransport receiver;
    FirmwareUpdateCanInit(&receiver,
                          FW_UPDATE_CAN_NODE_MDI,
                          FW_UPDATE_CAN_TESTER_ADDRESS,
                          CaptureCanFrame,
                          &capture,
                          10U);
    uint8_t first[8] = {0x10U, 0x08U, 1U, 2U, 3U, 4U, 5U, 6U};
    assert(FirmwareUpdateCanOnFrame(&receiver, receiver.receive_id, first, sizeof(first), 100U) ==
           FW_UPDATE_CAN_RX_ACCEPTED);
    FirmwareUpdateCanPoll(&receiver, 110U);
    assert(receiver.receive_state == FW_UPDATE_CAN_RX_IDLE);
    assert(receiver.last_error == FW_UPDATE_CAN_ERROR_TIMEOUT);
}

static void TestOverflowAndMalformedFrame(void)
{
    Capture capture = {0};
    FirmwareUpdateCanTransport receiver;
    FirmwareUpdateCanInit(&receiver,
                          FW_UPDATE_CAN_NODE_MDI,
                          FW_UPDATE_CAN_TESTER_ADDRESS,
                          CaptureCanFrame,
                          &capture,
                          20U);
    uint8_t too_large[8] = {0x1FU, 0xFFU, 1U, 2U, 3U, 4U, 5U, 6U};
    assert(FirmwareUpdateCanOnFrame(
               &receiver, receiver.receive_id, too_large, sizeof(too_large), 0U) ==
           FW_UPDATE_CAN_RX_ERROR);
    assert(capture.count == 1U);
    assert(capture.id ==
           FirmwareUpdateCanAddressedId(FW_UPDATE_CAN_TESTER_ADDRESS, FW_UPDATE_CAN_NODE_MDI));
    assert(capture.data[0] == 0x32U);
    assert(receiver.last_error == FW_UPDATE_CAN_ERROR_FRAME_TOO_LARGE);

    uint8_t invalid_single[8] = {0x08U};
    assert(FirmwareUpdateCanOnFrame(
               &receiver, receiver.receive_id, invalid_single, sizeof(invalid_single), 1U) ==
           FW_UPDATE_CAN_RX_ERROR);
    assert(receiver.last_error == FW_UPDATE_CAN_ERROR_MALFORMED_FRAME);

    uint8_t contains_zero[] = {1U, 0U, 2U, 0U};
    assert(FirmwareUpdateCanSendFrame(&receiver, contains_zero, sizeof(contains_zero), 2U));
    assert(receiver.last_error == FW_UPDATE_CAN_ERROR_NONE);
    assert(capture.data[0] == sizeof(contains_zero));
    assert(memcmp(&capture.data[1], contains_zero, sizeof(contains_zero)) == 0);
}

static void TestFlowControlFailures(void)
{
    Capture capture = {0};
    FirmwareUpdateCanTransport sender;
    uint8_t frame[16];
    memset(frame, 0x55, sizeof(frame));

    FirmwareUpdateCanInit(&sender,
                          FW_UPDATE_CAN_TESTER_ADDRESS,
                          FW_UPDATE_CAN_NODE_MDI,
                          CaptureCanFrame,
                          &capture,
                          20U);
    assert(FirmwareUpdateCanSendFrame(&sender, frame, sizeof(frame), 0U));
    uint8_t wait[8] = {0x31U, 0U, 0U};
    for (unsigned index = 0U; index < FW_UPDATE_CAN_MAX_WAIT_FRAMES; index++)
    {
        assert(
            FirmwareUpdateCanOnFrame(&sender, sender.receive_id, wait, sizeof(wait), index + 1U) ==
            FW_UPDATE_CAN_RX_ACCEPTED);
    }
    assert(FirmwareUpdateCanOnFrame(&sender, sender.receive_id, wait, sizeof(wait), 4U) ==
           FW_UPDATE_CAN_RX_ERROR);
    assert(!FirmwareUpdateCanTxBusy(&sender));
    assert(sender.last_error == FW_UPDATE_CAN_ERROR_FLOW_CONTROL_WAIT_LIMIT);

    assert(FirmwareUpdateCanSendFrame(&sender, frame, sizeof(frame), 10U));
    uint8_t overflow[8] = {0x32U, 0U, 0U};
    assert(FirmwareUpdateCanOnFrame(&sender, sender.receive_id, overflow, sizeof(overflow), 11U) ==
           FW_UPDATE_CAN_RX_ERROR);
    assert(!FirmwareUpdateCanTxBusy(&sender));
    assert(sender.last_error == FW_UPDATE_CAN_ERROR_FLOW_CONTROL_OVERFLOW);
}

static void TestBackpressureIsBounded(void)
{
    FirmwareUpdateCanTransport sender;
    uint8_t frame[16];
    memset(frame, 0x55, sizeof(frame));
    FirmwareUpdateCanInit(
        &sender, FW_UPDATE_CAN_TESTER_ADDRESS, FW_UPDATE_CAN_NODE_MDI, RejectCanFrame, NULL, 5U);
    assert(FirmwareUpdateCanSendFrame(&sender, frame, sizeof(frame), 0U));
    for (uint32_t now_ms = 1U; now_ms <= 5U; now_ms++)
    {
        FirmwareUpdateCanPoll(&sender, now_ms);
    }
    assert(!FirmwareUpdateCanTxBusy(&sender));
    assert(sender.last_error == FW_UPDATE_CAN_ERROR_TIMEOUT);

    FirmwareUpdateCanTransport receiver;
    FirmwareUpdateCanInit(
        &receiver, FW_UPDATE_CAN_NODE_MDI, FW_UPDATE_CAN_TESTER_ADDRESS, RejectCanFrame, NULL, 5U);
    uint8_t first[8] = {0x10U, 0x08U, 1U, 2U, 3U, 4U, 5U, 6U};
    assert(FirmwareUpdateCanOnFrame(&receiver, receiver.receive_id, first, sizeof(first), 10U) ==
           FW_UPDATE_CAN_RX_ACCEPTED);
    for (uint32_t now_ms = 11U; now_ms <= 15U; now_ms++)
    {
        FirmwareUpdateCanPoll(&receiver, now_ms);
    }
    assert(receiver.receive_state == FW_UPDATE_CAN_RX_IDLE);
    assert(receiver.last_error == FW_UPDATE_CAN_ERROR_TIMEOUT);
}

int main(void)
{
    TestAddressing();
    TestGoldenHelloRoundTrip();
    TestMaximumDataRoundTrip();
    TestBackpressure();
    TestWrongIdAndSequenceFailure();
    TestTimeouts();
    TestOverflowAndMalformedFrame();
    TestFlowControlFailures();
    TestBackpressureIsBounded();
    puts("Firmware update classic-CAN transport tests passed");
    return 0;
}
