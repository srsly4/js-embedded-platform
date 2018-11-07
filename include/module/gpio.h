#ifndef JS_EMBEDDED_PLATFORM_MODULE_GPIO_H
#define JS_EMBEDDED_PLATFORM_MODULE_GPIO_H

#include <module.h>

const module_t* module_gpio_get();

void module_gpio_platform_setup(uint32_t port, uint32_t pin, uint32_t direction, uint32_t pullup_type);

duk_number_list_entry* module_gpio_platform_get_const_list();

#endif
