#include "bootloader_sha256.h"

#include <string.h>

static const uint32_t round_constants[64] = {
    0x428A2F98U, 0x71374491U, 0xB5C0FBCFU, 0xE9B5DBA5U,
    0x3956C25BU, 0x59F111F1U, 0x923F82A4U, 0xAB1C5ED5U,
    0xD807AA98U, 0x12835B01U, 0x243185BEU, 0x550C7DC3U,
    0x72BE5D74U, 0x80DEB1FEU, 0x9BDC06A7U, 0xC19BF174U,
    0xE49B69C1U, 0xEFBE4786U, 0x0FC19DC6U, 0x240CA1CCU,
    0x2DE92C6FU, 0x4A7484AAU, 0x5CB0A9DCU, 0x76F988DAU,
    0x983E5152U, 0xA831C66DU, 0xB00327C8U, 0xBF597FC7U,
    0xC6E00BF3U, 0xD5A79147U, 0x06CA6351U, 0x14292967U,
    0x27B70A85U, 0x2E1B2138U, 0x4D2C6DFCU, 0x53380D13U,
    0x650A7354U, 0x766A0ABBU, 0x81C2C92EU, 0x92722C85U,
    0xA2BFE8A1U, 0xA81A664BU, 0xC24B8B70U, 0xC76C51A3U,
    0xD192E819U, 0xD6990624U, 0xF40E3585U, 0x106AA070U,
    0x19A4C116U, 0x1E376C08U, 0x2748774CU, 0x34B0BCB5U,
    0x391C0CB3U, 0x4ED8AA4AU, 0x5B9CCA4FU, 0x682E6FF3U,
    0x748F82EEU, 0x78A5636FU, 0x84C87814U, 0x8CC70208U,
    0x90BEFFFAU, 0xA4506CEBU, 0xBEF9A3F7U, 0xC67178F2U,
};

static uint32_t RotateRight(uint32_t value, uint32_t bits)
{
    return (value >> bits) | (value << (32U - bits));
}

static uint32_t ReadBe32(const uint8_t *data)
{
    return ((uint32_t)data[0] << 24U) |
           ((uint32_t)data[1] << 16U) |
           ((uint32_t)data[2] << 8U) |
           (uint32_t)data[3];
}

static void WriteBe32(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)(value >> 24U);
    data[1] = (uint8_t)(value >> 16U);
    data[2] = (uint8_t)(value >> 8U);
    data[3] = (uint8_t)value;
}

static void Transform(BootloaderSha256Context *context, const uint8_t block[64])
{
    uint32_t schedule[64];
    for (size_t index = 0U; index < 16U; index++) {
        schedule[index] = ReadBe32(&block[index * 4U]);
    }
    for (size_t index = 16U; index < 64U; index++) {
        uint32_t x = schedule[index - 15U];
        uint32_t y = schedule[index - 2U];
        uint32_t sigma0 = RotateRight(x, 7U) ^ RotateRight(x, 18U) ^ (x >> 3U);
        uint32_t sigma1 = RotateRight(y, 17U) ^ RotateRight(y, 19U) ^ (y >> 10U);
        schedule[index] = schedule[index - 16U] + sigma0 +
                          schedule[index - 7U] + sigma1;
    }

    uint32_t a = context->state[0];
    uint32_t b = context->state[1];
    uint32_t c = context->state[2];
    uint32_t d = context->state[3];
    uint32_t e = context->state[4];
    uint32_t f = context->state[5];
    uint32_t g = context->state[6];
    uint32_t h = context->state[7];

    for (size_t index = 0U; index < 64U; index++) {
        uint32_t sum1 = RotateRight(e, 6U) ^ RotateRight(e, 11U) ^ RotateRight(e, 25U);
        uint32_t choose = (e & f) ^ ((~e) & g);
        uint32_t temp1 = h + sum1 + choose + round_constants[index] + schedule[index];
        uint32_t sum0 = RotateRight(a, 2U) ^ RotateRight(a, 13U) ^ RotateRight(a, 22U);
        uint32_t majority = (a & b) ^ (a & c) ^ (b & c);
        uint32_t temp2 = sum0 + majority;
        h = g;
        g = f;
        f = e;
        e = d + temp1;
        d = c;
        c = b;
        b = a;
        a = temp1 + temp2;
    }

    context->state[0] += a;
    context->state[1] += b;
    context->state[2] += c;
    context->state[3] += d;
    context->state[4] += e;
    context->state[5] += f;
    context->state[6] += g;
    context->state[7] += h;
}

void BootloaderSha256Init(BootloaderSha256Context *context)
{
    static const uint32_t initial_state[8] = {
        0x6A09E667U, 0xBB67AE85U, 0x3C6EF372U, 0xA54FF53AU,
        0x510E527FU, 0x9B05688CU, 0x1F83D9ABU, 0x5BE0CD19U,
    };
    memcpy(context->state, initial_state, sizeof(initial_state));
    context->total_bytes = 0U;
    context->buffer_length = 0U;
}

void BootloaderSha256Update(BootloaderSha256Context *context,
                            const uint8_t *data,
                            size_t length)
{
    context->total_bytes += length;
    while (length > 0U) {
        size_t available = sizeof(context->buffer) - context->buffer_length;
        size_t copy_length = length < available ? length : available;
        memcpy(&context->buffer[context->buffer_length], data, copy_length);
        context->buffer_length += copy_length;
        data += copy_length;
        length -= copy_length;
        if (context->buffer_length == sizeof(context->buffer)) {
            Transform(context, context->buffer);
            context->buffer_length = 0U;
        }
    }
}

void BootloaderSha256Final(BootloaderSha256Context *context, uint8_t digest[32])
{
    uint64_t total_bits = context->total_bytes * 8U;
    context->buffer[context->buffer_length++] = 0x80U;
    if (context->buffer_length > 56U) {
        memset(&context->buffer[context->buffer_length],
               0,
               sizeof(context->buffer) - context->buffer_length);
        Transform(context, context->buffer);
        context->buffer_length = 0U;
    }
    memset(&context->buffer[context->buffer_length], 0, 56U - context->buffer_length);
    for (size_t index = 0U; index < 8U; index++) {
        context->buffer[63U - index] = (uint8_t)(total_bits >> (index * 8U));
    }
    Transform(context, context->buffer);
    for (size_t index = 0U; index < 8U; index++) {
        WriteBe32(&digest[index * 4U], context->state[index]);
    }
}
