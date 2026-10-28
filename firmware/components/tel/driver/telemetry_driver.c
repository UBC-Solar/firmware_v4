/**<
 * @file    telemetry_driver.c
 * @brief   Telemetry driver implementation for UBC Solar TEL board
 *
 * This file contains the implementation of the telemetry driver functions for the TEL board.
 *
 * @author  Gregory Bian and Tony Chen
 * @date    Jun 30 2026
 */
#include "telemetry_driver.h"
#include "xbee/serial.h"

/* DEFINES */
#define CELLULAR_TX_BUFFER_SIZE 64U

/* STATIC VARIABLES */
static uint8_t cellular_tx[CELLULAR_TX_BUFFER_SIZE];
static unsigned cellular_tx_used;
static bool cellular_write_error;

telemetry_diagnostics_t telemetry_diagnostic;

HAL_StatusTypeDef UART_telemetry_transmit(const uint8_t* data, uint16_t length)
{
    if (data == NULL || length == 0)
    {
        telemetry_diagnostic.telemetry_hal_transmit_failures++;
        osSemaphoreRelease(usart1_tx_semaphore);

        return HAL_ERROR;
    }

    // J-Link does not supply enough current for the Radio Module. Only use ST-Link for this.
    if (!car_config_cellular)
    {
        HAL_GPIO_WritePin(R_RESET_GPIO_Port, R_RESET_Pin, GPIO_PIN_SET);
    }

    UART_HandleTypeDef* huart = TELEMETRY_UART;

    HAL_StatusTypeDef status = HAL_UART_Transmit_DMA(huart, data, length);
    if (status != HAL_OK)
    {
        telemetry_diagnostic.telemetry_hal_transmit_failures++;
        osSemaphoreRelease(usart1_tx_semaphore);
    }
    else
    {
        telemetry_diagnostic.successful_telemetry_tx++;
    }

    return status;
}

/* FUNCTION DEFINITIONS */
void XBeePlatform_ResetTx(void)
{
    cellular_tx_used = 0;
    cellular_write_error = false;
}

HAL_StatusTypeDef XBeePlatform_Transmit(void)
{
    if (cellular_write_error)
    {
        osSemaphoreRelease(usart1_tx_semaphore);
        return HAL_ERROR;
    }

    return UART_telemetry_transmit(cellular_tx, (uint16_t)cellular_tx_used);
}

/* Digi serial hooks: open validates the UART; all hooks share one TX buffer. */
/**
 * @brief   Check whether a Digi serial descriptor lacks a UART handle.
 * @param   serial: Serial descriptor to validate; may be NULL.
 * @return  true if the descriptor or its UART handle is NULL; false otherwise.
 */
bool_t xbee_ser_invalid(xbee_serial_t* serial) { return serial == NULL || serial->uart == NULL; }

/**
 * @brief   Validate the baud rate of the Cube-initialized UART.
 * @param   serial: Serial descriptor to validate.
 * @param   baud_rate: Required modem baud rate in bits per second.
 * @return  0 on a matching configuration; -EINVAL for an invalid descriptor or baud rate.
 */
int xbee_ser_open(xbee_serial_t* serial, uint32_t baud_rate)
{
    if (xbee_ser_invalid(serial))
    {
        return -EINVAL;
    }

    /* Cube owns UART setup; only check that its baud matches the modem. */
    return serial->uart->Init.BaudRate == baud_rate ? 0 : -EINVAL;
}

/**
 * @brief   Append bytes to the shared Digi API frame buffer.
 * @param   serial: Unused; all serial hooks share the same transmit buffer.
 * @param   data: Non-NULL source buffer to copy.
 * @param   length: Number of bytes to append; must be nonnegative.
 * @return  Number of bytes copied, or -EINVAL on invalid input or insufficient space.
 * @note    Caller must hold usart1_tx_semaphore while accessing the shared buffer.
 */
int xbee_ser_write(xbee_serial_t* serial, const void* data, int length)
{
    (void)serial;
    if (data == NULL || length < 0 || (unsigned)length > sizeof(cellular_tx) - cellular_tx_used)
    {
        cellular_write_error = true;
        return -EINVAL;
    }

    memcpy(cellular_tx + cellular_tx_used, data, (unsigned)length);
    cellular_tx_used += (unsigned)length;
    return length;
}

/* Required by Digi to check whether a complete API frame fits. */
/**
 * @brief   Report available space for a complete Digi API frame.
 * @param   serial: Unused; all serial hooks share the same transmit buffer.
 * @return  Number of unused bytes in the transmit buffer.
 * @note    Caller must hold usart1_tx_semaphore while accessing the shared buffer.
 */
int xbee_ser_tx_free(xbee_serial_t* serial)
{
    (void)serial;
    return (int)(sizeof(cellular_tx) - cellular_tx_used);
}

/**
 * @brief   Report the number of buffered Digi API frame bytes.
 * @param   serial: Unused; all serial hooks share the same transmit buffer.
 * @return  Number of bytes currently in the transmit buffer.
 * @note    Caller must hold usart1_tx_semaphore while accessing the shared buffer.
 */
int xbee_ser_tx_used(xbee_serial_t* serial)
{
    (void)serial;
    return (int)cellular_tx_used;
}

/* Required Digi hooks; this board uses no hardware flow control. */
/**
 * @brief   Accept Digi flow-control requests without changing UART configuration.
 * @param   serial: Unused serial descriptor.
 * @param   enabled: Unused request; hardware flow control is disabled on this board.
 * @return  0; no hardware configuration is changed.
 */
int xbee_ser_flowcontrol(xbee_serial_t* serial, bool_t enabled)
{
    (void)serial;
    (void)enabled;
    return 0;
}

/**
 * @brief   Report that transmission is allowed without hardware CTS.
 * @param   serial: Unused serial descriptor.
 * @return  true; this board does not use hardware flow control.
 */
int xbee_ser_get_cts(xbee_serial_t* serial)
{
    (void)serial;
    return true;
}
