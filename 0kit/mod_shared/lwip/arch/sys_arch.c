//#include "lwip/def.h"
#include "lwip/sys.h"
#include "lwip/mem.h"

#include "arch/sys_arch.h"
#include "arch/queue.h"

//KSPIN_LOCK gNetworkSpinLock;

/*----------------------------------------------------------------------*/
err_t sys_sem_new(sys_sem_t *sem, UINT8 count)
{
	USE_GLOBAL_BLOCK

	pGlobalBlock->pCommonBlock->fnKeInitializeSemaphore(sem, count, MAXLONG);
	return ERR_OK;
}

void sys_sem_set_invalid(sys_sem_t *sem)
{
	sem->Limit = -1111111;
}

int sys_sem_valid(sys_sem_t *sem)
{
	return sem->Limit != -1111111;
}

/*----------------------------------------------------------------------*/
#if 0
void sys_sem_free(sys_sem_t* sem)
{
}
#endif // #if 0

/*----------------------------------------------------------------------*/
void sys_sem_signal(sys_sem_t* sem)
{
	USE_GLOBAL_BLOCK

	pGlobalBlock->pCommonBlock->fnKeReleaseSemaphore(sem, 0, 1, FALSE);
}


/*----------------------------------------------------------------------*/
uint32_t sys_arch_sem_wait(sys_sem_t* sem, uint32_t timeout)
{
	NTSTATUS ntStatus;
	LARGE_INTEGER li;
	USE_GLOBAL_BLOCK

#ifdef _WIN64
	li.QuadPart = -(10000I64 * (INT64)timeout);
#else
	li.QuadPart = -pGlobalBlock->pCommonBlock->fn_allmul(10000I64, (INT64)timeout);
#endif

	ntStatus = pGlobalBlock->pCommonBlock->fnKeWaitForSingleObject(sem, Executive, KernelMode, FALSE, (timeout != 0) ? &li : NULL);
    if (ntStatus == STATUS_SUCCESS) {
		return ERR_OK;
    }
	
	return SYS_ARCH_TIMEOUT;
// 	switch (ntStatus) {
// 		case STATUS_SUCCESS: 
// 			return ERR_OK;
// 		case STATUS_ABANDONED_WAIT_0:
// 		case STATUS_TIMEOUT: 
// 		default:
// 			return SYS_ARCH_TIMEOUT;
// 	}
}


/*----------------------------------------------------------------------*/
void sys_mbox_new(sys_mbox_t* gMBox, int size)
{
	pmod_tcpip_block_t pTcpipBlock;
	USE_GLOBAL_BLOCK

	pTcpipBlock = pGlobalBlock->pTcpipBlock;

	*gMBox = pTcpipBlock->fnqueue_create();
}

/*----------------------------------------------------------------------*/
void sys_mbox_free(sys_mbox_t* gMBox)
{
	USE_GLOBAL_BLOCK

	pGlobalBlock->pTcpipBlock->fnqueue_free(*gMBox);
}

void sys_mbox_set_invalid(sys_mbox_t *mbox)
{
	*mbox = NULL;
}

int sys_mbox_valid(sys_mbox_t *mbox)
{
	return *mbox != NULL;
}


err_t sys_mbox_trypost(sys_mbox_t* mbox, VOID *msg) 
{
	USE_GLOBAL_BLOCK

    if (pGlobalBlock->pTcpipBlock->fnqueue_push(*mbox, msg)) {
		return ERR_OK;
    }

	return ERR_MEM;
}

/*----------------------------------------------------------------------*/
uint32_t sys_arch_mbox_fetch(sys_mbox_t* mbox, VOID **msg, uint32_t timeout)
{
	LARGE_INTEGER tmWaited;
	USE_GLOBAL_BLOCK

	*msg = pGlobalBlock->pTcpipBlock->fnqueue_pop(*mbox, timeout, &tmWaited);
    if (*msg == NULL) {
		return SYS_ARCH_TIMEOUT;
    }

#ifdef _WIN64
	tmWaited.QuadPart = -(tmWaited.QuadPart / 10000I64);
#else
	tmWaited.QuadPart = -pGlobalBlock->pCommonBlock->fn_alldiv(tmWaited.QuadPart, 10000I64);
#endif
	return tmWaited.LowPart;
}

// sys_thread_t sys_thread_new(const char *name, lwip_thread_fn thread, VOID *arg, int stacksize, int prio)
// {
// 	NTSTATUS ntStatus;
// 	sys_thread_t hThread = NULL;
// 	PKTHREAD pkThread;
// 	OBJECT_ATTRIBUTES fObjectAttributes;
// 
// 	if (KeGetCurrentIrql() == PASSIVE_LEVEL) {
// 		InitializeObjectAttributes(&fObjectAttributes, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
// 
// 		ntStatus = PsCreateSystemThread(&hThread, /*0x001F03FF*/THREAD_ALL_ACCESS, &fObjectAttributes, 0, 0, (PKSTART_ROUTINE)thread, arg);
// 
// 		if (ntStatus != STATUS_SUCCESS)
// 			return NULL;
// 
// 		ntStatus = ObReferenceObjectByHandle(hThread, THREAD_ALL_ACCESS, NULL, KernelMode, (PVOID*)&pkThread, NULL);
// 
// 		if (ntStatus == STATUS_SUCCESS)
// 			KeSetBasePriorityThread(pkThread, (KPRIORITY)prio);
// 
// 		ZwClose(hThread);
// 
// 		return pkThread;
// 	}
// 	return NULL;	
// }
