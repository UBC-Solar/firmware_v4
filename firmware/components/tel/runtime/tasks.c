#include "tasks.h"
#include "CAN_comms.h"
#include "usart.h"
#include "rtc.h"
#include "telemetry_app.h"

volatile uint32_t g_time_since_bootup;

#define TIME_SINCE_BOOTUP_CAN_DATA_LENGTH          4       
#define TIME_SINCE_BOOTUP_CAN_ID                   0x750

#define TEL_FLAGS_CAN_DATA_LENGTH                  1       
#define TEL_FLAGS_BOOTUP_CAN_ID                    0x751

static CAN_TxHeaderTypeDef time_since_bootup_can_header = {
    .StdId = TIME_SINCE_BOOTUP_CAN_ID,
    .ExtId = 0x0000,
    .IDE = CAN_ID_STD,
    .RTR = CAN_RTR_DATA,
    .DLC = TIME_SINCE_BOOTUP_CAN_DATA_LENGTH};



void UART_test_transmit(void)
{
    // Use the actual peripheral handle from your main.c (usually &huart4)
    char *test_msg = "UART OK\r\n";
    // HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_SET);
    // HAL_Delay(500);
    // HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_RESET);
    // HAL_Delay(500);
    
    // // Timeout is set to 100ms - plenty of time for 9 bytes
    HAL_StatusTypeDef status = HAL_UART_Transmit(&huart4, (uint8_t*)test_msg, sizeof(test_msg)-1, HAL_MAX_DELAY);

    if (status == HAL_OK) {
        // This confirms the hardware registers accepted the data
    } else if (status == HAL_TIMEOUT) {
        // This usually means the UART is stuck Busy or the clock is wrong
    } else {
        // Hardware error
    }
}

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
        CAN_comms_Tx_msg_t time_since_bootup_can_tx = {
            .data[0] = (g_time_since_bootup & 0x000000FFU) >> 0,
            .data[1] = (g_time_since_bootup & 0x0000FF00U) >> 8,
            .data[2] = (g_time_since_bootup & 0x00FF0000U) >> 16,
            .data[3] = (g_time_since_bootup & 0xFF000000U) >> 24,
            .header = time_since_bootup_can_header,
        };

        TelAppTransmitMsg(&time_since_bootup_can_tx);
        
    }
}

