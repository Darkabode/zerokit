#ifndef __QUEUE_H_
#define __QUEUE_H_

#include "lwip/sys.h"

BOOLEAN queue_push(queue_t* q, VOID* msg);
void* queue_pop(queue_t* q, uint32_t timeout, PLARGE_INTEGER tmWaited);
queue_t* queue_create();
void queue_free(queue_t* q);

#endif // __QUEUE_H_
