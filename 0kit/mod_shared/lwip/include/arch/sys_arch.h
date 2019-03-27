#ifndef __SYS_ARCH_H_
#define __SYS_ARCH_H_

#define LWIP_STK_SIZE 65536

#define LWIP_TASK_MAX 5	//max number of lwip tasks
#define LWIP_START_PRIO -1   //first prio of lwip tasks
// 
// typedef struct _KSEMAPHORE {
// 	DISPATCHER_HEADER Header;
// 	LONG Limit;
// } KSEMAPHORE, *PKSEMAPHORE, *PRKSEMAPHORE;

//typedef PVOID HANDLE;
typedef struct _KTHREAD *PKTHREAD, *PRKTHREAD;

typedef struct _KSEMAPHORE sys_sem_t;
typedef struct queue* sys_mbox_t;
typedef PKTHREAD sys_thread_t;
typedef struct _FAST_MUTEX sys_mutex_t;

#define SYS_MBOX_NULL(sys_mbox_t)0
#define SYS_SEM_NULL(sys_sem_t)0

/* Global Critical Region Protection */
#define SYS_ARCH_DECL_PROTECT(x) KIRQL x
#define SYS_ARCH_PROTECT(x) x = pGlobalBlock->pCommonBlock->fnKfAcquireSpinLock(&pGlobalBlock->pTcpipBlock->spinLock)
#define SYS_ARCH_UNPROTECT(x) pGlobalBlock->pCommonBlock->fnKfReleaseSpinLock(&pGlobalBlock->pTcpipBlock->spinLock, x)

#endif // __SYS_ARCH_H_
