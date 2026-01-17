#include <stdio.h>
#include <stdarg.h>
#include "debug_io.h"
#include "stm32f1xx_hal.h"

int __io_putchar(int ch)
{
    ITM_SendChar((unsigned char)ch);
    return ch;
}

void DEBUG_IO_print(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    fflush(stdout);
}