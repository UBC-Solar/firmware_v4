#include "debug_io.h"


// Logging level definitions
#define LOG_LEVEL_DEBUG 0
#define LOG_LEVEL_INFO 1
#define LOG_LEVEL_ERROR 2


// Logging macros - only output if current level is sufficient
// Each macro expands to debug_io output or nothing based on CURRENT_LOG_LEVEL
#define LOG_DEBUG(fmt, ...) \
    do { if (CURRENT_LOG_LEVEL <= LOG_LEVEL_DEBUG) DEBUG_IO_PRINT("[DEBUG] " fmt "\r\n", ##__VA_ARGS__); } while(0)

#define LOG_INFO(fmt, ...) \
    do { if (CURRENT_LOG_LEVEL <= LOG_LEVEL_INFO) DEBUG_IO_PRINT("[INFO] " fmt "\r\n", ##__VA_ARGS__); } while(0)

#define LOG_ERROR(fmt, ...) \
    do { if (CURRENT_LOG_LEVEL <= LOG_LEVEL_ERROR) DEBUG_IO_PRINT("[ERROR] " fmt "\r\n", ##__VA_ARGS__); } while(0)

#define ERROR_HANDLER_LOGGED() \
    do { LOG_ERROR("Entering Error_Handler from %s:%d.", __FILE__, __LINE__); Error_Handler(); } while (0)
