#ifndef JS_EMBEDDED_PLATFORM_MEMORY_H
#define JS_EMBEDDED_PLATFORM_MEMORY_H

#include <stddef.h>

void* malloc(size_t size);
void free(void *ptr);
void* realloc(void* ptr, size_t size);
char* strdup(const char *s);
size_t get_memory_free();

#endif //JS_EMBEDDED_PLATFORM_MEMORY_H
