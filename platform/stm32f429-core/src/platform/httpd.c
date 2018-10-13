#include <platform/httpd.h>
#include <cmsis_os.h>
#include <lwip/sockets.h>

osThreadId httpdTaskHandle;

void httpd_task(const void* argument) {
    int err = 0;
    struct sockaddr_in sockaddr, accept_sockaddr;
    socklen_t socklen = sizeof(accept_sockaddr);
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = 80;
    sockaddr.sin_addr.s_addr = INADDR_ANY;
    int socket_fd = lwip_socket(AF_INET, SOCK_STREAM, 0);


    err = lwip_bind(socket_fd, (const struct sockaddr *) &sockaddr, sizeof(sockaddr));
    err = lwip_listen(socket_fd, 5);

    do {
        err = lwip_accept(socket_fd, (struct sockaddr *) &accept_sockaddr, &socklen);
        if (err == ERR_OK) {

        }
    } while (err == ERR_OK);

    err = lwip_close(socket_fd);


}


void httpd_init() {
    osThreadDef(httpdTask, httpd_task, osPriorityNormal, 0, 256);
    httpdTaskHandle = osThreadCreate(osThread(httpdTask), NULL);
}

