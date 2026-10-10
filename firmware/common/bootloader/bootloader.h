/**
 * @file    bootloader.h
 * @brief   CAN bootloader API, memory layout, and update engine entry point.
 */

#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Default STM32F103RCTx memory layout used by the current car boards.
 * Board build targets override these defaults as needed. Application limits
 * must match the application slot in the linker script.
 */
#ifndef BOOTLOADER_FLASH_START_ADDRESS
#define BOOTLOADER_FLASH_START_ADDRESS 0x08000000U
#endif

#ifndef BOOTLOADER_SIZE_BYTES
#define BOOTLOADER_SIZE_BYTES (32U * 1024U)
#endif

#ifndef BOOTLOADER_APP_START_ADDRESS
#define BOOTLOADER_APP_START_ADDRESS (BOOTLOADER_FLASH_START_ADDRESS + BOOTLOADER_SIZE_BYTES)
#endif

#ifndef BOOTLOADER_FLASH_SIZE_BYTES
#define BOOTLOADER_FLASH_SIZE_BYTES (256U * 1024U)
#endif

#ifndef BOOTLOADER_FLASH_END_ADDRESS
#define BOOTLOADER_FLASH_END_ADDRESS (BOOTLOADER_FLASH_START_ADDRESS + BOOTLOADER_FLASH_SIZE_BYTES)
#endif

#ifndef BOOTLOADER_FLASH_PAGE_SIZE_BYTES
#define BOOTLOADER_FLASH_PAGE_SIZE_BYTES (2U * 1024U)
#endif

/* Keep the existing 220 KiB slot; the final two pages are unused. */
#ifndef BOOTLOADER_APP_MAX_SIZE_BYTES
#define BOOTLOADER_APP_MAX_SIZE_BYTES                                                              \
    (BOOTLOADER_FLASH_END_ADDRESS - BOOTLOADER_APP_START_ADDRESS -                                 \
     (2U * BOOTLOADER_FLASH_PAGE_SIZE_BYTES))
#endif

#ifndef BOOTLOADER_SRAM_START_ADDRESS
#define BOOTLOADER_SRAM_START_ADDRESS 0x20000000U
#endif

#ifndef BOOTLOADER_SRAM_SIZE_BYTES
#define BOOTLOADER_SRAM_SIZE_BYTES (48U * 1024U)
#endif

#ifndef BOOTLOADER_SRAM_END_ADDRESS
#define BOOTLOADER_SRAM_END_ADDRESS (BOOTLOADER_SRAM_START_ADDRESS + BOOTLOADER_SRAM_SIZE_BYTES)
#endif

/* STARTUP, WATCHDOG, AND APPLICATION HANDOFF */

/**
 * @brief Checks that the application stack and reset vectors fit the configured memory layout.
 * @return True when the vectors are valid; false otherwise.
 * @note This is a vector check, not cryptographic image verification.
 */
bool BootloaderAppIsValid(void);

/**
 * @brief Evaluates application validity and the board update request.
 * @return True when the bootloader must stay in update mode.
 * @note May consume a one-shot board boot request.
 */
bool BootloaderShouldEnterUpdateMode(void);

/**
 * @brief Validates launch eligibility and transfers control to the application.
 * @note Returns if launch is rejected; a successful handoff does not return.
 */
void BootloaderJumpToApp(void);

/**
 * @brief Prepares the watchdog and starts the application or enters update mode.
 */
void BootloaderRun(void);

/**
 * @brief Extends an inherited watchdog timeout before firmware verification.
 * @note Does not start a watchdog that was previously stopped.
 */
void BootloaderPrepareWatchdog(void);

/**
 * @brief Reloads the watchdog during long-running update operations.
 */
void BootloaderServiceWatchdog(void);

/**
 * @brief Provides the board-specific request to remain in update mode.
 * @return True when the board requests update mode.
 */
bool BootloaderBoardStayInBootloader(void);

/**
 * @brief Runs the board-specific firmware update loop.
 */
void BootloaderEnterUpdateMode(void);

/* UPDATE ENGINE */

/**
 * @brief Transmits a complete encoded response through the selected transport.
 * @param frame Encoded response bytes.
 * @param frame_length Number of response bytes.
 * @param context Transport-specific caller context.
 * @return True when transmission succeeds; false on transport failure.
 */
typedef bool (*FirmwareUpdateBootloaderSendFrame)(const uint8_t* frame,
                                                  size_t frame_length,
                                                  void* context);

/**
 * @brief Decodes and processes one complete firmware update protocol frame.
 * @param frame Raw protocol frame including its CRC32.
 * @param frame_length Number of encoded bytes.
 * @param send_frame Callback used to transmit protocol responses.
 * @param send_context Caller context passed unchanged to the transmit callback.
 * @return True only for a valid HELLO addressed to this target.
 * @note The return value indicates a targeted HELLO, not overall update success.
 */
bool FirmwareUpdateBootloaderProcessFrame(const uint8_t* frame,
                                          size_t frame_length,
                                          FirmwareUpdateBootloaderSendFrame send_frame,
                                          void* send_context);

/* Boot-entry token. */
/**
 * @brief Arms a one-shot update request in the backup registers.
 * @note The caller must trigger a software reset after arming the request.
 */
void FirmwareUpdateRequestBootloader(void);

/**
 * @brief Validates and clears a pending bootloader-entry token.
 * @return True only for a matching token with an authorized software-reset cause.
 */
bool FirmwareUpdateConsumeBootloaderRequest(void);

/* Application flash programming. */
/**
 * @brief Unlocks flash and erases the pages needed for a new application.
 * @param image_size Application image size in bytes.
 * @return True if the size is valid and every required page was erased.
 * @note Call BootloaderFlashEndAppUpdate when the update finishes or fails.
 */
bool BootloaderFlashBeginAppUpdate(uint32_t image_size);

/**
 * @brief Programs and verifies halfwords within application flash.
 * @param address Halfword-aligned destination flash address.
 * @param data Readable source bytes.
 * @param length Number of source bytes to write.
 * @return True if the range is valid and every halfword was written and verified.
 * @note Flash must be unlocked. An odd final byte is padded with 0xFF.
 */
bool BootloaderFlashWrite(uint32_t address, const uint8_t* data, size_t length);

/**
 * @brief Locks flash after application writes.
 */
void BootloaderFlashEndAppUpdate(void);

#endif /* BOOTLOADER_H */
