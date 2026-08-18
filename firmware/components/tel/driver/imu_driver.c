/******************************************************************************
* @file    imu_driver.c
* @brief   reset/boot handling, SH2 feature-enable commands, and sensor report parsing.
******************************************************************************/

#include "imu_driver.h"
#include "bitops.h"
#include "main.h"
#include "i2c.h"
#include "imu_app.h"
#include "CAN_comms.h"
#include "telemetry_app.h"
#include "can_driver.h"

#define IMU_RESET_PULSE_MS      10
#define IMU_BOOT_WAIT_MS        200
#define IMU_SHTP_HEADER_LEN     4
#define IMU_SHTP_LENGTH_MASK    0x7FFF
#define IMU_RX_BUFFER_LEN       128
#define IMU_ACCEL_PAYLOAD_LEN   10    
#define IMU_GYRO_PAYLOAD_LEN   10
#define IMU_MAG_PAYLOAD_LEN   10

static uint8_t g_IMU_DRIVER_control_seq = 0;   // per-channel sequence counter SHTP requires on writes
static uint8_t g_IMU_DRIVER_rx_buffer[IMU_RX_BUFFER_LEN];
static uint16_t g_IMU_DRIVER_rx_len;

/**
 * @brief Read the I_INTN pin; the BNO086 drives it low when an SHTP packet is ready.
 *
 * @return true if a packet is waiting to be read, false otherwise
 */
bool ImuDriverDataReady(void)
{
    return HAL_GPIO_ReadPin(I_INTN_GPIO_Port, I_INTN_Pin) == GPIO_PIN_RESET;
}

/**
 * @brief Reset the BNO086 and read its power-on SHTP advertisement packet.
 *
 * @return None
 */
void ImuDriverInit(void)
{
    uint32_t start;
    uint8_t header[IMU_SHTP_HEADER_LEN];
    uint16_t packet_len;

    HAL_GPIO_WritePin(I_BOOTN_GPIO_Port, I_BOOTN_Pin, GPIO_PIN_SET);

    HAL_GPIO_WritePin(I_NRST_GPIO_Port, I_NRST_Pin, GPIO_PIN_RESET);
    HAL_Delay(IMU_RESET_PULSE_MS);
    HAL_GPIO_WritePin(I_NRST_GPIO_Port, I_NRST_Pin, GPIO_PIN_SET);

    start = HAL_GetTick();
    while (!ImuDriverDataReady()) {
        if ((HAL_GetTick() - start) > IMU_BOOT_WAIT_MS) {
            return;
        }
    }

    if (HAL_I2C_Master_Receive(&hi2c1, IMU_ADDRESS, header, IMU_SHTP_HEADER_LEN, HAL_MAX_DELAY) != HAL_OK) {
        return;
    }

    packet_len = ((header[1] << 8) | header[0]) & IMU_SHTP_LENGTH_MASK;
    if (packet_len == 0) {
        return;
    }

    g_IMU_DRIVER_rx_len = (packet_len > IMU_RX_BUFFER_LEN) ? IMU_RX_BUFFER_LEN : packet_len;
    HAL_I2C_Master_Receive(&hi2c1, IMU_ADDRESS, g_IMU_DRIVER_rx_buffer, g_IMU_DRIVER_rx_len, HAL_MAX_DELAY);
}

/**
 * @brief Send the SH2 Set Feature command to enable accelerometer reports.
 *
 * @return None
 */
void ImuDriverEnableAccel(void)
{
    uint8_t packet[4 + 17];
    uint16_t total_len = sizeof(packet);

    // --- SHTP header (channel 2 = control) ---
    packet[0] = total_len & 0xFF;
    packet[1] = (total_len >> 8) & 0xFF;
    packet[2] = SHTP_CHANNEL_CONTROL;
    packet[3] = g_IMU_DRIVER_control_seq++;

    // --- SH2 Set Feature Command payload ---
    packet[4]  = SH2_REPORT_SET_FEAATURE_CMD;
    packet[5]  = SH2_REPORT_ACCEL;
    packet[6]  = 0x00;                                   // feature flags
    packet[7]  = 0x00; packet[8]  = 0x00;                // change sensitivity
    packet[9]  = (IMU_ACCEL_REPORT_INTERVAL_US)       & 0xFF;
    packet[10] = (IMU_ACCEL_REPORT_INTERVAL_US >> 8)  & 0xFF;
    packet[11] = (IMU_ACCEL_REPORT_INTERVAL_US >> 16) & 0xFF;
    packet[12] = (IMU_ACCEL_REPORT_INTERVAL_US >> 24) & 0xFF;
    packet[13] = 0x00; packet[14] = 0x00; packet[15] = 0x00; packet[16] = 0x00; // batch interval
    packet[17] = 0x00; packet[18] = 0x00; packet[19] = 0x00; packet[20] = 0x00; // sensor-specific config

    HAL_I2C_Master_Transmit(&hi2c1, IMU_ADDRESS, packet, sizeof(packet), HAL_MAX_DELAY);
}

/**
 * @brief Send the SH2 Set Feature command to enable gyroscope reports.
 *
 * @return None
 */
void ImuDriverEnableGyro(void)
{
    uint8_t packet[4 + 17];
    uint16_t total_len = sizeof(packet);

    // --- SHTP header (channel 2 = control) ---
    packet[0] = total_len & 0xFF;
    packet[1] = (total_len >> 8) & 0xFF;
    packet[2] = SHTP_CHANNEL_CONTROL;
    packet[3] = g_IMU_DRIVER_control_seq++;

    // --- SH2 Set Feature Command payload ---
    packet[4]  = SH2_REPORT_SET_FEAATURE_CMD;
    packet[5]  = SH2_REPORT_GYRO;
    packet[6]  = 0x00;                                   // feature flags
    packet[7]  = 0x00; packet[8]  = 0x00;                // change sensitivity
    packet[9]  = (IMU_GYRO_REPORT_INTERVAL_US)       & 0xFF;
    packet[10] = (IMU_GYRO_REPORT_INTERVAL_US >> 8)  & 0xFF;
    packet[11] = (IMU_GYRO_REPORT_INTERVAL_US >> 16) & 0xFF;
    packet[12] = (IMU_GYRO_REPORT_INTERVAL_US >> 24) & 0xFF;
    packet[13] = 0x00; packet[14] = 0x00; packet[15] = 0x00; packet[16] = 0x00; // batch interval
    packet[17] = 0x00; packet[18] = 0x00; packet[19] = 0x00; packet[20] = 0x00; // sensor-specific config

    HAL_I2C_Master_Transmit(&hi2c1, IMU_ADDRESS, packet, sizeof(packet), HAL_MAX_DELAY);
}

/**
 * @brief Send the SH2 Set Feature command to enable magnetometer reports.
 *
 * @return None
 */
void ImuDriverEnableMag(void)
{
    uint8_t packet[4 + 17];
    uint16_t total_len = sizeof(packet);

    // --- SHTP header (channel 2 = control) ---
    packet[0] = total_len & 0xFF;
    packet[1] = (total_len >> 8) & 0xFF;
    packet[2] = SHTP_CHANNEL_CONTROL;
    packet[3] = g_IMU_DRIVER_control_seq++;

    // --- SH2 Set Feature Command payload ---
    packet[4]  = SH2_REPORT_SET_FEAATURE_CMD;
    packet[5]  = SH2_REPORT_MAG;
    packet[6]  = 0x00;                                   // feature flags
    packet[7]  = 0x00; packet[8]  = 0x00;                // change sensitivity
    packet[9]  = (IMU_MAG_REPORT_INTERVAL_US)       & 0xFF;
    packet[10] = (IMU_MAG_REPORT_INTERVAL_US >> 8)  & 0xFF;
    packet[11] = (IMU_MAG_REPORT_INTERVAL_US >> 16) & 0xFF;
    packet[12] = (IMU_MAG_REPORT_INTERVAL_US >> 24) & 0xFF;
    packet[13] = 0x00; packet[14] = 0x00; packet[15] = 0x00; packet[16] = 0x00; // batch interval
    packet[17] = 0x00; packet[18] = 0x00; packet[19] = 0x00; packet[20] = 0x00; // sensor-specific config

    HAL_I2C_Master_Transmit(&hi2c1, IMU_ADDRESS, packet, sizeof(packet), HAL_MAX_DELAY);
}

/**
 * @brief Read one pending SHTP packet off the I2C bus into the driver's RX buffer.
 *
 * Peeks the 4-byte SHTP header to determine the packet length, then re-reads
 * the full packet.
 *
 * @return true if a full packet was read successfully, false otherwise
 */
static bool ImuDriverReadPacket(void)
{
    uint8_t header[IMU_SHTP_HEADER_LEN];
    uint16_t packet_len;

    if (HAL_I2C_Master_Receive(&hi2c1, IMU_ADDRESS, header, IMU_SHTP_HEADER_LEN, HAL_MAX_DELAY) != HAL_OK) {
        return false;
    }

    packet_len = ((header[1] << 8) | header[0]) & IMU_SHTP_LENGTH_MASK;
    if (packet_len == 0) {
        return false;
    }

    g_IMU_DRIVER_rx_len = (packet_len > IMU_RX_BUFFER_LEN) ? IMU_RX_BUFFER_LEN : packet_len;
    return HAL_I2C_Master_Receive(&hi2c1, IMU_ADDRESS, g_IMU_DRIVER_rx_buffer, g_IMU_DRIVER_rx_len, HAL_MAX_DELAY) == HAL_OK;
}

/**
 * @brief Read one pending SHTP packet and parse every sensor report inside it.
 *
 * Reads the packet report-by-report, updating whichever fields in data
 * match a recognized report ID (accel, gyro, mag). Stops at the first
 * unrecognized report ID, since its length cannot be determined.
 *
 * @param data ImuAppData struct to update with any parsed sensor readings
 * @return true if at least one field in data was updated, false otherwise
 */
bool ImuDriverReadReport(ImuAppData *data)
{
    uint16_t offset;
    uint8_t report_id;
    bool updated;
    int16_t x, y, z;

    if (!ImuDriverDataReady() || !ImuDriverReadPacket()) {
        return false;
    }

    if (g_IMU_DRIVER_rx_buffer[2] != SHTP_CHANNEL_REPORTS) {
        return false;
    }

    offset = IMU_SHTP_HEADER_LEN;
    updated = false;

    while (offset < g_IMU_DRIVER_rx_len) {
        report_id = g_IMU_DRIVER_rx_buffer[offset];

        switch (report_id) {
            case 0xFB:  // Base Timestamp Reference — no sensor data, just skip it
            {
                offset += 5;
                break;
            }
            case SH2_REPORT_ACCEL:
            {
                if (offset + IMU_ACCEL_PAYLOAD_LEN > g_IMU_DRIVER_rx_len)
                    return updated;
                x = (int16_t)(g_IMU_DRIVER_rx_buffer[offset + 5] << 8 | g_IMU_DRIVER_rx_buffer[offset + 4]);
                y = (int16_t)(g_IMU_DRIVER_rx_buffer[offset + 7] << 8 | g_IMU_DRIVER_rx_buffer[offset + 6]);
                z = (int16_t)(g_IMU_DRIVER_rx_buffer[offset + 9] << 8 | g_IMU_DRIVER_rx_buffer[offset + 8]);

                data->accel_x = (float)x / 256.0f;   // Q8 fixed-point -> m/s^2 (BNO08x accel Q-point = 8)
                data->accel_y = (float)y / 256.0f;
                data->accel_z = (float)z / 256.0f;
                offset += IMU_ACCEL_PAYLOAD_LEN;
                updated = true;
                break;
            }
            case SH2_REPORT_GYRO:
            {
                if (offset + IMU_GYRO_PAYLOAD_LEN > g_IMU_DRIVER_rx_len)
                    return updated;
                x = (int16_t)(g_IMU_DRIVER_rx_buffer[offset +  5] << 8 | g_IMU_DRIVER_rx_buffer[offset +  4]);
                y = (int16_t)(g_IMU_DRIVER_rx_buffer[offset +  7] << 8 | g_IMU_DRIVER_rx_buffer[offset +  6]);
                z = (int16_t)(g_IMU_DRIVER_rx_buffer[offset +  9] << 8 | g_IMU_DRIVER_rx_buffer[offset +  8]);

                data->gyro_x = (float)x / 512.0f;   // Q9 fixed-point -> rad/s (BNO08x gyro Q-point = 9)
                data->gyro_y = (float)y / 512.0f;
                data->gyro_z = (float)z / 512.0f;
                offset += IMU_GYRO_PAYLOAD_LEN;
                updated = true;
                break;
            }

            case SH2_REPORT_MAG:
            {
                if (offset + IMU_MAG_PAYLOAD_LEN > g_IMU_DRIVER_rx_len)
                    return updated;
                x = (int16_t)(g_IMU_DRIVER_rx_buffer[offset +  5] << 8 | g_IMU_DRIVER_rx_buffer[offset +  4]);
                y = (int16_t)(g_IMU_DRIVER_rx_buffer[offset +  7] << 8 | g_IMU_DRIVER_rx_buffer[offset +  6]);
                z = (int16_t)(g_IMU_DRIVER_rx_buffer[offset +  9] << 8 | g_IMU_DRIVER_rx_buffer[offset +  8]);

                data->mag_x = (float)x / 16.0f;   // Q4 fixed-point -> MicroTesla (BNO08x gyro Q-point = 4)
                data->mag_y = (float)y / 16.0f;
                data->mag_z = (float)z / 16.0f;
                offset += IMU_MAG_PAYLOAD_LEN;
                updated = true;
                break;
            }

            default:
                return updated;
        }
    }

    return updated;
}

void CAN_tx_ag_msg(const CAN_TxHeaderTypeDef *header, float accel, float gyro)
{
	FloatToBytes float_bytes;
    CAN_comms_Tx_msg_t msg = { .header = *header  };

    float_bytes.f = accel;
    for (int i = 0; i < 4; i++)
    {
        msg.data[i] = float_bytes.bytes[i];
    }

    float_bytes.f = gyro;
    for (int i = 0; i < 4; i++)
    {
        msg.data[i + 4] = float_bytes.bytes[i];
    }

    CAN_comms_Add_Tx_message(&msg);
    osDelay(2);
    TelAppTransmitInternalMsg(&msg);
    osDelay(2);
}

void CAN_tx_m_msg(const CAN_TxHeaderTypeDef *header, float mag)
{
    FloatToBytes float_bytes;
    CAN_comms_Tx_msg_t msg = { .header = *header };

    float_bytes.f = mag;
    for (int i = 0; i < 4; i++)
    {
        msg.data[i] = float_bytes.bytes[i];
    }
    CAN_comms_Add_Tx_message(&msg);
    osDelay(2);
    TelAppTransmitInternalMsg(&msg);
    osDelay(2);
}

