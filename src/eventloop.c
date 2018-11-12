#include <platform/memory.h>
#include <platform/eventloop.h>
#include <eventloop.h>
#include <module.h>
#include <platform/firmware.h>
#include "eventloop.h"
#include "common.h"
#include "duktape.h"
#include "platform.h"

static duk_ret_t native_sleep(duk_context *ctx) {
    duk_int32_t ms = duk_to_int32(ctx, 0);
    platform_sleep((uint32_t)ms);
    return 0;
}

static duk_ret_t native_led_on(duk_context *ctx) {
    platform_debug_led_on();
    return 0;
}

static duk_ret_t native_led_off(duk_context *ctx) {
    platform_debug_led_off();
    return 0;
}

static void* _duk_alloc(void *udata, size_t size) {
    return malloc(size);
}

static void _duk_free(void *udata, void *ptr) {
    return free(ptr);
}

static void* _duk_realloc(void *udata, void *ptr, size_t size) {
    return realloc(ptr, size);
}

static void duk_fatal_handler(void *udata, const char *msg) {
    platform_debug_led_off();
}

static duk_context *ctx = NULL;
static char *code_ptr = NULL;
static const char test_code[] = "(function(){"
        "var timeTest = 100;"
        "while(true){"
        "ledOn();"
        "sleep(timeTest);"
        "ledOff();"
        "sleep(timeTest);"
        "timeTest += 100;"
        "if (timeTest > 1000) { timeTest = 100; }"
        "}"
        "})()";

// fixme: this is only a mock of event loop
void eventloop() {
    // load firmware from persistent storage
    code_ptr = firmware_platform_get_code();

    // if there is no code, load default's
    if (code_ptr == NULL) {
        code_ptr = (char *) test_code;
    }

    clear_eventloop();

    ctx = duk_create_heap(
            _duk_alloc,
            _duk_realloc,
            _duk_free,
            NULL,
            duk_fatal_handler
    );

    // create global callbacks handler
    duk_push_global_object(ctx);
    duk_push_object(ctx);


    duk_push_c_function(ctx, native_sleep, 1 /*nargs*/);
    duk_put_global_string(ctx, "sleep");

    duk_push_c_function(ctx, native_led_on, 0);
    duk_put_global_string(ctx, "ledOn");

    duk_push_c_function(ctx, native_led_off, 0);
    duk_put_global_string(ctx, "ledOff");

    duk_eval_string(ctx, code_ptr);
}

void set_user_code(char* code) {
    code_ptr = code;
}

char* get_user_code() {
    return code_ptr;
}

void clear_eventloop() {
    if (ctx != NULL) {
        duk_destroy_heap(ctx);
        ctx = NULL;
    }
}