/**
 * @file    bootloader.c
 * @brief   CAN bootloader startup, application handoff, and exception handlers.
 *
 * BootloaderRun chooses between launching the app and polling CAN for updates.
 * The CAN response callback waits for transmission before allowing a reboot.
 */

#include "bootloader.h"
#include "fw_update_can.h"
#include "stm32f1xx_hal.h"

/* FUNCTION PROTOTYPES */
__attribute__((naked, noinline, noreturn)) static void BootloaderBranchToApp(uint32_t app_stack,
                                                                             uint32_t app_reset);
int main(void);
void NMI_Handler(void);
void HardFault_Handler(void);
void MemManage_Handler(void);
void BusFault_Handler(void);
void UsageFault_Handler(void);
void SVC_Handler(void);
void DebugMon_Handler(void);
void PendSV_Handler(void);
void SysTick_Handler(void);

/* STARTUP AND APPLICATION HANDOFF */

/**
 * @brief Installs the application stack and branches to its reset handler.
 * @param app_stack Application initial stack pointer.
 * @param app_reset Application reset vector including the Thumb bit.
 *
 * Keep the MSP write and the branch in one naked function.  A normal C
 * function may restore its own stack frame after __set_MSP(), which would
 * read the application's (not the bootloader's) stack and corrupt the
 * handoff.  The AAPCS passes app_stack in r0 and app_reset in r1, so this
 * helper has no compiler-generated prologue/epilogue and cannot touch the
 * stack after installing the application's MSP.
 */
__attribute__((naked, noinline, noreturn)) static void BootloaderBranchToApp(uint32_t app_stack,
                                                                             uint32_t app_reset)
{
    __asm volatile("msr msp, r0\n"
                   "cpsie i\n"
                   "bx r1\n");
}

/**
 * @brief Checks that the application stack and reset vectors fit the configured memory layout.
 * @return True when the vectors are valid; false otherwise.
 * @note This is a vector check, not cryptographic image verification.
 */
bool BootloaderAppIsValid(void)
{
    uint32_t app_stack = *(uint32_t*)BOOTLOADER_APP_START_ADDRESS;
    uint32_t app_reset = *(uint32_t*)(BOOTLOADER_APP_START_ADDRESS + 4U);
    uint32_t app_reset_address = app_reset & ~1U;

    bool stack_valid = (app_stack >= BOOTLOADER_SRAM_START_ADDRESS) &&
                       (app_stack <= BOOTLOADER_SRAM_END_ADDRESS) && ((app_stack & 0x7U) == 0U);

    bool reset_valid =
        ((app_reset & 0x1U) != 0U) && (app_reset_address >= BOOTLOADER_APP_START_ADDRESS) &&
        (app_reset_address < (BOOTLOADER_APP_START_ADDRESS + BOOTLOADER_APP_MAX_SIZE_BYTES));

    return stack_valid && reset_valid;
}

/**
 * @brief Evaluates application validity and the board update request.
 * @return True when the bootloader must stay in update mode.
 * @note May consume a one-shot board boot request.
 */
bool BootloaderShouldEnterUpdateMode(void)
{
    if (!BootloaderAppIsValid())
    {
        /* An interrupted/blank application must remain recoverable. */
        return true;
    }

    /* With a valid application, only a board-approved boot token or an
     * explicit physical recovery hook may enter update mode.  Listening for
     * update requests on every ordinary reset would bypass the safety
     * interlock enforced by the running application. */
    return BootloaderBoardStayInBootloader();
}

/**
 * @brief Extends an inherited watchdog timeout before firmware verification.
 * @note Does not start a watchdog that was previously stopped.
 */
void BootloaderPrepareWatchdog(void)
{
    /* DRD/STR can arrive here with IWDG already running across software reset.
     * Extend it to the STM32F1 maximum period before image verification.
     * Programming these registers does not start a previously stopped IWDG. */
    IWDG->KR = 0x5555U;
    IWDG->PR = 0x06U; /* STM32F1 PR encoding for divider /256. */
    IWDG->RLR = 0x0FFFU;
    uint32_t wait = 100000U;
    while (((IWDG->SR & (IWDG_SR_PVU | IWDG_SR_RVU)) != 0U) && (wait-- > 0U))
    {
    }
    BootloaderServiceWatchdog();
}

/**
 * @brief Reloads the watchdog during long-running update operations.
 */
void BootloaderServiceWatchdog(void)
{
    /* IWDG keeps running across a software reset on STM32F1.  Writing the
     * reload key is harmless when it has not been started and prevents the
     * DRD/STR release watchdog from interrupting a CAN update. */
    IWDG->KR = 0xAAAAU;
}

/**
 * @brief Validates launch eligibility and transfers control to the application.
 * @note Returns if launch is rejected; a successful handoff does not return.
 */
void BootloaderJumpToApp(void)
{
    /* Normal boot, REBOOT, and idle fallback all use this vector check. */
    if (!BootloaderAppIsValid())
    {
        return;
    }

    uint32_t app_stack = *(uint32_t*)BOOTLOADER_APP_START_ADDRESS;
    uint32_t app_reset = *(uint32_t*)(BOOTLOADER_APP_START_ADDRESS + 4U);

    HAL_DeInit();

    __disable_irq();

    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL = 0;

    for (int i = 0; i < 8; i++)
    {
        NVIC->ICER[i] = 0xFFFFFFFF;
        NVIC->ICPR[i] = 0xFFFFFFFF;
    }

    /* SysTick and PendSV are system exceptions, not NVIC external IRQs. */
    SCB->ICSR = SCB_ICSR_PENDSTCLR_Msk | SCB_ICSR_PENDSVCLR_Msk;
    SCB->VTOR = BOOTLOADER_APP_START_ADDRESS;
    __DSB();
    __ISB();

    BootloaderBranchToApp(app_stack, app_reset);
}

/**
 * @brief Prepares the watchdog and starts the application or enters update mode.
 */
void BootloaderRun(void)
{
    BootloaderPrepareWatchdog();
    if (!BootloaderShouldEnterUpdateMode())
    {
        /* Re-check the vector immediately before the irreversible handoff. */
        if (BootloaderAppIsValid())
        {
            BootloaderJumpToApp();
        }
    }

    BootloaderEnterUpdateMode();
}

/**
 * @brief Initializes the HAL and starts the bootloader.
 * @return Does not return during normal operation.
 */
int main(void)
{
    HAL_Init();
    BootloaderRun();

    while (1)
    {
    }
}

/* EXCEPTION HANDLERS AND HAL TIME BASE */
/**
 * @brief Halts execution on a non-maskable interrupt.
 */
void NMI_Handler(void)
{
    while (1)
    {
    }
}

/**
 * @brief Halts execution after a hard fault until reset.
 */
void HardFault_Handler(void)
{
    while (1)
    {
    }
}

/**
 * @brief Halts execution after a memory-management fault.
 */
void MemManage_Handler(void)
{
    while (1)
    {
    }
}

/**
 * @brief Halts execution after a bus fault.
 */
void BusFault_Handler(void)
{
    while (1)
    {
    }
}

/**
 * @brief Halts execution after an instruction or processor-state fault.
 */
void UsageFault_Handler(void)
{
    while (1)
    {
    }
}

/**
 * @brief Provides the unused supervisor-call vector for the bare-metal bootloader.
 */
void SVC_Handler(void) {}

/**
 * @brief Provides the unused debug-monitor vector.
 */
void DebugMon_Handler(void) {}

/**
 * @brief Provides the unused context-switch vector for the bare-metal bootloader.
 */
void PendSV_Handler(void) {}

/**
 * @brief Advances the HAL millisecond tick used by update timeouts.
 */
void SysTick_Handler(void) { HAL_IncTick(); }

/* CAN update-mode initialization and polling. */
#ifndef FW_UPDATE_CAN_NODE_ID
#error "FW_UPDATE_CAN_NODE_ID must be defined by the board target"
#endif
#ifndef FW_UPDATE_CAN_REMAP
#error "FW_UPDATE_CAN_REMAP must be defined by the board target"
#endif

/* CAN SETUP AND UPDATE LOOP */
#define FW_UPDATE_CAN_RESPONSE_TIMEOUT_MS 2000U
#define FW_UPDATE_CAN_IDLE_BOOT_MS 30000U

static CAN_HandleTypeDef update_can;
static FirmwareUpdateCanLink can_link;
static bool can_initialized;
static uint32_t last_request_activity;

/* FUNCTION PROTOTYPES */
static void InitializeCanPins(void);
static void InitializeCan(void);
static bool SendCanResponse(const uint8_t* frame, size_t frame_length, void* context);
static void PollOneRequest(void);

/**
 * @brief Configures the selected STM32 CAN pins and peripheral clocks.
 */
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

/**
 * @brief Initializes the bootloader CAN link once at 500 kbit/s.
 */
static void InitializeCan(void)
{
    if (can_initialized)
    {
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
        FirmwareUpdateCanLinkInit(
            &can_link, &update_can, FW_UPDATE_CAN_NODE_ID, FW_UPDATE_CAN_TESTER_ADDRESS);
}

/**
 * @brief Sends a response and services the watchdog until CAN drains or times out.
 */
static bool SendCanResponse(const uint8_t* frame, size_t frame_length, void* context)
{
    (void)context;
    last_request_activity = HAL_GetTick();
    if (!FirmwareUpdateCanLinkSendFrame(&can_link, frame, frame_length))
    {
        return false;
    }

    uint32_t deadline = HAL_GetTick() + FW_UPDATE_CAN_RESPONSE_TIMEOUT_MS;
    while (FirmwareUpdateCanLinkTxBusy(&can_link) ||
           (HAL_CAN_GetTxMailboxesFreeLevel(&update_can) != 3U))
    {
        BootloaderServiceWatchdog();
        FirmwareUpdateCanLinkPoll(&can_link);
        if ((int32_t)(deadline - HAL_GetTick()) <= 0)
        {
            FirmwareUpdateCanLinkAbort(&can_link);
            return false;
        }
    }
    return true;
}

/**
 * @brief Processes one complete CAN request through the update engine.
 */
static void PollOneRequest(void)
{
    FirmwareUpdateCanLinkPoll(&can_link);
    const uint8_t* frame = NULL;
    size_t frame_length = 0U;
    if (!FirmwareUpdateCanLinkPeekFrame(&can_link, &frame, &frame_length))
    {
        return;
    }
    (void)FirmwareUpdateBootloaderProcessFrame(frame, frame_length, SendCanResponse, NULL);
    FirmwareUpdateCanLinkConsumeFrame(&can_link);
}

/**
 * @brief Provides the board-specific request to remain in update mode.
 * @return True when the board requests update mode.
 */
bool BootloaderBoardStayInBootloader(void) { return FirmwareUpdateConsumeBootloaderRequest(); }

/**
 * @brief Runs the board-specific firmware update loop.
 */
void BootloaderEnterUpdateMode(void)
{
    InitializeCan();
    last_request_activity = HAL_GetTick();
    for (;;)
    {
        BootloaderServiceWatchdog();
        if (can_initialized)
        {
            PollOneRequest();
        }
        if (BootloaderAppIsValid() &&
            ((uint32_t)(HAL_GetTick() - last_request_activity) >= FW_UPDATE_CAN_IDLE_BOOT_MS))
        {
            BootloaderJumpToApp();
        }
    }
}
