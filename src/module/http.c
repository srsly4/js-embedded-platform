#include <module.h>
#include <platform/sockets.h>
#include <eventloop.h>

#include "module/http.h"

#define HTTPS_MODULE_STASH_NAME "_http"

#define HTTP_BUFFER_SIZE 512

static https_request_sess_t *session_list = NULL;

static duk_ret_t http_request(duk_context *ctx){
    const char* method = duk_require_string(ctx, 0);
    const char* url = duk_require_string(ctx, 1);

    duk_idx_t callback_idx = duk_get_top(ctx);
    duk_require_callable(ctx, callback_idx);

    if (strncmp(url, "http://", 7) != 0) {
        duk_error(ctx, DUK_ERR_URI_ERROR, "Unsupported protocol");
        return 0;
    }

    uint16_t uri_len = (uint16_t) strlen(url);
    if (uri_len < 8) {
        duk_error(ctx, DUK_ERR_URI_ERROR, "Url to short");
        return 0;
    }

    char *host_ptr = (char *) (url + 7);
    uint16_t host_len = (uint16_t) (uri_len - 7);
    char *seek_ptr = host_ptr;
    char *loc_ptr = NULL;
    while (*seek_ptr != '\0') {
        if (*seek_ptr == '/') {
            host_len = (uint16_t)(seek_ptr - host_ptr);
            loc_ptr = seek_ptr;
            break;
        }
        seek_ptr += 1;
    }

    if (!loc_ptr) {
        loc_ptr = "/";
    }

    char *host = malloc(host_len+1);
    strncpy(host, url, host_len);
    host[host_len] = '\0';

    https_request_sess_t *session = malloc(sizeof(https_request_sess_t));

    callback_t *callback = eventloop_callback_create_from_context(ctx, callback_idx);

    session->buff_size = HTTP_BUFFER_SIZE;
    session->buff = malloc(HTTP_BUFFER_SIZE);
    session->socket_fd = -1;
    session->platform_data = NULL;
    session->prev = NULL;
    session->callback = callback;
    session->host = host;

    module_http_platform_mutex_lock();
    session->next = session_list;
    session_list = session;
    module_http_platform_mutex_unlock();

    // prepare HTTP request for further sending
    snprintf(session->buff, HTTP_BUFFER_SIZE-4, "%s %s HTTP/1.1\r\nHost: %s\r\n\r\n", method, loc_ptr, host);

    module_http_platform_start_request_thread(session);
    return 0;
}

void module_http_request_thread(https_request_sess_t *session) {
    struct addrinfo hints, *res;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags |= AI_CANONNAME;
    if (getaddrinfo(session->host, NULL, &hints, &res) != 0) {
        // fixme: handlee error;
        return;
    }

    session->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(session->socket_fd, res->ai_addr, res->ai_addrlen) != 0) {
        // fixme: handle error
        return;
    }

    if (send(session->socket_fd, session->buff, strlen(session->buff), 0) < 0) {
        // fixme: handle error
        return;
    }

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
