#ifndef JS_EMBEDDED_PLATFORM_QUEUE_H
#define JS_EMBEDDED_PLATFORM_QUEUE_H

#include <pthread.h>
#include <stdint.h>

typedef unsigned long UBaseType_t;
typedef long BaseType_t;
struct QueueDefinition;
typedef struct QueueDefinition Queue_t;
typedef Queue_t* QueueHandle_t;

typedef struct QueueDefinition
{
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int8_t *pcHead;					/*< Points to the beginning of the queue storage area. */
    int8_t *pcTail;					/*< Points to the byte at the end of the queue storage area.  Once more byte is allocated than necessary to store the queue items, this is used as a marker. */
    int8_t *pcWriteTo;				/*< Points to the free next place in the storage area. */

    int8_t *pcReadFrom;			/*< Points to the last place that a queued item was read from when the structure is used as a queue. */

    volatile UBaseType_t uxMessagesWaiting;/*< The number of items currently in the queue. */
    volatile UBaseType_t uxTasksWaiting;/*< The number of items currently in the queue. */
    UBaseType_t uxLength;			/*< The length of the queue defined as the number of items it will hold, not the number of bytes. */
    UBaseType_t uxItemSize;			/*< The size of each items that the queue will hold. */
    BaseType_t locked;
} xQueue;

#define pdFALSE			( ( BaseType_t ) 0 )
#define pdTRUE			( ( BaseType_t ) 1 )

#define pdPASS			( pdTRUE )
#define pdFAIL			( pdFALSE )

QueueHandle_t xQueueCreate(UBaseType_t uxQueueLength, UBaseType_t uxItemSize);
BaseType_t vQueueDelete(QueueHandle_t xQueue);
BaseType_t xQueueSend(QueueHandle_t xQueue, const void * pvItemToQueue);
BaseType_t xQueueReceive( QueueHandle_t xQueue, void * pvBuffer, BaseType_t xJustPeeking );


#endif //JS_EMBEDDED_PLATFORM_QUEUE_H
