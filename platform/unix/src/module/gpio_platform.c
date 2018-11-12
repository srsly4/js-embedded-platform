#include <duktape.h>
#include <stdio.h>
#include "module/gpio.h"


void module_gpio_platform_setup(uint32_t port, uint32_t pin, uint32_t direction, uint32_t pullup_type) {}


void module_gpio_platform_set(uint32_t port, uint32_t pin, uint32_t mode) {
    printf("Set: %d %d %d\n", port, pin, mode);
}


uint8_t module_gpio_platform_get(uint32_t port, uint32_t pin) {
    printf("Read: %d %d\n", port, pin);
    return 1;
}

const duk_number_list_entry module_gpio_platform_const_list[] = {
        { "PORTA", 1 },
        { "PORTB", 1 },
        { "PORTC", 1 },
        { "PORTD", 1 },
        { "PORTE", 1 },
        { "PORTF", 1 },
        { "PORTG", 1 },
        { "PORTH", 1 },
        { "PORTI", 1 },
        { "PIN0", 1 },
        { "PIN1", 1 },
        { "PIN2", 1 },
        { "PIN3", 1 },
        { "PIN4", 1 },
        { "PIN5", 1 },
        { "PIN6", 1 },
        { "PIN7", 1 },
        { "PIN8", 1 },
        { "PIN9", 1 },
        { "PIN10", 1 },
        { "PIN11", 1 },
        { "PIN12", 1 },
        { "PIN13", 1 },
        { "PIN14", 1 },
        { "PIN15", 1 },
        { "MODE_OUT_OD", 1 },
        { "MODE_OUT_PP", 1 },
        { "MODE_IN", 1 },
        { "MODE_ANALOG", 1 },
        { "NOPULL", 1 },
        { "PULLDOWN", 1 },
        { "PULLUP", 1 },
        { NULL, 0 }
};

duk_number_list_entry* module_gpio_platform_get_const_list() {
    return (duk_number_list_entry *) module_gpio_platform_const_list;
}
