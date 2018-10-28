#include <platform/eventloop.h>
#include <common.h>
#include <cmsis_os.h>
#include <eventloop.h>

static xTaskHandle eventloop_task_handle;
static QueueHandle_t eventloop_queue;

static callback_t _received_callback;

static void eventloop_task(void* args) {
    eventloop_queue = xQueueCreate(EVENTLOOP_PLATFORM_QUEUE_LENGTH, sizeof(callback_t));
    eventloop();
}

void start_eventloop_thread() {
    xTaskCreate(eventloop_task, "eventloop_task", 4096, NULL, 3, &eventloop_task_handle);
}

void kill_eventloop_thread() {
    if (eventloop_task_handle == NULL) {
        return;
    }
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

void _timer_callback_completion(callback_t *callback) {
    eventloop_callback_destroy(callback);
}

void _timer_trigger(TimerHandle_t xTimer) {
    callback_t *callback = pvTimerGetTimerID(xTimer);
    if (callback) {
        eventloop_callback_call(callback);
        if (xTimerIsTimerActive(xTimer) == pdFALSE) {
            xTimerDelete(xTimer, 0);
            free(callback);
        }
    }
}

void eventloop_platform_timer_start(callback_t *callback, long timeout, duk_bool_t repeat) {
    if (!repeat) { // destroy callback after first trigger
        callback->completion_handler = _timer_callback_completion;
    }
    TimerHandle_t timer = xTimerCreate(
            "setTimeout", pdMS_TO_TICKS(timeout), repeat ? pdTRUE : pdFALSE, 0, _timer_trigger);
    if (!timer) {
        return;
    }
    vTimerSetTimerID(timer, callback); // timerId is used to store callback ptr
    callback->user_data = timer;
    xTimerStart(timer, 0);
}

void eventloop_platform_timer_stop(callback_t *callback) {
    if (callback->user_data) {
        xTimerStop(callback->user_data, 0);
        xTimerDelete(callback->user_data, 0);
        free(callback);
    }
}