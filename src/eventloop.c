#include <platform/memory.h>
#include <platform/eventloop.h>
#include <platform/debug.h>
#include <eventloop.h>
#include "eventloop.h"
#include "common.h"
#include "duktape.h"
#include "platform.h"

#define EVENTLOOP_CALLBACKS_OBJECT_NAME "_callbacks"

static uint64_t _callback_sequence_id = 0;


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
    debug_platform_notify_duk_error(udata, msg);
}

static duk_context *ctx = NULL;
static char *code_ptr = NULL;
static const char test_code[] = "(function(){"
                                "ledOff();var sw = false;"
                                "setInterval(function(){"
                                "var inter = setInterval(function(){if (!sw) { ledOn(); } else { ledOff(); } sw = !sw;}, 50);"
                                "setTimeout(function(){clearInterval(inter); ledOff(); sw=false;}, 1000)"
                                "}, 1500);"
                                "})()";

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

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
void eventloop() {
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

    duk_push_global_stash(ctx);
    duk_push_object(ctx);
    duk_put_prop_string(ctx, -2, EVENTLOOP_CALLBACKS_OBJECT_NAME);
    duk_pop(ctx);

    duk_push_global_object(ctx);
    duk_push_object(ctx);


    duk_push_c_function(ctx, native_sleep, 1);
    duk_put_global_string(ctx, "sleep");

    duk_push_c_function(ctx, native_led_on, 0);
    duk_put_global_string(ctx, "ledOn");

    duk_push_c_function(ctx, native_led_off, 0);
    duk_put_global_string(ctx, "ledOff");

    duk_push_c_function(ctx, eventloop_native_set_timeout, 2);
    duk_put_global_string(ctx, "setTimeout");

    duk_push_c_function(ctx, eventloop_native_clear_timer, 1);
    duk_put_global_string(ctx, "clearTimeout");

    duk_push_c_function(ctx, eventloop_native_set_interval, 2);
    duk_put_global_string(ctx, "setInterval");

    duk_push_c_function(ctx, eventloop_native_clear_timer, 1);
    duk_put_global_string(ctx, "clearInterval");

    duk_eval_string(ctx, code_ptr);
    duk_pop(ctx);

    duk_push_global_stash(ctx);
    duk_get_prop_string(ctx, -1, EVENTLOOP_CALLBACKS_OBJECT_NAME);
    while (1) {
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
    if (ctx != NULL) {
        duk_destroy_heap(ctx);
        ctx = NULL;
        _callback_sequence_id = 0;
    }
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

    free(callback);
    duk_gc(ctx, 0);
}

void eventloop_callback_call(callback_t *callback) {
    eventloop_platform_queue_push(callback);
}