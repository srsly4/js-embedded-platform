/*******************************************************************************
 *
 * Copyright (c) 2013, 2014 Intel Corporation and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * The Eclipse Distribution License is available at
 *    http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    David Navarro, Intel Corporation - initial API and implementation
 *    Benjamin Cabé - Please refer to git log
 *    Fabien Fleutot - Please refer to git log
 *    Simon Bernard - Please refer to git log
 *    Julien Vermillard - Please refer to git log
 *    Axel Lorente - Please refer to git log
 *    Toby Jaffey - Please refer to git log
 *    Bosch Software Innovations GmbH - Please refer to git log
 *    Pascal Rieux - Please refer to git log
 *    Christian Renz - Please refer to git log
 *    Ricky Liu - Please refer to git log
 *
 *******************************************************************************/

/*
 Copyright (c) 2013, 2014 Intel Corporation

 Redistribution and use in source and binary forms, with or without modification,
 are permitted provided that the following conditions are met:

     * Redistributions of source code must retain the above copyright notice,
       this list of conditions and the following disclaimer.
     * Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.
     * Neither the name of Intel Corporation nor the names of its contributors
       may be used to endorse or promote products derived from this software
       without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 THE POSSIBILITY OF SUCH DAMAGE.

 David Navarro <david.navarro@intel.com>
 Bosch Software Innovations GmbH - Please refer to git log

*/

#include "lwm2m/client/lwm2mclient.h"
#include "lwm2m/core/liblwm2m.h"
#include "platform/connection.h"

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#define MAX_PACKET_SIZE 1152
#define MAX_DISCOVERY_PACKET_SIZE 256
#define MAX_ENDPOINT_LENGTH 40
#define MAX_HOST_LENGTH 40
#define MAX_PORT_LENGTH 10
#define MAX_FAILURES 10
#define DEFAULT_SERVER_IPV6 "[::1]"
#define DEFAULT_SERVER_IPV4 "127.0.0.1"

int g_reboot = 0;
static int g_quit = 0;
static u_int8_t bootstrap_ready = 0;
static char server_host_buffer[MAX_HOST_LENGTH];
static char server_port_buffer[MAX_PORT_LENGTH];
static char endpoint_buffer[MAX_ENDPOINT_LENGTH];

#define OBJ_COUNT 4
lwm2m_object_t * objArray[OBJ_COUNT];

// only backup security and server objects
# define BACKUP_OBJECT_COUNT 2
lwm2m_object_t * backupObjectArray[BACKUP_OBJECT_COUNT];

typedef struct
{
    lwm2m_object_t * securityObjP;
    lwm2m_object_t * serverObject;
    int sock;
#ifdef WITH_TINYDTLS
    dtls_connection_t * connList;
    lwm2m_context_t * lwm2mH;
#else
    connection_t * connList;
#endif
    int addressFamily;
} client_data_t;


void handle_value_changed(lwm2m_context_t * lwm2mH,
                          lwm2m_uri_t * uri,
                          const char * value,
                          size_t valueLength)
{
    lwm2m_object_t * object = (lwm2m_object_t *)LWM2M_LIST_FIND(lwm2mH->objectList, uri->objectId);

    if (NULL != object)
    {
        if (object->writeFunc != NULL)
        {
            lwm2m_data_t * dataP;
            int result;

            dataP = lwm2m_data_new(1);
            if (dataP == NULL)
            {
                return;
            }
            dataP->id = uri->resourceId;
            lwm2m_data_encode_nstring(value, valueLength, dataP);

            result = object->writeFunc(uri->instanceId, 1, dataP, object);

            if (COAP_204_CHANGED != result)
            {
            }
            else
            {
                lwm2m_resource_value_changed(lwm2mH, uri);
            }
            lwm2m_data_free(1, dataP);
            return;
        }
        else
        {
        }
        return;
    }
    else
    {
    }
}

#ifdef WITH_TINYDTLS
void * lwm2m_connect_server(uint16_t secObjInstID,
                            void * userData)
{
  client_data_t * dataP;
  lwm2m_list_t * instance;
  dtls_connection_t * newConnP = NULL;
  dataP = (client_data_t *)userData;
  lwm2m_object_t  * securityObj = dataP->securityObjP;

  instance = LWM2M_LIST_FIND(dataP->securityObjP->instanceList, secObjInstID);
  if (instance == NULL) return NULL;


  newConnP = connection_create(dataP->connList, dataP->sock, securityObj, instance->id, dataP->lwm2mH, dataP->addressFamily);
  if (newConnP == NULL)
  {
      fprintf(stderr, "Connection creation failed.\n");
      return NULL;
  }

  dataP->connList = newConnP;
  return (void *)newConnP;
}
#else
void * lwm2m_connect_server(uint16_t secObjInstID,
                            void * userData)
{
    client_data_t * dataP;
    char * uri;
    char * host;
    char * port;
    connection_t * newConnP = NULL;

    dataP = (client_data_t *)userData;

    uri = get_server_uri(dataP->securityObjP, secObjInstID);

    if (uri == NULL) return NULL;

    // parse uri in the form "coaps://[host]:[port]"
    if (0==strncmp(uri, "coaps://", strlen("coaps://"))) {
        host = uri+strlen("coaps://");
    }
    else if (0==strncmp(uri, "coap://",  strlen("coap://"))) {
        host = uri+strlen("coap://");
    }
    else {
        goto exit;
    }
    port = strrchr(host, ':');
    if (port == NULL) goto exit;
    // remove brackets
    if (host[0] == '[')
    {
        host++;
        if (*(port - 1) == ']')
        {
            *(port - 1) = 0;
        }
        else goto exit;
    }
    // split strings
    *port = 0;
    port++;

    newConnP = connection_create(dataP->connList, dataP->sock, host, port, dataP->addressFamily);
    if (newConnP == NULL) {
    }
    else {
        dataP->connList = newConnP;
    }

exit:
    lwm2m_free(uri);
    return (void *)newConnP;
}
#endif

void lwm2m_close_connection(void * sessionH,
                            void * userData)
{
    client_data_t * app_data;
#ifdef WITH_TINYDTLS
    dtls_connection_t * targetP;
#else
    connection_t * targetP;
#endif

    app_data = (client_data_t *)userData;
#ifdef WITH_TINYDTLS
    targetP = (dtls_connection_t *)sessionH;
#else
    targetP = (connection_t *)sessionH;
#endif

    if (targetP == app_data->connList)
    {
        app_data->connList = targetP->next;
        lwm2m_free(targetP);
    }
    else
    {
#ifdef WITH_TINYDTLS
        dtls_connection_t * parentP;
#else
        connection_t * parentP;
#endif

        parentP = app_data->connList;
        while (parentP != NULL && parentP->next != targetP)
        {
            parentP = parentP->next;
        }
        if (parentP != NULL)
        {
            parentP->next = targetP->next;
            lwm2m_free(targetP);
        }
    }
}

#ifdef LWM2M_BOOTSTRAP

static void prv_backup_objects(lwm2m_context_t * context)
{
    uint16_t i;

    for (i = 0; i < BACKUP_OBJECT_COUNT; i++) {
        if (NULL != backupObjectArray[i]) {
            switch (backupObjectArray[i]->objID)
            {
                case LWM2M_SECURITY_OBJECT_ID:
                    clean_security_object(backupObjectArray[i]);
                    lwm2m_free(backupObjectArray[i]);
                    break;
                case LWM2M_SERVER_OBJECT_ID:
                    clean_server_object(backupObjectArray[i]);
                    lwm2m_free(backupObjectArray[i]);
                    break;
                default:
                    break;
            }
        }
        backupObjectArray[i] = (lwm2m_object_t *)lwm2m_malloc(sizeof(lwm2m_object_t));
        memset(backupObjectArray[i], 0, sizeof(lwm2m_object_t));
    }

    /*
     * Backup content of objects 0 (security) and 1 (server)
     */
    copy_security_object(backupObjectArray[0], (lwm2m_object_t *)LWM2M_LIST_FIND(context->objectList, LWM2M_SECURITY_OBJECT_ID));
    copy_server_object(backupObjectArray[1], (lwm2m_object_t *)LWM2M_LIST_FIND(context->objectList, LWM2M_SERVER_OBJECT_ID));
}

static void prv_restore_objects(lwm2m_context_t * context)
{
    lwm2m_object_t * targetP;

    /*
     * Restore content  of objects 0 (security) and 1 (server)
     */
    targetP = (lwm2m_object_t *)LWM2M_LIST_FIND(context->objectList, LWM2M_SECURITY_OBJECT_ID);
    // first delete internal content
    clean_security_object(targetP);
    // then restore previous object
    copy_security_object(targetP, backupObjectArray[0]);

    targetP = (lwm2m_object_t *)LWM2M_LIST_FIND(context->objectList, LWM2M_SERVER_OBJECT_ID);
    // first delete internal content
    clean_server_object(targetP);
    // then restore previous object
    copy_server_object(targetP, backupObjectArray[1]);

    // restart the old servers
    fprintf(stdout, "[BOOTSTRAP] ObjectList restored\r\n");
}

static void update_bootstrap_info(lwm2m_client_state_t * previousBootstrapState,
                                  lwm2m_context_t * context)
{
    if (*previousBootstrapState != context->state)
    {
        *previousBootstrapState = context->state;
        switch(context->state)
        {
            case STATE_BOOTSTRAPPING:
#ifdef WITH_LOGS
                fprintf(stdout, "[BOOTSTRAP] backup security and server objects\r\n");
#endif
                prv_backup_objects(context);
                break;
            default:
                break;
        }
    }
}

static void close_backup_object()
{
    int i;
    for (i = 0; i < BACKUP_OBJECT_COUNT; i++) {
        if (NULL != backupObjectArray[i]) {
            switch (backupObjectArray[i]->objID)
            {
                case LWM2M_SECURITY_OBJECT_ID:
                    clean_security_object(backupObjectArray[i]);
                    lwm2m_free(backupObjectArray[i]);
                    break;
                case LWM2M_SERVER_OBJECT_ID:
                    clean_server_object(backupObjectArray[i]);
                    lwm2m_free(backupObjectArray[i]);
                    break;
                default:
                    break;
            }
        }
    }
}
#endif

int init_discovery()
{
    int discoverySocket;
    uint8_t buffer[MAX_DISCOVERY_PACKET_SIZE];
    ssize_t numBytes;

    discoverySocket = create_socket(LWM2M_LOCAL_DISCOVERY_PORT, AF_INET);
    if (discoverySocket < 0)
    {
        return -1;
    }

    while(g_quit == 0)
    {
        struct sockaddr_storage addr;
        socklen_t addrLen;
        addrLen = sizeof(addr);

        /*
         * We retrieve the data received
        */
        numBytes = recvfrom(discoverySocket, buffer, MAX_PACKET_SIZE, 0, (struct sockaddr *) &addr, &addrLen);

        if (0 > numBytes)
        {
        }
        else if (0 < numBytes)
        {
            if (numBytes > 3 && buffer[0] == 'E'
                             && buffer[1] == 'P'
                             && buffer[2] == ':')
            {
                memset(endpoint_buffer, 0, MAX_ENDPOINT_LENGTH);
                if (sscanf((const char*) buffer, "EP:%39c", endpoint_buffer) == 1)
                {
                    bootstrap_ready |= ENDPOINT_SET;
                }
                else
                {
                    bootstrap_ready &= ~ENDPOINT_SET;
                }
            }
            else if (numBytes > 4 && buffer[0] == 'U'
                                  && buffer[1] == 'R'
                                  && buffer[2] == 'I'
                                  && buffer[3] == ':')
            {
                memset(server_host_buffer, 0, MAX_HOST_LENGTH);
                memset(server_port_buffer, 0, MAX_PORT_LENGTH);
                if (sscanf((const char*) buffer, "URI:coap://%39[0-9.]:%9[0-9]", server_host_buffer, server_port_buffer) == 2)
                {
                    bootstrap_ready |= BOOTSTRAP_URI_SET;
                }
                else
                {
                    bootstrap_ready &= ~BOOTSTRAP_URI_SET;
                }
            }
            memset(buffer, 0, MAX_DISCOVERY_PACKET_SIZE);
        }
        else
        {
            sendto(discoverySocket, "", 0, 0, (struct sockaddr *) &addr, addrLen);
        }
    }
}


int init_lwm2m()
{
    client_data_t data;
    int result;
    short unavailableTimes = 0;
    lwm2m_context_t * lwm2mH = NULL;
    const char * server = "192.168.223.105";
    const char * serverPort = LWM2M_BSSERVER_PORT_STR;
    char endpoint[MAX_ENDPOINT_LENGTH] = "jsembedded";

#ifdef LWM2M_BOOTSTRAP
    lwm2m_client_state_t previousState = STATE_INITIAL;
#endif

    char * pskId = NULL;
    uint16_t pskLen = -1;
    char * pskBuffer = NULL;

    memset(&data, 0, sizeof(client_data_t));
    data.addressFamily = AF_INET;

    if (!server)
    {
        server = (AF_INET == data.addressFamily ? DEFAULT_SERVER_IPV4 : DEFAULT_SERVER_IPV6);
    }

    data.sock = create_socket(LWM2M_LOCAL_PORT, data.addressFamily);
    if (data.sock < 0)
    {
        return -1;
    }


    /*
     * Now the main function fill an array with each object, this list will be later passed to liblwm2m.
     * Those functions are located in their respective object file.
     */
#ifdef WITH_TINYDTLS
    if (psk != NULL)
    {
        pskLen = strlen(psk) / 2;
        pskBuffer = malloc(pskLen);

        if (NULL == pskBuffer)
        {
            fprintf(stderr, "Failed to create PSK binary buffer\r\n");
            return -1;
        }
        // Hex string to binary
        char *h = psk;
        char *b = pskBuffer;
        char xlate[] = "0123456789ABCDEF";

        for ( ; *h; h += 2, ++b)
        {
            char *l = strchr(xlate, toupper(*h));
            char *r = strchr(xlate, toupper(*(h+1)));

            if (!r || !l)
            {
                fprintf(stderr, "Failed to parse Pre-Shared-Key HEXSTRING\r\n");
                return -1;
            }

            *b = ((l - xlate) << 4) + (r - xlate);
        }
    }
#endif

    char serverUri[50];
    int serverId = 123;
#ifdef WITH_TINYDTLS
    sprintf (serverUri, "coaps://%s:%s", server, serverPort);
#else
    sprintf (serverUri, "coap://%s:%s", server, serverPort);
#endif
#ifdef LWM2M_BOOTSTRAP
    objArray[0] = get_security_object(serverId, serverUri, pskId, pskBuffer, pskLen, true);
#else
    objArray[0] = get_security_object(serverId, serverUri, pskId, pskBuffer, pskLen, false);
#endif
    if (NULL == objArray[0])
    {
        return -1;
    }
    data.securityObjP = objArray[0];

    objArray[1] = get_server_object(serverId, "U", 300, false);
    if (NULL == objArray[1])
    {
        return -1;
    }

    objArray[2] = get_object_device();
    if (NULL == objArray[2])
    {
        return -1;
    }

    objArray[3] = get_object_firmware();
    if (NULL == objArray[1])
    {
        return -1;
    }

    /*
     * The liblwm2m library is now initialized with the functions that will be in
     * charge of communication
     */
    lwm2mH = lwm2m_init(&data);
    if (NULL == lwm2mH)
    {
        return -1;
    }

#ifdef WITH_TINYDTLS
    data.lwm2mH = lwm2mH;
#endif

    /*
     * We configure the liblwm2m library with the name of the client - which shall be unique for each client -
     * the number of objects we will be passing through and the objects array
     */
    result = lwm2m_configure(lwm2mH, endpoint, NULL, NULL, OBJ_COUNT, objArray);
    if (result != 0)
    {
        return -1;
    }

    /**
     * Initialize value changed callback.
     */
    init_value_change(lwm2mH);

    /*
     * As you now have your lwm2m context complete you can pass it as an argument to all the command line functions
     * precedently viewed (first point)
     */
    while (0 == g_quit)
    {
        update_memory_free(objArray[2]);
        struct timeval tv;
        fd_set readfds;

#ifdef LWM2M_BOOTSTRAP
        if (bootstrap_ready == BOOTSTRAP_READY) {
            sprintf(serverUri, "coap://%s:%s", server_host_buffer, server_port_buffer);
            strcpy(endpoint, endpoint_buffer);

            clean_security_object(objArray[0]);
            clean_server_object(objArray[1]);

            if (
                    create_bootstrap_security_instance(objArray[0], serverUri) != -1 &&
                    create_bootstrap_server_instance(objArray[1]) != -1)
            {
                lwm2m_close(lwm2mH);
                unavailableTimes = 0;
                lwm2mH = lwm2m_init(&data);
                lwm2m_configure(lwm2mH, endpoint, NULL, NULL, OBJ_COUNT, objArray);
            }
            bootstrap_ready = 0;
        }
#endif
//        if (g_reboot)
//        {
//            time_t tv_sec;
//
//            tv_sec = lwm2m_gettime();
//
//            if (0 == reboot_time)
//            {
//                reboot_time = tv_sec + 5;
//            }
//            if (reboot_time < tv_sec)
//            {
//                /*
//                 * Message should normally be lost with reboot ...
//                 */
//                fprintf(stderr, "reboot time expired, rebooting ...");
//                system_reboot();
//            }
//            else
//            {
//                tv.tv_sec = reboot_time - tv_sec;
//            }
//        }
//        else
        {
            tv.tv_sec = 60;
        }
        tv.tv_usec = 0;

        FD_ZERO(&readfds);
        FD_SET(data.sock, &readfds);

        /*
         * This function does two things:
         *  - first it does the work needed by liblwm2m (eg. (re)sending some packets).
         *  - Secondly it adjusts the timeout value (default 60s) depending on the state of the transaction
         *    (eg. retransmission) and the time between the next operation
         */
        result = lwm2m_step(lwm2mH, &(tv.tv_sec));

        if (result == COAP_503_SERVICE_UNAVAILABLE)
        {
            if (++unavailableTimes >= MAX_FAILURES) {
                lwm2mH->state = STATE_INITIAL;
                lwm2m_server_t * bs_server = lwm2mH->bootstrapServerList;
                while (bs_server != NULL) {
                    bs_server->dirty = true;
                    bs_server = bs_server->next;
                }
            }
        }
        else
        {
            unavailableTimes = 0;
        }

#ifdef LWM2M_BOOTSTRAP
        update_bootstrap_info(&previousState, lwm2mH);
#endif
        /*
         * This part will set up an interruption until an event happen on SDTIN or the socket until "tv" timed out (set
         * with the precedent function)
         */
        result = select(FD_SETSIZE, &readfds, NULL, NULL, &tv);

        if (result < 0)
        {
            if (errno != EINTR)
            {
            }
        }
        else if (result > 0)
        {
            uint8_t buffer[MAX_PACKET_SIZE];
            int numBytes;

            /*
             * If an event happens on the socket
             */
            if (FD_ISSET(data.sock, &readfds))
            {
                struct sockaddr_storage addr;
                socklen_t addrLen;

                addrLen = sizeof(addr);

                /*
                 * We retrieve the data received
                 */
                numBytes = recvfrom(data.sock, buffer, MAX_PACKET_SIZE, 0, (struct sockaddr *)&addr, &addrLen);

                if (0 > numBytes)
                {
                }
                else if (0 < numBytes)
                {
                    char s[INET_ADDRSTRLEN];

#ifdef WITH_TINYDTLS
                    dtls_connection_t * connP;
#else
                    connection_t * connP;
#endif
                    if (AF_INET == addr.ss_family)
                    {
                        struct sockaddr_in *saddr = (struct sockaddr_in *)&addr;
                        inet_ntop(saddr->sin_family, &saddr->sin_addr, s, INET_ADDRSTRLEN);
                    }


                    connP = connection_find(data.connList, &addr, addrLen);
                    if (connP != NULL)
                    {
                        /*
                         * Let liblwm2m respond to the query depending on the context
                         */
#ifdef WITH_TINYDTLS
                        int result = connection_handle_packet(connP, buffer, numBytes);
                        if (0 != result)
                        {
                             printf("error handling message %d\n",result);
                        }
#else
                        lwm2m_handle_packet(lwm2mH, buffer, numBytes, connP);
#endif
                    }
                    else
                    {
                    }
                }
            }
        }
    }

    /*
     * Finally when the loop is left smoothly - asked by user in the command line interface - we unregister our client from it
     */
    if (g_quit == 1)
    {
#ifdef WITH_TINYDTLS
        free(pskBuffer);
#endif

#ifdef LWM2M_BOOTSTRAP
        close_backup_object();
#endif
        lwm2m_close(lwm2mH);
    }
    close(data.sock);
    connection_free(data.connList);

    clean_security_object(objArray[0]);
    lwm2m_free(objArray[0]);
    clean_server_object(objArray[1]);
    lwm2m_free(objArray[1]);
    free_object_device(objArray[2]);
    free_object_firmware(objArray[3]);

#ifdef MEMORY_TRACE
    if (g_quit == 1)
    {
        trace_print(0, 1);
    }
#endif

    return 0;
}
