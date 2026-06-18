#include "gps_driver.h"
#include "i2c.h"

HAL_StatusTypeDef gps_check_ready(void)
{
  return HAL_I2C_IsDeviceReady(&hi2c2, GPS_ADDR, 1, GPS_I2C_TIMEOUT);
}

HAL_StatusTypeDef read_i2c_gps_module(uint8_t *receive_buffer)
{
  if (!receive_buffer) return HAL_ERROR;

  return HAL_I2C_Master_Receive(&hi2c2, GPS_ADDR, receive_buffer, GPS_MESSAGE_LEN, GPS_I2C_TIMEOUT);
}
