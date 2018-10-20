#include <platform/eventloop.h>
#include <common.h>

#include <unistd.h>
#include <pthread.h>
#include <platform.h>

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
