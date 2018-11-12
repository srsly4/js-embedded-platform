#include <platform/memory.h>
#include <cmsis_os.h>
#include <string.h>

void *malloc(size_t size) {
    return pvPortMalloc(size);
}

void free(void *ptr) {
    return vPortFree(ptr);
}

void *realloc(void *ptr, size_t size) {
    // heap_4 does not provide realloc implementation
    if (size == 0) {
        free(ptr);
        return NULL;
    }

    void *new_ptr = malloc(size);
    if (new_ptr && ptr) {
        memcpy(new_ptr, ptr, size); // fixme: it should be previous' segment size
        free(ptr);
    }
    return new_ptr;
}

size_t get_memory_free() {
    return xPortGetFreeHeapSize();
}