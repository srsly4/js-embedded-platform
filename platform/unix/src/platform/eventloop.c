#include <platform/eventloop.h>
#include <common.h>

#include <unistd.h>
#include <pthread.h>
#include <platform.h>

struct timer_item_struct_t ;
typedef struct timer_item_struct_t timer_item_t;
struct timer_item_struct_t {
    callback_t *callback;
    duk_bool_t repeat;
    timer_item_t *next;
    timer_item_t *prev;
};

pthread_t eventloop_thread;
int err = 1;

static void* eventloop_task(void* args) {
    eventloop();

    for (;;) {
        platform_sleep(50);
    }
}

void start_eventloop_thread() {
    err = pthread_create(&eventloop_thread, NULL, eventloop_task, NULL);
}

void kill_eventloop_thread() {
    if (err) {
        return;
    }
    pthread_cancel(eventloop_thread);
    pthread_join(eventloop_thread, NULL);
}

void eventloop_platform_queue_push(callback_t *callback) { }


callback_t* eventloop_platform_queue_receive() {
    return NULL;
}

module_ret_t _timer_cleanup(timer_item_t *timer) {
    return ERR_MODULE_TIMEOUT;
}

void _timer_callback_completion(callback_t *callback) {}

void eventloop_platform_timers_cleanup() {}


module_ret_t eventloop_platform_timer_start(callback_t *callback, long timeout, duk_bool_t repeat) {
    return ERR_MODULE_TIMEOUT;
}

module_ret_t eventloop_platform_timer_stop(callback_t *callback) {
    return ERR_MODULE_TIMEOUT;
}