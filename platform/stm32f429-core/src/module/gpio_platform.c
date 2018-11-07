#include <duktape.h>
#include "module/gpio.h"
#include "stm32f4xx_hal.h"


void module_gpio_platform_setup(uint32_t port, uint32_t pin, uint32_t direction, uint32_t pullup_type) {
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.Pin = pin;
    GPIO_InitStruct.Mode = direction;
    GPIO_InitStruct.Pull = pullup_type;
    // any usage of high-freq GPIO in javascript is useless due to performance
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init((GPIO_TypeDef *)(uintptr_t)port, &GPIO_InitStruct);
}


void module_gpio_platform_set(uint32_t port, uint32_t pin, uint32_t mode) {
    HAL_GPIO_WritePin((GPIO_TypeDef *)(uintptr_t)port, (uint16_t) pin, (GPIO_PinState) mode);
}


const duk_number_list_entry module_gpio_platform_const_list[] = {
        { "PORTA", (double)(uintptr_t)(GPIOA) },
        { "PORTB", (double)(uintptr_t)(GPIOB) },
        { "PORTC", (double)(uintptr_t)(GPIOC) },
        { "PORTD", (double)(uintptr_t)(GPIOD) },
        { "PORTE", (double)(uintptr_t)(GPIOE) },
        { "PORTF", (double)(uintptr_t)(GPIOF) },
        { "PORTG", (double)(uintptr_t)(GPIOG) },
        { "PORTH", (double)(uintptr_t)(GPIOH) },
        { "PORTI", (double)(uintptr_t)(GPIOI) },
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
        { "MODE_OUT_OD", (double)GPIO_MODE_OUTPUT_OD },
        { "MODE_OUT_PP", (double)GPIO_MODE_OUTPUT_PP },
        { "MODE_IN", (double)GPIO_MODE_INPUT },
        { "MODE_ANALOG", (double)GPIO_MODE_ANALOG },
        { "NOPULL", (double)GPIO_NOPULL },
        { "PULLDOWN", (double)GPIO_PULLDOWN },
        { "PULLUP", (double)GPIO_PULLUP },
        { NULL, 0 }
};

duk_number_list_entry* module_gpio_platform_get_const_list() {
    return (duk_number_list_entry *) module_gpio_platform_const_list;
}
