
#ifndef FN_ZFS_CREATE_MUTEX 

void FN_ZFS_CREATE_MUTEX(void* pMutex)
{
    HANDLE hMutex = CreateMutex(NULL, FALSE, NULL);
    *(HANDLE*)pMutex = hMutex;
}

#endif // FN_ZFS_CREATE_MUTEX 

#ifndef FN_ZFS_LOCK_MUTEX
#define FN_ZFS_LOCK_MUTEX(pMutex) WaitForSingleObject(*(HANDLE*)pMutex, INFINITE)
#endif // FN_ZFS_LOCK_MUTEX

#ifndef FN_ZFS_UNLOCK_MUTEX
#define FN_ZFS_UNLOCK_MUTEX(pMutex) ReleaseMutex(*(HANDLE*)pMutex);
#endif // FN_ZFS_UNLOCK_MUTEX

#ifndef FN_ZFS_DESTROY_MUTEX
#define FN_ZFS_DESTROY_MUTEX(pMutex) CloseHandle(*(HANDLE*)pMutex);
#endif // FN_ZFS_DESTROY_MUTEX

#ifndef FN_ZFS_THREAD_YIELD
#define FN_ZFS_THREAD_YIELD() SwitchToThread();
#endif // FN_ZFS_THREAD_YIELD


#ifndef FN_ZFS_GET_SYSTEM_TIME

/*
 * Number of 100 nanosecond units from 1/1/1601 to 1/1/1970
 */
#define EPOCH_BIAS  116444736000000000i64

#define _MAX__TIME32_T     0x7fffd27f           /* number of seconds from
                                                   00:00:00, 01/01/1970 UTC to
                                                   23:59:59, 01/18/2038 UTC */

/*
 * Union to facilitate converting from FILETIME to unsigned __int64
 */
typedef union {
    unsigned __int64 ft_scalar;
    FILETIME ft_struct;
} FT;

uint32_t FN_ZFS_GET_SYSTEM_TIME()
{
    SYSTEMTIME st;
    __time64_t unixTime;
    FT nt_time;

    GetLocalTime(&st);
    SystemTimeToFileTime(&st, &nt_time.ft_struct);

    //GetSystemTimeAsFileTime(&nt_time.ft_struct);

    unixTime = (__time64_t)((nt_time.ft_scalar - EPOCH_BIAS) / 10000000i64);

    if (unixTime > (__time64_t)(_MAX__TIME32_T)) {
        unixTime = (__time64_t)-1;
    }

    return (uint32_t)unixTime;
}

#endif // FN_ZFS_GET_SYSTEM_TIME
