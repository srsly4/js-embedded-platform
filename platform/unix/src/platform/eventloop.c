#include <platform/eventloop.h>
#include <common.h>

#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <platform.h>
#include <platform/queue.h>
#include <platform/critical_section.h>

#define NSECPERSECOND 1000000000

struct timer_spec;
typedef struct timer_spec timer_spec;

struct timer_spec {
    struct timespec trigger_time;
    struct timespec timeout;
};

struct timer_item_struct_t ;
typedef struct timer_item_struct_t timer_item_t;

struct timer_item_struct_t {
    callback_t *callback;
    pthread_t timer;
    timer_spec spec;
    duk_bool_t repeat;
    timer_item_t *next;
    timer_item_t *prev;
};


pthread_t eventloop_thread;
QueueHandle_t queue;

static callback_t _received_callback;

static timer_item_t *timer_first;
static timer_item_t *timer_last;
int err = -1;

struct timespec _timespec_add(struct timespec ts1, struct timespec ts2)
{
    struct timespec ts;
    ts.tv_sec = ts1.tv_sec + ts2.tv_sec;
    ts.tv_nsec = ts1.tv_nsec + ts2.tv_nsec;
    if (ts.tv_nsec >= NSECPERSECOND)
    {
        ts.tv_sec++;
        ts.tv_nsec -= NSECPERSECOND;
    }
    return ts;
}

static void* eventloop_task(void* args) {
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    CRITICAL_SECTION_ENTER();
    queue = xQueueCreate(EVENTLOOP_PLATFORM_QUEUE_LENGTH, sizeof(callback_t));

    timer_first = NULL;
    timer_last = NULL;

    CRITICAL_SECTION_EXIT();
    eventloop();
}

void start_eventloop_thread() {
    CRITICAL_SECTION_ENTER();
    err = pthread_create(&eventloop_thread, NULL, eventloop_task, NULL);
    CRITICAL_SECTION_EXIT();
}

void kill_eventloop_thread() {
    CRITICAL_SECTION_ENTER();
    if (err) {
        goto eventloop_killed;
    }
    pthread_cancel(eventloop_thread);
    pthread_join(eventloop_thread, NULL);
    clear_eventloop();
    vQueueDelete(queue);
    eventloop_killed:
    queue = NULL;
    err = -1;
    CRITICAL_SECTION_EXIT();
}

void eventloop_platform_queue_push(callback_t *callback) {
    xQueueSend(queue, callback);
}


callback_t* eventloop_platform_queue_receive() {
    xQueueReceive(queue, &_received_callback, pdFALSE);
    return &_received_callback;
}

module_ret_t _timer_cleanup(timer_item_t *timer) {
    CRITICAL_SECTION_ENTER();
    pthread_cancel(timer->timer);
    pthread_join(timer->timer, NULL);

    eventloop_callback_destroy(timer->callback);

    // delete timer from linked list
    if (timer->prev) {
        timer->prev->next = timer->next;
    } else {
        timer_first = timer->next;
    }
    if (timer->next) {
        timer->next->prev = timer->prev;
    } else {
        timer_last = timer->prev;
    }

    free(timer);
    CRITICAL_SECTION_EXIT();
    return ERR_MODULE_SUCC;


}

void _timer_callback_completion(callback_t *callback) {
    if (!callback->user_data) {
        return;
    }
    timer_item_t *timer = callback->user_data;
    if (!timer->repeat) {
        _timer_cleanup(timer);
    }
}

void* _timer_task(void * data) {
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    timer_item_t* timer_item = (timer_item_t *) data;
    timer_spec spec = timer_item->spec;
    do {
        while (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &spec.trigger_time, NULL));
        spec.trigger_time = _timespec_add(spec.trigger_time, spec.timeout);
        eventloop_callback_call(timer_item->callback);
    } while (timer_item->repeat);
}

void eventloop_platform_timers_cleanup() {
    timer_item_t *ptr = timer_first;
    timer_item_t *next;
    while (ptr) {
        next = ptr->next;
        pthread_cancel(ptr->timer);
        pthread_join(ptr->timer, NULL);
        free(ptr);
        ptr = next;
    }
}


module_ret_t eventloop_platform_timer_start(callback_t *callback, long timeout, duk_bool_t repeat) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    if (!repeat) { // destroy callback after first trigger
        callback->completion_handler = _timer_callback_completion;
    }
    timer_item_t *timer_item = malloc(sizeof(timer_item_t));
    if (timer_item == NULL) {
        return ERR_MODULE_MEM;
    }


    timer_item->callback = callback;
    timer_item->repeat = repeat;
    timer_item->prev = NULL;
    timer_item->next = NULL;

    // put timer item into the linked list
    if (timer_first == NULL) {
        timer_first = timer_last = timer_item;
    } else {
        timer_last->next = timer_item;
        timer_item->prev = timer_last;
        timer_last = timer_item;
    }

    timer_item->spec.timeout.tv_sec = timeout / 1000;
    timer_item->spec.timeout.tv_nsec = (timeout % 1000) * 1000000;
    timer_item->spec.trigger_time = _timespec_add(timer_item->spec.timeout, ts);

    callback->user_data = timer_item;

    if (pthread_create(&timer_item->timer, NULL, _timer_task, timer_item)) {
        return ERR_MODULE_TIMEOUT;
    }
    return ERR_MODULE_SUCC;
}

module_ret_t eventloop_platform_timer_stop(callback_t *callback) {
    if (callback->user_data) {
        timer_item_t *timer = callback->user_data;
        _timer_cleanup(timer);
        return ERR_MODULE_SUCC;
    } else {
        return ERR_MODULE_MEM;
    }
}