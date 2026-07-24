#ifndef __IMU__DRIVER__H__
#define __IMU__DRIVER__H__

#include <stdbool.h>

/**
 * @brief Reset the BNO086 and read its power-on SHTP advertisement packet.
 */
void imu_init(void);

/**
 * @brief Read the I_INTN pin; the BNO086 drives it low when an SHTP packet is ready.
 * @retval true if a packet is waiting to be read.
 */
bool imu_data_ready(void);

#endif /* __IMU__DRIVER__H__ */