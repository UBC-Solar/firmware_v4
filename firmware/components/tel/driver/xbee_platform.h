#ifndef TEL_XBEE_PLATFORM_H
#define TEL_XBEE_PLATFORM_H
#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
/* Must match the cellular modem and Cube USART2 configuration. */
#define CELLULAR_BAUD_RATE 230400U

#define PACKED_STRUCT struct __attribute__((__packed__))
#define XBEE_PACKED(name, decl) PACKED_STRUCT name decl
#define XBEE_NATIVE_64BIT
#define XBEE_CELLULAR_ENABLED 1
#define XBEE_WIFI_ENABLED 0
#define XBEE_MAX_RFPAYLOAD 64
#define XBEE_MS_TIMER_RESOLUTION 1
#define _f_memcpy memcpy
#define _f_memset memset
#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN 1234
#define BIG_ENDIAN 4321
#define BYTE_ORDER LITTLE_ENDIAN
#endif
typedef int bool_t;
typedef struct {
    uint32_t baudrate;
    UART_HandleTypeDef *uart;
} xbee_serial_t;

#endif
