#define FN_ZFS_CREATE_MUTEX(pMutex) pGlobalBlock->pCommonBlock->fnKeInitializeMutex(pMutex, FALSE)
#define FN_ZFS_LOCK_MUTEX(pMutex) pGlobalBlock->pCommonBlock->fnKeWaitForSingleObject(pMutex, Executive, KernelMode, FALSE, NULL)
#define FN_ZFS_UNLOCK_MUTEX(pMutex) pGlobalBlock->pCommonBlock->fnKeReleaseMutex(pMutex, FALSE);
#define FN_ZFS_DESTROY_MUTEX(pMutex)
#define FN_ZFS_THREAD_YIELD()

#include "../../../zfs/zfs/zfs.c"
