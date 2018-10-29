#include <cmsis_os.h>
#include "platform/debug.h"
#include "stm32f4xx_hal.h"

void debug_platform_notify_duk_error(void *udata, const char *msg) {
    for (int i = 3; i--;) {
        debug_platform_error_led_on();
        osDelay(250);
        debug_platform_error_led_off();
        osDelay(250);
    }
}

void debug_platform_error_led_on() {
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);
}
void debug_platform_error_led_off() {
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);

}