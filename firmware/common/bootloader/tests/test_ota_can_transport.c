#include "sunlite_ota_can_transport.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#define BUS_CAPACITY 512U

typedef struct {
    uint32_t id;
    uint8_t data[8];
    uint8_t dlc;
} QueuedCanFrame;

typedef struct {
    QueuedCanFrame frames[BUS_CAPACITY];
    size_t head;
    size_t tail;
    size_t count;
    size_t transmitted_count;
    size_t flow_control_count;
} TestBus;

typedef struct {
    TestBus *bus;
    unsigned reject_count;
} TestSender;

typedef struct {
    uint32_t id;
    uint8_t data[8];
    uint8_t dlc;
    size_t count;
} Capture;

static bool QueueCanFrame(void *user,
                          uint32_t extended_id,
                          const uint8_t data[8],
                          uint8_t dlc)
{
    TestSender *sender = user;
    if (sender->reject_count > 0U) {
        sender->reject_count--;
        return false;
    }
    TestBus *bus = sender->bus;
    assert(bus->count < BUS_CAPACITY);
    QueuedCanFrame *frame = &bus->frames[bus->tail];
    frame->id = extended_id;
    frame->dlc = dlc;
    memcpy(frame->data, data, sizeof(frame->data));
    bus->tail = (bus->tail + 1U) % BUS_CAPACITY;
    bus->count++;
    bus->transmitted_count++;
    if ((data[0] & 0xF0U) == 0x30U) {
        bus->flow_control_count++;
    }
    return true;
}

static bool CaptureCanFrame(void *user,
                            uint32_t extended_id,
                            const uint8_t data[8],
                            uint8_t dlc)
{
    Capture *capture = user;
    capture->id = extended_id;
    capture->dlc = dlc;
    memcpy(capture->data, data, sizeof(capture->data));
    capture->count++;
    return true;
}

static bool RejectCanFrame(void *user,
                           uint32_t extended_id,
                           const uint8_t data[8],
                           uint8_t dlc)
{
    (void)user;
    (void)extended_id;
    (void)data;
    (void)dlc;
    return false;
}

static bool PopCanFrame(TestBus *bus, QueuedCanFrame *frame)
{
    if (bus->count == 0U) {
        return false;
    }
    *frame = bus->frames[bus->head];
    bus->head = (bus->head + 1U) % BUS_CAPACITY;
    bus->count--;
    return true;
}

static uint32_t DrivePair(SunliteOtaCanTransport *first,
                          SunliteOtaCanTransport *second,
                          TestBus *bus,
                          uint32_t now_ms,
                          uint32_t maximum_iterations)
{
    for (uint32_t iteration = 0U;
         iteration < maximum_iterations;
         iteration++, now_ms++) {
        SunliteOtaCanPoll(first, now_ms);
        SunliteOtaCanPoll(second, now_ms);

        QueuedCanFrame frame;
        if (!PopCanFrame(bus, &frame)) {
            continue;
        }
        if (frame.id == first->receive_id) {
            (void)SunliteOtaCanOnFrame(first,
                                       frame.id,
                                       frame.data,
                                       frame.dlc,
                                       now_ms);
        } else if (frame.id == second->receive_id) {
            (void)SunliteOtaCanOnFrame(second,
                                       frame.id,
                                       frame.data,
                                       frame.dlc,
                                       now_ms);
        } else {
            assert(!"test bus emitted an unaddressed frame");
        }
    }
    return now_ms;
}

static size_t TestCobsEncode(const uint8_t *input,
                             size_t input_length,
                             uint8_t *output,
                             size_t output_capacity)
{
    assert(output_capacity >= 2U);
    size_t read_index = 0U;
    size_t write_index = 1U;
    size_t code_index = 0U;
    uint8_t code = 1U;

    while (read_index < input_length) {
        if (input[read_index] == 0U) {
            output[code_index] = code;
            code_index = write_index++;
            code = 1U;
            read_index++;
        } else {
            assert(write_index < output_capacity);
            output[write_index++] = input[read_index++];
            code++;
            if (code == 0xFFU) {
                output[code_index] = code;
                code_index = write_index++;
                code = 1U;
            }
        }
    }
    output[code_index] = code;
    return write_index;
}

static size_t BuildMaximumDataFrame(uint8_t *encoded,
                                    size_t encoded_capacity)
{
    uint8_t raw[SUNLITE_OTA_MAX_RAW_FRAME] = {0};
    raw[0] = 'S';
    raw[1] = 'U';
    raw[2] = SUNLITE_OTA_PROTOCOL_VERSION;
    raw[3] = SUNLITE_OTA_MESSAGE_DATA;
    SunliteOtaWriteBe32(&raw[4], 0x12345678U);
    SunliteOtaWriteBe32(&raw[8], 0x89ABCDEFU);
    SunliteOtaWriteBe16(&raw[12], SUNLITE_OTA_MAX_RX_PAYLOAD);
    SunliteOtaWriteBe32(&raw[SUNLITE_OTA_HEADER_SIZE], 0x00020000U);
    for (size_t index = SUNLITE_OTA_DATA_OFFSET_SIZE;
         index < SUNLITE_OTA_MAX_RX_PAYLOAD;
         index++) {
        raw[SUNLITE_OTA_HEADER_SIZE + index] =
            (uint8_t)((index * 37U) & 0xFFU);
    }
    size_t body_length = SUNLITE_OTA_HEADER_SIZE +
                         SUNLITE_OTA_MAX_RX_PAYLOAD;
    SunliteOtaWriteBe32(&raw[body_length],
                        SunliteOtaCrc32(raw, body_length));
    size_t raw_length = body_length + SUNLITE_OTA_FRAME_CRC_SIZE;
    size_t encoded_length = TestCobsEncode(raw,
                                           raw_length,
                                           encoded,
                                           encoded_capacity - 1U);
    assert(encoded_length < encoded_capacity);
    encoded[encoded_length++] = 0U;
    return encoded_length;
}

static void AssertReceivedFrame(SunliteOtaCanTransport *transport,
                                const uint8_t *expected,
                                size_t expected_length)
{
    const uint8_t *received = NULL;
    size_t received_length = 0U;
    assert(SunliteOtaCanPeekFrame(transport,
                                  &received,
                                  &received_length));
    assert(expected_length > 0U);
    if (expected[expected_length - 1U] == 0U) {
        expected_length--;
    }
    assert(received_length == expected_length);
    assert(memcmp(received, expected, expected_length) == 0);
}

static void TestAddressing(void)
{
    assert(SunliteOtaCanRequestId(SUNLITE_OTA_CAN_NODE_MDI) ==
           0x18DA10F1U);
    assert(SunliteOtaCanResponseId(SUNLITE_OTA_CAN_NODE_MDI) ==
           0x18DAF110U);
    uint8_t destination = 0U;
    uint8_t source = 0U;
    assert(SunliteOtaCanDecodeAddressedId(0x18DA11F1U,
                                          &destination,
                                          &source));
    assert(destination == SUNLITE_OTA_CAN_NODE_DRD);
    assert(source == SUNLITE_OTA_CAN_TESTER_ADDRESS);
    assert(!SunliteOtaCanDecodeAddressedId(0x00000123U,
                                           &destination,
                                           &source));
    assert(!SunliteOtaCanDecodeAddressedId(0x20000000U,
                                           &destination,
                                           &source));
}

static void TestGoldenHelloRoundTrip(void)
{
    static const uint8_t expected[] = {
        0x09, 0x53, 0x55, 0x01, 0x01, 0x12, 0x34, 0x56, 0x78, 0x01,
        0x01, 0x02, 0x01, 0x01, 0x05, 0xD9, 0xD5, 0x4E, 0x7D, 0x00,
    };
    TestBus bus = {0};
    TestSender tester_sender = {.bus = &bus};
    TestSender target_sender = {.bus = &bus};
    SunliteOtaCanTransport tester;
    SunliteOtaCanTransport target;
    SunliteOtaCanInit(&tester,
                      SUNLITE_OTA_CAN_TESTER_ADDRESS,
                      SUNLITE_OTA_CAN_NODE_MDI,
                      QueueCanFrame,
                      &tester_sender,
                      250U);
    SunliteOtaCanInit(&target,
                      SUNLITE_OTA_CAN_NODE_MDI,
                      SUNLITE_OTA_CAN_TESTER_ADDRESS,
                      QueueCanFrame,
                      &target_sender,
                      250U);

    assert(SunliteOtaCanSendFrame(&tester,
                                  expected,
                                  sizeof(expected),
                                  0U));
    (void)DrivePair(&tester, &target, &bus, 0U, 100U);
    assert(!SunliteOtaCanTxBusy(&tester));
    AssertReceivedFrame(&target, expected, sizeof(expected));

    const uint8_t *received = NULL;
    size_t received_length = 0U;
    uint8_t raw[SUNLITE_OTA_MAX_RAW_FRAME];
    SunliteOtaMessage message;
    assert(SunliteOtaCanPeekFrame(&target, &received, &received_length));
    assert(SunliteOtaFrameDecode(received,
                                 received_length,
                                 raw,
                                 sizeof(raw),
                                 &message));
    assert(message.type == SUNLITE_OTA_MESSAGE_HELLO);
    assert(message.session_id == 0x12345678U);
    SunliteOtaCanConsumeFrame(&target);
    assert(!SunliteOtaCanRxBusy(&target));
}

static void TestMaximumDataRoundTrip(void)
{
    uint8_t expected[SUNLITE_OTA_MAX_ENCODED_FRAME];
    size_t expected_length = BuildMaximumDataFrame(expected,
                                                   sizeof(expected));
    assert(expected_length == SUNLITE_OTA_MAX_ENCODED_FRAME - 1U);

    TestBus bus = {0};
    TestSender tester_sender = {.bus = &bus};
    TestSender target_sender = {.bus = &bus};
    SunliteOtaCanTransport tester;
    SunliteOtaCanTransport target;
    SunliteOtaCanInit(&tester,
                      SUNLITE_OTA_CAN_TESTER_ADDRESS,
                      SUNLITE_OTA_CAN_NODE_DRD,
                      QueueCanFrame,
                      &tester_sender,
                      250U);
    SunliteOtaCanInit(&target,
                      SUNLITE_OTA_CAN_NODE_DRD,
                      SUNLITE_OTA_CAN_TESTER_ADDRESS,
                      QueueCanFrame,
                      &target_sender,
                      250U);
    assert(SunliteOtaCanSendFrame(&tester,
                                  expected,
                                  expected_length,
                                  1000U));
    (void)DrivePair(&tester, &target, &bus, 1000U, 1000U);
    assert(!SunliteOtaCanTxBusy(&tester));
    assert(bus.flow_control_count > 1U);
    AssertReceivedFrame(&target, expected, expected_length);

    const uint8_t *received = NULL;
    size_t received_length = 0U;
    uint8_t raw[SUNLITE_OTA_MAX_RAW_FRAME];
    SunliteOtaMessage message;
    assert(SunliteOtaCanPeekFrame(&target, &received, &received_length));
    assert(SunliteOtaFrameDecode(received,
                                 received_length,
                                 raw,
                                 sizeof(raw),
                                 &message));
    assert(message.type == SUNLITE_OTA_MESSAGE_DATA);
    assert(message.payload_length == SUNLITE_OTA_MAX_RX_PAYLOAD);
    assert(SunliteOtaReadBe32(message.payload) == 0x00020000U);
}

static void TestBackpressureAndPeerLock(void)
{
    static const uint8_t frame[] = {1U, 2U, 3U};
    TestBus bus = {0};
    TestSender sender = {.bus = &bus, .reject_count = 2U};
    SunliteOtaCanTransport transport;
    SunliteOtaCanInit(&transport,
                      SUNLITE_OTA_CAN_TESTER_ADDRESS,
                      SUNLITE_OTA_CAN_NODE_STR,
                      QueueCanFrame,
                      &sender,
                      20U);
    assert(SunliteOtaCanSendFrame(&transport,
                                  frame,
                                  sizeof(frame),
                                  0U));
    assert(SunliteOtaCanTxBusy(&transport));
    assert(!SunliteOtaCanSetPeer(&transport, SUNLITE_OTA_CAN_NODE_HVC));
    assert(SunliteOtaCanLastError(&transport) ==
           SUNLITE_OTA_CAN_ERROR_BUSY);
    SunliteOtaCanPoll(&transport, 1U);
    assert(SunliteOtaCanTxBusy(&transport));
    SunliteOtaCanPoll(&transport, 2U);
    assert(!SunliteOtaCanTxBusy(&transport));
    assert(bus.count == 1U);
    assert(SunliteOtaCanSetPeer(&transport, SUNLITE_OTA_CAN_NODE_HVC));
    assert(transport.transmit_id == 0x18DA13F1U);
}

static void TestWrongIdAndSequenceFailure(void)
{
    Capture capture = {0};
    SunliteOtaCanTransport target;
    SunliteOtaCanInit(&target,
                      SUNLITE_OTA_CAN_NODE_MDI,
                      SUNLITE_OTA_CAN_TESTER_ADDRESS,
                      CaptureCanFrame,
                      &capture,
                      20U);

    uint8_t first[8] = {0x10U, 0x08U, 1U, 2U, 3U, 4U, 5U, 6U};
    assert(SunliteOtaCanOnFrame(&target,
                                0x18DA11F1U,
                                first,
                                sizeof(first),
                                0U) == SUNLITE_OTA_CAN_RX_IGNORED);
    assert(!SunliteOtaCanRxBusy(&target));
    assert(SunliteOtaCanOnFrame(&target,
                                target.receive_id,
                                first,
                                sizeof(first),
                                0U) == SUNLITE_OTA_CAN_RX_ACCEPTED);
    assert(capture.count == 1U);
    assert((capture.data[0] & 0x0FU) == 0U);

    uint8_t wrong_sequence[8] = {0x22U, 7U, 8U, 0U, 0U, 0U, 0U, 0U};
    assert(SunliteOtaCanOnFrame(&target,
                                target.receive_id,
                                wrong_sequence,
                                sizeof(wrong_sequence),
                                1U) == SUNLITE_OTA_CAN_RX_ERROR);
    assert(SunliteOtaCanLastError(&target) ==
           SUNLITE_OTA_CAN_ERROR_SEQUENCE);
    assert(!SunliteOtaCanRxBusy(&target));
}

static void TestTimeouts(void)
{
    Capture capture = {0};
    SunliteOtaCanTransport sender;
    SunliteOtaCanInit(&sender,
                      SUNLITE_OTA_CAN_TESTER_ADDRESS,
                      SUNLITE_OTA_CAN_NODE_MDI,
                      CaptureCanFrame,
                      &capture,
                      10U);
    uint8_t frame[16];
    memset(frame, 0x55, sizeof(frame));
    assert(SunliteOtaCanSendFrame(&sender,
                                  frame,
                                  sizeof(frame),
                                  0xFFFFFFF8U));
    assert(sender.transmit_state ==
           SUNLITE_OTA_CAN_TX_WAIT_FLOW_CONTROL);
    SunliteOtaCanPoll(&sender, 2U);
    assert(!SunliteOtaCanTxBusy(&sender));
    assert(SunliteOtaCanLastError(&sender) ==
           SUNLITE_OTA_CAN_ERROR_TIMEOUT);

    SunliteOtaCanTransport receiver;
    SunliteOtaCanInit(&receiver,
                      SUNLITE_OTA_CAN_NODE_MDI,
                      SUNLITE_OTA_CAN_TESTER_ADDRESS,
                      CaptureCanFrame,
                      &capture,
                      10U);
    uint8_t first[8] = {0x10U, 0x08U, 1U, 2U, 3U, 4U, 5U, 6U};
    assert(SunliteOtaCanOnFrame(&receiver,
                                receiver.receive_id,
                                first,
                                sizeof(first),
                                100U) == SUNLITE_OTA_CAN_RX_ACCEPTED);
    SunliteOtaCanPoll(&receiver, 110U);
    assert(!SunliteOtaCanRxBusy(&receiver));
    assert(SunliteOtaCanLastError(&receiver) ==
           SUNLITE_OTA_CAN_ERROR_TIMEOUT);
}

static void TestOverflowAndMalformedFrame(void)
{
    Capture capture = {0};
    SunliteOtaCanTransport receiver;
    SunliteOtaCanInit(&receiver,
                      SUNLITE_OTA_CAN_NODE_MDI,
                      SUNLITE_OTA_CAN_TESTER_ADDRESS,
                      CaptureCanFrame,
                      &capture,
                      20U);
    uint8_t too_large[8] = {0x1FU, 0xFFU, 1U, 2U, 3U, 4U, 5U, 6U};
    assert(SunliteOtaCanOnFrame(&receiver,
                                receiver.receive_id,
                                too_large,
                                sizeof(too_large),
                                0U) == SUNLITE_OTA_CAN_RX_ERROR);
    assert(capture.count == 1U);
    assert(capture.id == SunliteOtaCanResponseId(
                              SUNLITE_OTA_CAN_NODE_MDI));
    assert(capture.data[0] == 0x32U);
    assert(SunliteOtaCanLastError(&receiver) ==
           SUNLITE_OTA_CAN_ERROR_FRAME_TOO_LARGE);

    uint8_t invalid_single[8] = {0x08U};
    assert(SunliteOtaCanOnFrame(&receiver,
                                receiver.receive_id,
                                invalid_single,
                                sizeof(invalid_single),
                                1U) == SUNLITE_OTA_CAN_RX_ERROR);
    assert(SunliteOtaCanLastError(&receiver) ==
           SUNLITE_OTA_CAN_ERROR_MALFORMED_FRAME);

    uint8_t contains_delimiter[] = {1U, 0U, 2U};
    assert(!SunliteOtaCanSendFrame(&receiver,
                                   contains_delimiter,
                                   sizeof(contains_delimiter),
                                   2U));
    assert(SunliteOtaCanLastError(&receiver) ==
           SUNLITE_OTA_CAN_ERROR_MALFORMED_FRAME);
}

static void TestFlowControlFailures(void)
{
    Capture capture = {0};
    SunliteOtaCanTransport sender;
    uint8_t frame[16];
    memset(frame, 0x55, sizeof(frame));

    SunliteOtaCanInit(&sender,
                      SUNLITE_OTA_CAN_TESTER_ADDRESS,
                      SUNLITE_OTA_CAN_NODE_MDI,
                      CaptureCanFrame,
                      &capture,
                      20U);
    assert(SunliteOtaCanSendFrame(&sender,
                                  frame,
                                  sizeof(frame),
                                  0U));
    uint8_t wait[8] = {0x31U, 0U, 0U};
    for (unsigned index = 0U;
         index < SUNLITE_OTA_CAN_MAX_WAIT_FRAMES;
         index++) {
        assert(SunliteOtaCanOnFrame(&sender,
                                    sender.receive_id,
                                    wait,
                                    sizeof(wait),
                                    index + 1U) ==
               SUNLITE_OTA_CAN_RX_ACCEPTED);
    }
    assert(SunliteOtaCanOnFrame(&sender,
                                sender.receive_id,
                                wait,
                                sizeof(wait),
                                4U) == SUNLITE_OTA_CAN_RX_ERROR);
    assert(!SunliteOtaCanTxBusy(&sender));
    assert(SunliteOtaCanLastError(&sender) ==
           SUNLITE_OTA_CAN_ERROR_FLOW_CONTROL_WAIT_LIMIT);

    SunliteOtaCanClearError(&sender);
    assert(SunliteOtaCanSendFrame(&sender,
                                  frame,
                                  sizeof(frame),
                                  10U));
    uint8_t overflow[8] = {0x32U, 0U, 0U};
    assert(SunliteOtaCanOnFrame(&sender,
                                sender.receive_id,
                                overflow,
                                sizeof(overflow),
                                11U) == SUNLITE_OTA_CAN_RX_ERROR);
    assert(!SunliteOtaCanTxBusy(&sender));
    assert(SunliteOtaCanLastError(&sender) ==
           SUNLITE_OTA_CAN_ERROR_FLOW_CONTROL_OVERFLOW);
}

static void TestBackpressureIsBounded(void)
{
    SunliteOtaCanTransport sender;
    uint8_t frame[16];
    memset(frame, 0x55, sizeof(frame));
    SunliteOtaCanInit(&sender,
                      SUNLITE_OTA_CAN_TESTER_ADDRESS,
                      SUNLITE_OTA_CAN_NODE_MDI,
                      RejectCanFrame,
                      NULL,
                      5U);
    assert(SunliteOtaCanSendFrame(&sender,
                                  frame,
                                  sizeof(frame),
                                  0U));
    for (uint32_t now_ms = 1U; now_ms <= 5U; now_ms++) {
        SunliteOtaCanPoll(&sender, now_ms);
    }
    assert(!SunliteOtaCanTxBusy(&sender));
    assert(SunliteOtaCanLastError(&sender) ==
           SUNLITE_OTA_CAN_ERROR_TIMEOUT);

    SunliteOtaCanTransport receiver;
    SunliteOtaCanInit(&receiver,
                      SUNLITE_OTA_CAN_NODE_MDI,
                      SUNLITE_OTA_CAN_TESTER_ADDRESS,
                      RejectCanFrame,
                      NULL,
                      5U);
    uint8_t first[8] = {0x10U, 0x08U, 1U, 2U, 3U, 4U, 5U, 6U};
    assert(SunliteOtaCanOnFrame(&receiver,
                                receiver.receive_id,
                                first,
                                sizeof(first),
                                10U) == SUNLITE_OTA_CAN_RX_ACCEPTED);
    for (uint32_t now_ms = 11U; now_ms <= 15U; now_ms++) {
        SunliteOtaCanPoll(&receiver, now_ms);
    }
    assert(!SunliteOtaCanRxBusy(&receiver));
    assert(SunliteOtaCanLastError(&receiver) ==
           SUNLITE_OTA_CAN_ERROR_TIMEOUT);
}

int main(void)
{
    TestAddressing();
    TestGoldenHelloRoundTrip();
    TestMaximumDataRoundTrip();
    TestBackpressureAndPeerLock();
    TestWrongIdAndSequenceFailure();
    TestTimeouts();
    TestOverflowAndMalformedFrame();
    TestFlowControlFailures();
    TestBackpressureIsBounded();
    puts("Sunlite OTA classic-CAN transport tests passed");
    return 0;
}
