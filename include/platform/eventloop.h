#ifndef JS_EMBEDDED_PLATFORM_PLATFORM_EVENTLOOP_H
#define JS_EMBEDDED_PLATFORM_PLATFORM_EVENTLOOP_H

#include <eventloop.h>
#include <module.h>

#define EVENTLOOP_PLATFORM_QUEUE_LENGTH 16

void start_eventloop_thread();
void kill_eventloop_thread();

callback_t* eventloop_platform_queue_receive();
void eventloop_platform_queue_push(callback_t *callback);
void eventloop_platform_queue_push_isr(callback_t *callback);

module_ret_t eventloop_platform_timer_start(callback_t *callback, long timeout, duk_bool_t repeat);
module_ret_t eventloop_platform_timer_stop(callback_t *callback);
void eventloop_platform_timers_cleanup();

#endif //JS_EMBEDDED_PLATFORM_EVENTLOOP_H
