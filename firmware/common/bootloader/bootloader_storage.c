/**
 * @file    bootloader_storage.c
 * @brief   Application flash erase, programming, and readback verification.
 *
 * Each programmed halfword is checked immediately against its source bytes.
 */

#include "bootloader.h"
#include "stm32f1xx_hal.h"

/* Host tests supply an emulated flash address space through their HAL stub. */
#ifndef BOOTLOADER_STORAGE_POINTER
#define BOOTLOADER_STORAGE_POINTER(address) ((void*)(uintptr_t)(address))
#endif

/* FUNCTION PROTOTYPES */
static bool BootloaderFlashRangeIsValid(uint32_t address, size_t length);

/* APPLICATION FLASH PROGRAMMING */

/**
 * @brief Checks that a flash write stays within the application slot.
 */
static bool BootloaderFlashRangeIsValid(uint32_t address, size_t length)
{
    uint32_t app_end = BOOTLOADER_APP_START_ADDRESS + BOOTLOADER_APP_MAX_SIZE_BYTES;
    return (address >= BOOTLOADER_APP_START_ADDRESS) && (address <= app_end) &&
           (length <= (size_t)(app_end - address));
}

/**
 * @brief Unlocks flash and erases the pages needed for a new application.
 * @param image_size Application image size in bytes.
 * @return True if the size is valid and every required page was erased.
 * @note Call BootloaderFlashEndAppUpdate when the update finishes or fails.
 */
bool BootloaderFlashBeginAppUpdate(uint32_t image_size)
{
    FLASH_EraseInitTypeDef erase = {0};
    uint32_t page_error = 0U;

    if ((image_size == 0U) || (image_size > BOOTLOADER_APP_MAX_SIZE_BYTES))
    {
        return false;
    }

    uint32_t page_count =
        (image_size + BOOTLOADER_FLASH_PAGE_SIZE_BYTES - 1U) / BOOTLOADER_FLASH_PAGE_SIZE_BYTES;

    HAL_FLASH_Unlock();

    erase.TypeErase = FLASH_TYPEERASE_PAGES;
    erase.NbPages = 1U;

    /* Erase one page at a time.  A multi-page HAL erase can take longer than
     * the watchdog period inherited across reset by DRD/STR. */
    for (uint32_t page = 0U; page < page_count; page++)
    {
        erase.PageAddress =
            BOOTLOADER_APP_START_ADDRESS + (page * BOOTLOADER_FLASH_PAGE_SIZE_BYTES);
        BootloaderServiceWatchdog();
        if (HAL_FLASHEx_Erase(&erase, &page_error) != HAL_OK)
        {
            HAL_FLASH_Lock();
            return false;
        }
    }

    return true;
}

/**
 * @brief Programs and verifies halfwords within application flash.
 * @param address Halfword-aligned destination flash address.
 * @param data Readable source bytes.
 * @param length Number of source bytes to write.
 * @return True if the range is valid and every halfword was written and verified.
 * @note Flash must be unlocked. An odd final byte is padded with 0xFF.
 */
bool BootloaderFlashWrite(uint32_t address, const uint8_t* data, size_t length)
{
    if (((address & 1U) != 0U) || !BootloaderFlashRangeIsValid(address, length))
    {
        return false;
    }

    for (size_t index = 0U; index < length; index += 2U)
    {
        BootloaderServiceWatchdog();
        uint16_t halfword = data[index];

        if ((index + 1U) < length)
        {
            halfword |= (uint16_t)data[index + 1U] << 8U;
        }
        else
        {
            halfword |= 0xFF00U;
        }

        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, address + (uint32_t)index, halfword) !=
            HAL_OK)
        {
            return false;
        }

        const volatile uint16_t* programmed =
            (const volatile uint16_t*)BOOTLOADER_STORAGE_POINTER(address + (uint32_t)index);
        if (*programmed != halfword)
        {
            return false;
        }
    }

    return true;
}

/**
 * @brief Locks flash after application writes.
 */
void BootloaderFlashEndAppUpdate(void) { HAL_FLASH_Lock(); }
