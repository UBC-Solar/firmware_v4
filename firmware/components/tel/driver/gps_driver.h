/**
 * @file    gps_driver.h
 * @brief   GPS driver header file for UBC Solar TEL board
 *
 * This file contains the prototypes and variables for the u-blox GPS driver for the TEL board.
 * The receiver is read over I2C (u-blox DDC) using the UBX binary protocol only.
 *
 * @author  Raymond Shen
 * @date    Oct 3 2026
 */
#ifndef __GPS_DRIVER__H__
#define __GPS_DRIVER__H__

#include <stdint.h>
#include <stdbool.h>

/* UBX-NAV-PVT valid field bits */
#define GPS_DRIVER_VALID_DATE           0x01U
#define GPS_DRIVER_VALID_TIME           0x02U
#define GPS_DRIVER_FULLY_RESOLVED       0x04U

/* UBX-NAV-PVT flags field bits */
#define GPS_DRIVER_FLAG_GNSS_FIX_OK     0x01U
#define GPS_DRIVER_FLAG_DIFF_SOLN       0x02U

typedef enum {
    GPS_DRIVER_OK = 0,
    GPS_DRIVER_WRITE_FAIL,
    GPS_DRIVER_READ_FAIL,
} GpsDriverStatus;

/* Position, velocity and time solution, units as in UBX-NAV-PVT */
typedef struct {
    uint16_t year;
    uint8_t  month;
    uint8_t  day;
    uint8_t  hour;
    uint8_t  minute;
    uint8_t  second;
    uint8_t  valid;             // GPS_DRIVER_VALID_* bits
    uint8_t  fix_type;          // 0 none, 1 DR, 2 2D, 3 3D, 4 GNSS+DR, 5 time only
    uint8_t  flags;             // GPS_DRIVER_FLAG_* bits
    uint8_t  num_sv;            // satellites used in the solution
    int32_t  lon_e7;            // degrees * 1e7
    int32_t  lat_e7;            // degrees * 1e7
    int32_t  height_msl_mm;     // height above mean sea level, mm
    uint32_t h_acc_mm;          // horizontal accuracy estimate, mm
    int32_t  ground_speed_mm_s; // 2D ground speed, mm/s
    int32_t  heading_e5;        // heading of motion, degrees * 1e5
    uint16_t pdop_e2;           // position DOP * 100
} GpsDriverPvtData;

/**
 * @brief Checks the receiver is on the bus and switches its I2C output to UBX only.
 *        The setting is written to RAM, so it reverts on a power cycle.
 * @return GPS_DRIVER_OK on success, otherwise the failed step
 */
GpsDriverStatus GpsDriverInit(void);

/**
 * @brief Requests a UBX-NAV-PVT solution from the receiver.
 * @return GPS_DRIVER_OK on success, GPS_DRIVER_WRITE_FAIL if the I2C write failed
 */
GpsDriverStatus GpsDriverRequestPvt(void);

/**
 * @brief Reads everything the receiver has queued and parses the UBX frames in it.
 * @param pvt Filled with the newest NAV-PVT solution if one was received
 * @param new_pvt Set to true if pvt was updated
 * @return GPS_DRIVER_OK on success, GPS_DRIVER_READ_FAIL if an I2C read failed
 */
GpsDriverStatus GpsDriverRead(GpsDriverPvtData *pvt, bool *new_pvt);

#endif /* __GPS_DRIVER__H__ */
