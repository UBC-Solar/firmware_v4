#include "fw_update_protocol.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

/**
 * @brief Checks the protocol-3 HELLO bytes against an independent Python/zlib vector.
 */
static void TestHelloGoldenFrame(void)
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
    uint8_t frame[64];
    size_t length = FirmwareUpdateFrameEncode(
        FW_UPDATE_MESSAGE_HELLO, 0x12345678U, 1U, NULL, 0U, frame, sizeof(frame));
    assert(length == sizeof(expected));
    assert(memcmp(frame, expected, length) == 0);
    FirmwareUpdateMessage message;
    assert(FirmwareUpdateFrameDecode(frame, length, &message));
    assert(message.type == FW_UPDATE_MESSAGE_HELLO && message.payload_length == 0U);
    /* Reject old versions even when their raw header and CRC are consistent. */
    for (uint8_t version = 1U; version < FW_UPDATE_PROTOCOL_VERSION; version++)
    {
        frame[2] = version;
        FirmwareUpdateWriteBe32(&frame[length - 4U], FirmwareUpdateCrc32(frame, length - 4U));
        assert(!FirmwareUpdateFrameDecode(frame, length, &message));
    }
}

/**
 * @brief Checks BOARD_INFO layout, payload ownership, CRC rejection, and exact length.
 */
static void TestBoardInfoGoldenFrame(void)
{
    static const uint8_t frame[] = {
        0x53, 0x55, 0x03, 0x02, 0x12, 0x34, 0x56, 0x78, 0x00, 0x00, 0x00, 0x01, 0x00, 0x17,
        0x54, 0x45, 0x4C, 0x45, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x11, 0x01, 0x04,
        0x00, 0x00, 0x03, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5C, 0x08, 0x13, 0x0E,
    };
    FirmwareUpdateMessage message;
    assert(FirmwareUpdateFrameDecode(frame, sizeof(frame), &message));
    assert(message.type == FW_UPDATE_MESSAGE_BOARD_INFO);
    assert(message.session_id == 0x12345678U && message.sequence == 1U);
    assert(message.payload_length == FW_UPDATE_BOARD_INFO_SIZE);
    assert(message.payload == &frame[FW_UPDATE_HEADER_SIZE]);
    assert(FirmwareUpdateReadBe32(&message.payload[0]) == 0x54454C45U);
    assert(FirmwareUpdateReadBe32(&message.payload[8]) == 17U);
    assert(FirmwareUpdateReadBe32(&message.payload[15]) == 225280U);
    assert(!FirmwareUpdateFrameDecode(frame, sizeof(frame) - 1U, &message));
    uint8_t corrupted[sizeof(frame)];
    memcpy(corrupted, frame, sizeof(frame));
    corrupted[20] ^= 0x40U;
    assert(!FirmwareUpdateFrameDecode(corrupted, sizeof(corrupted), &message));
}

/**
 * @brief Checks the standard CRC32 vector and incremental processing across block boundaries.
 */
static void TestCrc32(void)
{
    const uint8_t* bytes = (const uint8_t*)"123456789";
    assert(FirmwareUpdateCrc32(bytes, 9U) == 0xCBF43926U);
    assert(FirmwareUpdateCrc32(NULL, 0U) == 0U);
    uint32_t crc = FirmwareUpdateCrc32Update(0xFFFFFFFFU, bytes, 4U);
    crc = FirmwareUpdateCrc32Update(crc, bytes + 4U, 5U);
    assert((crc ^ 0xFFFFFFFFU) == 0xCBF43926U);
}

/**
 * @brief Rejects invalid codec arguments without accessing undersized buffers.
 */
static void TestFrameBounds(void)
{
    uint8_t frame[18];
    FirmwareUpdateMessage message;
    assert(FirmwareUpdateFrameEncode(FW_UPDATE_MESSAGE_HELLO, 1U, 1U, NULL, 0U, frame, 17U) == 0U);
    assert(FirmwareUpdateFrameEncode(
               FW_UPDATE_MESSAGE_DATA, 1U, 1U, NULL, 1U, frame, sizeof(frame)) == 0U);
    assert(!FirmwareUpdateFrameDecode(NULL, sizeof(frame), &message));
    assert(!FirmwareUpdateFrameDecode(frame, 17U, &message));
}

int main(void)
{
    TestHelloGoldenFrame();
    TestBoardInfoGoldenFrame();
    TestCrc32();
    TestFrameBounds();
    puts("Firmware update C/Python contract vectors passed");
    return 0;
}
