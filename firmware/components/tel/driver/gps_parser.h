#ifndef GPS_PARSER_H
#define GPS_PARSER_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    volatile bool message_valid;
    volatile bool fix_valid;
    volatile bool date_valid;
    volatile bool time_valid;
    volatile bool attitude_valid;
    volatile uint32_t update_count;
    volatile uint8_t message_id;
    volatile uint32_t i_tow_ms;
    volatile uint16_t year;
    volatile uint8_t month;
    volatile uint8_t day;
    volatile uint8_t hour;
    volatile uint8_t minute;
    volatile uint8_t second;
    volatile uint8_t fix_type;
    volatile uint8_t satellites;
    volatile int32_t longitude_e7;
    volatile int32_t latitude_e7;
    volatile double longitude_deg;
    volatile double latitude_deg;
    volatile int32_t height_mm;
    volatile int32_t height_msl_mm;
    volatile uint32_t horizontal_accuracy_mm;
    volatile uint32_t vertical_accuracy_mm;
    volatile int32_t velocity_north_mm_s;
    volatile int32_t velocity_east_mm_s;
    volatile int32_t velocity_down_mm_s;
    volatile int32_t ground_speed_mm_s;
    volatile uint32_t speed_accuracy_mm_s;
    volatile int32_t vehicle_roll_e5;
    volatile int32_t vehicle_pitch_e5;
    volatile int32_t vehicle_heading_e5;
    volatile int32_t motion_heading_e5;
} GPSNavigationData;

typedef struct
{
    volatile uint32_t valid_frame_count;
    volatile uint32_t nav_frame_count;
    volatile uint32_t checksum_error_count;
    volatile uint32_t oversize_frame_count;
    volatile uint32_t ignored_frame_count;
    volatile uint8_t last_message_class;
    volatile uint8_t last_message_id;
    volatile uint16_t last_payload_length;
} GPSParserDebugInfo;

extern volatile GPSNavigationData g_gps_navigation;
extern volatile GPSParserDebugInfo g_gps_parser_debug;

/** Reset the streaming UBX parser and all decoded/debug state. */
void GpsParserInit(void);

/** Consume an arbitrary fragment of the GPS byte stream. */
void GpsParserConsume(const uint8_t *data, uint16_t length);

#endif /* GPS_PARSER_H */
