#include <duktape.h>
#include "module/gpio.h"
#include "stm32f4xx_hal.h"


void module_gpio_platform_setup(uint32_t port, uint32_t pin, uint32_t direction, uint32_t pullup_type) {

}

static const duk_number_list_entry module_gpio_platform_port_list[] = {
        { "GPIOA", (double)(uintptr_t)(GPIOA) },
        { "GPIOB", (double)(uintptr_t)(GPIOB) },
        { "GPIOC", (double)(uintptr_t)(GPIOC) },
        { "GPIOD", (double)(uintptr_t)(GPIOD) },
        { "GPIOE", (double)(uintptr_t)(GPIOE) },
        { "GPIOF", (double)(uintptr_t)(GPIOF) },
        { "GPIOG", (double)(uintptr_t)(GPIOG) },
        { "GPIOH", (double)(uintptr_t)(GPIOH) },
        { "GPIOI", (double)(uintptr_t)(GPIOI) },
        { NULL, 0 }
};

static const duk_number_list_entry module_gpio_platform_pin_list[] = {
    { "PIN0", (double)GPIO_PIN_0 },
    { "PIN1", (double)GPIO_PIN_1 },
    { "PIN2", (double)GPIO_PIN_2 },
    { "PIN3", (double)GPIO_PIN_3 },
    { "PIN4", (double)GPIO_PIN_4 },
    { "PIN5", (double)GPIO_PIN_5 },
    { "PIN6", (double)GPIO_PIN_6 },
    { "PIN7", (double)GPIO_PIN_7 },
    { "PIN8", (double)GPIO_PIN_8 },
    { "PIN9", (double)GPIO_PIN_9 },
    { "PIN10", (double)GPIO_PIN_10 },
    { "PIN11", (double)GPIO_PIN_11 },
    { "PIN12", (double)GPIO_PIN_12 },
    { "PIN13", (double)GPIO_PIN_13 },
    { "PIN14", (double)GPIO_PIN_14 },
    { "PIN15", (double)GPIO_PIN_15 },
    { NULL, 0 }
};

duk_number_list_entry* module_gpio_platform_get_port_list() {
    return (duk_number_list_entry *) module_gpio_platform_port_list;
}

duk_number_list_entry* module_gpio_platform_get_pin_list() {
    return (duk_number_list_entry *) module_gpio_platform_pin_list;
}