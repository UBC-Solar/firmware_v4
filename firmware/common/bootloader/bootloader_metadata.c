#include "bootloader_metadata.h"

#include "bootloader_config.h"
#include "bootloader_crc32.h"
#include "bootloader_flash.h"
#include "stm32f1xx_hal.h"
#include "sunlite_ota_protocol.h"

#include <string.h>

#ifndef BOOTLOADER_METADATA_PAGE_ADDRESS
#error "BOOTLOADER_METADATA_PAGE_ADDRESS must reserve a flash page for OTA metadata"
#endif
#ifndef BOOTLOADER_METADATA_BACKUP_PAGE_ADDRESS
#error "BOOTLOADER_METADATA_BACKUP_PAGE_ADDRESS must reserve a second metadata page"
#endif

#define METADATA_RECORD_SIZE       56U
#define METADATA_SCHEMA            2U
#define METADATA_GENERATION_OFFSET 8U
#define METADATA_VERSION_OFFSET    12U
#define METADATA_IMAGE_SIZE_OFFSET 16U
#define METADATA_SHA256_OFFSET     20U
#define METADATA_CRC_OFFSET        52U

static bool RecordIsValid(uint32_t address)
{
    const uint8_t *record = (const uint8_t *)address;
    if ((record[0] != 'S') || (record[1] != 'U') ||
        (record[2] != 'M') || (record[3] != 'D') ||
        (SunliteOtaReadBe32(&record[4]) != METADATA_SCHEMA)) {
        return false;
    }
    uint32_t expected_crc = SunliteOtaReadBe32(&record[METADATA_CRC_OFFSET]);
    uint32_t crc = BootloaderCrc32Finalize(
        BootloaderCrc32Update(BOOTLOADER_CRC32_INITIAL,
                              record,
                              METADATA_CRC_OFFSET));
    return crc == expected_crc;
}

static uint32_t RecordGeneration(uint32_t address)
{
    const uint8_t *record = (const uint8_t *)address;
    return SunliteOtaReadBe32(&record[METADATA_GENERATION_OFFSET]);
}

static bool GenerationIsNewer(uint32_t candidate, uint32_t reference)
{
    return (int32_t)(candidate - reference) > 0;
}

static uint32_t CurrentRecordAddress(void)
{
    bool primary_valid = RecordIsValid(BOOTLOADER_METADATA_PAGE_ADDRESS);
    bool backup_valid = RecordIsValid(BOOTLOADER_METADATA_BACKUP_PAGE_ADDRESS);
    if (!primary_valid) {
        return backup_valid ? BOOTLOADER_METADATA_BACKUP_PAGE_ADDRESS : 0U;
    }
    if (!backup_valid) {
        return BOOTLOADER_METADATA_PAGE_ADDRESS;
    }
    return GenerationIsNewer(
               RecordGeneration(BOOTLOADER_METADATA_BACKUP_PAGE_ADDRESS),
               RecordGeneration(BOOTLOADER_METADATA_PAGE_ADDRESS)) ?
        BOOTLOADER_METADATA_BACKUP_PAGE_ADDRESS :
        BOOTLOADER_METADATA_PAGE_ADDRESS;
}

bool BootloaderMetadataRead(uint32_t *firmware_version,
                            uint32_t *image_size,
                            uint8_t image_sha256[32])
{
    uint32_t address = CurrentRecordAddress();
    if (address == 0U) {
        return false;
    }
    const uint8_t *record = (const uint8_t *)address;
    *firmware_version = SunliteOtaReadBe32(&record[METADATA_VERSION_OFFSET]);
    *image_size = SunliteOtaReadBe32(&record[METADATA_IMAGE_SIZE_OFFSET]);
    memcpy(image_sha256, &record[METADATA_SHA256_OFFSET], 32U);
    return true;
}

bool BootloaderMetadataWrite(uint32_t firmware_version,
                             uint32_t image_size,
                             const uint8_t image_sha256[32])
{
    uint8_t record[METADATA_RECORD_SIZE] = {0};
    uint32_t current_address = CurrentRecordAddress();
    uint32_t generation = (current_address == 0U) ?
        1U : RecordGeneration(current_address) + 1U;
    uint32_t destination =
        (current_address == BOOTLOADER_METADATA_PAGE_ADDRESS) ?
        BOOTLOADER_METADATA_BACKUP_PAGE_ADDRESS :
        BOOTLOADER_METADATA_PAGE_ADDRESS;

    record[0] = 'S';
    record[1] = 'U';
    record[2] = 'M';
    record[3] = 'D';
    SunliteOtaWriteBe32(&record[4], METADATA_SCHEMA);
    SunliteOtaWriteBe32(&record[METADATA_GENERATION_OFFSET], generation);
    SunliteOtaWriteBe32(&record[METADATA_VERSION_OFFSET], firmware_version);
    SunliteOtaWriteBe32(&record[METADATA_IMAGE_SIZE_OFFSET], image_size);
    memcpy(&record[METADATA_SHA256_OFFSET], image_sha256, 32U);
    uint32_t crc = BootloaderCrc32Finalize(
        BootloaderCrc32Update(BOOTLOADER_CRC32_INITIAL,
                              record,
                              METADATA_CRC_OFFSET));
    SunliteOtaWriteBe32(&record[METADATA_CRC_OFFSET], crc);

    FLASH_EraseInitTypeDef erase = {0};
    uint32_t page_error = 0U;
    erase.TypeErase = FLASH_TYPEERASE_PAGES;
    erase.PageAddress = destination;
    erase.NbPages = 1U;
    if (HAL_FLASHEx_Erase(&erase, &page_error) != HAL_OK) {
        return false;
    }
    if (!BootloaderFlashWrite(destination, record, sizeof(record))) {
        return false;
    }
    return RecordIsValid(destination);
}
