#include "platform/critical_section.h"


static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void _enter_critical_section()
{
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    pthread_mutex_lock(&mutex);
}


void _exit_critical_section()
{
    pthread_mutex_unlock(&mutex);
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
}