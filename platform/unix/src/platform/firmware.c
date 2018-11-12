#include <platform/firmware.h>
#include <firmware_downloader.h>
#include <pthread.h>

typedef struct {
    uint32_t preambule;
    uint32_t name_len;
    uint32_t code_len;
    char *name_ptr;
    char *code_ptr;
} firmware_metadata_t;

static firmware_metadata_t write_metadata;

__attribute__((__section__(".firmware_code"))) firmware_metadata_t flash_metadata;

uint8_t firmware_platform_write_start(const char* firmware_name) {
    return FIRMWARE_SUCC;
}

uint8_t firmware_platform_write_code_chunk(const char *data, uint32_t offset, uint32_t length) {
    return FIRMWARE_SUCC;
}

uint8_t firmware_platform_write_finish(void) {
    return FIRMWARE_SUCC;
}

const char *firmware_platform_get_name() {
    return flash_metadata.preambule == FIRMWARE_BINARY_PREAMBLE ? flash_metadata.name_ptr : NULL;
}

char *firmware_platform_get_code() {
    return flash_metadata.preambule == FIRMWARE_BINARY_PREAMBLE ? flash_metadata.code_ptr : NULL;
}


static pthread_t downloader_task = 0;
static char* downloader_uri = NULL;
void* _downloader_task(void* args) {
    firmware_downloader_start(downloader_uri);

    downloader_task = 0;
}

void firmware_platform_downloader_task_start(const char *uri) {
    if (downloader_task) {
        return;
    }
    downloader_uri = (char *) uri;
    pthread_create(&downloader_task, NULL, _downloader_task, NULL);
}
