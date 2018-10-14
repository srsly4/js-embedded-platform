#include <platform/lwm2md.h>
#include <platform/connection.h>
#include <cmsis_os.h>
#include <lwm2m/client/lwm2mclient.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"

#define MAX_PACKET_SIZE 1024

typedef struct {
    int sock;
} client_data_t;


static uint8_t buffer[MAX_PACKET_SIZE];
static osThreadId lwm2mdTaskHandle;
static lwm2m_context_t* context;
static client_data_t client_data;
static lwm2m_object_t* obj_array[2];

void lwm2md_task(void const *args) {
    init_lwm2m_server();
}

void lwm2md_init(void) {
    osThreadDef(lwm2mdTask, lwm2md_task, osPriorityNormal, 0, 2048);
    lwm2mdTaskHandle = osThreadCreate(osThread(lwm2mdTask), NULL);
}

#pragma clang diagnostic pop