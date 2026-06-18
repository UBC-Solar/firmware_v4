#ifndef GPS_DRIVER_H
#define GPS_DRIVER_H

#include "main.h"

#define GPS_ADDR          (0x42 << 1)
#define GPS_MESSAGE_LEN   500
#define GPS_I2C_TIMEOUT   100

HAL_StatusTypeDef gps_check_ready(void);
HAL_StatusTypeDef read_i2c_gps_module(uint8_t *receive_buffer);

#endif // GPS_DRIVER_H
