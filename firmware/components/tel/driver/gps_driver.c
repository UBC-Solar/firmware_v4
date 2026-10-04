/**
 * @file    gps_driver.c
 * @brief   GPS driver implementation for UBC Solar TEL board
 *
 * This file contains the implementation of the u-blox GPS driver for the TEL board.
 * I2C registers: 0xFD/0xFE = bytes waiting, 0xFF = data stream (Integration Manual 3.6.2.1).
 *
 * @author  Raymond Shen
 * @date    Oct 3 2026
 */
#include "gps_driver.h"
#include "i2c.h"
#include <string.h>

#define GPS_I2C_ADDR                (0x42U << 1)    // 7-bit address 0x42, shifted for the HAL
#define GPS_I2C_TIMEOUT             100U
#define GPS_I2C_READY_TRIALS        5U
#define GPS_REG_BYTES_AVAIL         0xFDU
#define GPS_REG_DATA_STREAM         0xFFU
#define GPS_READ_CHUNK              128U            // bytes per stream read
#define GPS_READ_MAX                1024U           // max bytes read per call

#define UBX_SYNC_1                  0xB5U
#define UBX_SYNC_2                  0x62U
#define UBX_FRAME_OVERHEAD          8U              // sync(2) + class + id + length(2) + checksum(2)
#define UBX_MAX_TX_PAYLOAD          16U
#define UBX_MAX_RX_PAYLOAD          100U            // largest message parsed is NAV-PVT (92 bytes)

#define UBX_CLASS_NAV               0x01U
#define UBX_CLASS_ACK               0x05U
#define UBX_CLASS_CFG               0x06U
#define UBX_ID_NAV_PVT              0x07U
#define UBX_ID_CFG_VALSET           0x8AU
#define UBX_NAV_PVT_LEN             92U

typedef enum {
    UBX_STATE_SYNC_1 = 0,
    UBX_STATE_SYNC_2,
    UBX_STATE_CLASS,
    UBX_STATE_ID,
    UBX_STATE_LEN_LOW,
    UBX_STATE_LEN_HIGH,
    UBX_STATE_PAYLOAD,
    UBX_STATE_CK_A,
    UBX_STATE_CK_B,
} UbxParseState;

typedef struct {
    UbxParseState state;
    uint8_t  cls;
    uint8_t  id;
    uint16_t len;
    uint16_t idx;
    uint8_t  ck_a;
    uint8_t  ck_b;
    uint8_t  payload[UBX_MAX_RX_PAYLOAD];
} UbxParser;

/* Static so they stay off the GPS task stack */
static UbxParser ubx_parser;
static uint8_t   rx_chunk[GPS_READ_CHUNK];
static uint8_t   tx_frame[UBX_MAX_TX_PAYLOAD + UBX_FRAME_OVERHEAD];

/**
 * @brief Frames and sends a UBX message
 * @param cls UBX message class
 * @param id UBX message ID
 * @param payload Message payload, may be NULL if len is 0
 * @param len Payload length, at most UBX_MAX_TX_PAYLOAD
 * @return HAL status of the I2C write
 */
static HAL_StatusTypeDef UbxSend(uint8_t cls, uint8_t id, const uint8_t *payload, uint16_t len);
/**
 * @brief Feeds one received byte to the UBX parser. NMEA text and partial frames are skipped.
 * @param c Received byte
 * @param pvt Filled when a complete NAV-PVT frame is parsed
 * @return true if pvt was updated
 */
static bool UbxParseByte(uint8_t c, GpsDriverPvtData *pvt);
/**
 * @brief Decodes a UBX-NAV-PVT payload
 * @param p Payload of at least UBX_NAV_PVT_LEN bytes
 * @param pvt Decoded solution
 */
static void UbxDecodePvt(const uint8_t *p, GpsDriverPvtData *pvt);

static uint16_t UbxU2(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static uint32_t UbxU4(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

GpsDriverStatus GpsDriverInit(void)
{
    /* CFG-VALSET: CFG-I2COUTPROT-NMEA (0x10720002) = false in the RAM layer */
    static const uint8_t nmea_off[] = {
        0x00, 0x01, 0x00, 0x00,     // version 0, layer RAM, reserved
        0x02, 0x00, 0x72, 0x10,     // key, little-endian
        0x00 };                     // value: false

    memset(&ubx_parser, 0, sizeof(ubx_parser));

    if (HAL_I2C_IsDeviceReady(&hi2c2, GPS_I2C_ADDR, GPS_I2C_READY_TRIALS, GPS_I2C_TIMEOUT) != HAL_OK)
    {
        return GPS_DRIVER_READ_FAIL;
    }

    if (UbxSend(UBX_CLASS_CFG, UBX_ID_CFG_VALSET, nmea_off, sizeof(nmea_off)) != HAL_OK)
    {
        return GPS_DRIVER_WRITE_FAIL;
    }

    return GPS_DRIVER_OK;
}

GpsDriverStatus GpsDriverRequestPvt(void)
{
    if (UbxSend(UBX_CLASS_NAV, UBX_ID_NAV_PVT, NULL, 0U) != HAL_OK)
    {
        return GPS_DRIVER_WRITE_FAIL;
    }
    return GPS_DRIVER_OK;
}

GpsDriverStatus GpsDriverRead(GpsDriverPvtData *pvt, bool *new_pvt)
{
    uint8_t  avail_bytes[2];
    uint16_t budget = GPS_READ_MAX;

    *new_pvt = false;

    while (budget > 0U)
    {
        if (HAL_I2C_Mem_Read(&hi2c2, GPS_I2C_ADDR, GPS_REG_BYTES_AVAIL, I2C_MEMADD_SIZE_8BIT,
                             avail_bytes, sizeof(avail_bytes), GPS_I2C_TIMEOUT) != HAL_OK)
        {
            return GPS_DRIVER_READ_FAIL;
        }

        uint16_t avail = ((uint16_t)avail_bytes[0] << 8) | avail_bytes[1];
        if (avail == 0U)
        {
            break;
        }

        uint16_t n = (avail > GPS_READ_CHUNK) ? GPS_READ_CHUNK : avail;
        if (n > budget)
        {
            n = budget;
        }

        if (HAL_I2C_Mem_Read(&hi2c2, GPS_I2C_ADDR, GPS_REG_DATA_STREAM, I2C_MEMADD_SIZE_8BIT,
                             rx_chunk, n, GPS_I2C_TIMEOUT) != HAL_OK)
        {
            return GPS_DRIVER_READ_FAIL;
        }

        for (uint16_t i = 0U; i < n; i++)
        {
            if (UbxParseByte(rx_chunk[i], pvt))
            {
                *new_pvt = true;
            }
        }
        budget -= n;
    }

    return GPS_DRIVER_OK;
}

static HAL_StatusTypeDef UbxSend(uint8_t cls, uint8_t id, const uint8_t *payload, uint16_t len)
{
    uint8_t ck_a = 0U;
    uint8_t ck_b = 0U;

    if (len > UBX_MAX_TX_PAYLOAD)
    {
        return HAL_ERROR;
    }

    tx_frame[0] = UBX_SYNC_1;
    tx_frame[1] = UBX_SYNC_2;
    tx_frame[2] = cls;
    tx_frame[3] = id;
    tx_frame[4] = (uint8_t)(len & 0xFFU);
    tx_frame[5] = (uint8_t)(len >> 8);
    if (len > 0U)
    {
        memcpy(&tx_frame[6], payload, len);
    }

    /* 8-bit Fletcher checksum over class, id, length and payload */
    for (uint16_t i = 2U; i < (6U + len); i++)
    {
        ck_a += tx_frame[i];
        ck_b += ck_a;
    }
    tx_frame[6U + len] = ck_a;
    tx_frame[7U + len] = ck_b;

    return HAL_I2C_Master_Transmit(&hi2c2, GPS_I2C_ADDR, tx_frame, (uint16_t)(len + UBX_FRAME_OVERHEAD), GPS_I2C_TIMEOUT);
}

static bool UbxParseByte(uint8_t c, GpsDriverPvtData *pvt)
{
    UbxParser *ps = &ubx_parser;
    bool new_pvt = false;

    /* Checksum covers everything from the class byte up to the end of the payload */
    if ((ps->state >= UBX_STATE_CLASS) && (ps->state <= UBX_STATE_PAYLOAD))
    {
        ps->ck_a += c;
        ps->ck_b += ps->ck_a;
    }

    switch (ps->state)
    {
    case UBX_STATE_SYNC_1:
        if (c == UBX_SYNC_1)
        {
            ps->state = UBX_STATE_SYNC_2;
        }
        break;
    case UBX_STATE_SYNC_2:
        if (c == UBX_SYNC_2)
        {
            ps->ck_a = 0U;
            ps->ck_b = 0U;
            ps->state = UBX_STATE_CLASS;
        }
        else if (c != UBX_SYNC_1)
        {
            ps->state = UBX_STATE_SYNC_1;
        }
        break;
    case UBX_STATE_CLASS:
        ps->cls = c;
        ps->state = UBX_STATE_ID;
        break;
    case UBX_STATE_ID:
        ps->id = c;
        ps->state = UBX_STATE_LEN_LOW;
        break;
    case UBX_STATE_LEN_LOW:
        ps->len = c;
        ps->state = UBX_STATE_LEN_HIGH;
        break;
    case UBX_STATE_LEN_HIGH:
        ps->len |= (uint16_t)c << 8;
        ps->idx = 0U;
        if (ps->len > UBX_MAX_RX_PAYLOAD)
        {
            ps->state = UBX_STATE_SYNC_1;   // Not a message we parse, resync on the next frame
        }
        else
        {
            ps->state = (ps->len == 0U) ? UBX_STATE_CK_A : UBX_STATE_PAYLOAD;
        }
        break;
    case UBX_STATE_PAYLOAD:
        ps->payload[ps->idx++] = c;
        if (ps->idx >= ps->len)
        {
            ps->state = UBX_STATE_CK_A;
        }
        break;
    case UBX_STATE_CK_A:
        ps->state = (c == ps->ck_a) ? UBX_STATE_CK_B : UBX_STATE_SYNC_1;
        break;
    case UBX_STATE_CK_B:
        if ((c == ps->ck_b) && (ps->cls == UBX_CLASS_NAV) && (ps->id == UBX_ID_NAV_PVT) &&
            (ps->len >= UBX_NAV_PVT_LEN))
        {
            UbxDecodePvt(ps->payload, pvt);
            new_pvt = true;
        }
        ps->state = UBX_STATE_SYNC_1;
        break;
    default:
        ps->state = UBX_STATE_SYNC_1;
        break;
    }

    return new_pvt;
}

static void UbxDecodePvt(const uint8_t *p, GpsDriverPvtData *pvt)
{
    /* Offsets from the UBX-NAV-PVT payload description in the interface description */
    pvt->year              = UbxU2(&p[4]);
    pvt->month             = p[6];
    pvt->day               = p[7];
    pvt->hour              = p[8];
    pvt->minute            = p[9];
    pvt->second            = p[10];
    pvt->valid             = p[11];
    pvt->fix_type          = p[20];
    pvt->flags             = p[21];
    pvt->num_sv            = p[23];
    pvt->lon_e7            = (int32_t)UbxU4(&p[24]);
    pvt->lat_e7            = (int32_t)UbxU4(&p[28]);
    pvt->height_msl_mm     = (int32_t)UbxU4(&p[36]);
    pvt->h_acc_mm          = UbxU4(&p[40]);
    pvt->ground_speed_mm_s = (int32_t)UbxU4(&p[60]);
    pvt->heading_e5        = (int32_t)UbxU4(&p[64]);
    pvt->pdop_e2           = UbxU2(&p[76]);
}
