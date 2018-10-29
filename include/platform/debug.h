#ifndef JS_EMBEDDED_PLATFORM_DEBUG_H
#define JS_EMBEDDED_PLATFORM_DEBUG_H

void debug_platform_notify_duk_error(void *udata, const char *msg);

void debug_platform_error_led_on();
void debug_platform_error_led_off();

#endif //JS_EMBEDDED_PLATFORM_DEBUG_H
