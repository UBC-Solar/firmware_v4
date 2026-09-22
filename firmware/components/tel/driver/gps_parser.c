#include "gps_parser.h"

#include <string.h>

#define GPS_UBX_SYNC_1                 0xB5U
#define GPS_UBX_SYNC_2                 0x62U
#define GPS_UBX_CLASS_NAV              0x01U
#define GPS_UBX_NAV_PVT_ID             0x07U
#define GPS_UBX_NAV_PVAT_ID            0x17U
#define GPS_UBX_NAV_PVT_LENGTH         92U
#define GPS_UBX_NAV_PVAT_LENGTH        116U
#define GPS_UBX_MAX_PAYLOAD_LENGTH     GPS_UBX_NAV_PVAT_LENGTH

typedef enum
{
    GPS_PARSE_SYNC_1 = 0,
    GPS_PARSE_SYNC_2,
    GPS_PARSE_CLASS,
    GPS_PARSE_ID,
    GPS_PARSE_LENGTH_LOW,
    GPS_PARSE_LENGTH_HIGH,
    GPS_PARSE_PAYLOAD,
    GPS_PARSE_CHECKSUM_A,
    GPS_PARSE_CHECKSUM_B
} GPSParserState;

typedef struct
{
    GPSParserState state;
    uint8_t message_class;
    uint8_t message_id;
    uint16_t payload_length;
    uint16_t payload_index;
    uint8_t payload[GPS_UBX_MAX_PAYLOAD_LENGTH];
    uint8_t checksum_a;
    uint8_t checksum_b;
    uint8_t received_checksum_a;
} GPSParserContext;

volatile GPSNavigationData g_gps_navigation = {0};
volatile GPSParserDebugInfo g_gps_parser_debug = {0};
static GPSParserContext g_gps_parser = {0};

static uint16_t GpsParserReadU16(const uint8_t *data)
{
    return (uint16_t)data[0] | ((uint16_t)data[1] << 8U);
}

static uint32_t GpsParserReadU32(const uint8_t *data)
{
    return (uint32_t)data[0] |
           ((uint32_t)data[1] << 8U) |
           ((uint32_t)data[2] << 16U) |
           ((uint32_t)data[3] << 24U);
}

static int32_t GpsParserReadI32(const uint8_t *data)
{
    uint32_t unsigned_value = GpsParserReadU32(data);
    int32_t signed_value;

    memcpy(&signed_value, &unsigned_value, sizeof(signed_value));
    return signed_value;
}

static void GpsParserChecksumByte(uint8_t byte)
{
    g_gps_parser.checksum_a = (uint8_t)(g_gps_parser.checksum_a + byte);
    g_gps_parser.checksum_b =
        (uint8_t)(g_gps_parser.checksum_b + g_gps_parser.checksum_a);
}

static void GpsParserResetFrame(void)
{
    g_gps_parser.state = GPS_PARSE_SYNC_1;
    g_gps_parser.payload_length = 0U;
    g_gps_parser.payload_index = 0U;
    g_gps_parser.checksum_a = 0U;
    g_gps_parser.checksum_b = 0U;
    g_gps_parser.received_checksum_a = 0U;
}

static void GpsParserPublishPvat(const uint8_t *payload)
{
    GPSNavigationData navigation = {0};
    uint8_t valid_flags = payload[5];
    uint8_t fix_flags = payload[25];

    navigation.message_valid = true;
    navigation.fix_valid = ((fix_flags & 0x01U) != 0U);
    navigation.date_valid = ((valid_flags & 0x01U) != 0U);
    navigation.time_valid = ((valid_flags & 0x02U) != 0U);
    navigation.attitude_valid = ((fix_flags & 0x38U) == 0x38U);
    navigation.update_count = g_gps_navigation.update_count + 1U;
    navigation.message_id = GPS_UBX_NAV_PVAT_ID;
    navigation.i_tow_ms = GpsParserReadU32(&payload[0]);
    navigation.year = GpsParserReadU16(&payload[6]);
    navigation.month = payload[8];
    navigation.day = payload[9];
    navigation.hour = payload[10];
    navigation.minute = payload[11];
    navigation.second = payload[12];
    navigation.fix_type = payload[24];
    navigation.satellites = payload[27];
    navigation.longitude_e7 = GpsParserReadI32(&payload[28]);
    navigation.latitude_e7 = GpsParserReadI32(&payload[32]);
    navigation.longitude_deg = (double)navigation.longitude_e7 / 10000000.0;
    navigation.latitude_deg = (double)navigation.latitude_e7 / 10000000.0;
    navigation.height_mm = GpsParserReadI32(&payload[36]);
    navigation.height_msl_mm = GpsParserReadI32(&payload[40]);
    navigation.horizontal_accuracy_mm = GpsParserReadU32(&payload[44]);
    navigation.vertical_accuracy_mm = GpsParserReadU32(&payload[48]);
    navigation.velocity_north_mm_s = GpsParserReadI32(&payload[52]);
    navigation.velocity_east_mm_s = GpsParserReadI32(&payload[56]);
    navigation.velocity_down_mm_s = GpsParserReadI32(&payload[60]);
    navigation.ground_speed_mm_s = GpsParserReadI32(&payload[64]);
    navigation.speed_accuracy_mm_s = GpsParserReadU32(&payload[68]);
    navigation.vehicle_roll_e5 = GpsParserReadI32(&payload[72]);
    navigation.vehicle_pitch_e5 = GpsParserReadI32(&payload[76]);
    navigation.vehicle_heading_e5 = GpsParserReadI32(&payload[80]);
    navigation.motion_heading_e5 = GpsParserReadI32(&payload[84]);

    g_gps_navigation = navigation;
}

static void GpsParserPublishPvt(const uint8_t *payload)
{
    GPSNavigationData navigation = {0};
    uint8_t valid_flags = payload[11];
    uint8_t fix_flags = payload[21];

    navigation.message_valid = true;
    navigation.fix_valid = ((fix_flags & 0x01U) != 0U);
    navigation.date_valid = ((valid_flags & 0x01U) != 0U);
    navigation.time_valid = ((valid_flags & 0x02U) != 0U);
    navigation.update_count = g_gps_navigation.update_count + 1U;
    navigation.message_id = GPS_UBX_NAV_PVT_ID;
    navigation.i_tow_ms = GpsParserReadU32(&payload[0]);
    navigation.year = GpsParserReadU16(&payload[4]);
    navigation.month = payload[6];
    navigation.day = payload[7];
    navigation.hour = payload[8];
    navigation.minute = payload[9];
    navigation.second = payload[10];
    navigation.fix_type = payload[20];
    navigation.satellites = payload[23];
    navigation.longitude_e7 = GpsParserReadI32(&payload[24]);
    navigation.latitude_e7 = GpsParserReadI32(&payload[28]);
    navigation.longitude_deg = (double)navigation.longitude_e7 / 10000000.0;
    navigation.latitude_deg = (double)navigation.latitude_e7 / 10000000.0;
    navigation.height_mm = GpsParserReadI32(&payload[32]);
    navigation.height_msl_mm = GpsParserReadI32(&payload[36]);
    navigation.horizontal_accuracy_mm = GpsParserReadU32(&payload[40]);
    navigation.vertical_accuracy_mm = GpsParserReadU32(&payload[44]);
    navigation.velocity_north_mm_s = GpsParserReadI32(&payload[48]);
    navigation.velocity_east_mm_s = GpsParserReadI32(&payload[52]);
    navigation.velocity_down_mm_s = GpsParserReadI32(&payload[56]);
    navigation.ground_speed_mm_s = GpsParserReadI32(&payload[60]);
    navigation.motion_heading_e5 = GpsParserReadI32(&payload[64]);
    navigation.speed_accuracy_mm_s = GpsParserReadU32(&payload[68]);

    g_gps_navigation = navigation;
}

static void GpsParserHandleFrame(void)
{
    g_gps_parser_debug.valid_frame_count++;
    g_gps_parser_debug.last_message_class = g_gps_parser.message_class;
    g_gps_parser_debug.last_message_id = g_gps_parser.message_id;
    g_gps_parser_debug.last_payload_length = g_gps_parser.payload_length;

    if ((g_gps_parser.message_class == GPS_UBX_CLASS_NAV) &&
        (g_gps_parser.message_id == GPS_UBX_NAV_PVAT_ID) &&
        (g_gps_parser.payload_length == GPS_UBX_NAV_PVAT_LENGTH))
    {
        GpsParserPublishPvat(g_gps_parser.payload);
        g_gps_parser_debug.nav_frame_count++;
    }
    else if ((g_gps_parser.message_class == GPS_UBX_CLASS_NAV) &&
             (g_gps_parser.message_id == GPS_UBX_NAV_PVT_ID) &&
             (g_gps_parser.payload_length == GPS_UBX_NAV_PVT_LENGTH))
    {
        GpsParserPublishPvt(g_gps_parser.payload);
        g_gps_parser_debug.nav_frame_count++;
    }
    else
    {
        g_gps_parser_debug.ignored_frame_count++;
    }
}

void GpsParserInit(void)
{
    g_gps_navigation = (GPSNavigationData){0};
    g_gps_parser_debug = (GPSParserDebugInfo){0};
    memset(&g_gps_parser, 0, sizeof(g_gps_parser));
    GpsParserResetFrame();
}

void GpsParserConsume(const uint8_t *data, uint16_t length)
{
    uint16_t data_index;

    if (data == NULL)
    {
        return;
    }

    for (data_index = 0U; data_index < length; data_index++)
    {
        uint8_t byte = data[data_index];

        switch (g_gps_parser.state)
        {
            case GPS_PARSE_SYNC_1:
                if (byte == GPS_UBX_SYNC_1)
                {
                    g_gps_parser.state = GPS_PARSE_SYNC_2;
                }
                break;

            case GPS_PARSE_SYNC_2:
                if (byte == GPS_UBX_SYNC_2)
                {
                    g_gps_parser.state = GPS_PARSE_CLASS;
                }
                else if (byte != GPS_UBX_SYNC_1)
                {
                    g_gps_parser.state = GPS_PARSE_SYNC_1;
                }
                break;

            case GPS_PARSE_CLASS:
                g_gps_parser.message_class = byte;
                GpsParserChecksumByte(byte);
                g_gps_parser.state = GPS_PARSE_ID;
                break;

            case GPS_PARSE_ID:
                g_gps_parser.message_id = byte;
                GpsParserChecksumByte(byte);
                g_gps_parser.state = GPS_PARSE_LENGTH_LOW;
                break;

            case GPS_PARSE_LENGTH_LOW:
                g_gps_parser.payload_length = byte;
                GpsParserChecksumByte(byte);
                g_gps_parser.state = GPS_PARSE_LENGTH_HIGH;
                break;

            case GPS_PARSE_LENGTH_HIGH:
                g_gps_parser.payload_length |= (uint16_t)byte << 8U;
                GpsParserChecksumByte(byte);
                if (g_gps_parser.payload_length > GPS_UBX_MAX_PAYLOAD_LENGTH)
                {
                    g_gps_parser_debug.oversize_frame_count++;
                    GpsParserResetFrame();
                }
                else if (g_gps_parser.payload_length == 0U)
                {
                    g_gps_parser.state = GPS_PARSE_CHECKSUM_A;
                }
                else
                {
                    g_gps_parser.payload_index = 0U;
                    g_gps_parser.state = GPS_PARSE_PAYLOAD;
                }
                break;

            case GPS_PARSE_PAYLOAD:
                g_gps_parser.payload[g_gps_parser.payload_index++] = byte;
                GpsParserChecksumByte(byte);
                if (g_gps_parser.payload_index == g_gps_parser.payload_length)
                {
                    g_gps_parser.state = GPS_PARSE_CHECKSUM_A;
                }
                break;

            case GPS_PARSE_CHECKSUM_A:
                g_gps_parser.received_checksum_a = byte;
                g_gps_parser.state = GPS_PARSE_CHECKSUM_B;
                break;

            case GPS_PARSE_CHECKSUM_B:
                if ((g_gps_parser.received_checksum_a == g_gps_parser.checksum_a) &&
                    (byte == g_gps_parser.checksum_b))
                {
                    GpsParserHandleFrame();
                }
                else
                {
                    g_gps_parser_debug.checksum_error_count++;
                }
                GpsParserResetFrame();
                break;

            default:
                GpsParserResetFrame();
                break;
        }
    }
}
