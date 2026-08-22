#include "sunlite_ota_protocol.h"

#include <string.h>

#define SUNLITE_OTA_MAGIC_0 'S'
#define SUNLITE_OTA_MAGIC_1 'U'
#define SUNLITE_OTA_MAX_RESPONSE_PAYLOAD SUNLITE_OTA_BOARD_INFO_SIZE

uint16_t SunliteOtaReadBe16(const uint8_t *data)
{
    return ((uint16_t)data[0] << 8U) | (uint16_t)data[1];
}

uint32_t SunliteOtaReadBe32(const uint8_t *data)
{
    return ((uint32_t)data[0] << 24U) |
           ((uint32_t)data[1] << 16U) |
           ((uint32_t)data[2] << 8U) |
           (uint32_t)data[3];
}

void SunliteOtaWriteBe16(uint8_t *data, uint16_t value)
{
    data[0] = (uint8_t)(value >> 8U);
    data[1] = (uint8_t)value;
}

void SunliteOtaWriteBe32(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)(value >> 24U);
    data[1] = (uint8_t)(value >> 16U);
    data[2] = (uint8_t)(value >> 8U);
    data[3] = (uint8_t)value;
}

static uint32_t SunliteOtaCrc32Update(uint32_t crc,
                                     const uint8_t *data,
                                     size_t length)
{
    for (size_t index = 0U; index < length; index++) {
        crc ^= data[index];
        for (uint32_t bit = 0U; bit < 8U; bit++) {
            crc = ((crc & 1U) != 0U) ?
                ((crc >> 1U) ^ 0xEDB88320U) :
                (crc >> 1U);
        }
    }
    return crc;
}

uint32_t SunliteOtaCrc32(const uint8_t *data, size_t length)
{
    return SunliteOtaCrc32Update(0xFFFFFFFFU, data, length) ^ 0xFFFFFFFFU;
}

static size_t SunliteOtaCobsEncode(const uint8_t *input,
                                   size_t input_length,
                                   uint8_t *output,
                                   size_t output_capacity)
{
    if (output_capacity < 2U) {
        return 0U;
    }

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
            if (write_index >= output_capacity) {
                return 0U;
            }
            output[write_index++] = input[read_index++];
            code++;
            if (code == 0xFFU) {
                output[code_index] = code;
                code_index = write_index++;
                code = 1U;
            }
        }
        if (write_index > output_capacity) {
            return 0U;
        }
    }

    output[code_index] = code;
    return write_index;
}

static size_t SunliteOtaCobsDecode(const uint8_t *input,
                                   size_t input_length,
                                   uint8_t *output,
                                   size_t output_capacity)
{
    size_t read_index = 0U;
    size_t write_index = 0U;

    while (read_index < input_length) {
        uint8_t code = input[read_index++];
        if (code == 0U) {
            return 0U;
        }
        size_t copy_length = (size_t)code - 1U;
        if ((copy_length > input_length - read_index) ||
            (copy_length > output_capacity - write_index)) {
            return 0U;
        }
        memcpy(&output[write_index], &input[read_index], copy_length);
        read_index += copy_length;
        write_index += copy_length;
        if ((code != 0xFFU) && (read_index < input_length)) {
            if (write_index >= output_capacity) {
                return 0U;
            }
            output[write_index++] = 0U;
        }
    }
    return write_index;
}

size_t SunliteOtaFrameEncode(SunliteOtaMessageType type,
                             uint32_t session_id,
                             uint32_t sequence,
                             const uint8_t *payload,
                             uint16_t payload_length,
                             uint8_t *encoded,
                             size_t encoded_capacity)
{
    uint8_t raw[SUNLITE_OTA_HEADER_SIZE + SUNLITE_OTA_MAX_RESPONSE_PAYLOAD +
                SUNLITE_OTA_FRAME_CRC_SIZE] = {0};
    if ((payload_length > SUNLITE_OTA_MAX_RESPONSE_PAYLOAD) ||
        ((payload_length > 0U) && (payload == NULL))) {
        return 0U;
    }

    raw[0] = SUNLITE_OTA_MAGIC_0;
    raw[1] = SUNLITE_OTA_MAGIC_1;
    raw[2] = SUNLITE_OTA_PROTOCOL_VERSION;
    raw[3] = (uint8_t)type;
    SunliteOtaWriteBe32(&raw[4], session_id);
    SunliteOtaWriteBe32(&raw[8], sequence);
    SunliteOtaWriteBe16(&raw[12], payload_length);
    if (payload_length > 0U) {
        memcpy(&raw[SUNLITE_OTA_HEADER_SIZE], payload, payload_length);
    }
    size_t body_length = SUNLITE_OTA_HEADER_SIZE + payload_length;
    SunliteOtaWriteBe32(&raw[body_length], SunliteOtaCrc32(raw, body_length));
    size_t raw_length = body_length + SUNLITE_OTA_FRAME_CRC_SIZE;
    size_t encoded_length = SunliteOtaCobsEncode(
        raw,
        raw_length,
        encoded,
        encoded_capacity > 0U ? encoded_capacity - 1U : 0U);
    if ((encoded_length == 0U) || (encoded_length >= encoded_capacity)) {
        return 0U;
    }
    encoded[encoded_length++] = 0U;
    return encoded_length;
}

bool SunliteOtaFrameDecode(const uint8_t *encoded,
                           size_t encoded_length,
                           uint8_t *raw,
                           size_t raw_capacity,
                           SunliteOtaMessage *message)
{
    if ((encoded == NULL) || (raw == NULL) || (message == NULL) ||
        (encoded_length == 0U)) {
        return false;
    }
    if (encoded[encoded_length - 1U] == 0U) {
        encoded_length--;
    }
    size_t raw_length = SunliteOtaCobsDecode(
        encoded, encoded_length, raw, raw_capacity);
    if (raw_length < SUNLITE_OTA_HEADER_SIZE + SUNLITE_OTA_FRAME_CRC_SIZE) {
        return false;
    }
    size_t body_length = raw_length - SUNLITE_OTA_FRAME_CRC_SIZE;
    uint32_t received_crc = SunliteOtaReadBe32(&raw[body_length]);
    if (received_crc != SunliteOtaCrc32(raw, body_length)) {
        return false;
    }
    if ((raw[0] != SUNLITE_OTA_MAGIC_0) ||
        (raw[1] != SUNLITE_OTA_MAGIC_1) ||
        (raw[2] != SUNLITE_OTA_PROTOCOL_VERSION) ||
        (raw[3] < SUNLITE_OTA_MESSAGE_HELLO) ||
        (raw[3] > SUNLITE_OTA_MESSAGE_ABORT)) {
        return false;
    }
    uint16_t payload_length = SunliteOtaReadBe16(&raw[12]);
    if ((size_t)payload_length != body_length - SUNLITE_OTA_HEADER_SIZE) {
        return false;
    }

    message->type = (SunliteOtaMessageType)raw[3];
    message->session_id = SunliteOtaReadBe32(&raw[4]);
    message->sequence = SunliteOtaReadBe32(&raw[8]);
    message->payload_length = payload_length;
    message->payload = &raw[SUNLITE_OTA_HEADER_SIZE];
    return true;
}
