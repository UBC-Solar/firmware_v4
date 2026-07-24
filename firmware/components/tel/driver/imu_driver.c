#include "imu_driver.h"

#include "main.h"
#include "i2c.h"
#include "imu_app.h"

#define IMU_RESET_PULSE_MS      10
#define IMU_BOOT_WAIT_MS        200
#define IMU_SHTP_HEADER_LEN     4
#define IMU_SHTP_LENGTH_MASK    0x7FFF
#define IMU_RX_BUFFER_LEN       128

static uint8_t imu_rx_buffer[IMU_RX_BUFFER_LEN];
static uint16_t imu_rx_len;

bool imu_data_ready(void)
{
    return HAL_GPIO_ReadPin(I_INTN_GPIO_Port, I_INTN_Pin) == GPIO_PIN_RESET;
}

void imu_init(void)
{
    HAL_GPIO_WritePin(I_BOOTN_GPIO_Port, I_BOOTN_Pin, GPIO_PIN_SET);

    HAL_GPIO_WritePin(I_NRST_GPIO_Port, I_NRST_Pin, GPIO_PIN_RESET);
    HAL_Delay(IMU_RESET_PULSE_MS);
    HAL_GPIO_WritePin(I_NRST_GPIO_Port, I_NRST_Pin, GPIO_PIN_SET);

    uint32_t start = HAL_GetTick();
    while (!imu_data_ready()) {
        if ((HAL_GetTick() - start) > IMU_BOOT_WAIT_MS) {
            return;
        }
    }

    uint8_t header[IMU_SHTP_HEADER_LEN];
    if (HAL_I2C_Master_Receive(&hi2c1, IMU_ADDRESS, header, IMU_SHTP_HEADER_LEN, HAL_MAX_DELAY) != HAL_OK) {
        return;
    }

    uint16_t packet_len = ((header[1] << 8) | header[0]) & IMU_SHTP_LENGTH_MASK;
    if (packet_len == 0) {
        return;
    }

    imu_rx_len = (packet_len > IMU_RX_BUFFER_LEN) ? IMU_RX_BUFFER_LEN : packet_len;
    HAL_I2C_Master_Receive(&hi2c1, IMU_ADDRESS, imu_rx_buffer, imu_rx_len, HAL_MAX_DELAY);
}