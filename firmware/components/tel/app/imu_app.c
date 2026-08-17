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
 * @return N/A
 */
void ImuAppTask(void)
{
    if (ImuDriverReadReport(&g_IMU_APP_data))
    {
        CAN_tx_ag_x_msg(g_IMU_APP_data.accel_x, g_IMU_APP_data.gyro_x);
        CAN_tx_ag_y_msg(g_IMU_APP_data.accel_y, g_IMU_APP_data.gyro_y);
        CAN_tx_ag_z_msg(g_IMU_APP_data.accel_z, g_IMU_APP_data.gyro_z);
        CAN_tx_m_x_msg(g_IMU_APP_data.mag_x);
        CAN_tx_m_y_msg(g_IMU_APP_data.mag_y);
        CAN_tx_m_z_msg(g_IMU_APP_data.mag_z);
    }
}
