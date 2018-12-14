#include <time.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <platform/httpd.h>
#include <platform/lwm2md.h>
#include <platform/debug.h>
#include <eventloop.h>
#include "common.h"
#include "platform.h"
#include <pthread.h>
#include <stdio.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"

pthread_t mainTaskHandle;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_USB_OTG_FS_PCD_Init(void);

void _Error_Handler(char *file, int line)
{
    while(1)
    {
    }
}

void SystemClock_Config(void) {}

static void MX_USART3_UART_Init(void) {}

static void MX_USB_OTG_FS_PCD_Init(void) {}

static void GPIO_Init(void) {}

void start_lwm2m_task(void const *args) {

}

void tcpip_init_callback(void *args) {
    httpd_init();
    lwm2md_init();
}

void* start_main_task(void* argument) {
    char msg[] = "Hello there!\n\0";

    tcpip_init_callback(NULL);

    entrypoint();

    struct sockaddr_in my_addr;
    my_addr.sin_port = htons(9999);
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int s = socket(AF_INET, SOCK_DGRAM, 0);
    int broadcast_enabled = 1;
    setsockopt(s, SOL_SOCKET, SO_BROADCAST, &broadcast_enabled, sizeof(broadcast_enabled));
    bind(s, (struct sockaddr *) &my_addr, sizeof(my_addr));

    struct sockaddr_in dest_addr;
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(9998);
    dest_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

    for(;;) {
        sleep(5);
        sendto(s, msg, sizeof(msg), 0, (struct sockaddr *) &dest_addr, sizeof(dest_addr));
    }
}

int main(void){
    SystemClock_Config();
    GPIO_Init();
    srand(time(NULL));

    pthread_create(&mainTaskHandle, NULL, start_main_task, NULL);

    pthread_exit(0);
}

void platform_sleep(uint32_t ms) {
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000;
    nanosleep(&ts, NULL);
}

void platform_debug_led_on() {
//    printf("LedON\n");
}
void platform_debug_led_off() {
//    printf("LedOFF\n");
}

#ifdef UNIX
void platform_register_modules() { }
#endif

uint32_t platform_rand() {
    return rand();
}

#pragma clang diagnostic pop