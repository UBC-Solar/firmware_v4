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
#define CELLULAR_TX_BUFFER_SIZE         64U

/* STATIC VARIABLES */
static uint8_t cellular_tx[CELLULAR_TX_BUFFER_SIZE];
static unsigned cellular_tx_used;
static bool cellular_write_error;

telemetry_diagnostics_t telemetry_diagnostic;

HAL_StatusTypeDef UART_telemetry_transmit(const uint8_t *data, uint16_t length)
{
    if (data == NULL || length == 0)
    {
        telemetry_diagnostic.telemetry_hal_transmit_failures++;
        osSemaphoreRelease(usart1_tx_semaphore);

        return HAL_ERROR;
    }

    // J-Link does not supply enough current for the Radio Module. Only use ST-Link for this.
    if (!car_config_cellular) {
        HAL_GPIO_WritePin(R_RESET_GPIO_Port, R_RESET_Pin, GPIO_PIN_SET);
    }

    UART_HandleTypeDef *huart = TELEMETRY_UART;

    HAL_StatusTypeDef status = HAL_UART_Transmit_DMA(huart, data, length);
    if (status != HAL_OK)
    {
        telemetry_diagnostic.telemetry_hal_transmit_failures++;
        osSemaphoreRelease(usart1_tx_semaphore);
    } else {
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
bool_t xbee_ser_invalid(xbee_serial_t *serial)
{
    return serial == NULL || serial->uart == NULL;
}

int xbee_ser_open(xbee_serial_t *serial, uint32_t baud_rate)
{
    if (xbee_ser_invalid(serial))
    {
        return -EINVAL;
    }

    /* Cube owns UART setup; only check that its baud matches the modem. */
    return serial->uart->Init.BaudRate == baud_rate ? 0 : -EINVAL;
}

int xbee_ser_write(xbee_serial_t *serial, const void *data, int length)
{
    (void)serial;
    if (data == NULL || length < 0 ||
        (unsigned)length > sizeof(cellular_tx) - cellular_tx_used)
    {
        cellular_write_error = true;
        return -EINVAL;
    }

    memcpy(cellular_tx + cellular_tx_used, data, (unsigned)length);
    cellular_tx_used += (unsigned)length;
    return length;
}

/* Required by Digi to check whether a complete API frame fits. */
int xbee_ser_tx_free(xbee_serial_t *serial)
{
    (void)serial;
    return (int)(sizeof(cellular_tx) - cellular_tx_used);
}

int xbee_ser_tx_used(xbee_serial_t *serial)
{
    (void)serial;
    return (int)cellular_tx_used;
}

/* Required Digi hooks; this board uses no hardware flow control. */
int xbee_ser_flowcontrol(xbee_serial_t *serial, bool_t enabled)
{
    (void)serial;
    (void)enabled;
    return 0;
}

int xbee_ser_get_cts(xbee_serial_t *serial)
{
    (void)serial;
    return true;
}
