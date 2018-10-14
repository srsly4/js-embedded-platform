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
    osThreadTerminate(eventloop_thread);
}
