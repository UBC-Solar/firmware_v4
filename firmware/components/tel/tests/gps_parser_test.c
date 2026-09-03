#include <assert.h>
#include <stdint.h>

#include "gps_parser.h"

#define TEST_PAYLOAD_LENGTH 116U
#define TEST_FRAME_LENGTH   (TEST_PAYLOAD_LENGTH + 8U)

static void WriteU16(uint8_t *destination, uint16_t value)
{
    destination[0] = (uint8_t)value;
    destination[1] = (uint8_t)(value >> 8U);
}

static void WriteU32(uint8_t *destination, uint32_t value)
{
    destination[0] = (uint8_t)value;
    destination[1] = (uint8_t)(value >> 8U);
    destination[2] = (uint8_t)(value >> 16U);
    destination[3] = (uint8_t)(value >> 24U);
}

static void WriteI32(uint8_t *destination, int32_t value)
{
    WriteU32(destination, (uint32_t)value);
}

static void CompleteChecksum(uint8_t *frame)
{
    uint8_t checksum_a = 0U;
    uint8_t checksum_b = 0U;
    uint16_t index;

    for (index = 2U; index < (TEST_FRAME_LENGTH - 2U); index++)
    {
        checksum_a = (uint8_t)(checksum_a + frame[index]);
        checksum_b = (uint8_t)(checksum_b + checksum_a);
    }

    frame[TEST_FRAME_LENGTH - 2U] = checksum_a;
    frame[TEST_FRAME_LENGTH - 1U] = checksum_b;
}

int main(void)
{
    uint8_t frame[TEST_FRAME_LENGTH] =
    {
        0xB5U, 0x62U, 0x01U, 0x17U, TEST_PAYLOAD_LENGTH, 0x00U
    };
    uint8_t *payload = &frame[6];

    WriteU32(&payload[0], 123456U);
    payload[4] = 0U;
    payload[5] = 0x03U;
    WriteU16(&payload[6], 2026U);
    payload[8] = 8U;
    payload[9] = 22U;
    payload[10] = 21U;
    payload[11] = 5U;
    payload[12] = 7U;
    payload[24] = 3U;
    payload[25] = 0x39U;
    payload[27] = 12U;
    WriteI32(&payload[28], -1231207000);
    WriteI32(&payload[32], 492827000);
    WriteI32(&payload[36], 70000);
    WriteI32(&payload[40], 65000);
    WriteU32(&payload[44], 1500U);
    WriteU32(&payload[48], 2500U);
    WriteI32(&payload[52], 1000);
    WriteI32(&payload[56], -2000);
    WriteI32(&payload[60], 50);
    WriteI32(&payload[64], 2236);
    WriteU32(&payload[68], 100U);
    WriteI32(&payload[72], -125000);
    WriteI32(&payload[76], 250000);
    WriteI32(&payload[80], 9000000);
    WriteI32(&payload[84], 9100000);
    CompleteChecksum(frame);

    GpsParserInit();
    GpsParserConsume(frame, 3U);
    GpsParserConsume(&frame[3], 17U);
    GpsParserConsume(&frame[20], TEST_FRAME_LENGTH - 20U);

    assert(g_gps_parser_debug.valid_frame_count == 1U);
    assert(g_gps_parser_debug.nav_frame_count == 1U);
    assert(g_gps_navigation.update_count == 1U);
    assert(g_gps_navigation.message_id == 0x17U);
    assert(g_gps_navigation.fix_valid);
    assert(g_gps_navigation.attitude_valid);
    assert(g_gps_navigation.year == 2026U);
    assert(g_gps_navigation.satellites == 12U);
    assert(g_gps_navigation.longitude_e7 == -1231207000);
    assert(g_gps_navigation.latitude_e7 == 492827000);
    assert(g_gps_navigation.longitude_deg == -123.1207);
    assert(g_gps_navigation.latitude_deg == 49.2827);
    assert(g_gps_navigation.ground_speed_mm_s == 2236);
    assert(g_gps_navigation.vehicle_roll_e5 == -125000);

    frame[TEST_FRAME_LENGTH - 1U] ^= 0x01U;
    GpsParserConsume(frame, TEST_FRAME_LENGTH);
    assert(g_gps_parser_debug.checksum_error_count == 1U);
    assert(g_gps_navigation.update_count == 1U);

    return 0;
}
