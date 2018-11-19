#include <module/gpio.h>
#include <eventloop.h>
#include "platform.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"

void platform_register_modules() {
    eventloop_register_module((module_t *) module_gpio_get());
}

#pragma clang diagnostic pop