#include "bootloader.h"
#include "fw_update_protocol.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define TEST_TARGET_ID 0x54454C45U
#define TEST_FIRMWARE_VERSION 42U
#define TEST_SESSION_ID 0x10203040U
static const uint8_t image[] = {
    0x00,
    0xC0,
    0x00,
    0x20, /* initial SP: 0x2000c000 */
    0x09,
    0x80,
    0x00,
    0x08, /* reset handler: 0x08008009 */
    0x42,
    0x43,
    0x44,
    0x45,
    0x46, /* body with an odd final byte */
};
static uint8_t flash_image[sizeof(image)];
static unsigned body_writes;
static unsigned flash_begin_count;

/**
 * @brief Maps the engine's readback address to emulated application flash.
 */
const uint8_t* TestEngineFlashPointer(uint32_t address)
{
    assert(address >= BOOTLOADER_APP_START_ADDRESS);
    assert(address < BOOTLOADER_APP_START_ADDRESS + sizeof(image));
    return &flash_image[address - BOOTLOADER_APP_START_ADDRESS];
}

typedef struct
{
    bool send_succeeds;
    uint8_t frame[FW_UPDATE_MAX_FRAME_SIZE];
    size_t frame_length;
    unsigned calls;
} SendCapture;

static bool flash_open;
static bool vector_written;
static unsigned jump_count;
static unsigned delay_count;
static uint32_t last_delay_ms;

bool BootloaderAppIsValid(void) { return vector_written; }

bool BootloaderShouldEnterUpdateMode(void) { return false; }

void BootloaderJumpToApp(void) { jump_count++; }

void BootloaderRun(void) {}

void BootloaderPrepareWatchdog(void) {}

void BootloaderServiceWatchdog(void) {}

bool BootloaderBoardStayInBootloader(void) { return false; }

void BootloaderEnterUpdateMode(void) {}

bool BootloaderFlashBeginAppUpdate(uint32_t image_size)
{
    assert(image_size == sizeof(image));
    flash_begin_count++;
    flash_open = true;
    vector_written = false;
    body_writes = 0U;
    memset(flash_image, 0xFF, sizeof(flash_image));
    return true;
}

bool BootloaderFlashWrite(uint32_t address, const uint8_t* data, size_t length)
{
    assert(flash_open);
    assert(address >= BOOTLOADER_APP_START_ADDRESS);
    uint32_t offset = address - BOOTLOADER_APP_START_ADDRESS;
    assert(length > 0U && length <= sizeof(image) - offset);
    assert(memcmp(data, &image[offset], length) == 0);
    if (offset < 8U)
    {
        assert((offset == 0U || offset == 4U) && length == 4U);
        if (offset == 0U)
        {
            assert(memcmp(&flash_image[4], &image[4], 4U) == 0);
            vector_written = true;
        }
    }
    else
    {
        assert(offset >= 8U);
        body_writes++;
    }
    memcpy(&flash_image[offset], data, length);
    return true;
}

void BootloaderFlashEndAppUpdate(void) { flash_open = false; }

void HAL_Delay(uint32_t delay_ms)
{
    delay_count++;
    last_delay_ms = delay_ms;
}

static bool CaptureResponse(const uint8_t* frame, size_t frame_length, void* context)
{
    SendCapture* capture = context;
    assert(frame_length <= sizeof(capture->frame));
    memcpy(capture->frame, frame, frame_length);
    capture->frame_length = frame_length;
    capture->calls++;
    return capture->send_succeeds;
}

/**
 * @brief Builds a raw request through the production wire codec.
 */
static size_t EncodeRequest(FirmwareUpdateMessageType type,
                            uint32_t sequence,
                            const uint8_t* payload,
                            uint16_t payload_length,
                            uint8_t frame[FW_UPDATE_MAX_FRAME_SIZE])
{
    size_t length = FirmwareUpdateFrameEncode(
        type, TEST_SESSION_ID, sequence, payload, payload_length, frame, FW_UPDATE_MAX_FRAME_SIZE);
    assert(length > 0U);
    return length;
}

/**
 * @brief Checks the engine's response header and CRC32.
 */
static FirmwareUpdateMessage DecodeResponse(const SendCapture* capture)
{
    FirmwareUpdateMessage response;
    assert(capture->calls == 1U);
    assert(FirmwareUpdateFrameDecode(capture->frame, capture->frame_length, &response));
    return response;
}

static void AssertOkAck(const SendCapture* capture, FirmwareUpdateMessageType request_type)
{
    FirmwareUpdateMessage response = DecodeResponse(capture);
    assert(response.type == FW_UPDATE_MESSAGE_ACK);
    assert(response.payload_length == FW_UPDATE_ACK_SIZE);
    assert(response.payload[0] == (uint8_t)request_type);
    assert(response.payload[1] == FW_UPDATE_STATUS_OK);
}

static void ProcessRequest(FirmwareUpdateMessageType type,
                           uint32_t sequence,
                           const uint8_t* payload,
                           uint16_t payload_length,
                           SendCapture* capture)
{
    uint8_t frame[FW_UPDATE_MAX_FRAME_SIZE];
    size_t frame_length = EncodeRequest(type, sequence, payload, payload_length, frame);
    capture->frame_length = 0U;
    capture->calls = 0U;
    (void)FirmwareUpdateBootloaderProcessFrame(frame, frame_length, CaptureResponse, capture);
}

/**
 * @brief Describes the test image using the protocol-3 board/version/size/CRC fields.
 */
static void BuildBeginPayload(uint8_t payload[FW_UPDATE_BEGIN_UPDATE_SIZE])
{
    FirmwareUpdateWriteBe32(&payload[0], TEST_TARGET_ID);
    FirmwareUpdateWriteBe32(&payload[4], TEST_FIRMWARE_VERSION);
    FirmwareUpdateWriteBe32(&payload[8], sizeof(image));
    FirmwareUpdateWriteBe32(&payload[12], FirmwareUpdateCrc32(image, sizeof(image)));
}

/**
 * @brief Rejects the old manifest length and prevents a corrupt image from booting.
 */
static void TestInvalidImageStaysInRecovery(void)
{
    SendCapture capture = {.send_succeeds = true};
    uint8_t begin_payload[FW_UPDATE_BEGIN_UPDATE_SIZE];
    BuildBeginPayload(begin_payload);

    uint8_t legacy_payload[116] = {0};
    memcpy(legacy_payload, begin_payload, sizeof(begin_payload));
    ProcessRequest(
        FW_UPDATE_MESSAGE_BEGIN_UPDATE, 90U, legacy_payload, sizeof(legacy_payload), &capture);
    FirmwareUpdateMessage rejected = DecodeResponse(&capture);
    assert(rejected.type == FW_UPDATE_MESSAGE_NACK);
    assert(rejected.payload[1] == FW_UPDATE_STATUS_BAD_STATE);
    assert(!flash_open);

    begin_payload[12] ^= 1U;
    ProcessRequest(
        FW_UPDATE_MESSAGE_BEGIN_UPDATE, 91U, begin_payload, sizeof(begin_payload), &capture);
    AssertOkAck(&capture, FW_UPDATE_MESSAGE_BEGIN_UPDATE);

    uint8_t data_payload[FW_UPDATE_DATA_OFFSET_SIZE + sizeof(image)] = {0};
    memcpy(&data_payload[FW_UPDATE_DATA_OFFSET_SIZE], image, sizeof(image));
    ProcessRequest(FW_UPDATE_MESSAGE_DATA, 92U, data_payload, sizeof(data_payload), &capture);
    AssertOkAck(&capture, FW_UPDATE_MESSAGE_DATA);
    ProcessRequest(FW_UPDATE_MESSAGE_END_UPDATE, 93U, NULL, 0U, &capture);
    rejected = DecodeResponse(&capture);
    assert(rejected.type == FW_UPDATE_MESSAGE_NACK);
    assert(rejected.payload[1] == FW_UPDATE_STATUS_BAD_CRC);
    assert(!flash_open);
    assert(!vector_written);

    /* Correct transferred bytes must still fail if programmed flash is corrupted. */
    BuildBeginPayload(begin_payload);
    ProcessRequest(
        FW_UPDATE_MESSAGE_BEGIN_UPDATE, 94U, begin_payload, sizeof(begin_payload), &capture);
    AssertOkAck(&capture, FW_UPDATE_MESSAGE_BEGIN_UPDATE);
    ProcessRequest(FW_UPDATE_MESSAGE_DATA, 95U, data_payload, sizeof(data_payload), &capture);
    AssertOkAck(&capture, FW_UPDATE_MESSAGE_DATA);
    flash_image[8] ^= 1U;
    ProcessRequest(FW_UPDATE_MESSAGE_END_UPDATE, 96U, NULL, 0U, &capture);
    rejected = DecodeResponse(&capture);
    assert(rejected.type == FW_UPDATE_MESSAGE_NACK);
    assert(rejected.payload[1] == FW_UPDATE_STATUS_BAD_CRC);
    assert(!flash_open && !vector_written);

    /* The final CRC32 must include vectors held in RAM, not just the flash body. */
    ProcessRequest(
        FW_UPDATE_MESSAGE_BEGIN_UPDATE, 97U, begin_payload, sizeof(begin_payload), &capture);
    AssertOkAck(&capture, FW_UPDATE_MESSAGE_BEGIN_UPDATE);
    data_payload[FW_UPDATE_DATA_OFFSET_SIZE] ^= 1U;
    ProcessRequest(FW_UPDATE_MESSAGE_DATA, 98U, data_payload, sizeof(data_payload), &capture);
    AssertOkAck(&capture, FW_UPDATE_MESSAGE_DATA);
    ProcessRequest(FW_UPDATE_MESSAGE_END_UPDATE, 99U, NULL, 0U, &capture);
    rejected = DecodeResponse(&capture);
    assert(rejected.type == FW_UPDATE_MESSAGE_NACK);
    assert(rejected.payload[1] == FW_UPDATE_STATUS_BAD_CRC);
    assert(!flash_open && !vector_written);

    /* A matching CRC cannot make an unaligned stack vector bootable. */
    FirmwareUpdateWriteBe32(
        &begin_payload[12],
        FirmwareUpdateCrc32(&data_payload[FW_UPDATE_DATA_OFFSET_SIZE], sizeof(image)));
    ProcessRequest(
        FW_UPDATE_MESSAGE_BEGIN_UPDATE, 103U, begin_payload, sizeof(begin_payload), &capture);
    AssertOkAck(&capture, FW_UPDATE_MESSAGE_BEGIN_UPDATE);
    ProcessRequest(FW_UPDATE_MESSAGE_DATA, 104U, data_payload, sizeof(data_payload), &capture);
    AssertOkAck(&capture, FW_UPDATE_MESSAGE_DATA);
    ProcessRequest(FW_UPDATE_MESSAGE_END_UPDATE, 105U, NULL, 0U, &capture);
    rejected = DecodeResponse(&capture);
    assert(rejected.type == FW_UPDATE_MESSAGE_NACK);
    assert(rejected.payload[1] == FW_UPDATE_STATUS_FLASH_ERROR);
    assert(!flash_open && !vector_written);
}

/**
 * @brief Keeps split vector bytes erased while writing and verifying the body directly.
 */
static void TestSplitVectorsAndDirectBodyWrite(void)
{
    SendCapture capture = {.send_succeeds = true};
    uint8_t begin_payload[FW_UPDATE_BEGIN_UPDATE_SIZE];
    BuildBeginPayload(begin_payload);
    ProcessRequest(
        FW_UPDATE_MESSAGE_BEGIN_UPDATE, 200U, begin_payload, sizeof(begin_payload), &capture);
    AssertOkAck(&capture, FW_UPDATE_MESSAGE_BEGIN_UPDATE);
    const uint32_t offsets[] = {0U, 2U, 4U};
    const size_t lengths[] = {2U, 2U, sizeof(image) - 4U};
    for (size_t i = 0U; i < 3U; i++)
    {
        uint8_t data[FW_UPDATE_DATA_OFFSET_SIZE + sizeof(image)];
        FirmwareUpdateWriteBe32(data, offsets[i]);
        memcpy(&data[FW_UPDATE_DATA_OFFSET_SIZE], &image[offsets[i]], lengths[i]);
        ProcessRequest(FW_UPDATE_MESSAGE_DATA,
                       201U + (uint32_t)i,
                       data,
                       FW_UPDATE_DATA_OFFSET_SIZE + lengths[i],
                       &capture);
        AssertOkAck(&capture, FW_UPDATE_MESSAGE_DATA);
        for (size_t j = 0U; j < 8U; j++)
        {
            assert(flash_image[j] == 0xFFU);
        }
        assert(body_writes == (i == 2U ? 1U : 0U));
    }
    ProcessRequest(FW_UPDATE_MESSAGE_END_UPDATE, 204U, NULL, 0U, &capture);
    AssertOkAck(&capture, FW_UPDATE_MESSAGE_END_UPDATE);
    assert(memcmp(flash_image, image, sizeof(image)) == 0);
    assert(vector_written);
}

/**
 * @brief Replays identical requests once and rejects changed payloads with reused IDs.
 */
static void TestRetryPayloadFingerprint(void)
{
    vector_written = false;
    flash_begin_count = 0U;
    SendCapture capture = {.send_succeeds = true};
    uint8_t begin_payload[FW_UPDATE_BEGIN_UPDATE_SIZE];
    BuildBeginPayload(begin_payload);
    ProcessRequest(
        FW_UPDATE_MESSAGE_BEGIN_UPDATE, 300U, begin_payload, sizeof(begin_payload), &capture);
    AssertOkAck(&capture, FW_UPDATE_MESSAGE_BEGIN_UPDATE);
    ProcessRequest(
        FW_UPDATE_MESSAGE_BEGIN_UPDATE, 300U, begin_payload, sizeof(begin_payload), &capture);
    AssertOkAck(&capture, FW_UPDATE_MESSAGE_BEGIN_UPDATE);
    assert(flash_begin_count == 1U);

    begin_payload[12] ^= 1U;
    ProcessRequest(
        FW_UPDATE_MESSAGE_BEGIN_UPDATE, 300U, begin_payload, sizeof(begin_payload), &capture);
    FirmwareUpdateMessage rejected = DecodeResponse(&capture);
    assert(rejected.type == FW_UPDATE_MESSAGE_NACK);
    assert(rejected.payload[1] == FW_UPDATE_STATUS_BAD_STATE);
    assert(flash_begin_count == 1U);

    uint8_t data[FW_UPDATE_DATA_OFFSET_SIZE + sizeof(image)] = {0};
    memcpy(&data[FW_UPDATE_DATA_OFFSET_SIZE], image, sizeof(image));
    ProcessRequest(FW_UPDATE_MESSAGE_DATA, 301U, data, sizeof(data), &capture);
    AssertOkAck(&capture, FW_UPDATE_MESSAGE_DATA);
    ProcessRequest(FW_UPDATE_MESSAGE_DATA, 301U, data, sizeof(data), &capture);
    AssertOkAck(&capture, FW_UPDATE_MESSAGE_DATA);
    assert(body_writes == 1U);

    data[sizeof(data) - 1U] ^= 1U;
    ProcessRequest(FW_UPDATE_MESSAGE_DATA, 301U, data, sizeof(data), &capture);
    rejected = DecodeResponse(&capture);
    assert(rejected.type == FW_UPDATE_MESSAGE_NACK);
    assert(rejected.payload[1] == FW_UPDATE_STATUS_BAD_STATE);
    assert(body_writes == 1U);
    ProcessRequest(FW_UPDATE_MESSAGE_ABORT, 302U, NULL, 0U, &capture);
    AssertOkAck(&capture, FW_UPDATE_MESSAGE_ABORT);
    assert(!flash_open);
}

static void TestPendingImageAndLostRebootAckRecovery(void)
{
    vector_written = false;
    SendCapture capture = {.send_succeeds = true};
    uint8_t begin_payload[FW_UPDATE_BEGIN_UPDATE_SIZE];
    BuildBeginPayload(begin_payload);

    /* Target checks must run before flash erase before any write. */
    uint8_t wrong_target[sizeof(begin_payload)];
    memcpy(wrong_target, begin_payload, sizeof(begin_payload));
    wrong_target[0] ^= 1U;
    ProcessRequest(
        FW_UPDATE_MESSAGE_BEGIN_UPDATE, 100U, wrong_target, sizeof(wrong_target), &capture);
    FirmwareUpdateMessage target_rejected = DecodeResponse(&capture);
    assert(target_rejected.payload[1] == FW_UPDATE_STATUS_BAD_TARGET);
    assert(!flash_open);

    ProcessRequest(
        FW_UPDATE_MESSAGE_BEGIN_UPDATE, 1U, begin_payload, sizeof(begin_payload), &capture);
    AssertOkAck(&capture, FW_UPDATE_MESSAGE_BEGIN_UPDATE);

    uint8_t data_payload[FW_UPDATE_DATA_OFFSET_SIZE + sizeof(image)] = {0};
    FirmwareUpdateWriteBe32(data_payload, 0U);
    memcpy(&data_payload[FW_UPDATE_DATA_OFFSET_SIZE], image, sizeof(image));
    ProcessRequest(FW_UPDATE_MESSAGE_DATA, 2U, data_payload, sizeof(data_payload), &capture);
    AssertOkAck(&capture, FW_UPDATE_MESSAGE_DATA);

    ProcessRequest(FW_UPDATE_MESSAGE_END_UPDATE, 3U, NULL, 0U, &capture);
    AssertOkAck(&capture, FW_UPDATE_MESSAGE_END_UPDATE);
    assert(BootloaderAppIsValid());

    ProcessRequest(FW_UPDATE_MESSAGE_HELLO, 4U, NULL, 0U, &capture);
    FirmwareUpdateMessage board_info = DecodeResponse(&capture);
    assert(board_info.type == FW_UPDATE_MESSAGE_BOARD_INFO);
    assert(board_info.payload_length == FW_UPDATE_BOARD_INFO_SIZE);
    assert(FirmwareUpdateReadBe32(&board_info.payload[8]) == 0U);
    assert(board_info.payload[12] == FW_UPDATE_BOARD_PENDING_IMAGE);

    uint8_t reboot_frame[FW_UPDATE_MAX_FRAME_SIZE];
    size_t reboot_frame_length =
        EncodeRequest(FW_UPDATE_MESSAGE_REBOOT, 5U, NULL, 0U, reboot_frame);
    capture.send_succeeds = false;
    capture.calls = 0U;
    assert(!FirmwareUpdateBootloaderProcessFrame(
        reboot_frame, reboot_frame_length, CaptureResponse, &capture));
    AssertOkAck(&capture, FW_UPDATE_MESSAGE_REBOOT);
    assert(jump_count == 0U);
    assert(delay_count == 0U);
    uint8_t first_reboot_response[FW_UPDATE_MAX_FRAME_SIZE];
    size_t first_reboot_response_length = capture.frame_length;
    memcpy(first_reboot_response, capture.frame, first_reboot_response_length);

    capture.send_succeeds = true;
    capture.calls = 0U;
    assert(!FirmwareUpdateBootloaderProcessFrame(
        reboot_frame, reboot_frame_length, CaptureResponse, &capture));
    AssertOkAck(&capture, FW_UPDATE_MESSAGE_REBOOT);
    assert(capture.frame_length == first_reboot_response_length);
    assert(memcmp(capture.frame, first_reboot_response, first_reboot_response_length) == 0);
    assert(jump_count == 1U);
    assert(delay_count == 1U);
    assert(last_delay_ms == 50U);
}

/**
 * @brief Accepts identical and older versions without removing board checks.
 */
static void TestReflashAndDowngrade(void)
{
    SendCapture capture = {.send_succeeds = true};
    uint8_t manifest[FW_UPDATE_BEGIN_UPDATE_SIZE];
    const uint32_t versions[] = {TEST_FIRMWARE_VERSION, 1U};
    for (size_t i = 0U; i < sizeof(versions) / sizeof(versions[0]); i++)
    {
        vector_written = true;
        BuildBeginPayload(manifest);
        FirmwareUpdateWriteBe32(&manifest[4], versions[i]);
        ProcessRequest(FW_UPDATE_MESSAGE_BEGIN_UPDATE,
                       400U + (uint32_t)i * 2U,
                       manifest,
                       sizeof(manifest),
                       &capture);
        AssertOkAck(&capture, FW_UPDATE_MESSAGE_BEGIN_UPDATE);
        assert(flash_open);
        ProcessRequest(FW_UPDATE_MESSAGE_ABORT, 401U + (uint32_t)i * 2U, NULL, 0U, &capture);
        AssertOkAck(&capture, FW_UPDATE_MESSAGE_ABORT);
        assert(!flash_open);
    }
}

int main(void)
{
    TestInvalidImageStaysInRecovery();
    TestSplitVectorsAndDirectBodyWrite();
    TestRetryPayloadFingerprint();
    TestPendingImageAndLostRebootAckRecovery();
    TestReflashAndDowngrade();
    puts("Firmware update bootloader engine recovery tests passed");
    return 0;
}
