/******************************************************************************
* @file    imu_driver.h
* @brief   Public interface for the BNO086 IMU SHTP/I2C driver.
******************************************************************************/

#ifndef __IMU__DRIVER__H__
#define __IMU__DRIVER__H__

#include <stdbool.h>
#include "imu_app.h"

/**
 * @brief Reset the BNO086 and read its power-on SHTP advertisement packet.
 *
 * @return None
 */
void ImuDriverInit(void);

/**
 * @brief Read the I_INTN pin; the BNO086 drives it low when an SHTP packet is ready.
 *
 * @return true if a packet is waiting to be read, false otherwise
 */
bool ImuDriverDataReady(void);

/**
 * @brief Send the SH2 Set Feature command to enable accelerometer reports.
 *
 * @return None
 */
void ImuDriverEnableAccel(void);

/**
 * @brief Send the SH2 Set Feature command to enable gyroscope reports.
 *
 * @return None
 */
void ImuDriverEnableGyro(void);

/**
 * @brief Send the SH2 Set Feature command to enable magnetometer reports.
 *
 * @return None
 */
void ImuDriverEnableMag(void);

/**
 * @brief Read one pending SHTP packet and parse every sensor report inside it.
 *
 * Read the packet report-by-report, updating whichever fields in data
 * match a recognized report ID (accel, gyro, mag). Stops at the first
 * unrecognized report ID, since its length cannot be safely determined.
 *
 * @param data ImuAppData struct to update with any parsed sensor readings
 * @return true if at least one field in data was updated, false otherwise
 */
bool ImuDriverReadReport(ImuAppData *data);

#endif /* __IMU__DRIVER__H__ */
