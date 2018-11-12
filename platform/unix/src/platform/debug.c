#include "platform/debug.h"
#include <platform.h>
#include <stdio.h>

void debug_platform_notify_duk_error(void *udata, const char *msg) {
    for (int i = 0; i < 3; i++) {
        debug_platform_error_led_on();
        platform_sleep(250);
        debug_platform_error_led_off();
        platform_sleep(250);
    }
}

void debug_platform_error_led_on() {
    printf("LedOn\n");
}
void debug_platform_error_led_off() {
    printf("LedOff\n");
}