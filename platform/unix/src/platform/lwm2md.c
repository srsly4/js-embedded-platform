#include <platform/lwm2md.h>
#include <platform/connection.h>
#include <pthread.h>
#include <lwm2m/client/lwm2mclient.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"

#define MAX_PACKET_SIZE 1024

typedef struct {
    int sock;
} client_data_t;


static uint8_t buffer[MAX_PACKET_SIZE];
static pthread_t lwm2mdTaskHandle;
static pthread_t discoverydTaskHandle;
static lwm2m_context_t* context;
static client_data_t client_data;
static lwm2m_object_t* obj_array[2];

void* lwm2md_task(void* args) {
    init_lwm2m();
}

void* discoveryd_task(void* args) {
    init_discovery();
}

void lwm2md_init(void) {
    pthread_create(&lwm2mdTaskHandle, NULL, lwm2md_task, NULL);
    pthread_create(&discoverydTaskHandle, NULL, discoveryd_task, NULL);
}

#pragma clang diagnostic pop