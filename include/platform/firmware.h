#ifndef JS_EMBEDDED_PLATFORM_FIRMWARE_H
#define JS_EMBEDDED_PLATFORM_FIRMWARE_H

#include <module.h>

#define FIRMWARE_BINARY_PREAMBLE 0xAAAAAAAA

#define FIRMWARE_SUCC 0
#define FIRMWARE_ERASE_ERR 1
#define FIRMWARE_WRITE_ERR 2

uint8_t firmware_platform_write_start(const char* firmware_name);
uint8_t firmware_platform_write_code_chunk(const char* data, uint32_t offset, uint32_t length);
uint8_t firmware_platform_write_finish(void);
uint64_t firmware_platform_max_memory(void);

const char* firmware_platform_get_name();
char * firmware_platform_get_code();

void firmware_platform_downloader_task_start(const char* uri);

#endif //JS_EMBEDDED_PLATFORM_FIRMWARE_H
