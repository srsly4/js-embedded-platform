#include <platform/memory.h>
#include <platform/eventloop.h>
#include <platform/debug.h>
#include <module.h>
#include <platform/firmware.h>
#include "eventloop.h"
#include "common.h"
#include "duktape.h"
#include "platform.h"

#define EVENTLOOP_CALLBACKS_OBJECT_NAME "_callbacks"

struct module_entry_struct_t;
typedef struct module_entry_struct_t module_entry_t;

struct module_entry_struct_t {
    module_t *module;
    uint8_t is_loaded;
    module_entry_t *next;
};

static uint64_t _callback_sequence_id = 0;
static callback_t *callback_list_first = NULL;

static module_entry_t *registered_modules = NULL;
static module_entry_t *loaded_modules = NULL;


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
    debug_platform_notify_duk_error(udata, msg);
}

static duk_context *ctx = NULL;
char *code_ptr = NULL;
static const char test_code[] = "var gpio = require('gpio');var sw = false;"
                                "gpio.setup(gpio.PORTB, gpio.PIN0, gpio.MODE_OUT_PP, gpio.NOPULL);"
                                "gpio.setup(gpio.PORTB, gpio.PIN7, gpio.MODE_OUT_PP, gpio.NOPULL);"
                                "gpio.setup(gpio.PORTC, gpio.PIN13, gpio.MODE_IN, gpio.PULLDOWN);"
                                "setInterval(function(){ gpio.set(gpio.PORTB, gpio.PIN0, sw); sw = !sw; }, 250);"
                                "setInterval(function(){ var isSet = gpio.get(gpio.PORTC, gpio.PIN13); gpio.set(gpio.PORTB, gpio.PIN7, isSet); }, 150);";

static duk_ret_t eventloop_native_set_timeout(duk_context *ctx) {
    uint64_t timeout;
    timeout = duk_require_uint(ctx, 1);

    callback_t *callback = eventloop_callback_create_from_context(ctx, 0);
    if (eventloop_platform_timer_start(callback, (long) timeout, 0) != ERR_MODULE_SUCC) {
        duk_error(ctx, DUK_ERR_EVAL_ERROR, "Can't start timer");
        return 0;
    }

    duk_push_pointer(ctx, callback); // return handler to the setTimeout object
    return 1;
}

static duk_ret_t eventloop_native_clear_timer(duk_context *ctx) {
    callback_t *callback = duk_require_pointer(ctx, 0);
    if (callback) {
        eventloop_platform_timer_stop(callback);
    }
    return 0;
}

static duk_ret_t eventloop_native_set_interval(duk_context *ctx) {
    uint64_t timeout;
    timeout = duk_require_uint(ctx, 1);

    callback_t *callback = eventloop_callback_create_from_context(ctx, 0);
    if (eventloop_platform_timer_start(callback, (long) timeout, 1) != ERR_MODULE_SUCC) {
        duk_error(ctx, DUK_ERR_EVAL_ERROR, "Can't start timer");
        return 0;
    }

    duk_push_pointer(ctx, callback);
    return 1;
}

static duk_ret_t eventloop_native_load_module(duk_context *ctx) {
    const char* loaded_name = duk_require_string(ctx, 0);

    module_entry_t *ptr = registered_modules;
    while (ptr != NULL) {
        if (strcmp(ptr->module->keyword, loaded_name) == 0) {
            break;
        }
        ptr = ptr->next;
    }

    if (ptr == NULL) {
        duk_error(ctx, DUK_ERR_ERROR, "Module not found");
        return 0;
    }

    if (ptr->module->init_func(ctx) != ERR_MODULE_SUCC) {
        duk_error(ctx, DUK_ERR_ERROR, "Module initialization failed");
        return 0;
    }

    if (!ptr->is_loaded)  {
        module_entry_t *loaded_module = malloc(sizeof(module_entry_t));
        loaded_module->is_loaded = 1;
        loaded_module->module = ptr->module;
        loaded_module->next = loaded_modules;
        loaded_modules = loaded_module;
        ptr->is_loaded = 1;
    }

    return 1; // on stack should be only returned module
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
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

    // put the callbacks object onto stash
    duk_push_global_stash(ctx);
    duk_push_object(ctx);
    duk_put_prop_string(ctx, -2, EVENTLOOP_CALLBACKS_OBJECT_NAME);
    duk_pop(ctx);

    duk_push_global_object(ctx);
    duk_push_object(ctx);

    // fixme: replace test functions with module
    duk_push_c_function(ctx, native_led_on, 0);
    duk_put_global_string(ctx, "ledOn");

    duk_push_c_function(ctx, native_led_off, 0);
    duk_put_global_string(ctx, "ledOff");

    // native timer async functions
    duk_push_c_function(ctx, eventloop_native_set_timeout, 2);
    duk_put_global_string(ctx, "setTimeout");

    duk_push_c_function(ctx, eventloop_native_clear_timer, 1);
    duk_put_global_string(ctx, "clearTimeout");

    duk_push_c_function(ctx, eventloop_native_set_interval, 2);
    duk_put_global_string(ctx, "setInterval");

    duk_push_c_function(ctx, eventloop_native_clear_timer, 1);
    duk_put_global_string(ctx, "clearInterval");

    // native module loader function
    duk_push_c_function(ctx, eventloop_native_load_module, 1);
    duk_put_global_string(ctx, "require");

    platform_register_modules();

    // compile and execute top-most level function
    duk_eval_string(ctx, code_ptr);
    duk_pop(ctx); // ignore result

    duk_push_global_stash(ctx);
    duk_get_prop_string(ctx, -1, EVENTLOOP_CALLBACKS_OBJECT_NAME);
    while (1) {
        // call module-provided non-blocking procedures
        module_entry_t *entry = loaded_modules;
        while (entry) {
            module_eventloop_func_t func = entry->module->eventloop_func;
            if (func) {
                func();
            }
            entry = entry->next;
        }

        // receive and execute first callback in queue
        callback_t *callback = eventloop_platform_queue_receive();
        duk_push_number(ctx, callback->_id);
        if (duk_get_prop(ctx, -2)) {
            duk_require_function(ctx, -1);
            duk_idx_t func_idx = duk_get_top(ctx);
            if (callback->arg_handler) {
                callback->arg_handler(ctx, callback->user_data);
            }
            duk_idx_t func_idx_args = duk_get_top(ctx);
            duk_int_t ret = duk_pcall(ctx, func_idx_args - func_idx);
        }
        duk_pop(ctx); // function return or undefined

        // call completion handler of executed callback
        if (callback->completion_handler != NULL) {
            callback->completion_handler(callback);
        }
    }
}
#pragma clang diagnostic pop

void set_user_code(char* code) {
    code_ptr = code;
}

char* get_user_code() {
    return code_ptr;
}

void clear_eventloop() {
    eventloop_platform_timers_cleanup();
    duk_gc(ctx, 0);

    // destroy all callbacks
    callback_t *c_ptr, *c_next;
    c_ptr = callback_list_first;

    while (c_ptr) {
        c_next = c_ptr->_next;
        free(c_ptr);
        c_ptr = c_next;
    }
    callback_list_first = NULL;

    if (ctx != NULL) {
        duk_destroy_heap(ctx);
        ctx = NULL;
        _callback_sequence_id = 0;
    }

    module_entry_t *ptr, *old_ptr;

    ptr = loaded_modules;
    while (ptr) {
        old_ptr = ptr;
        if (ptr->module->cleanup_func) {
            ptr->module->cleanup_func(ctx);
        }
        ptr = ptr->next;
        free(old_ptr);
    }

    ptr = registered_modules;
    while (ptr) {
        old_ptr = ptr;
        ptr = ptr->next;
        free(old_ptr);
    }

    loaded_modules = NULL;
    registered_modules = NULL;
}

callback_t *eventloop_callback_create_from_context(duk_context *ctx, duk_idx_t from_idx) {
    duk_require_function(ctx, from_idx);

    callback_t* callback = malloc(sizeof(callback_t));
    if (callback <= 0) {
        return NULL;
    }

    callback->_id = _callback_sequence_id++;
    callback->arg_handler = NULL;
    callback->user_data = NULL;
    callback->completion_handler = NULL;
    callback->_prev = NULL;
    callback->_next = callback_list_first;
    if (callback_list_first) {
        callback_list_first->_prev = callback;
    }
    callback_list_first = callback;

    duk_push_global_stash(ctx);
    duk_get_prop_string(ctx, -1, EVENTLOOP_CALLBACKS_OBJECT_NAME);
    duk_push_number(ctx, (double) callback->_id); // fixme: change to heapptr-based
    duk_dup(ctx, from_idx);
    duk_put_prop(ctx, -3);
    duk_pop(ctx); // pop callbacks object
    duk_pop(ctx); // pop global stash

    return callback;
}

void eventloop_callback_destroy(callback_t *callback) {
    duk_push_global_stash(ctx);
    duk_get_prop_string(ctx, -1, EVENTLOOP_CALLBACKS_OBJECT_NAME);
    duk_push_number(ctx, callback->_id);
    duk_del_prop(ctx, -2);
    duk_pop_n(ctx, 2);

    if (callback->_prev) {
        callback->_prev->_next = callback->_next;
    } else {
        callback_list_first = callback;
    }
    if (callback->_next) {
        callback->_next->_prev = callback->_prev;
    }
    free(callback);

    // force garbage collection
    duk_gc(ctx, 0);
}

void eventloop_callback_call(callback_t *callback) {
    eventloop_platform_queue_push(callback);
}

void eventloop_register_module(module_t *module) {
    module_entry_t *entry = malloc(sizeof(module_entry_t));
    entry->module = module;
    entry->next = registered_modules;
    entry->is_loaded = 0;
    registered_modules = entry;
}