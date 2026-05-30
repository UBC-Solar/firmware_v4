#include "debug_io.h"

void AppMain(void)
{

    for(;;)
    {
        static int count = 0;
        DEBUG_IO_PRINT("Hello World! %d\n", count);
        ++count;
    }
}