/******************************************************************************
* @file    imu_app.c
* @brief   Application-layer orchestration for reading IMU data.
******************************************************************************/

#include "imu_app.h"
#include "imu_driver.h"
#include "main.h"

static ImuAppData g_IMU_APP_data;

/**
 * @brief Initialize the IMU driver and enable accelerometer, gyroscope, and magnetometer.
 *
 * @return None
 */
void ImuAppInit(void)
{
    ImuDriverInit();
    ImuDriverEnableAccel();
    ImuDriverEnableGyro();
    ImuDriverEnableMag();
}

/**
 * @brief Poll for and process one pending IMU report, updating the shared IMU data.
 *
 * @return None
 */
void ImuAppTask(void)
{
    ImuDriverReadReport(&g_IMU_APP_data);
}
