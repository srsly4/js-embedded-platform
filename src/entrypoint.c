#include <platform/eventloop.h>
#include "common.h"
#include "duktape.h"

void entrypoint(void){
    start_eventloop_thread();
}