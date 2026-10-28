/**
 * @file    xbee_platform.h
 * @brief   STM32 HAL platform definitions for the Digi XBee ANSI C library.
 * @author  Tony Chen
 */

#ifndef TEL_XBEE_PLATFORM_H
#define TEL_XBEE_PLATFORM_H
#include "stm32f1xx_hal.h"
#include <inttypes.h>
#include <stdint.h>
#include <string.h>
/** @brief Baud rate shared by the cellular modem and Cube USART2 configuration. */
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
/** @brief Boolean type required by the Digi platform interface. */
typedef int bool_t;
/** @brief UART configuration passed to the Digi serial hooks. */
typedef struct
{
    uint32_t baudrate;        /**< Configured UART baud rate in bits per second. */
    UART_HandleTypeDef* uart; /**< Cube-initialized UART handle. */
} xbee_serial_t;

#endif
