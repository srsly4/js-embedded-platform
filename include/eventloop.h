#ifndef JS_EMBEDDED_PLATFORM_EVENTLOOP_H
#define JS_EMBEDDED_PLATFORM_EVENTLOOP_H

#include <duktape.h>
#include <module.h>


struct callback_struct_t;
typedef struct callback_struct_t callback_t;

typedef void (*callback_arg_handler_t)(duk_context *ctx, void *user_data);
typedef void (*callback_completion_handler_t)(callback_t* callback);


struct callback_struct_t {
    uint64_t _id;
    callback_arg_handler_t arg_handler;
    callback_completion_handler_t completion_handler;
    void *user_data;
};

callback_t* eventloop_callback_create_from_context(duk_context* ctx, duk_idx_t from_idx);
void eventloop_callback_call(callback_t *callback);
void eventloop_callback_destroy(callback_t *callback);

void eventloop_register_module(module_t *module);

#endif //JS_EMBEDDED_PLATFORM_EVENTLOOP_H
