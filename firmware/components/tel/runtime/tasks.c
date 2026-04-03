#include "tasks.h"

/* IMU TASK */
void TasksIMU(void* argument)
{
    (void)argument; // Unused parameter
    static int counter = 0;

    for (;;)
    {
        counter++;
        osDelay(1000); // Delay for 1000ms (1 second)
    }
}