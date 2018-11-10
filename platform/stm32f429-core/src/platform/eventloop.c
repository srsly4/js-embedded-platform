#include <platform/eventloop.h>
#include <common.h>
#include <cmsis_os.h>

osThreadId eventloop_thread = NULL;

static void eventloop_task(const void* args) {
    eventloop();
    // inifnite loop for securing RTOS task
    for (;;) {
        osDelay(50);
    }
}

void start_eventloop_thread() {
    osThreadDef(eventloopThread, eventloop_task, osPriorityNormal, 0, 4096);
    eventloop_thread = osThreadCreate(osThread(eventloopThread), NULL);
}

void kill_eventloop_thread() {
    if (eventloop_thread == NULL) {
        return;
    }
    vTaskSuspend(eventloop_task_handle);
    clear_eventloop();
    xQueueReset(eventloop_queue);
    vTaskDelete(eventloop_task_handle);
    vQueueDelete(eventloop_queue);
    eventloop_queue = NULL;
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
    timer_item_t *ptr = timer_first;
    timer_item_t *next;
    while (ptr) {
        next = ptr->next;
        xTimerDelete(ptr->timer, 0);
        vPortFree(ptr);
        ptr = next;
    }
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
