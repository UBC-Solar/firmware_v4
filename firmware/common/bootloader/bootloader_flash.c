#include "bootloader_flash.h"

#include "bootloader_config.h"
#include "stm32f1xx_hal.h"

static bool BootloaderFlashRangeIsValid(uint32_t address, size_t length)
{
    uint32_t end_address = address + (uint32_t)length;

    if (end_address < address) {
        return false;
    }

    return (address >= BOOTLOADER_APP_START_ADDRESS) &&
           (end_address <= BOOTLOADER_FLASH_END_ADDRESS);
}

bool BootloaderFlashBeginAppUpdate(uint32_t image_size)
{
    FLASH_EraseInitTypeDef erase = {0};
    uint32_t page_error = 0U;

    if ((image_size == 0U) || (image_size > BOOTLOADER_APP_MAX_SIZE_BYTES)) {
        return false;
    }

    uint32_t page_count =
        (image_size + BOOTLOADER_FLASH_PAGE_SIZE_BYTES - 1U) /
        BOOTLOADER_FLASH_PAGE_SIZE_BYTES;

    HAL_FLASH_Unlock();

    erase.TypeErase = FLASH_TYPEERASE_PAGES;
    erase.PageAddress = BOOTLOADER_APP_START_ADDRESS;
    erase.NbPages = page_count;

    if (HAL_FLASHEx_Erase(&erase, &page_error) != HAL_OK) {
        HAL_FLASH_Lock();
        return false;
    }

    return true;
}

bool BootloaderFlashWrite(uint32_t address, const uint8_t *data, size_t length)
{
    if (((address & 1U) != 0U) || !BootloaderFlashRangeIsValid(address, length)) {
        return false;
    }

    for (size_t index = 0U; index < length; index += 2U) {
        uint16_t halfword = data[index];

        if ((index + 1U) < length) {
            halfword |= (uint16_t)data[index + 1U] << 8U;
        } else {
            halfword |= 0xFF00U;
        }

        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,
                              address + (uint32_t)index,
                              halfword) != HAL_OK) {
            return false;
        }
    }

    return true;
}

void BootloaderFlashEndAppUpdate(void)
{
    HAL_FLASH_Lock();
}
