#include <platform/httpd.h>
#include <platform/memory.h>
#include <cmsis_os.h>
#include <lwip/sockets.h>
#include <string.h>

osThreadId httpdTaskHandle;

#define HTTPD_CLIENT_BUFF_LENGTH 256

static char *defaultResponse =
        "<html><head><title>Js embedded platform</title></head>"
        "<body><h1>Hello there!</h1></body></html>";

struct http_client {
    int socket_fd;
    TaskHandle_t task;
    uint8_t buff[HTTPD_CLIENT_BUFF_LENGTH];
};

void prepare_http_response(uint8_t *buff, int size, char *content) {
    int content_length = strlen(content) + 2;
    snprintf((char *) buff, (size_t) size,
             "HTTP/1.1 200 OK\r\nContent-type: text/html\r\n"
                     "Content-Length: %d\r\n"
                     "\r\n\r\n%s",
        content_length, content
    );
}

static void http_serve(void *arg) {
    struct http_client *client = arg;
    char response_buff[128];
    int read_size = 0;
    do {
        read_size = lwip_recv(client->socket_fd, client->buff, HTTPD_CLIENT_BUFF_LENGTH, 0);
        if (read_size > 5 && client->buff[0]=='G' &&
                             client->buff[1]=='E' &&
                             client->buff[2]=='T' &&
                             client->buff[3]==' ' &&
                             client->buff[4]=='/') {
            if (strncmp((const char *) &client->buff[5], "heap", 4) == 0) {
                snprintf(response_buff, 128,
                         "<html><head><title>Heap usage</title><body>Free heap bytes: %d</body></head></html>",
                         xPortGetFreeHeapSize()
                );
                prepare_http_response(client->buff, HTTPD_CLIENT_BUFF_LENGTH,
                    response_buff
                );
            } else {
                prepare_http_response(client->buff, HTTPD_CLIENT_BUFF_LENGTH, defaultResponse);
            }

            int buf_len = strlen((const char *) client->buff);
            lwip_send(client->socket_fd, client->buff, (size_t) buf_len, 0);
        }
    } while (read_size > 0);
    lwip_close(client->socket_fd);

    TaskHandle_t curr_task = client->task;
    free(client);
    vTaskDelete(curr_task);
}

void httpd_task(const void* argument) {
    int err = 0;
    struct sockaddr_in sockaddr, accept_sockaddr;
    socklen_t socklen = sizeof(accept_sockaddr);

    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(80);
    sockaddr.sin_addr.s_addr = INADDR_ANY;
    int socket_fd = lwip_socket(AF_INET, SOCK_STREAM, 0);


    err = lwip_bind(socket_fd, (const struct sockaddr *) &sockaddr, sizeof(sockaddr));
    err = lwip_listen(socket_fd, 5);

    int accepted_socket_id = 0;
    do {
        accepted_socket_id = lwip_accept(socket_fd, (struct sockaddr *) &accept_sockaddr, &socklen);
        struct http_client *client = malloc(sizeof(struct http_client));
        client->socket_fd = accepted_socket_id;
        xTaskCreate(http_serve, "client_task", 1024, client, 3, &client->task);

    } while (accepted_socket_id >= 0);

    err = lwip_close(socket_fd);


}


void httpd_init() {
    osThreadDef(httpdTask, httpd_task, osPriorityNormal, 0, 4096);
    httpdTaskHandle = osThreadCreate(osThread(httpdTask), NULL);
}

