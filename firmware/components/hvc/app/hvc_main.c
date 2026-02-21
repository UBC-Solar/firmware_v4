#include "debug_io.h"
#include "stdio.h"

void hvcMain(void)
{
    DEBUG_IO_PRINT("Hello from hvcMain!\n");
    printf("Hello from printf!\n");
}
