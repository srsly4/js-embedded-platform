#include "firmware_downloader.h"

#include <platform/firmware.h>
#include <platform/sockets.h>
#include <platform/memory.h>
#include <string.h>
#include <platform/eventloop.h>
#include <platform/debug.h>

#define FIRMWARE_DOWNLOADER_URI_PROTOCOL "firm://"
#define FIRMWARE_DOWNLOADER_CHUNK_SIZE 512
#define FIRMWARE_DOWNLOADER_CHUNK_MULTIPLIER 8
#define FIRMWARE_DOWNLOADER_BUFFER_SIZE FIRMWARE_DOWNLOADER_CHUNK_SIZE+16

#define PAYLOAD_TYPE_REQUEST (uint8_t)0x04
#define PAYLOAD_TYPE_DESCRIPTION (uint8_t)0x05
#define PAYLOAD_TYPE_FLOW_START (uint8_t)0xA0
#define PAYLOAD_TYPE_FLOW_END (uint8_t)0xA3
#define PAYLOAD_TYPE_CHUNK (uint8_t)0xA1
#define PAYLOAD_TYPE_CHUNK_ACK (uint8_t)0xA2

struct __packed payload_chunk {
    uint8_t id;
    uint16_t chunk_id;
    uint32_t length;
    char data[0];
};

struct __packed payload_chunk_ack {
    uint8_t id;
    uint16_t chunk_id;
};

struct __packed payload_description {
    uint8_t id;
    uint16_t chunk_num;
    uint8_t name_len;
    char name[0];
};

struct __packed payload_request {
    uint8_t id;
    uint8_t chunk_size;
};

union payload {
    uint8_t id;
    struct payload_request request;
    struct payload_description description;
    struct payload_chunk chunk;
    struct payload_chunk_ack chunk_ack;
};

uint8_t firmware_downloader_start(const char *uri) {
    kill_eventloop_thread();
    debug_platform_error_led_off();

    uint8_t res = FIRMWARE_DOWNLOADER_SUCC;
    char *buff = NULL;

    if (strncmp(uri, FIRMWARE_DOWNLOADER_URI_PROTOCOL, sizeof(FIRMWARE_DOWNLOADER_URI_PROTOCOL) - 1) != 0) {
        return FIRMWARE_DOWNLOADER_ERR_BAD_URI;
    }

    char ip[20] = "";
    int port = 0;

    sscanf(uri, "firm://%99[^:]:%99d", ip, &port);

    if (port == 0 || ip[0] == '\0') {
        return FIRMWARE_DOWNLOADER_ERR_BAD_URI;
    }

    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        return FIRMWARE_DOWNLOADER_ERR_CONN;
    }

    struct sockaddr_in server;
    server.sin_addr.s_addr = inet_addr(ip);
    server.sin_family = AF_INET;
    server.sin_port = htons((uint16_t) port);

    if (connect(socket_fd, (struct sockaddr *) &server, sizeof(server)) < 0) {
        return FIRMWARE_DOWNLOADER_ERR_CONN;
    }

    buff = malloc(FIRMWARE_DOWNLOADER_BUFFER_SIZE);
    if (!buff) {
        res = FIRMWARE_DOWNLOADER_ERR_MEM;
        goto cleanup;
    }
    union payload *pay = (union payload *) buff;

    // send request
    pay->id = PAYLOAD_TYPE_REQUEST;
    pay->request.chunk_size = FIRMWARE_DOWNLOADER_CHUNK_SIZE / FIRMWARE_DOWNLOADER_CHUNK_MULTIPLIER;
    if (send(socket_fd, buff, sizeof(struct payload_request), 0) < 0) {
        res = FIRMWARE_DOWNLOADER_ERR_CONN;
        goto cleanup;
    }

    // receive firmware description and start writing firmware
    int recv_len = 0;
    do {
        if ((recv_len = recv(socket_fd, buff, FIRMWARE_DOWNLOADER_BUFFER_SIZE, 0)) <= 0) {
            res = FIRMWARE_DOWNLOADER_ERR_CONN;
            goto cleanup;
        }
    } while (recv_len < 2 || pay->id != PAYLOAD_TYPE_DESCRIPTION);

    // check if firmware will fit into the memory
    uint16_t chunk_count = pay->description.chunk_num;
    if (((uint32_t)chunk_count) * FIRMWARE_DOWNLOADER_CHUNK_SIZE > firmware_platform_max_memory()) {
        res = FIRMWARE_DOWNLOADER_ERR_MEM;
        goto cleanup;
    }

    char *name = pay->description.name;
    if (firmware_platform_write_start(name) != FIRMWARE_SUCC) {
        res = FIRMWARE_DOWNLOADER_ERR_WRITE;
        goto cleanup;
    }

    // send flow start
    pay->id = PAYLOAD_TYPE_FLOW_START;
    if (send(socket_fd, buff, 1, 0) < 0) {
        res = FIRMWARE_DOWNLOADER_ERR_CONN;
        goto cleanup;
    }

    // receive chunks
    uint8_t is_end = 0;
    uint16_t curr_chunk = 0;
    uint32_t offset = 0;
    while (!is_end) {
        if ((recv_len = recv(socket_fd, buff, FIRMWARE_DOWNLOADER_BUFFER_SIZE, 0)) <= 0) {
            res = FIRMWARE_DOWNLOADER_ERR_CONN;
            goto cleanup;
        }
        if (recv_len > 0 && pay->id == PAYLOAD_TYPE_FLOW_END) {
            is_end = 1;
            continue;
        }

        if (recv_len > 2 && pay->id == PAYLOAD_TYPE_CHUNK) {
            curr_chunk = pay->chunk.chunk_id;
            uint32_t chunk_len = pay->chunk.length;
            char *chunk_ptr = pay->chunk.data;

            // write chunk
            if (firmware_platform_write_code_chunk(
                    chunk_ptr,
                    (uint32_t) (curr_chunk * FIRMWARE_DOWNLOADER_CHUNK_SIZE),
                    chunk_len) != FIRMWARE_SUCC) {
                res = FIRMWARE_DOWNLOADER_ERR_WRITE;
                goto cleanup;
            }

            // confirm receiving and writing of chunk
            pay->id = PAYLOAD_TYPE_CHUNK_ACK;
            pay->chunk_ack.chunk_id = curr_chunk;
            if (send(socket_fd, buff, sizeof(struct payload_chunk_ack), 0) < 0) {
                res = FIRMWARE_DOWNLOADER_ERR_CONN;
                goto cleanup;
            }
        }
    }

    if (firmware_platform_write_finish() != FIRMWARE_SUCC) {
        res = FIRMWARE_WRITE_ERR;
        goto cleanup;
    }

    cleanup:
    close(socket_fd);
    if (buff) {
        free(buff);
    }

    if (res == FIRMWARE_SUCC) {
        start_eventloop_thread();
    } else {
        debug_platform_error_led_on();
    }

    return res;
}
