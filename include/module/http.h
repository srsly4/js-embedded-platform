#ifndef JS_EMBEDDED_PLATFORM_HTTPS_H
#define JS_EMBEDDED_PLATFORM_HTTPS_H

#include <module.h>
#include <eventloop.h>

struct https_request_sess_struct_t;
typedef struct https_request_sess_struct_t https_request_sess_t;

struct https_request_sess_struct_t {
    int socket_fd;
    callback_t *callback;
    char *host;
    uint32_t buff_size;
    char *buff;
    https_request_sess_t *next;
    https_request_sess_t *prev;
    void* platform_data;
};

void module_http_platform_mutex_lock();
void module_http_platform_mutex_unlock();

void module_http_platform_start_request_thread(https_request_sess_t *session);
void module_http_platform_stop_request_thread(https_request_sess_t *session);

void module_http_request_thread(https_request_sess_t *session);

const module_t* module_http_get();

#endif //JS_EMBEDDED_PLATFORM_HTTPS_H
