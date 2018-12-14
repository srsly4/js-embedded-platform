#include <duktape.h>
#include "module/gpio.h"
#include "c_gpio.h"

module_ret_t module_gpio_platform_init()
{
    if (setup() != SETUP_OK)
    {
        return ERR_MODULE_TIMEOUT;
    }
    return ERR_MODULE_SUCC;
}

void module_gpio_platform_setup(uint32_t port, uint32_t pin, uint32_t direction, uint32_t pullup_type) {
    setup_gpio(pin, direction, pullup_type);
}

void module_gpio_platform_cleanup(duk_context *ctx) {
    cleanup();
}

void module_gpio_platform_set(uint32_t port, uint32_t pin, uint32_t mode) {
    output_gpio(pin, mode);
}


uint8_t module_gpio_platform_get(uint32_t port, uint32_t pin) {
    return input_gpio(pin);
}

const duk_number_list_entry module_gpio_platform_const_list[] = {
        { "PIN2", (double)2 },
        { "PIN3", (double)3 },
        { "PIN4", (double)4 },
        { "PIN5", (double)5 },
        { "PIN6", (double)6 },
        { "PIN7", (double)7 },
        { "PIN8", (double)8 },
        { "PIN9", (double)9 },
        { "PIN10", (double)10 },
        { "PIN11", (double)11 },
        { "PIN12", (double)12 },
        { "PIN13", (double)13 },
        { "PIN16", (double)16 },
        { "PIN17", (double)17 },
        { "PIN18", (double)18 },
        { "PIN19", (double)19 },
        { "PIN20", (double)20 },
        { "PIN21", (double)21 },
        { "PIN22", (double)22 },
        { "PIN23", (double)23 },
        { "PIN24", (double)24 },
        { "PIN25", (double)25 },
        { "PIN26", (double)26 },
        { "PIN27", (double)27 },
        { "PIN35", (double)35 },
        { "PIN47", (double)47 },
        { "MODE_OUT", (double)OUTPUT },
        { "MODE_IN", (double)INPUT },
        { "NOPULL", (double)PUD_OFF },
        { "PULLDOWN", (double)PUD_DOWN },
        { "PULLUP", (double)PUD_UP },
        { NULL, 0 }
};

duk_number_list_entry* module_gpio_platform_get_const_list() {
    return (duk_number_list_entry *) module_gpio_platform_const_list;
}
