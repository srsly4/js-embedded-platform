#ifndef JS_EMBEDDED_PLATFORM_PLATFORM_H
#define JS_EMBEDDED_PLATFORM_PLATFORM_H

#include <stdint.h>

void platform_sleep(uint32_t ms);

void platform_debug_led_on();
void platform_debug_led_off();


#endif //JS_EMBEDDED_PLATFORM_PLATFORM_H
