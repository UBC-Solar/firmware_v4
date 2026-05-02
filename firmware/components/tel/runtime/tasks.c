#include "tasks.h"
#include "CAN_comms.h"

/* IMU TASK */
void TasksIMU(void* argument)
{
    (void)argument; // Unused parameter

    for (;;)
    {
        // CAN TX Testing - replace with actual IMU reading and CAN transmission logic

        // CAN_comms_Tx_msg_t imu_msg;
        // imu_msg.header.StdId = 0x300; // Example CAN ID for IMU
        // imu_msg.header.IDE = CAN_ID_STD;
        // imu_msg.header.DLC = 8; // Example data length
        // // Fill imu_msg.data with IMU sensor readings here
        // CAN_comms_Add_Tx_message(&imu_msg);
    }
}