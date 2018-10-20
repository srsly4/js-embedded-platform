#include <platform/httpd.h>
#include <platform/memory.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <pthread.h>

pthread_t httpdTaskHandle;

#define HTTPD_CLIENT_BUFF_LENGTH 256

static char *defaultResponse =
        "<html><head><title>Js embedded platform</title></head>"
        "<body><h1>Hello there!</h1></body></html>";

struct http_client {
    int socket_fd;
    pthread_t task;
    uint8_t buff[HTTPD_CLIENT_BUFF_LENGTH];
};

void prepare_http_response(uint8_t *buff, int size, char *content) {
    size_t content_length = strlen(content) + 2;
    snprintf((char *) buff, (size_t) size,
             "HTTP/1.1 200 OK\r\nContent-type: text/html\r\n"
                     "Content-Length: %lu\r\n"
                     "\r\n\r\n%s",
        content_length, content
    );
}

static void* http_serve(void *arg) {
    struct http_client *client = arg;
    char response_buff[128];
    ssize_t read_size = 0;
    do {
        read_size = recv(client->socket_fd, client->buff, HTTPD_CLIENT_BUFF_LENGTH, 0);
        if (read_size > 5 && client->buff[0]=='G' &&
                             client->buff[1]=='E' &&
                             client->buff[2]=='T' &&
                             client->buff[3]==' ' &&
                             client->buff[4]=='/') {
            if (strncmp((const char *) &client->buff[5], "heap", 4) == 0) {
                snprintf(response_buff, 128,
                         "<html><head><title>Heap usage</title><body>Free heap bytes: %d</body></head></html>",
                         -1
                );
                prepare_http_response(client->buff, HTTPD_CLIENT_BUFF_LENGTH,
                    response_buff
                );
            } else {
                prepare_http_response(client->buff, HTTPD_CLIENT_BUFF_LENGTH, defaultResponse);
            }

            size_t buf_len = strlen((const char *) client->buff);
            send(client->socket_fd, client->buff, (size_t) buf_len, 0);
        }
    } while (read_size > 0);
    close(client->socket_fd);

    free(client);
}

void* httpd_task(void* argument) {
    int err = 0;
    struct sockaddr_in sockaddr, accept_sockaddr;
    socklen_t socklen = sizeof(accept_sockaddr);

    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(12345);
    sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    err = bind(socket_fd, (const struct sockaddr *) &sockaddr, sizeof(sockaddr));
    err = listen(socket_fd, 5);

    int accepted_socket_id = 0;
    do {
        accepted_socket_id = accept(socket_fd, (struct sockaddr *) &accept_sockaddr, &socklen);
        struct http_client *client = malloc(sizeof(struct http_client));
        client->socket_fd = accepted_socket_id;
        pthread_create(&client->task, NULL, http_serve, client);
    } while (accepted_socket_id >= 0);

    err = close(socket_fd);
}


void httpd_init() {
    pthread_create(&httpdTaskHandle, NULL, httpd_task, NULL);
}

