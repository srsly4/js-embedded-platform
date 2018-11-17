#ifndef JS_EMBEDDED_PLATFORM_HTTPS_H
#define JS_EMBEDDED_PLATFORM_HTTPS_H

#include <module.h>
#include <eventloop.h>
#include <http_parser/http_parser.h>

#define MODULE_HTTP_RESULT_TIMEOUT 0x01
#define MODULE_HTTP_RESULT_RESPONSE 0x02
#define MODULE_HTTP_RESULT_MEMORY_ERR 0x03

struct https_request_sess_struct_t;
typedef struct https_request_sess_struct_t https_request_sess_t;

struct https_request_sess_struct_t {
    int socket_fd;
    callback_t *callback;
    uint8_t request_result;
    callback_t *data_callback;
    callback_t *end_callback;
    uint32_t response_obj_id;
    http_parser parser;
    char *uri_host;
    uint16_t uri_host_len;
    uint16_t uri_port;
    char *uri_path;
    uint16_t uri_path_len;
    char *buff;
    uint32_t buff_size;
    https_request_sess_t *next;
    https_request_sess_t *prev;
    void* platform_data;
};

void module_http_platform_mutex_lock();
void module_http_platform_mutex_unlock();

void module_http_platform_start_request_thread(https_request_sess_t *session);
void module_http_platform_stop_request_thread(https_request_sess_t *session);

void module_http_platform_request_thread_wait(https_request_sess_t *session);
void module_http_platform_request_thread_notify(https_request_sess_t *notified_session);

void module_http_request_thread(https_request_sess_t *session);

const module_t* module_http_get();

#endif //JS_EMBEDDED_PLATFORM_HTTPS_H
