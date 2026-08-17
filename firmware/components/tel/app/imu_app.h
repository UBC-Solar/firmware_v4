/******************************************************************************
* @file    imu_app.h
* @brief   Shared types, macros, and public interface for the IMU application layer.
******************************************************************************/

#ifndef __IMU_H__
#define __IMU_H__

/* IMU I2C Address */
#define IMU_ADDRESS (0x4B << 1)  // IMU I2C device address (shifted for STM32 HAL) this is or the sparkplug board, 0x4A could also work for the custom one

//SHTP Channels
#define SHTP_CHANNEL_COMMAND   0
#define SHTP_CHANNEL_EXE       1
#define SHTP_CHANNEL_CONTROL   2
#define SHTP_CHANNEL_REPORTS   3

//SH2 report IDs
#define SH2_REPORT_SET_FEAATURE_CMD    0xFD
#define SH2_REPORT_ACCEL               0x04
#define SH2_REPORT_GYRO                0x02
#define SH2_REPORT_MAG                 0x03

//TODO: Intervals for each sensor report, figure out the intervals
#define IMU_ACCEL_REPORT_INTERVAL_US   10000
#define IMU_GYRO_REPORT_INTERVAL_US    10000
#define IMU_MAG_REPORT_INTERVAL_US     10000

//FreeeRTOS
#define IMU_TASK_DELAY      100
#define IMU_TASK_DELAY_SINGLE      25
#define IMU_TASK_OFFSET_DELAY       33


/* Define the CAN message IDs for each IMU axes */
#define IMU_AG_X_CAN_MESSAGE_ID 0x752
#define IMU_AG_Y_CAN_MESSAGE_ID 0x753
#define IMU_AG_Z_CAN_MESSAGE_ID 0x754
#define IMU_M_X_CAN_MESSAGE_ID 0x755
#define IMU_M_Y_CAN_MESSAGE_ID 0x756
#define IMU_M_Z_CAN_MESSAGE_ID 0x757

//Bucket to store IMU data before being transmitted over CAN
typedef struct {
   float accel_x, accel_y, accel_z;
   float gyro_x, gyro_y, gyro_z;
   float mag_x, mag_y, mag_z;
} ImuAppData;

/**
* @brief Initialize the IMU driver and enable accelerometer, gyroscope, and magnetometer.
*
* @return None
*/
void ImuAppInit(void);

/**
* @brief Poll for and process one pending IMU report, updating the shared IMU data.
*
* @return None
*/
void ImuAppTask(void);

#endif /* __IMU__APP__H__ */
