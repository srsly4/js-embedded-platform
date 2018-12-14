#include <platform/firmware.h>
#include <firmware_downloader.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_SIZE 1024 * 1024
#define FIRMWARE_NAME "firmware"

char code[MAX_SIZE];
int write_fd = -1;

uint8_t firmware_platform_write_start(const char* firmware_name) {
    write_fd = open(FIRMWARE_NAME, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);
    if (write_fd < 0) return FIRMWARE_WRITE_ERR;
    return FIRMWARE_SUCC;
}

uint8_t firmware_platform_write_code_chunk(const char *data, uint32_t offset, uint32_t length) {
    if (write_fd < 0) return FIRMWARE_WRITE_ERR;
    if (length != write(write_fd, data, length))
    {
        close(write_fd);
        write_fd = -1;
        return FIRMWARE_WRITE_ERR;
    }
    return FIRMWARE_SUCC;
}

uint8_t firmware_platform_write_finish(void) {
    close(write_fd);
    write_fd = -1;
    return FIRMWARE_SUCC;
}

void firmware_platform_write_cleanup(void) {
    if (write_fd < 0) return;
    close(write_fd);
    write_fd = -1;
}

const char *firmware_platform_get_name() {
    return FIRMWARE_NAME;
}

char *firmware_platform_get_code() {
    int fd = open(FIRMWARE_NAME, O_RDONLY);
    if (fd < 0) return NULL;
    memset(code, 0, MAX_SIZE);
    if (read(fd, code, MAX_SIZE) < 0)
    {
        close(fd);
        return NULL;
    }
    close(fd);
    return code;
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

uint64_t firmware_platform_max_memory()
{
    return MAX_SIZE;
}
