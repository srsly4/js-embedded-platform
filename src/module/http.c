#include <module.h>
#include <platform/sockets.h>
#include <eventloop.h>
#include <http_parser/curi.h>

#include "module/http.h"

#define HTTPS_MODULE_STASH_NAME "_http"

#define HTTP_MODULE_STASH_RESPONSE_NAME "_http_res"
#define HTTP_RESPONSE_OBJECT_POINTER_KEY "_obj"

#define HTTP_BUFFER_SIZE 512

static https_request_sess_t *session_list = NULL;
static uint32_t http_response_sequence_id = 0;

// before calling on_end event
void _on_data_arg_handler(duk_context *ctx, void *user_data) {

}

// after calling on_end event
void _on_data_completion_handler(callback_t* callback) {

}

// adds on_data event handler
duk_ret_t _callback_response_on_data_handler(duk_context *ctx) {
    duk_require_function(ctx, 0);
    duk_push_this(ctx);
    duk_get_prop_string(ctx, -1, HTTP_RESPONSE_OBJECT_POINTER_KEY);
    https_request_sess_t *session = duk_get_pointer(ctx, -1);
    duk_pop(ctx);
    duk_pop(ctx);

    session->data_callback = eventloop_callback_create_from_context(ctx, 0);
    session->data_callback->user_data = session;
    session->data_callback->arg_handler = _on_data_arg_handler;
    session->data_callback->completion_handler = _on_data_completion_handler;

    return 0;
}

// adds on_end event handler
duk_ret_t _callback_response_on_end_handler(duk_context *ctx) {
    duk_require_function(ctx, 0);
    duk_push_this(ctx);
    duk_get_prop_string(ctx, -1, HTTP_RESPONSE_OBJECT_POINTER_KEY);
    https_request_sess_t *session = duk_get_pointer(ctx, -1);
    duk_pop(ctx);
    duk_pop(ctx);

    session->end_callback = eventloop_callback_create_from_context(ctx, 0);
    session->end_callback->user_data = session;
    return 0;
}

void _callback_arg_handler(duk_context *ctx, void *user_data) {
    https_request_sess_t *session = user_data;
    if (session->request_result != MODULE_HTTP_RESULT_RESPONSE) {
        switch (session->request_result) {
            case MODULE_HTTP_RESULT_TIMEOUT:
                duk_push_string(ctx, "timeout");
                break;
            default:
                duk_push_string(ctx, "unknown error");
                break;
        }
    } else {
        duk_push_undefined(ctx); // no error
        duk_push_object(ctx);

        duk_push_pointer(ctx, session);
        duk_put_prop_string(ctx, -2, HTTP_RESPONSE_OBJECT_POINTER_KEY);

        duk_push_c_function(ctx, _callback_response_on_data_handler, 1);
        duk_put_prop_string(ctx, -2, "onData");

        duk_push_c_function(ctx, _callback_response_on_end_handler, 1);
        duk_put_prop_string(ctx, -2, "onEnd");

        duk_push_global_stash(ctx);
        duk_get_prop_string(ctx, -1, HTTP_MODULE_STASH_RESPONSE_NAME);
        duk_dup(ctx, -3);
        session->response_obj_id = http_response_sequence_id++;
        duk_push_number(ctx, session->response_obj_id);
        duk_put_prop(ctx, -3);
        duk_pop(ctx); // pop global stash
    }
}

// after calling on_end event
void _callback_completion_handler(callback_t* callback) {
    https_request_sess_t *session = callback->user_data;
    if (!session->data_callback && !session->end_callback) {
        eventloop_callback_destroy(callback);
    }
}

int _uri_parser_host_callback(void* user_data, const char* data, size_t len) {
    https_request_sess_t *session = user_data;
    session->uri_host = (char *) data;
    session->uri_host_len = (uint16_t) len;
    return 1;
}
int _uri_parser_path_callback(void* user_data, const char* data, size_t len) {
    https_request_sess_t *session = user_data;
    session->uri_path = (char *) data;
    session->uri_path_len = (uint16_t) len;
    return 1;
}
int _uri_parser_port_callback(void* user_data, unsigned int port) {
    https_request_sess_t *session = user_data;
    session->uri_port = (uint16_t) port;
    return 1;
}

static duk_ret_t http_request(duk_context *ctx){
    const char* method = duk_require_string(ctx, 0);
    const char* url = duk_require_string(ctx, 1);
    duk_require_function(ctx, 2);

    https_request_sess_t *session = malloc(sizeof(https_request_sess_t));

    curi_settings settings;
    curi_default_settings(&settings);
    settings.host_callback = _uri_parser_host_callback;
    settings.port_callback = _uri_parser_port_callback;
    settings.path_callback = _uri_parser_path_callback;

    if (curi_parse_full_uri_nt(url, &settings, session) != curi_status_success) {
        duk_error(ctx, DUK_ERR_URI_ERROR, "Invalid uri");
        return 0;
    }

    callback_t *callback = eventloop_callback_create_from_context(ctx, 2);

    session->buff_size = HTTP_BUFFER_SIZE;
    session->buff = malloc(HTTP_BUFFER_SIZE);
    session->socket_fd = -1;
    session->platform_data = NULL;
    session->prev = NULL;
    session->callback = callback;
    session->data_callback = NULL;
    session->end_callback = NULL;

    callback->user_data = session;
    callback->arg_handler = _callback_arg_handler;
    callback->completion_handler = _callback_completion_handler;

    module_http_platform_mutex_lock();
    session->next = session_list;
    session_list = session;
    module_http_platform_mutex_unlock();

    // prepare HTTP request for further sending
    snprintf(session->buff, HTTP_BUFFER_SIZE-4, "%s %.*s HTTP/1.1\r\nHost: %.*s\r\n\r\n",
            method,
            session->uri_path_len, session->uri_path,
            session->uri_host_len, session->uri_host);

    module_http_platform_start_request_thread(session);
    return 0;
}

int _parser_on_headers_complete(http_parser *parser) {
    https_request_sess_t *session = parser->data;
    session->request_result = MODULE_HTTP_RESULT_RESPONSE;
    eventloop_callback_call(session->callback);

    return 0;
}

void module_http_request_thread(https_request_sess_t *session) {
    struct addrinfo hints, *res;
    struct sockaddr_in *sockaddr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags |= AI_CANONNAME;

    char *host_nt = malloc(session->uri_host_len+1);
    memcpy(host_nt, session->uri_host, session->uri_host_len);
    host_nt[session->uri_host_len] = '\0';

    char *port_nt = malloc(6);
    snprintf(port_nt, 6, "%d", session->uri_port);

    if (getaddrinfo(host_nt, port_nt, &hints, &res) != 0) {
        // fixme: handle error;
        free(host_nt);
        return;
    }
    free(host_nt);
    free(port_nt);

    session->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (session->socket_fd < 0) {
        session->request_result = MODULE_HTTP_RESULT_MEMORY_ERR;
        goto cleanup;
    }
//    fcntl(session->socket_fd, F_SETFL, O_NONBLOCK);
    int conn_res = connect(session->socket_fd, res->ai_addr, res->ai_addrlen);
    if (conn_res != 0 && errno != EINPROGRESS) {
        session->request_result = MODULE_HTTP_RESULT_TIMEOUT;
        eventloop_callback_call(session->callback);
        goto cleanup;
    }

//    fd_set fdset;
//    struct timeval tv;
//    FD_ZERO(&fdset);
//    FD_SET(session->socket_fd, &fdset);
//
//    tv.tv_sec = 10;
//    tv.tv_usec = 0;
//
//    if (select(session->socket_fd + 1, NULL, &fdset, NULL, &tv) != 1) {
//        // timeout
//        session->request_result = MODULE_HTTP_RESULT_TIMEOUT;
//        eventloop_callback_call(session->callback);
//        goto cleanup;
//    }

    if (send(session->socket_fd, session->buff, strlen(session->buff), 0) < 0) {
        goto cleanup;
    }

    // now receive only headers and detect start of the content
    http_parser_settings parser_settings;
    memset(&parser_settings, 0, sizeof(http_parser_settings));
    parser_settings.on_headers_complete = _parser_on_headers_complete;
    http_parser_init(&(session->parser), HTTP_RESPONSE);
    session->parser.data = session;

    int recv_bytes = 1;
    while (recv_bytes) {
        recv_bytes = recv(session->socket_fd, session->buff, session->buff_size, 0);

        if (recv_bytes < 0) {
            //fixme: handle error
            goto cleanup;
        }

        http_parser_execute(&(session->parser), &parser_settings, session->buff, (size_t) recv_bytes);

    }

    cleanup:
    close(session->socket_fd);
    free(session->buff);
    free(session);
    session->socket_fd = 0;
    module_http_platform_stop_request_thread(session);
}


static const duk_function_list_entry gpio_module_funcs[] = {
        { "request", http_request, DUK_VARARGS},
        { NULL, NULL, 0 }
};

module_ret_t module_http_init(duk_context *ctx) {
    void *module_ptr = NULL;
    duk_push_global_stash(ctx);
    if (duk_get_prop_string(ctx, -1, HTTPS_MODULE_STASH_NAME) == 1) {
        module_ptr = duk_get_heapptr(ctx, -1);
        duk_pop(ctx);
    } else {
        duk_pop(ctx); // we don't want undefined on stack
        duk_push_object(ctx);
        duk_put_function_list(ctx, -1, gpio_module_funcs);

        module_ptr = duk_get_heapptr(ctx, -1);
        duk_put_prop_string(ctx, -2, HTTPS_MODULE_STASH_NAME);

        duk_push_object(ctx);
        duk_put_prop_string(ctx, -2, HTTP_MODULE_STASH_RESPONSE_NAME);
    }
    duk_pop(ctx); // pop global stack
    duk_push_heapptr(ctx, module_ptr); // will be returned module
    return ERR_MODULE_SUCC;
}

static const module_t https_module = {
        "http",
        &module_http_init,
        NULL,
        NULL
};

const module_t* module_http_get() {
    return &https_module;
}
