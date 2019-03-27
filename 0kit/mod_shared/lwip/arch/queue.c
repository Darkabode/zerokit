#include "lwip/mem.h"
#include "arch/sys_arch.h"
#include "arch/queue.h"

BOOLEAN queue_push(queue_t* q, VOID* msg)
{
	queue_node_t* node;
	SYS_ARCH_DECL_PROTECT(old_level);
	USE_GLOBAL_BLOCK

	node = (queue_node_t*)memp_malloc(MEMP_QUEUE_POOL);
	if (node != NULL) {
		node->msg = msg; 
		node->next = NULL; 

		SYS_ARCH_PROTECT(old_level);
		if (q->head == NULL) { 
			q->head = q->tail = node;
		} 
		else {
			q->tail->next = node;
			q->tail = node;
		}
		q->enqueue += 1;
		SYS_ARCH_UNPROTECT(old_level);
		return TRUE;
	}
	return FALSE;
}

void* queue_pop(queue_t* q, uint32_t timeout, PLARGE_INTEGER tmWaited)
{
	PVOID msg;
	queue_node_t* node;
	NTSTATUS ntStatus;
	LARGE_INTEGER delay;
	LARGE_INTEGER maxDelay;
	SYS_ARCH_DECL_PROTECT(old_level);
	pmod_tcpip_block_t pTcpipBlock;
	USE_GLOBAL_BLOCK

	pTcpipBlock = pGlobalBlock->pTcpipBlock;

#ifdef _WIN64
	maxDelay.QuadPart = -(10000I64 * (INT64)timeout);
#else
	maxDelay.QuadPart = -pGlobalBlock->pCommonBlock->fn_allmul(10000I64, (INT64)timeout);
#endif
	delay.QuadPart = -300000I64;  // 0.03 секунды
	tmWaited->QuadPart = 0;
    if (maxDelay.QuadPart == 0I64) {
        maxDelay.QuadPart = -30000000I64; // 3 секунду
    }

    if (q == NULL) {
		return NULL;
    }

	if (maxDelay.QuadPart != 0I64) {
		while (q->head == NULL) {
            if (tmWaited->QuadPart <= maxDelay.QuadPart) {
                break;
            }
			pGlobalBlock->pCommonBlock->fnKeDelayExecutionThread(KernelMode, FALSE, &delay);
			tmWaited->QuadPart += delay.QuadPart;
		}
	}

    if (q->head == NULL) {
		return NULL;
    }

	node = q->head;
	msg = node->msg;

	SYS_ARCH_PROTECT(old_level);
	q->head = node->next;
    if (q->head == NULL) {
		q->tail = q->head;
    }
	q->dequeue += 1;
	SYS_ARCH_UNPROTECT(old_level);
	memp_free(MEMP_QUEUE_POOL, node);

	node = NULL;
	return msg;
}

queue_t* queue_create()
{
	queue_t* q = NULL;
	pmod_common_block_t pCommonBlock;
	USE_GLOBAL_BLOCK

	pCommonBlock = pGlobalBlock->pCommonBlock;

	pCommonBlock->fncommon_allocate_memory(pCommonBlock, &q, sizeof(queue_t), NonPagedPool);
	q->head = q->tail = NULL;
	q->enqueue = q->dequeue = 0;
	return q;
}

void queue_free(queue_t* q)
{
  queue_node_t* node;
  queue_node_t* next;
  SYS_ARCH_DECL_PROTECT(old_level);
  USE_GLOBAL_BLOCK

  node = q->head;

  SYS_ARCH_PROTECT(old_level);
  while(node != NULL) {
    next = node->next;
	memp_free(MEMP_QUEUE_POOL, node);
    node = next;
  }
  pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(q, LOADER_TAG);
  q = NULL;
  SYS_ARCH_UNPROTECT(old_level);
}
