#include "bootloader.h"

#include "bootloader_crc32.h"
#include "bootloader_flash.h"
#include "stm32f1xx_hal.h"

#include <string.h>

#define NUCLEO_USER_BUTTON_GPIO_Port GPIOC
#define NUCLEO_USER_BUTTON_Pin       GPIO_PIN_13
#define NUCLEO_USER_LED_GPIO_Port    GPIOA
#define NUCLEO_USER_LED_Pin          GPIO_PIN_5

#define NUCLEO_UPDATE_UART           USART3
#define NUCLEO_UPDATE_UART_GPIO_Port GPIOC
#define NUCLEO_UPDATE_UART_TX_Pin    GPIO_PIN_10
#define NUCLEO_UPDATE_UART_RX_Pin    GPIO_PIN_11
#define NUCLEO_UPDATE_UART_BAUD      115200U
#define NUCLEO_UPDATE_UART_TIMEOUT_MS 5000U
#define NUCLEO_UPDATE_CHUNK_BYTES    256U

#define BOOTLOADER_PROTOCOL_MAGIC_0  'U'
#define BOOTLOADER_PROTOCOL_MAGIC_1  'B'
#define BOOTLOADER_PROTOCOL_MAGIC_2  'S'
#define BOOTLOADER_PROTOCOL_MAGIC_3  'L'
#define BOOTLOADER_PROTOCOL_ACK      0x79U
#define BOOTLOADER_PROTOCOL_NACK     0x1FU

static UART_HandleTypeDef update_uart;

static void NucleoBusyWait(uint32_t count)
{
    for (volatile uint32_t delay = 0; delay < count; delay++) {
    }
}

static void NucleoInitUserButton(void)
{
    GPIO_InitTypeDef gpio_init = {0};

    __HAL_RCC_GPIOC_CLK_ENABLE();

    gpio_init.Pin = NUCLEO_USER_BUTTON_Pin;
    gpio_init.Mode = GPIO_MODE_INPUT;
    gpio_init.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(NUCLEO_USER_BUTTON_GPIO_Port, &gpio_init);
}

static void NucleoInitUserLed(void)
{
    GPIO_InitTypeDef gpio_init = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();

    HAL_GPIO_WritePin(NUCLEO_USER_LED_GPIO_Port, NUCLEO_USER_LED_Pin, GPIO_PIN_RESET);

    gpio_init.Pin = NUCLEO_USER_LED_Pin;
    gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(NUCLEO_USER_LED_GPIO_Port, &gpio_init);
}

static void NucleoInitUpdateUart(void)
{
    GPIO_InitTypeDef gpio_init = {0};

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_AFIO_CLK_ENABLE();
    __HAL_RCC_USART3_CLK_ENABLE();
    __HAL_AFIO_REMAP_USART3_PARTIAL();

    gpio_init.Pin = NUCLEO_UPDATE_UART_TX_Pin;
    gpio_init.Mode = GPIO_MODE_AF_PP;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(NUCLEO_UPDATE_UART_GPIO_Port, &gpio_init);

    gpio_init.Pin = NUCLEO_UPDATE_UART_RX_Pin;
    gpio_init.Mode = GPIO_MODE_INPUT;
    gpio_init.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(NUCLEO_UPDATE_UART_GPIO_Port, &gpio_init);

    update_uart.Instance = NUCLEO_UPDATE_UART;
    update_uart.Init.BaudRate = NUCLEO_UPDATE_UART_BAUD;
    update_uart.Init.WordLength = UART_WORDLENGTH_8B;
    update_uart.Init.StopBits = UART_STOPBITS_1;
    update_uart.Init.Parity = UART_PARITY_NONE;
    update_uart.Init.Mode = UART_MODE_TX_RX;
    update_uart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    update_uart.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&update_uart);
}

static uint32_t NucleoReadLe32(const uint8_t *data)
{
    return (uint32_t)data[0] |
           ((uint32_t)data[1] << 8U) |
           ((uint32_t)data[2] << 16U) |
           ((uint32_t)data[3] << 24U);
}

static void NucleoSendProtocolByte(uint8_t value)
{
    HAL_UART_Transmit(&update_uart, &value, 1U, NUCLEO_UPDATE_UART_TIMEOUT_MS);
}

static void NucleoSendAck(void)
{
    NucleoSendProtocolByte(BOOTLOADER_PROTOCOL_ACK);
}

static void NucleoSendNack(void)
{
    NucleoSendProtocolByte(BOOTLOADER_PROTOCOL_NACK);
}

static bool NucleoReceiveExact(uint8_t *data, uint16_t length, uint32_t timeout_ms)
{
    return HAL_UART_Receive(&update_uart, data, length, timeout_ms) == HAL_OK;
}

static bool NucleoTryReceiveUpdateHeader(uint32_t *image_size, uint32_t *expected_crc)
{
    uint8_t header[12] = {0};

    if (HAL_UART_Receive(&update_uart, &header[0], 1U, 50U) != HAL_OK) {
        return false;
    }

    if (header[0] != BOOTLOADER_PROTOCOL_MAGIC_0) {
        return false;
    }

    if (!NucleoReceiveExact(&header[1], sizeof(header) - 1U, 500U)) {
        return false;
    }

    if ((header[1] != BOOTLOADER_PROTOCOL_MAGIC_1) ||
        (header[2] != BOOTLOADER_PROTOCOL_MAGIC_2) ||
        (header[3] != BOOTLOADER_PROTOCOL_MAGIC_3)) {
        return false;
    }

    *image_size = NucleoReadLe32(&header[4]);
    *expected_crc = NucleoReadLe32(&header[8]);

    return true;
}

static bool NucleoReceiveAndFlashImage(uint32_t image_size, uint32_t expected_crc)
{
    uint8_t chunk[NUCLEO_UPDATE_CHUNK_BYTES] = {0};
    uint8_t vector_table_prefix[8] = {0};
    uint32_t crc = BOOTLOADER_CRC32_INITIAL;
    uint32_t offset = 0U;

    if ((image_size < sizeof(vector_table_prefix)) ||
        (image_size > BOOTLOADER_APP_MAX_SIZE_BYTES)) {
        return false;
    }

    if (!BootloaderFlashBeginAppUpdate(image_size)) {
        return false;
    }

    NucleoSendAck();

    while (offset < image_size) {
        uint32_t remaining = image_size - offset;
        uint16_t chunk_size = (remaining > sizeof(chunk)) ?
            sizeof(chunk) :
            (uint16_t)remaining;

        if (!NucleoReceiveExact(chunk, chunk_size, NUCLEO_UPDATE_UART_TIMEOUT_MS)) {
            BootloaderFlashEndAppUpdate();
            return false;
        }

        crc = BootloaderCrc32Update(crc, chunk, chunk_size);

        if (offset == 0U) {
            memcpy(vector_table_prefix, chunk, sizeof(vector_table_prefix));
            memset(chunk, 0xFF, sizeof(vector_table_prefix));
        }

        if (!BootloaderFlashWrite(BOOTLOADER_APP_START_ADDRESS + offset,
                                  chunk,
                                  chunk_size)) {
            BootloaderFlashEndAppUpdate();
            return false;
        }

        offset += chunk_size;
        NucleoSendAck();
    }

    crc = BootloaderCrc32Finalize(crc);

    if (crc != expected_crc) {
        BootloaderFlashEndAppUpdate();
        return false;
    }

    if (!BootloaderFlashWrite(BOOTLOADER_APP_START_ADDRESS,
                              vector_table_prefix,
                              sizeof(vector_table_prefix))) {
        BootloaderFlashEndAppUpdate();
        return false;
    }

    BootloaderFlashEndAppUpdate();

    return true;
}

bool BootloaderBoardStayInBootloader(void)
{
    NucleoInitUserButton();

    for (uint32_t sample = 0; sample < 200U; sample++) {
        if (HAL_GPIO_ReadPin(NUCLEO_USER_BUTTON_GPIO_Port,
                             NUCLEO_USER_BUTTON_Pin) == GPIO_PIN_RESET) {
            return true;
        }

        NucleoBusyWait(50000U);
    }

    return false;
}

void BootloaderEnterUpdateMode(void)
{
    static const uint8_t ready_message[] = "UBCBOOT UART READY\r\n";
    uint32_t last_blink_ms = HAL_GetTick();

    NucleoInitUserLed();
    NucleoInitUpdateUart();

    HAL_UART_Transmit(&update_uart,
                      (uint8_t *)ready_message,
                      sizeof(ready_message) - 1U,
                      NUCLEO_UPDATE_UART_TIMEOUT_MS);

    for (;;) {
        uint32_t image_size = 0U;
        uint32_t expected_crc = 0U;

        if (NucleoTryReceiveUpdateHeader(&image_size, &expected_crc)) {
            if ((image_size == 0U) || (image_size > BOOTLOADER_APP_MAX_SIZE_BYTES)) {
                NucleoSendNack();
                continue;
            }

            if (NucleoReceiveAndFlashImage(image_size, expected_crc)) {
                NucleoSendAck();
                HAL_Delay(100U);

                if (BootloaderAppIsValid()) {
                    BootloaderJumpToApp();
                }
            } else {
                NucleoSendNack();
            }
        }

        if ((HAL_GetTick() - last_blink_ms) >= 500U) {
            HAL_GPIO_TogglePin(NUCLEO_USER_LED_GPIO_Port, NUCLEO_USER_LED_Pin);
            last_blink_ms = HAL_GetTick();
        }
    }
}
