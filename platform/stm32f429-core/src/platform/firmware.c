#include <platform/firmware.h>
#include <firmware_downloader.h>

#include <stm32f429_firmware.h>
#include <stm32f4xx_hal.h>
#include <cmsis_os.h>

typedef struct {
    uint32_t preambule;
    uint32_t name_len;
    uint32_t code_len;
    char *name_ptr;
    char *code_ptr;
} firmware_metadata_t;

static FLASH_EraseInitTypeDef erase_init_struct;
static firmware_metadata_t write_metadata;

__attribute__((__section__(".firmware_code"))) firmware_metadata_t flash_metadata;

uint8_t firmware_platform_write_start(const char* firmware_name) {
    if (HAL_FLASH_Unlock() != HAL_OK) {
        return FIRMWARE_ERASE_ERR;
    }
    erase_init_struct.TypeErase = FLASH_TYPEERASE_SECTORS;
    erase_init_struct.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    erase_init_struct.Sector = STM32F429_FIRMWARE_SECTOR_NUMBER;
    erase_init_struct.NbSectors = 1;

    uint32_t sector_error = 0;

    if (HAL_FLASHEx_Erase(&erase_init_struct, &sector_error) != HAL_OK) {
        return FIRMWARE_ERASE_ERR;
    }

    // prepare metadata
    write_metadata.preambule = FIRMWARE_BINARY_PREAMBLE;
    write_metadata.code_len = 0;
    write_metadata.name_len = strlen(firmware_name);
    write_metadata.name_ptr = (char *)(STM32F429_FIRMWARE_SECTOR_ADDR + sizeof(firmware_metadata_t));
    write_metadata.code_ptr = write_metadata.name_ptr + write_metadata.name_len + 1;

    // write name
    uint32_t address = (uint32_t) write_metadata.name_ptr;
    uint32_t address_end = (uint32_t) write_metadata.name_ptr + write_metadata.name_len;
    uint8_t char_ndx = 0;
    while (address <= address_end) {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, address, (uint64_t) firmware_name[char_ndx]) != HAL_OK) {
            return FIRMWARE_WRITE_ERR;
        }
        address += 1;
        char_ndx += 1;
    }

    return FIRMWARE_SUCC;
}

uint8_t firmware_platform_write_code_chunk(const char *data, uint32_t offset, uint32_t length) {
    uint32_t curr_code_len = offset + length;
    if (curr_code_len > write_metadata.code_len) {
        write_metadata.code_len = curr_code_len;
    }

    uint32_t address = (uint32_t)(write_metadata.code_ptr) + offset;
    uint32_t address_end = address + length;

    uint32_t byte_ndx = 0;
    while (address + byte_ndx < address_end) {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, address+byte_ndx, (uint64_t)data[byte_ndx]) != HAL_OK) {
            return FIRMWARE_WRITE_ERR;
        }
        byte_ndx += 1;
    }

    return FIRMWARE_SUCC;
}

uint8_t firmware_platform_write_finish(void) {
    // write write_metadata
    uint32_t address = STM32F429_FIRMWARE_SECTOR_ADDR;
    uint32_t address_end = STM32F429_FIRMWARE_SECTOR_ADDR + sizeof(firmware_metadata_t);

    uint32_t byte_ndx = 0;
    while (address + byte_ndx < address_end) {
        uint8_t* curr_stuct_addr = (uint8_t*)(&write_metadata) + byte_ndx;
        uint64_t val = (uint64_t)(*curr_stuct_addr);
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, address+byte_ndx, val) != HAL_OK) {
            return FIRMWARE_WRITE_ERR;
        }
        byte_ndx += 1;
    }

    HAL_FLASH_Lock();

    return FIRMWARE_SUCC;
}

const char *firmware_platform_get_name() {
    return flash_metadata.preambule == FIRMWARE_BINARY_PREAMBLE ? flash_metadata.name_ptr : NULL;
}

char *firmware_platform_get_code() {
    return flash_metadata.preambule == FIRMWARE_BINARY_PREAMBLE ? flash_metadata.code_ptr : NULL;
}


static TaskHandle_t downloader_task = NULL;
static char* downloader_uri = NULL;
void _downloader_task(void* args) {
    firmware_downloader_start(downloader_uri);

    TaskHandle_t free_task = downloader_task;
    downloader_task = NULL;
    vTaskDelete(free_task);
}

void firmware_platform_downloader_task_start(const char *uri) {
    if (downloader_task) {
        return;
    }
    downloader_uri = (char *) uri;
    xTaskCreate(_downloader_task, "downloader_task", 1024, NULL, 3, &downloader_task);
}
