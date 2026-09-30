#include "bootloader.h"

#include "bootloader_boot_request.h"
#include "stm32f1xx_hal.h"
#include "fw_update_bootloader_engine.h"
#include "fw_update_can.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifndef FW_UPDATE_CAN_NODE_ID
#error "FW_UPDATE_CAN_NODE_ID must be defined by the board target"
#endif
#ifndef FW_UPDATE_CAN_REMAP
#error "FW_UPDATE_CAN_REMAP must be defined by the board target"
#endif

#define FW_UPDATE_CAN_RESPONSE_TIMEOUT_MS 2000U
#define FW_UPDATE_CAN_IDLE_BOOT_MS        30000U

static CAN_HandleTypeDef update_can;
static FirmwareUpdateCanLink can_link;
static bool can_initialized;
static uint32_t last_request_activity;

static void InitializeCanPins(void)
{
    GPIO_InitTypeDef gpio = {0};
    __HAL_RCC_AFIO_CLK_ENABLE();
    __HAL_RCC_CAN1_CLK_ENABLE();

#if FW_UPDATE_CAN_REMAP
    __HAL_RCC_GPIOB_CLK_ENABLE();
    gpio.Pin = GPIO_PIN_8;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &gpio);
    gpio.Pin = GPIO_PIN_9;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &gpio);
    __HAL_AFIO_REMAP_CAN1_2();
#else
    __HAL_RCC_GPIOA_CLK_ENABLE();
    gpio.Pin = GPIO_PIN_11;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio);
    gpio.Pin = GPIO_PIN_12;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);
    __HAL_AFIO_REMAP_CAN1_1();
#endif
}

static void InitializeCan(void)
{
    if (can_initialized) {
        return;
    }

    /* The bootloader intentionally stays on reset-default HSI at 8 MHz.
     * 1 * (1 + 13 + 2) time quanta produces the car's 500 kbit/s rate. */
    InitializeCanPins();
    update_can.Instance = CAN1;
    update_can.Init.Prescaler = 1U;
    update_can.Init.Mode = CAN_MODE_NORMAL;
    update_can.Init.SyncJumpWidth = CAN_SJW_1TQ;
    update_can.Init.TimeSeg1 = CAN_BS1_13TQ;
    update_can.Init.TimeSeg2 = CAN_BS2_2TQ;
    update_can.Init.TimeTriggeredMode = DISABLE;
    update_can.Init.AutoBusOff = ENABLE;
    update_can.Init.AutoWakeUp = DISABLE;
    update_can.Init.AutoRetransmission = ENABLE;
    update_can.Init.ReceiveFifoLocked = DISABLE;
    update_can.Init.TransmitFifoPriority = ENABLE;

    can_initialized =
        (HAL_CAN_Init(&update_can) == HAL_OK) &&
        FirmwareUpdateCanLinkInit(&can_link,
                              &update_can,
                              FW_UPDATE_CAN_NODE_ID,
                              FW_UPDATE_CAN_TESTER_ADDRESS);
}

static bool SendCanResponse(const uint8_t *frame,
                            size_t frame_length,
                            void *context)
{
    (void)context;
    last_request_activity = HAL_GetTick();
    if (!FirmwareUpdateCanLinkSendFrame(&can_link, frame, frame_length)) {
        return false;
    }

    uint32_t deadline = HAL_GetTick() + FW_UPDATE_CAN_RESPONSE_TIMEOUT_MS;
    while (FirmwareUpdateCanLinkTxBusy(&can_link) ||
           (HAL_CAN_GetTxMailboxesFreeLevel(&update_can) != 3U)) {
        BootloaderServiceWatchdog();
        FirmwareUpdateCanLinkPoll(&can_link);
        if ((int32_t)(deadline - HAL_GetTick()) <= 0) {
            FirmwareUpdateCanLinkAbort(&can_link);
            return false;
        }
    }
    return true;
}

static bool PollOneRequest(void)
{
    FirmwareUpdateCanLinkPoll(&can_link);
    const uint8_t *frame = NULL;
    size_t frame_length = 0U;
    if (!FirmwareUpdateCanLinkPeekFrame(&can_link, &frame, &frame_length)) {
        return false;
    }
    bool hello_received = FirmwareUpdateBootloaderProcessFrame(
        frame,
        frame_length,
        SendCanResponse,
        NULL);
    FirmwareUpdateCanLinkConsumeFrame(&can_link);
    return hello_received;
}

bool BootloaderBoardStayInBootloader(void)
{
    return FirmwareUpdateConsumeBootloaderRequest();
}

void BootloaderEnterUpdateMode(void)
{
    InitializeCan();
    last_request_activity = HAL_GetTick();
    for (;;) {
        BootloaderServiceWatchdog();
        if (can_initialized) {
            (void)PollOneRequest();
        }
        if (BootloaderAppIsValid() &&
            !FirmwareUpdateTrialBootRequiresRecovery() &&
            ((uint32_t)(HAL_GetTick() - last_request_activity) >=
             FW_UPDATE_CAN_IDLE_BOOT_MS)) {
            BootloaderJumpToApp();
        }
    }
}
