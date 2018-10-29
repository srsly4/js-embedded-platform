#include <platform/eventloop.h>
#include <platform/memory.h>
#include <common.h>
#include <cmsis_os.h>
#include <eventloop.h>
#include <platform/module.h>

struct timer_item_struct_t ;
typedef struct timer_item_struct_t timer_item_t;
struct timer_item_struct_t {
    callback_t *callback;
    TimerHandle_t *timer;
    duk_bool_t repeat;
    timer_item_t *next;
    timer_item_t *prev;
};

static xTaskHandle eventloop_task_handle;
static QueueHandle_t eventloop_queue;

static callback_t _received_callback;

static timer_item_t *timer_first;
static timer_item_t *timer_last;

static void eventloop_task(void* args) {
    eventloop_queue = xQueueCreate(EVENTLOOP_PLATFORM_QUEUE_LENGTH, sizeof(callback_t));

    timer_first = NULL;
    timer_last = NULL;

    eventloop();
}

void start_eventloop_thread() {
    xTaskCreate(eventloop_task, "eventloop_task", 4096, NULL, 3, &eventloop_task_handle);
}

void kill_eventloop_thread() {
    if (eventloop_task_handle == NULL) {
        return;
    }
    vTaskSuspend(eventloop_task_handle);
    clear_eventloop();
    xQueueReset(eventloop_queue);
    vTaskDelete(eventloop_task_handle);
    eventloop_task_handle = NULL;
}

void eventloop_platform_queue_push(callback_t *callback) {
    xQueueSend(eventloop_queue, callback, portMAX_DELAY);
}

void eventloop_platform_queue_push_isr(callback_t *callback) {
    xQueueSendFromISR(eventloop_queue, callback, NULL);
}


callback_t* eventloop_platform_queue_receive() {
    xQueueReceive(eventloop_queue, &_received_callback, portMAX_DELAY);
    return &_received_callback;
}

module_ret_t _timer_cleanup(timer_item_t *timer) {
    if (xTimerDelete(timer->timer, 0) == pdFAIL) {
        return ERR_MODULE_TIMEOUT;
    }
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

    vPortFree(timer);
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

void _timer_trigger(TimerHandle_t xTimer) {
    timer_item_t *timer = pvTimerGetTimerID(xTimer);
    if (timer) {
        eventloop_callback_call(timer->callback);
    }
}

void eventloop_platform_timers_cleanup() {

}

module_ret_t eventloop_platform_timer_start(callback_t *callback, long timeout, duk_bool_t repeat) {
    if (!repeat) { // destroy callback after first trigger
        callback->completion_handler = _timer_callback_completion;
    }
    TimerHandle_t timer = xTimerCreate(
            "setTimeout", pdMS_TO_TICKS(timeout), repeat ? pdTRUE : pdFALSE, 0, _timer_trigger);
    if (!timer) {
        return ERR_MODULE_MEM;
    }

    timer_item_t *timer_item = pvPortMalloc(sizeof(timer_item_t));
    if (!timer_item) {
        return ERR_MODULE_MEM;
    }

    timer_item->timer = timer;
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

    vTimerSetTimerID(timer, timer_item); // timerId is used to store timer_item ptr
    callback->user_data = timer_item;

    if (xTimerStart(timer, 0) == pdFAIL) {
        return ERR_MODULE_TIMEOUT;
    }
    return ERR_MODULE_SUCC;
}

module_ret_t eventloop_platform_timer_stop(callback_t *callback) {
    if (callback->user_data) {
        timer_item_t *timer = callback->user_data;
        if (xTimerStop(timer->timer, 0) == pdFAIL) {
            return ERR_MODULE_TIMEOUT;
        }
        _timer_cleanup(timer);
        return ERR_MODULE_SUCC;
    } else {
        return ERR_MODULE_MEM;
    }
}