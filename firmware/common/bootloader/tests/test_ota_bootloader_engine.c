#include "bootloader.h"
#include "bootloader_flash.h"
#include "bootloader_metadata.h"
#include "bootloader_sha256.h"
#include "monocypher-ed25519.h"
#include "sunlite_ota_bootloader_engine.h"
#include "sunlite_ota_protocol.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define TEST_TARGET_ID        0x54454C45U
#define TEST_FIRMWARE_VERSION 42U
#define TEST_SESSION_ID       0x10203040U
#define SIGNING_DOMAIN_SIZE   24U

static const uint8_t signing_domain[SIGNING_DOMAIN_SIZE] = {
    'S', 'U', 'N', 'L', 'I', 'T', 'E', '-', 'O', 'T', 'A', '-',
    'M', 'A', 'N', 'I', 'F', 'E', 'S', 'T', '-', 'V', '1', 0,
};

static const uint8_t test_seed[32] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
};

static const uint8_t image[] = {
    0x00, 0xC0, 0x00, 0x20, /* initial SP: 0x2000c000 */
    0x01, 0x80, 0x00, 0x08, /* reset handler: 0x08008001 */
};

typedef struct {
    bool send_succeeds;
    uint8_t frame[SUNLITE_OTA_MAX_ENCODED_FRAME];
    size_t frame_length;
    unsigned calls;
} SendCapture;

static bool flash_open;
static bool vector_written;
static bool metadata_valid;
static uint32_t metadata_version;
static uint32_t metadata_image_size;
static uint8_t metadata_digest[SUNLITE_OTA_SHA256_SIZE];
static unsigned jump_count;
static unsigned delay_count;
static uint32_t last_delay_ms;
static bool trial_armed;

bool SunliteOtaArmTrialBoot(void)
{
    trial_armed = true;
    return true;
}

bool BootloaderAppIsValid(void)
{
    return metadata_valid && vector_written;
}

bool BootloaderShouldEnterUpdateMode(void)
{
    return false;
}

void BootloaderJumpToApp(void)
{
    jump_count++;
}

void BootloaderRun(void)
{
}

void BootloaderPrepareWatchdog(void)
{
}

void BootloaderServiceWatchdog(void)
{
}

bool BootloaderBoardStayInBootloader(void)
{
    return false;
}

void BootloaderEnterUpdateMode(void)
{
}

bool BootloaderFlashBeginAppUpdate(uint32_t image_size)
{
    assert(image_size == sizeof(image));
    flash_open = true;
    vector_written = false;
    metadata_valid = false;
    trial_armed = false;
    return true;
}

bool BootloaderFlashWrite(uint32_t address,
                          const uint8_t *data,
                          size_t length)
{
    assert(flash_open);
    assert(address == BOOTLOADER_APP_START_ADDRESS);
    assert(length == sizeof(image));

    static const uint8_t erased_vector[sizeof(image)] = {
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    };
    if (memcmp(data, erased_vector, sizeof(erased_vector)) == 0) {
        return true;
    }
    assert(trial_armed);
    assert(memcmp(data, image, sizeof(image)) == 0);
    vector_written = true;
    return true;
}

void BootloaderFlashEndAppUpdate(void)
{
    flash_open = false;
}

bool BootloaderMetadataRead(uint32_t *firmware_version,
                            uint32_t *image_size,
                            uint8_t image_sha256[SUNLITE_OTA_SHA256_SIZE])
{
    if (firmware_version != NULL) {
        *firmware_version = metadata_valid ? metadata_version : 0U;
    }
    if (image_size != NULL) {
        *image_size = metadata_valid ? metadata_image_size : 0U;
    }
    if (image_sha256 != NULL) {
        if (metadata_valid) {
            memcpy(image_sha256, metadata_digest, sizeof(metadata_digest));
        } else {
            memset(image_sha256, 0, sizeof(metadata_digest));
        }
    }
    return metadata_valid;
}

bool BootloaderMetadataWrite(uint32_t firmware_version,
                             uint32_t image_size,
                             const uint8_t image_sha256[SUNLITE_OTA_SHA256_SIZE])
{
    assert(flash_open);
    metadata_version = firmware_version;
    metadata_image_size = image_size;
    memcpy(metadata_digest, image_sha256, sizeof(metadata_digest));
    metadata_valid = true;
    return true;
}

void HAL_Delay(uint32_t delay_ms)
{
    delay_count++;
    last_delay_ms = delay_ms;
}

static bool CaptureResponse(const uint8_t *frame,
                            size_t frame_length,
                            void *context)
{
    SendCapture *capture = context;
    assert(frame_length <= sizeof(capture->frame));
    memcpy(capture->frame, frame, frame_length);
    capture->frame_length = frame_length;
    capture->calls++;
    return capture->send_succeeds;
}

static size_t CobsEncodeRequest(const uint8_t *input,
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
            assert(write_index < output_capacity);
            code_index = write_index++;
            code = 1U;
            read_index++;
        } else {
            assert(write_index < output_capacity);
            output[write_index++] = input[read_index++];
            code++;
            if (code == 0xFFU) {
                output[code_index] = code;
                assert(write_index < output_capacity);
                code_index = write_index++;
                code = 1U;
            }
        }
    }
    output[code_index] = code;
    return write_index;
}

static size_t EncodeRequest(SunliteOtaMessageType type,
                            uint32_t sequence,
                            const uint8_t *payload,
                            uint16_t payload_length,
                            uint8_t frame[SUNLITE_OTA_MAX_ENCODED_FRAME])
{
    assert(payload_length <= SUNLITE_OTA_MAX_RX_PAYLOAD);
    assert((payload_length == 0U) || (payload != NULL));
    uint8_t raw[SUNLITE_OTA_MAX_RAW_FRAME] = {0};
    raw[0] = 'S';
    raw[1] = 'U';
    raw[2] = SUNLITE_OTA_PROTOCOL_VERSION;
    raw[3] = (uint8_t)type;
    SunliteOtaWriteBe32(&raw[4], TEST_SESSION_ID);
    SunliteOtaWriteBe32(&raw[8], sequence);
    SunliteOtaWriteBe16(&raw[12], payload_length);
    if (payload_length > 0U) {
        memcpy(&raw[SUNLITE_OTA_HEADER_SIZE], payload, payload_length);
    }
    size_t body_length = SUNLITE_OTA_HEADER_SIZE + payload_length;
    SunliteOtaWriteBe32(&raw[body_length],
                        SunliteOtaCrc32(raw, body_length));
    size_t raw_length = body_length + SUNLITE_OTA_FRAME_CRC_SIZE;
    size_t frame_length = CobsEncodeRequest(raw,
                                            raw_length,
                                            frame,
                                            SUNLITE_OTA_MAX_ENCODED_FRAME - 1U);
    assert(frame_length < SUNLITE_OTA_MAX_ENCODED_FRAME);
    frame[frame_length++] = 0U;
    return frame_length;
}

static SunliteOtaMessage DecodeResponse(const SendCapture *capture,
                                        uint8_t raw[SUNLITE_OTA_MAX_RAW_FRAME])
{
    SunliteOtaMessage response;
    assert(capture->calls == 1U);
    assert(SunliteOtaFrameDecode(capture->frame,
                                 capture->frame_length,
                                 raw,
                                 SUNLITE_OTA_MAX_RAW_FRAME,
                                 &response));
    return response;
}

static void AssertOkAck(const SendCapture *capture,
                        SunliteOtaMessageType request_type)
{
    uint8_t raw[SUNLITE_OTA_MAX_RAW_FRAME];
    SunliteOtaMessage response = DecodeResponse(capture, raw);
    assert(response.type == SUNLITE_OTA_MESSAGE_ACK);
    assert(response.payload_length == SUNLITE_OTA_ACK_SIZE);
    assert(response.payload[0] == (uint8_t)request_type);
    assert(response.payload[1] == SUNLITE_OTA_STATUS_OK);
}

static void ProcessRequest(SunliteOtaMessageType type,
                           uint32_t sequence,
                           const uint8_t *payload,
                           uint16_t payload_length,
                           SendCapture *capture)
{
    uint8_t frame[SUNLITE_OTA_MAX_ENCODED_FRAME];
    size_t frame_length = EncodeRequest(type,
                                        sequence,
                                        payload,
                                        payload_length,
                                        frame);
    capture->frame_length = 0U;
    capture->calls = 0U;
    (void)SunliteOtaBootloaderProcessFrame(frame,
                                           frame_length,
                                           CaptureResponse,
                                           capture);
}

static void BuildSignedBeginPayload(
    uint8_t payload[SUNLITE_OTA_BEGIN_UPDATE_SIZE])
{
    uint8_t digest[SUNLITE_OTA_SHA256_SIZE];
    BootloaderSha256Context sha256;
    BootloaderSha256Init(&sha256);
    BootloaderSha256Update(&sha256, image, sizeof(image));
    BootloaderSha256Final(&sha256, digest);

    memset(payload, 0, SUNLITE_OTA_BEGIN_UPDATE_SIZE);
    SunliteOtaWriteBe32(&payload[0], TEST_TARGET_ID);
    SunliteOtaWriteBe16(&payload[4], 1U);
    SunliteOtaWriteBe16(&payload[6], 1U);
    SunliteOtaWriteBe32(&payload[8], TEST_FIRMWARE_VERSION);
    SunliteOtaWriteBe32(&payload[12], sizeof(image));
    SunliteOtaWriteBe32(&payload[16], 1U);
    memcpy(&payload[20], digest, sizeof(digest));

    uint8_t signed_message[SIGNING_DOMAIN_SIZE +
                           SUNLITE_OTA_SIGNING_FIELDS_SIZE];
    memcpy(signed_message, signing_domain, sizeof(signing_domain));
    memcpy(&signed_message[sizeof(signing_domain)],
           payload,
           SUNLITE_OTA_SIGNING_FIELDS_SIZE);

    uint8_t seed[sizeof(test_seed)];
    uint8_t secret_key[64];
    uint8_t public_key[32];
    memcpy(seed, test_seed, sizeof(seed));
    crypto_ed25519_key_pair(secret_key, public_key, seed);
    static const uint8_t expected_public_key[32] = {
        0x03, 0xA1, 0x07, 0xBF, 0xF3, 0xCE, 0x10, 0xBE,
        0x1D, 0x70, 0xDD, 0x18, 0xE7, 0x4B, 0xC0, 0x99,
        0x67, 0xE4, 0xD6, 0x30, 0x9B, 0xA5, 0x0D, 0x5F,
        0x1D, 0xDC, 0x86, 0x64, 0x12, 0x55, 0x31, 0xB8,
    };
    assert(memcmp(public_key, expected_public_key, sizeof(public_key)) == 0);
    crypto_ed25519_sign(&payload[SUNLITE_OTA_SIGNING_FIELDS_SIZE],
                        secret_key,
                        signed_message,
                        sizeof(signed_message));
}

static void TestPendingImageAndLostRebootAckRecovery(void)
{
    SendCapture capture = {.send_succeeds = true};
    uint8_t begin_payload[SUNLITE_OTA_BEGIN_UPDATE_SIZE];
    BuildSignedBeginPayload(begin_payload);

    ProcessRequest(SUNLITE_OTA_MESSAGE_BEGIN_UPDATE,
                   1U,
                   begin_payload,
                   sizeof(begin_payload),
                   &capture);
    AssertOkAck(&capture, SUNLITE_OTA_MESSAGE_BEGIN_UPDATE);

    uint8_t data_payload[SUNLITE_OTA_DATA_OFFSET_SIZE + sizeof(image)] = {0};
    SunliteOtaWriteBe32(data_payload, 0U);
    memcpy(&data_payload[SUNLITE_OTA_DATA_OFFSET_SIZE], image, sizeof(image));
    ProcessRequest(SUNLITE_OTA_MESSAGE_DATA,
                   2U,
                   data_payload,
                   sizeof(data_payload),
                   &capture);
    AssertOkAck(&capture, SUNLITE_OTA_MESSAGE_DATA);

    ProcessRequest(SUNLITE_OTA_MESSAGE_END_UPDATE,
                   3U,
                   NULL,
                   0U,
                   &capture);
    AssertOkAck(&capture, SUNLITE_OTA_MESSAGE_END_UPDATE);
    assert(BootloaderAppIsValid());
    assert(trial_armed);

    ProcessRequest(SUNLITE_OTA_MESSAGE_HELLO,
                   4U,
                   NULL,
                   0U,
                   &capture);
    uint8_t raw[SUNLITE_OTA_MAX_RAW_FRAME];
    SunliteOtaMessage board_info = DecodeResponse(&capture, raw);
    assert(board_info.type == SUNLITE_OTA_MESSAGE_BOARD_INFO);
    assert(board_info.payload_length == SUNLITE_OTA_BOARD_INFO_SIZE);
    assert(SunliteOtaReadBe32(&board_info.payload[10]) ==
           TEST_FIRMWARE_VERSION);
    assert(board_info.payload[15] == SUNLITE_OTA_BOARD_PENDING_IMAGE);

    uint8_t reboot_frame[SUNLITE_OTA_MAX_ENCODED_FRAME];
    size_t reboot_frame_length = EncodeRequest(SUNLITE_OTA_MESSAGE_REBOOT,
                                               5U,
                                               NULL,
                                               0U,
                                               reboot_frame);
    capture.send_succeeds = false;
    capture.calls = 0U;
    assert(!SunliteOtaBootloaderProcessFrame(reboot_frame,
                                             reboot_frame_length,
                                             CaptureResponse,
                                             &capture));
    AssertOkAck(&capture, SUNLITE_OTA_MESSAGE_REBOOT);
    assert(jump_count == 0U);
    assert(delay_count == 0U);
    uint8_t first_reboot_response[SUNLITE_OTA_MAX_ENCODED_FRAME];
    size_t first_reboot_response_length = capture.frame_length;
    memcpy(first_reboot_response,
           capture.frame,
           first_reboot_response_length);

    capture.send_succeeds = true;
    capture.calls = 0U;
    assert(!SunliteOtaBootloaderProcessFrame(reboot_frame,
                                             reboot_frame_length,
                                             CaptureResponse,
                                             &capture));
    AssertOkAck(&capture, SUNLITE_OTA_MESSAGE_REBOOT);
    assert(capture.frame_length == first_reboot_response_length);
    assert(memcmp(capture.frame,
                  first_reboot_response,
                  first_reboot_response_length) == 0);
    assert(jump_count == 1U);
    assert(delay_count == 1U);
    assert(last_delay_ms == 50U);
}

int main(void)
{
    TestPendingImageAndLostRebootAckRecovery();
    puts("Sunlite OTA bootloader engine recovery tests passed");
    return 0;
}
