#ifndef JS_EMBEDDED_PLATFORM_MODULE_H
#define JS_EMBEDDED_PLATFORM_MODULE_H

#include <stdint.h>
#include <duktape.h>

typedef uint8_t module_ret_t;

#define ERR_MODULE_SUCC 0
#define ERR_MODULE_MEM 1
#define ERR_MODULE_TIMEOUT 2


typedef module_ret_t (*module_init_func_t)(duk_context *ctx);
typedef void (*module_cleanup_func_t)(duk_context *ctx);
typedef void (*module_eventloop_func_t)();

typedef struct {
    const char* keyword;
    module_init_func_t init_func;
    module_eventloop_func_t eventloop_func;
    module_cleanup_func_t cleanup_func;
} module_t;

#endif //JS_EMBEDDED_PLATFORM_MODULE_H
