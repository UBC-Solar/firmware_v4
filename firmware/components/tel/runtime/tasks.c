#include "tasks.h"

/* IMU TASK */
void TasksIMU(void* argument)
{
    (void)argument; // Unused parameter
    static int counter = 0;

    for (;;)
    {
        counter++;
    }
}