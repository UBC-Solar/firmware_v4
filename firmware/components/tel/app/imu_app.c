#include "imu_app.h"
#include "imu_driver.h"

void imu_app_init(void)
{
    imu_init();
}

void imu_app_task(void)
{
    // next milestone: parse SHTP_CHANNEL_REPORTS payloads into imu_data_t
}