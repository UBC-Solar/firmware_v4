#include "bootloader.h"

#include "bootloader_boot_request.h"
#include "stm32f1xx_hal.h"
#include "sunlite_ota_bootloader_engine.h"
#include "sunlite_ota_protocol.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SUNLITE_OTA_UART_BAUD       230400U
#define SUNLITE_OTA_UART_TIMEOUT_MS 1000U
#define SUNLITE_OTA_UART_IDLE_BOOT_MS 30000U

static UART_HandleTypeDef update_uart;
static bool uart_initialized;
static uint8_t encoded_frame[SUNLITE_OTA_MAX_ENCODED_FRAME];
static size_t encoded_length;
static bool discard_until_delimiter;
static uint32_t last_request_activity;

static bool ConfigureSystemClock(void)
{
    RCC_OscInitTypeDef oscillator = {0};
    RCC_ClkInitTypeDef clocks = {0};

    /* Match TEL's application clock tree.  Leaving the bootloader on the
     * reset-default HSI makes 230400-baud UART timing unnecessarily sensitive
     * to the internal oscillator tolerance, even though the same UART works
     * reliably after the application switches to its 72 MHz HSE/PLL clock. */
    oscillator.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    oscillator.HSEState = RCC_HSE_ON;
    oscillator.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    oscillator.PLL.PLLState = RCC_PLL_ON;
    oscillator.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    oscillator.PLL.PLLMUL = RCC_PLL_MUL9;
    if (HAL_RCC_OscConfig(&oscillator) != HAL_OK) {
        return false;
    }

    clocks.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                       RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clocks.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clocks.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clocks.APB1CLKDivider = RCC_HCLK_DIV2;
    clocks.APB2CLKDivider = RCC_HCLK_DIV1;
    return HAL_RCC_ClockConfig(&clocks, FLASH_LATENCY_2) == HAL_OK;
}

static void InitializeUart(void)
{
    if (uart_initialized) {
        return;
    }

    /* The external clock is preferred, but recovery UART must never depend on
     * it.  If HSE/PLL startup fails, the MCU is still running from reset-safe
     * HSI; update the CMSIS frequency from the actual RCC registers and bring
     * UART5 up on that clock instead of making the bootloader unreachable. */
    (void)ConfigureSystemClock();
    SystemCoreClockUpdate();

    GPIO_InitTypeDef gpio = {0};
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_AFIO_CLK_ENABLE();
    __HAL_RCC_UART5_CLK_ENABLE();

    gpio.Pin = GPIO_PIN_12;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOC, &gpio);
    gpio.Pin = GPIO_PIN_2;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOD, &gpio);

    update_uart.Instance = UART5;
    update_uart.Init.BaudRate = SUNLITE_OTA_UART_BAUD;
    update_uart.Init.WordLength = UART_WORDLENGTH_8B;
    update_uart.Init.StopBits = UART_STOPBITS_1;
    update_uart.Init.Parity = UART_PARITY_NONE;
    update_uart.Init.Mode = UART_MODE_TX_RX;
    update_uart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    update_uart.Init.OverSampling = UART_OVERSAMPLING_16;
    uart_initialized = HAL_UART_Init(&update_uart) == HAL_OK;
}

static bool SendUartFrame(const uint8_t *frame,
                          size_t frame_length,
                          void *context)
{
    (void)context;
    last_request_activity = HAL_GetTick();
    return (frame_length <= UINT16_MAX) &&
           (HAL_UART_Transmit(&update_uart,
                              (uint8_t *)frame,
                              (uint16_t)frame_length,
                              SUNLITE_OTA_UART_TIMEOUT_MS) == HAL_OK);
}

static bool ConsumeByte(uint8_t byte)
{
    if (byte != 0U) {
        if (discard_until_delimiter) {
            return false;
        }
        if (encoded_length >= sizeof(encoded_frame)) {
            encoded_length = 0U;
            discard_until_delimiter = true;
            return false;
        }
        encoded_frame[encoded_length++] = byte;
        return false;
    }
    if (discard_until_delimiter) {
        discard_until_delimiter = false;
        encoded_length = 0U;
        return false;
    }
    if (encoded_length == 0U) {
        return false;
    }

    bool hello_received = SunliteOtaBootloaderProcessFrame(
        encoded_frame,
        encoded_length,
        SendUartFrame,
        NULL);
    encoded_length = 0U;
    return hello_received;
}

bool BootloaderBoardStayInBootloader(void)
{
    return SunliteOtaConsumeBootloaderRequest();
}

void BootloaderEnterUpdateMode(void)
{
    InitializeUart();
    last_request_activity = HAL_GetTick();
    for (;;) {
        uint8_t byte;
        BootloaderServiceWatchdog();
        if (uart_initialized &&
            (HAL_UART_Receive(&update_uart, &byte, 1U, 100U) == HAL_OK)) {
            (void)ConsumeByte(byte);
        }
        if (BootloaderAppIsValid() &&
            !SunliteOtaTrialBootRequiresRecovery() &&
            ((uint32_t)(HAL_GetTick() - last_request_activity) >=
             SUNLITE_OTA_UART_IDLE_BOOT_MS)) {
            BootloaderJumpToApp();
        }
    }
}
