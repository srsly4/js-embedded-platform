#include <duktape.h>
#include <eventloop.h>
#include <module.h>
#include <module/gpio.h>

#include "module/gpio.h"

#define GPIO_MODULE_STASH_NAME "_gpio"

static duk_ret_t gpio_setup(duk_context *ctx){
    uint32_t port = duk_require_uint(ctx, 0);
    uint32_t pin = duk_require_uint(ctx, 1);
    uint32_t direction = duk_require_uint(ctx, 2);
    uint32_t pullup_type = duk_require_uint(ctx, 3);

    module_gpio_platform_setup(port, pin, direction, pullup_type);

    duk_pop_n(ctx, 4);
    return 0;
}

static duk_ret_t gpio_set(duk_context *ctx){
    uint32_t port = duk_require_uint(ctx, 0);
    uint32_t pin = duk_require_uint(ctx, 1);
    duk_bool_t value = duk_require_boolean(ctx, 2);

    module_gpio_platform_set(port, pin, value);

    duk_pop_n(ctx, 3);
    return 0;
}

static duk_ret_t gpio_get(duk_context *ctx){
    uint32_t port = duk_require_uint(ctx, 0);
    uint32_t pin = duk_require_uint(ctx, 1);

    uint8_t ret = module_gpio_platform_get(port, pin);

    duk_pop_n(ctx, 2);
    duk_push_boolean(ctx, ret);
    return 1;
}

static const duk_function_list_entry gpio_module_funcs[] = {
        { "setup", gpio_setup, 4},
        { "set", gpio_set, 3},
        { "get", gpio_get, 2},
        { NULL, NULL, 0 }
};

module_ret_t module_gpio_init(duk_context *ctx) {
    void *module_ptr = NULL;
    duk_push_global_stash(ctx);
    if (duk_get_prop_string(ctx, -1, GPIO_MODULE_STASH_NAME) == 1) {
        module_ptr = duk_get_heapptr(ctx, -1);
        duk_pop(ctx);
    } else {
        duk_pop(ctx); // we don't want undefined on stack
        duk_push_object(ctx);
        duk_put_function_list(ctx, -1, gpio_module_funcs);
        duk_put_number_list(ctx, -1, module_gpio_platform_get_const_list());

        module_ptr = duk_get_heapptr(ctx, -1);
        duk_put_prop_string(ctx, -2, GPIO_MODULE_STASH_NAME);
    }
    duk_pop(ctx); // pop global stack
    duk_push_heapptr(ctx, module_ptr); // will be returned module
    return ERR_MODULE_SUCC;
}

static const module_t gpio_module = {
        "gpio",
        &module_gpio_init,
        NULL
};

const module_t* module_gpio_get() {
    return &gpio_module;
}