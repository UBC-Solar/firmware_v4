#ifndef TEST_STORAGE_HAL_H
#define TEST_STORAGE_HAL_H
#include <stdint.h>
typedef enum { HAL_OK, HAL_ERROR } HAL_StatusTypeDef;
typedef struct {
    uint32_t TypeErase;
    uint32_t PageAddress;
    uint32_t NbPages;
} FLASH_EraseInitTypeDef;
#define FLASH_TYPEERASE_PAGES 1U
#define FLASH_TYPEPROGRAM_HALFWORD 1U
HAL_StatusTypeDef HAL_FLASH_Unlock(void);
HAL_StatusTypeDef HAL_FLASH_Lock(void);
HAL_StatusTypeDef HAL_FLASHEx_Erase(FLASH_EraseInitTypeDef *erase, uint32_t *error);
HAL_StatusTypeDef HAL_FLASH_Program(uint32_t type, uint32_t address, uint64_t data);
void HAL_Delay(uint32_t delay_ms);
void *TestStoragePointer(uint32_t address);
#define BOOTLOADER_STORAGE_POINTER(address) TestStoragePointer(address)
#endif
