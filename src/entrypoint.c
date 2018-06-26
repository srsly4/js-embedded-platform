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

void entrypoint(void){

    duk_context *ctx = duk_create_heap_default();
    duk_push_c_function(ctx, native_sleep, 1 /*nargs*/);
    duk_put_global_string(ctx, "sleep");

    duk_push_c_function(ctx, native_led_on, 0);
    duk_put_global_string(ctx, "ledOn");

    duk_push_c_function(ctx, native_led_off, 0);
    duk_put_global_string(ctx, "ledOff");

    duk_eval_string(ctx, "(function(){"
            "var timeTest = 100;"
            "while(true){"
            "ledOn();"
            "sleep(timeTest);"
            "ledOff();"
            "sleep(timeTest);"
            "}"
            "})()");

    duk_destroy_heap(ctx);

}