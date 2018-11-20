#ifdef UNIX
#include <duktape.h>
#include "module/gpio.h"

module_ret_t module_gpio_platform_init()
{
    return ERR_MODULE_TIMEOUT;
}

void module_gpio_platform_setup(uint32_t port, uint32_t pin, uint32_t direction, uint32_t pullup_type) { }

void module_gpio_platform_cleanup(duk_context *ctx) { }

void module_gpio_platform_set(uint32_t port, uint32_t pin, uint32_t mode) { }


uint8_t module_gpio_platform_get(uint32_t port, uint32_t pin) { }

const duk_number_list_entry module_gpio_platform_const_list[] = {
        { NULL, 0 }
};

duk_number_list_entry* module_gpio_platform_get_const_list() {
    return (duk_number_list_entry *) module_gpio_platform_const_list;
}
#endif // UNIX
