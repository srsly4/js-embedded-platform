#include <string.h>
#include <stdlib.h>
#include "platform/queue.h"

static void prvCopyDataToQueue( Queue_t * const pxQueue, const void *pvItemToQueue )
{
    UBaseType_t uxMessagesWaiting;

    /* This function is called from a critical section. */

    uxMessagesWaiting = pxQueue->uxMessagesWaiting;

    memcpy( ( void * ) pxQueue->pcWriteTo, pvItemToQueue, ( size_t ) pxQueue->uxItemSize ); /*lint !e961 !e418 MISRA exception as the casts are only redundant for some ports, plus previous logic ensures a null pointer can only be passed to memcpy() if the copy size is 0. */
    pxQueue->pcWriteTo += pxQueue->uxItemSize;
    if( pxQueue->pcWriteTo >= pxQueue->pcTail ) /*lint !e946 MISRA exception justified as comparison of pointers is the cleanest solution. */
    {
        pxQueue->pcWriteTo = pxQueue->pcHead;
    }

    pxQueue->uxMessagesWaiting = uxMessagesWaiting + 1;
}
/*-----------------------------------------------------------*/

static void prvCopyDataFromQueue( Queue_t * const pxQueue, void * const pvBuffer )
{
    if( pxQueue->uxItemSize != ( UBaseType_t ) 0 )
    {
        pxQueue->pcReadFrom += pxQueue->uxItemSize;
        if( pxQueue->pcReadFrom >= pxQueue->pcTail ) /*lint !e946 MISRA exception justified as use of the relational operator is the cleanest solutions. */
        {
            pxQueue->pcReadFrom = pxQueue->pcHead;
        }
        memcpy( ( void * ) pvBuffer, ( void * ) pxQueue->pcReadFrom, ( size_t ) pxQueue->uxItemSize ); /*lint !e961 !e418 MISRA exception as the casts are only redundant for some ports.  Also previous logic ensures a null pointer can only be passed to memcpy() when the count is 0. */
    }
}

static void prvInitialiseNewQueue( const UBaseType_t uxQueueLength, const UBaseType_t uxItemSize, uint8_t *pucQueueStorage, Queue_t *pxNewQueue )
{
    if( uxItemSize == ( UBaseType_t ) 0 )
    {
        /* No RAM was allocated for the queue storage area, but PC head cannot
        be set to NULL because NULL is used as a key to say the queue is used as
        a mutex.  Therefore just set pcHead to point to the queue as a benign
        value that is known to be within the memory map. */
        pxNewQueue->pcHead = ( int8_t * ) pxNewQueue;
    }
    else
    {
        /* Set the head to the start of the queue storage area. */
        pxNewQueue->pcHead = ( int8_t * ) pucQueueStorage;
    }

    /* Initialise the queue members as described where the queue type is
    defined. */
    pxNewQueue->locked = pdFALSE;
    pxNewQueue->uxLength = uxQueueLength;
    pxNewQueue->uxItemSize = uxItemSize;
    pxNewQueue->pcTail = pxNewQueue->pcHead + ( uxQueueLength * uxItemSize );
    pxNewQueue->uxMessagesWaiting = ( UBaseType_t ) 0U;
    pxNewQueue->pcWriteTo = pxNewQueue->pcHead;
    pxNewQueue->pcReadFrom = pxNewQueue->pcHead + ( ( uxQueueLength - ( UBaseType_t ) 1U ) * uxItemSize );

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_ROBUST);

    pthread_mutex_init(&pxNewQueue->mutex, &attr);
    pthread_cond_init(&pxNewQueue->cond, NULL);
}

QueueHandle_t xQueueCreate(const UBaseType_t uxQueueLength, const UBaseType_t uxItemSize)
{
    Queue_t *pxNewQueue;
    size_t xQueueSizeInBytes;
    uint8_t *pucQueueStorage;

    if (uxQueueLength <= 0) return NULL;

    if( uxItemSize == ( UBaseType_t ) 0 )
    {
        xQueueSizeInBytes = ( size_t ) 0;
    }
    else
    {
        xQueueSizeInBytes = ( size_t ) ( uxQueueLength * uxItemSize );
    }

    pxNewQueue = ( Queue_t * ) malloc( sizeof( Queue_t ) + xQueueSizeInBytes );

    if( pxNewQueue != NULL )
    {
        /* Jump past the queue structure to find the location of the queue
        storage area. */
        pucQueueStorage = ( ( uint8_t * ) pxNewQueue ) + sizeof( Queue_t );

        prvInitialiseNewQueue( uxQueueLength, uxItemSize, pucQueueStorage, pxNewQueue );
    }

    return pxNewQueue;
}

BaseType_t vQueueDelete(QueueHandle_t xQueue)
{
    Queue_t * pxQueue = ( Queue_t * ) xQueue;
    if (pthread_mutex_destroy(&pxQueue->mutex) || pthread_cond_destroy(&pxQueue->cond))
    {
        return pdFAIL;
    }
    free(pxQueue);
    return pdPASS;
}

BaseType_t xQueueSend(QueueHandle_t xQueue, const void * const pvItemToQueue )
{
    Queue_t * const pxQueue = ( Queue_t * ) xQueue;

    if (xQueue == NULL || (pvItemToQueue == NULL && pxQueue->uxItemSize != ( UBaseType_t ) 0U ))
        return pdFAIL;

    if (pthread_mutex_lock(&pxQueue->mutex))
        return pdFAIL;

    if (pxQueue->locked == pdTRUE) {
        pthread_cond_broadcast(&pxQueue->cond);
        pthread_mutex_unlock(&pxQueue->mutex);
        return pdFAIL;
    }
    while(pxQueue->uxMessagesWaiting >= pxQueue->uxLength)
    {
        if (pthread_cond_wait(&pxQueue->cond, &pxQueue->mutex))
            return pdFAIL;
        if (pxQueue->locked == pdTRUE) {
            pthread_cond_broadcast(&pxQueue->cond);
            pthread_mutex_unlock(&pxQueue->mutex);
            return pdFAIL;
        }
    }

    prvCopyDataToQueue(pxQueue, pvItemToQueue);
    pthread_cond_broadcast(&pxQueue->cond);
    pthread_mutex_unlock(&pxQueue->mutex);
    return pdPASS;
}


BaseType_t xQueueReceive( QueueHandle_t xQueue, void * const pvBuffer, const BaseType_t xJustPeeking )
{
    int8_t *pcOriginalReadPosition;
    Queue_t * const pxQueue = ( Queue_t * ) xQueue;

    if (pxQueue == NULL || (pvBuffer == NULL && pxQueue->uxItemSize != ( UBaseType_t ) 0U ))
        return pdFAIL;

    if (pthread_mutex_lock(&pxQueue->mutex))
        return pdFAIL;
    if (pxQueue->locked == pdTRUE) {
        pthread_cond_broadcast(&pxQueue->cond);
        pthread_mutex_unlock(&pxQueue->mutex);
        return pdFAIL;
    }
    while(pxQueue->uxMessagesWaiting <= 0)
    {
        if (pthread_cond_wait(&pxQueue->cond, &pxQueue->mutex))
            return pdFAIL;
        if (pxQueue->locked == pdTRUE) {
            pthread_cond_broadcast(&pxQueue->cond);
            pthread_mutex_unlock(&pxQueue->mutex);
            return pdFAIL;
        }
    }

    pcOriginalReadPosition = pxQueue->pcReadFrom;
    prvCopyDataFromQueue( pxQueue, pvBuffer );
    if( xJustPeeking == pdTRUE )
    {
        pxQueue->pcReadFrom = pcOriginalReadPosition;
    }
    else
    {
        pxQueue->uxMessagesWaiting--;
    }
    pthread_cond_broadcast(&pxQueue->cond);
    pthread_mutex_unlock(&pxQueue->mutex);
    return pdPASS;
}