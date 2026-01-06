#ifndef DEBUG_IO_H
#define DEBUG_IO_H


/**
 * @brief Implementation of debug printf output
 * 
 * Handles the actual formatting and transmission of debug messages.
 * Should not be called directly — use DEBUG_IO_PRINT() macro instead.;
 * Open to implementation change, but currently uses SWD SWO for printing.
 * 
 * @param fmt Format string (printf-style)
 * @param ... Variable arguments matching the format string
 * 
 * @internal
 */
void DEBUG_IO_print(const char* fmt, ...);

 /**
 * @brief Generic Macro for printf style debug logging. Users should call this macro for all
 *      printf style printing they wish to do.
 * 
 *
 * @param fmt Format string (printf-style)
 * @param ... Variable arguments matching the format string
 *
 */
#ifdef DEBUG
    #define DEBUG_IO_PRINT(fmt, ...) DEBUG_IO_print(fmt, ##__VA_ARGS__)
#else
    #define DEBUG_IO_PRINT(fmt, ...) do { } while(0)
#endif



#endif // DEBUG_IO_H