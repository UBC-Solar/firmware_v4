#include "bootloader.h"
#include "fw_update_protocol.h"
#include "stm32f1xx_hal.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

_Alignas(uint16_t) static uint8_t emulated_flash[BOOTLOADER_FLASH_SIZE_BYTES];
void* TestStoragePointer(uint32_t address)
{
    assert(address >= BOOTLOADER_FLASH_START_ADDRESS);
    assert(address < BOOTLOADER_FLASH_END_ADDRESS);
    return &emulated_flash[address - BOOTLOADER_FLASH_START_ADDRESS];
}

static bool unlocked;
static int writes_until_failure = -1;
static bool fail_erase;
static bool corrupt_program;
static bool corrupt_reset_high;
static uint32_t watchdog_feeds;
static uint32_t program_calls;
static unsigned jumps;

void BootloaderServiceWatchdog(void) { watchdog_feeds++; }
HAL_StatusTypeDef HAL_FLASH_Unlock(void)
{
    unlocked = true;
    return HAL_OK;
}
HAL_StatusTypeDef HAL_FLASH_Lock(void)
{
    unlocked = false;
    return HAL_OK;
}
HAL_StatusTypeDef HAL_FLASHEx_Erase(FLASH_EraseInitTypeDef* erase, uint32_t* error)
{
    (void)error;
    if (!unlocked || fail_erase)
        return HAL_ERROR;
    assert(erase->PageAddress >= BOOTLOADER_APP_START_ADDRESS);
    assert(erase->PageAddress + erase->NbPages * BOOTLOADER_FLASH_PAGE_SIZE_BYTES <=
           BOOTLOADER_APP_START_ADDRESS + BOOTLOADER_APP_MAX_SIZE_BYTES);
    memset(TestStoragePointer(erase->PageAddress),
           0xff,
           erase->NbPages * BOOTLOADER_FLASH_PAGE_SIZE_BYTES);
    return HAL_OK;
}
HAL_StatusTypeDef HAL_FLASH_Program(uint32_t type, uint32_t address, uint64_t data)
{
    program_calls++;
    assert(type == FLASH_TYPEPROGRAM_HALFWORD);
    if (!unlocked || writes_until_failure == 0)
        return HAL_ERROR;
    if (writes_until_failure > 0)
        writes_until_failure--;
    uint16_t* destination = (uint16_t*)TestStoragePointer(address);
    if ((*destination & (uint16_t)data) != (uint16_t)data)
        return HAL_ERROR;
    *destination = (uint16_t)data;
    if (corrupt_program)
        *destination ^= 1U;
    if (corrupt_reset_high && address == BOOTLOADER_APP_START_ADDRESS + 6U)
        *destination ^= 1U;
    return HAL_OK;
}

/**
 * @brief Checks launch vectors directly in emulated flash, as startup does on hardware.
 */
bool BootloaderAppIsValid(void)
{
    uint32_t vectors[2];
    memcpy(vectors, TestStoragePointer(BOOTLOADER_APP_START_ADDRESS), sizeof(vectors));
    uint32_t reset = vectors[1] & ~1U;
    return vectors[0] >= BOOTLOADER_SRAM_START_ADDRESS &&
           vectors[0] <= BOOTLOADER_SRAM_END_ADDRESS && (vectors[0] & 7U) == 0U &&
           (vectors[1] & 1U) != 0U && reset >= BOOTLOADER_APP_START_ADDRESS &&
           reset < BOOTLOADER_APP_START_ADDRESS + BOOTLOADER_APP_MAX_SIZE_BYTES;
}

void BootloaderJumpToApp(void)
{
    assert(BootloaderAppIsValid());
    jumps++;
}

void HAL_Delay(uint32_t delay_ms) { (void)delay_ms; }

/**
 * @brief Checks the real engine's encoded status and next-byte offset.
 */
static bool CheckResponse(const uint8_t* frame, size_t length, void* context)
{
    uint32_t* expected = context;
    expected[2]++;
    FirmwareUpdateMessage reply;
    assert(FirmwareUpdateFrameDecode(frame, length, &reply));
    assert(reply.type ==
           (expected[0] == FW_UPDATE_STATUS_OK ? FW_UPDATE_MESSAGE_ACK : FW_UPDATE_MESSAGE_NACK));
    assert(reply.payload_length == FW_UPDATE_ACK_SIZE);
    assert(reply.payload[1] == expected[0]);
    assert(FirmwareUpdateReadBe32(&reply.payload[2]) == expected[1]);
    return true;
}

/**
 * @brief Sends a request through the production codec, engine, and flash implementation.
 */
static void Request(FirmwareUpdateMessageType type,
                    const uint8_t* payload,
                    uint16_t length,
                    FirmwareUpdateStatus status,
                    uint32_t offset)
{
    static uint32_t sequence;
    uint8_t frame[FW_UPDATE_MAX_FRAME_SIZE];
    size_t frame_length = FirmwareUpdateFrameEncode(
        type, 0x12345678U, ++sequence, payload, length, frame, sizeof(frame));
    assert(frame_length > 0U);
    uint32_t expected[] = {status, offset, 0};
    (void)FirmwareUpdateBootloaderProcessFrame(frame, frame_length, CheckResponse, expected);
    assert(expected[2] == 1U);
}

/**
 * @brief Programs a complete image and optionally fails a vector halfword during commit.
 */
static void TestImage(const uint8_t* image, size_t image_size, int vector_cut)
{
    uint8_t begin[FW_UPDATE_BEGIN_UPDATE_SIZE];
    FirmwareUpdateWriteBe32(&begin[0], 0x54454C45U);
    FirmwareUpdateWriteBe32(&begin[4], 7U);
    FirmwareUpdateWriteBe32(&begin[8], (uint32_t)image_size);
    FirmwareUpdateWriteBe32(&begin[12], FirmwareUpdateCrc32(image, image_size));
    writes_until_failure = -1;
    memset(emulated_flash, 0x5a, sizeof(emulated_flash));
    Request(FW_UPDATE_MESSAGE_BEGIN_UPDATE, begin, sizeof(begin), FW_UPDATE_STATUS_OK, 0U);
    for (uint32_t offset = 0; offset < image_size;)
    {
        size_t chunk = image_size - offset;
        if (chunk > FW_UPDATE_MAX_CHUNK_SIZE)
            chunk = FW_UPDATE_MAX_CHUNK_SIZE;
        uint8_t data[FW_UPDATE_DATA_OFFSET_SIZE + FW_UPDATE_MAX_CHUNK_SIZE];
        FirmwareUpdateWriteBe32(data, offset);
        memcpy(&data[FW_UPDATE_DATA_OFFSET_SIZE], &image[offset], chunk);
        Request(FW_UPDATE_MESSAGE_DATA,
                data,
                (uint16_t)(chunk + 4U),
                FW_UPDATE_STATUS_OK,
                offset + (uint32_t)chunk);
        assert(!BootloaderAppIsValid());
        uint64_t withheld_vectors;
        memcpy(&withheld_vectors,
               TestStoragePointer(BOOTLOADER_APP_START_ADDRESS),
               sizeof(withheld_vectors));
        assert(withheld_vectors == UINT64_MAX);
        offset += (uint32_t)chunk;
    }
    uint32_t writes_before_end = program_calls;
    bool success = vector_cut == -1;
    writes_until_failure = vector_cut >= 0 ? vector_cut : -1;
    corrupt_reset_high = vector_cut == -2;
    Request(FW_UPDATE_MESSAGE_END_UPDATE,
            NULL,
            0,
            success ? FW_UPDATE_STATUS_OK : FW_UPDATE_STATUS_FLASH_ERROR,
            (uint32_t)image_size);
    assert(!unlocked);
    assert(BootloaderAppIsValid() == success);
    if (success)
    {
        assert(program_calls == writes_before_end + 4U);
        assert(memcmp(TestStoragePointer(BOOTLOADER_APP_START_ADDRESS), image, image_size) == 0);
        unsigned old_jumps = jumps;
        Request(FW_UPDATE_MESSAGE_REBOOT, NULL, 0, FW_UPDATE_STATUS_OK, (uint32_t)image_size);
        assert(jumps == old_jumps + 1U);
    }
    for (size_t i = 0; i < BOOTLOADER_SIZE_BYTES; i++)
        assert(emulated_flash[i] == 0x5a);
    for (uint32_t address = BOOTLOADER_APP_START_ADDRESS + BOOTLOADER_APP_MAX_SIZE_BYTES;
         address < BOOTLOADER_FLASH_END_ADDRESS;
         address++)
        assert(*(uint8_t*)TestStoragePointer(address) == 0x5a);
    writes_until_failure = -1;
    corrupt_reset_high = false;
}

int main(int argc, char** argv)
{
    memset(emulated_flash, 0xff, sizeof(emulated_flash));
    const uint32_t app_end = BOOTLOADER_APP_START_ADDRESS + BOOTLOADER_APP_MAX_SIZE_BYTES;
    assert(!BootloaderFlashBeginAppUpdate(0));
    assert(!BootloaderFlashBeginAppUpdate(BOOTLOADER_APP_MAX_SIZE_BYTES + 1));
    assert(BootloaderFlashBeginAppUpdate(3));
    const uint8_t image[] = {0x12, 0x34, 0x56};
    assert(!BootloaderFlashWrite(BOOTLOADER_FLASH_START_ADDRESS, image, 3));
    assert(!BootloaderFlashWrite(BOOTLOADER_APP_START_ADDRESS + 1, image, 3));
    assert(!BootloaderFlashWrite(BOOTLOADER_FLASH_END_ADDRESS - 2, image, 3));
    assert(!BootloaderFlashWrite(app_end, image, 1));
    assert(!BootloaderFlashWrite(app_end - 2, image, 3));
    assert(!BootloaderFlashWrite(BOOTLOADER_APP_START_ADDRESS, image, SIZE_MAX));
    assert(BootloaderFlashWrite(BOOTLOADER_APP_START_ADDRESS, image, 3));
    assert(memcmp(TestStoragePointer(BOOTLOADER_APP_START_ADDRESS), image, 3) == 0);
    assert(*(uint8_t*)TestStoragePointer(BOOTLOADER_APP_START_ADDRESS + 3) == 0xff);
    assert(BootloaderFlashWrite(app_end - 2, image, 1));
    assert(*(uint8_t*)TestStoragePointer(app_end - 1) == 0xff);
    BootloaderFlashEndAppUpdate();

    /* Failed application writes must be reported so the engine cannot commit vectors. */
    for (int cut = 0; cut < 2; cut++)
    {
        assert(BootloaderFlashBeginAppUpdate(sizeof(image)));
        writes_until_failure = cut;
        assert(!BootloaderFlashWrite(BOOTLOADER_APP_START_ADDRESS + 8, image, sizeof(image)));
        for (unsigned i = 0; i < 8; i++)
            assert(*(uint8_t*)TestStoragePointer(BOOTLOADER_APP_START_ADDRESS + i) == 0xff);
        BootloaderFlashEndAppUpdate();
        assert(!unlocked);
    }
    writes_until_failure = -1;
    fail_erase = true;
    assert(!BootloaderFlashBeginAppUpdate(sizeof(image)));
    assert(!unlocked);
    fail_erase = false;

    assert(BootloaderFlashBeginAppUpdate(sizeof(image)));
    corrupt_program = true;
    assert(!BootloaderFlashWrite(BOOTLOADER_APP_START_ADDRESS + 8, image, sizeof(image)));
    corrupt_program = false;
    BootloaderFlashEndAppUpdate();

    /* Erasing a full slot must preserve both bootloader and unused final pages. */
    memset(emulated_flash, 0x5a, sizeof(emulated_flash));
    watchdog_feeds = 0;
    assert(BootloaderFlashBeginAppUpdate(BOOTLOADER_APP_MAX_SIZE_BYTES));
    assert(watchdog_feeds == BOOTLOADER_APP_MAX_SIZE_BYTES / BOOTLOADER_FLASH_PAGE_SIZE_BYTES);
    for (size_t i = 0; i < sizeof(emulated_flash); i++)
    {
        uint32_t address = BOOTLOADER_FLASH_START_ADDRESS + (uint32_t)i;
        assert(emulated_flash[i] ==
               ((address >= BOOTLOADER_APP_START_ADDRESS && address < app_end) ? 0xff : 0x5a));
    }
    BootloaderFlashEndAppUpdate();
    assert(!unlocked && watchdog_feeds > 0);
    assert(!BootloaderFlashWrite(BOOTLOADER_APP_START_ADDRESS, image, 3));
    puts("Bootloader flash bounds, readback, and write-failure tests passed");

    uint8_t* full_image = malloc(BOOTLOADER_APP_MAX_SIZE_BYTES);
    assert(full_image != NULL);
    for (size_t i = 0; i < BOOTLOADER_APP_MAX_SIZE_BYTES; i++)
        full_image[i] = (uint8_t)i;
    const uint32_t vectors[] = {BOOTLOADER_SRAM_END_ADDRESS, BOOTLOADER_APP_START_ADDRESS + 9U};
    memcpy(full_image, vectors, sizeof(vectors));
    TestImage(full_image, BOOTLOADER_APP_MAX_SIZE_BYTES, -1);
    TestImage(full_image, FW_UPDATE_MAX_CHUNK_SIZE + 1U, -1);
    for (int cut = 0; cut < 4; cut++)
        TestImage(full_image, FW_UPDATE_MAX_CHUNK_SIZE + 1U, cut);
    TestImage(full_image, FW_UPDATE_MAX_CHUNK_SIZE + 1U, -2);

    /* Optional binary arguments exercise actual built Release application images. */
    for (int i = 1; i < argc; i++)
    {
        FILE* binary = fopen(argv[i], "rb");
        assert(binary != NULL);
        size_t image_size = fread(full_image, 1, BOOTLOADER_APP_MAX_SIZE_BYTES, binary);
        assert(image_size >= sizeof(vectors) && !ferror(binary) && fgetc(binary) == EOF);
        fclose(binary);
        TestImage(full_image, image_size, -1);
        printf("Verified application image: %s (%zu bytes)\n", argv[i], image_size);
    }
    free(full_image);
    puts("Combined engine/flash full-slot and interrupted-vector tests passed");
    return 0;
}
