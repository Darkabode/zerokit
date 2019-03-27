#include <Ntddvol.h>
#include <scsi.h>
#include <ntddscsi.h>
#include "../../../shared/pe.h"

NTSTATUS protector_irp_completion_routine(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN void* Context)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    pcompletion_context_t pContext = (pcompletion_context_t)Context;
    psectors_region_t pHeadRegion, pRegion;
    uint64_t reqSectOffset = pContext->offset;
    ULONG sizeInBytes = pContext->dataSize;
    uint64_t minRight, maxLeft;
    int64_t protSize;
    USE_GLOBAL_BLOCK

    pHeadRegion = &pGlobalBlock->pProtectorBlock->headSectorRegion;
    pRegion = (psectors_region_t)pHeadRegion->Flink;

    while (pRegion != pHeadRegion) {
        minRight = LWIP_MIN(pRegion->startOffset + pRegion->size, reqSectOffset + sizeInBytes);
        maxLeft = LWIP_MAX(pRegion->startOffset, reqSectOffset);
        protSize = (int64_t)(minRight - maxLeft);
        if (protSize > 0&& pRegion->needProtect) {
            pGlobalBlock->pCommonBlock->fnmemcpy(pContext->pData + (maxLeft - reqSectOffset), pRegion->pFakeData + (maxLeft - pRegion->startOffset), (size_t)protSize);
        }

        pRegion = (psectors_region_t)pRegion->Flink;
    }

    if (pContext->isMdlAllocated) {
        pGlobalBlock->pCommonBlock->fnMmUnlockPages(pContext->pMdl);
    }

    if (pContext->origFunc != NULL) {
        ntStatus = pContext->origFunc(DeviceObject, Irp, pContext->pOrigContext);
    }

    pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(pContext, LOADER_TAG);

    return ntStatus; 
}

pcompletion_context_t protector_set_completion_routine(PIRP pIrp, PIO_STACK_LOCATION pIrpStack, uint8_t* pData, uint32_t dataSize, uint64_t dataOffset)
{
    pcompletion_context_t pContext;
    PMDL pMdl = pIrp->MdlAddress;
    IO_STACK_LOCATION* pPrevStack = pIrpStack + 1;
    USE_GLOBAL_BLOCK

    if ((pData == NULL && pMdl == NULL && pIrp->UserBuffer == NULL)) {
        return NULL;
    }

    pGlobalBlock->pCommonBlock->fncommon_allocate_memory(pGlobalBlock->pCommonBlock, &pContext, sizeof(completion_context_t), NonPagedPool);
    if (pIrpStack->CompletionRoutine == NULL) {
        if (pPrevStack->CompletionRoutine != NULL) {
            if (pPrevStack->CompletionRoutine == pGlobalBlock->pProtectorBlock->fnprotector_irp_completion_routine) {
                return NULL;
            }
            pContext->origFunc = pPrevStack->CompletionRoutine;
            pContext->pOrigContext = pPrevStack->Context;
        }
    }
    else {
        if (pIrpStack->CompletionRoutine == pGlobalBlock->pProtectorBlock->fnprotector_irp_completion_routine) {
            return NULL;
        }
        pContext->origFunc = pIrpStack->CompletionRoutine;
        pContext->pOrigContext = pIrpStack->Context;
    }

    pContext->dataSize = dataSize;
    pContext->offset = dataOffset;

    if (pData != NULL) {
        pMdl = pGlobalBlock->pCommonBlock->fnIoAllocateMdl(pData, dataSize, FALSE, FALSE, NULL);
        if (pMdl == NULL) {
            pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(pContext, LOADER_TAG);
            return NULL;
        }
        pGlobalBlock->pCommonBlock->fnMmProbeAndLockPages(pMdl, KernelMode, IoWriteAccess);
        pContext->isMdlAllocated = TRUE;
    }
    else if (pMdl == NULL) { 
        pMdl = pGlobalBlock->pCommonBlock->fnIoAllocateMdl(pIrp->UserBuffer, dataSize, FALSE, FALSE, NULL);
        if (pMdl == NULL) {
            pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(pContext, LOADER_TAG);
            return NULL;
        }
        pGlobalBlock->pCommonBlock->fnMmProbeAndLockPages(pMdl, KernelMode, IoWriteAccess);
        pContext->isMdlAllocated = TRUE;
    }

    pContext->pMdl = pMdl;
    pContext->pData = mmGetSystemAddressForMdl(pMdl);

    if (pIrpStack->CompletionRoutine == NULL) {
        if (pPrevStack->CompletionRoutine != NULL) {
            pPrevStack->CompletionRoutine = pGlobalBlock->pProtectorBlock->fnprotector_irp_completion_routine;
            pPrevStack->Context = pContext;
        }
    }
    else {
        pIrpStack->CompletionRoutine = pGlobalBlock->pProtectorBlock->fnprotector_irp_completion_routine;
        pIrpStack->Context = pContext;
    }

    return pContext;
}

NTSTATUS protector_verify_write_sectors(psectors_region_t pRegion, uint8_t* pData, uint32_t dataSize, uint64_t dataOffset)
{
    uint64_t minRight, maxLeft;
    int64_t protSize;
    USE_GLOBAL_BLOCK

    while (pRegion != &pGlobalBlock->pProtectorBlock->headSectorRegion) {
        minRight = LWIP_MIN(pRegion->startOffset + pRegion->size, dataOffset + dataSize);
        maxLeft = LWIP_MAX(pRegion->startOffset, dataOffset);
        protSize = (int64_t)(minRight - maxLeft);
        if (protSize > 0 && pRegion->needProtect) {
            return STATUS_INSUFFICIENT_RESOURCES;
//             // Копируем новые данные в фейковую область.
//             pGlobalBlock->pCommonBlock->fnmemcpy(pRegion->pFakeData + (maxLeft - pRegion->startOffset), pData + (maxLeft - dataOffset), (size_t)protSize);
// 
//             // Копируем защищаемые данные в бефер записи.
//             pGlobalBlock->pCommonBlock->fnmemcpy(pData + (maxLeft - dataOffset), pRegion->pRealData + (maxLeft - pRegion->startOffset), (size_t)protSize);
        }

        pRegion = (psectors_region_t)pRegion->Flink;
    }

    return STATUS_SUCCESS;
}


NTSTATUS protector_common(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
    PIO_STACK_LOCATION pIrpStack = pIrp->Tail.Overlay.CurrentStackLocation; // IoGetCurrentIrpStackLocation(Irp);
    PSCSI_REQUEST_BLOCK pSrb = NULL;
    PCDB pCdb;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    psectors_region_t pHeadRegion;
    uint8_t* pData;
    uint32_t dataSize;
    uint64_t dataOffset = 0;
    uint8_t opCode;
    PMDL pMdl = pIrp->MdlAddress;
    bool_t isMdlAllocated = FALSE;
    uint32_t bytesPerSector;
    Fnprotector_set_completion_routine fnprotector_set_completion_routine;
    Fnprotector_verify_write_sectors fnprotector_verify_write_sectors;
    USE_GLOBAL_BLOCK

    fnprotector_set_completion_routine = (Fnprotector_set_completion_routine)((uint8_t*)pGlobalBlock->pProtectorBlock->fnprotector_common - ((uint8_t*)protector_common - (uint8_t*)protector_set_completion_routine));
    fnprotector_verify_write_sectors = (Fnprotector_verify_write_sectors)((uint8_t*)pGlobalBlock->pProtectorBlock->fnprotector_common - ((uint8_t*)protector_common - (uint8_t*)protector_verify_write_sectors));

    // Проверяем защищаемые регионы
    pHeadRegion = &pGlobalBlock->pProtectorBlock->headSectorRegion;
    bytesPerSector = pGlobalBlock->pCommonBlock->pBootDiskInfo->bytesPerSector;

    if (pIrpStack->MajorFunction == IRP_MJ_SCSI) {
        pSrb = pIrpStack->Parameters.Scsi.Srb;

        if (pSrb != NULL &&  (pSrb->Function == SRB_FUNCTION_EXECUTE_SCSI || pSrb->Function == 0xC9)) {
            do {
                // Проверяем есть ли у нас защищаемые регионы.
                if (pHeadRegion->Flink == (PLIST_ENTRY)pHeadRegion) {
                    break;
                }

                if (pSrb->Function != 0xC9) {
                    pCdb = (PCDB)pSrb->Cdb;
                    opCode = pCdb->CDB10.OperationCode;

                    if (opCode != SCSIOP_READ && opCode != SCSIOP_WRITE) {
                        break;
                    }

                    ((PFOUR_BYTE)&dataOffset)->Byte3 = pCdb->CDB10.LogicalBlockByte0;
                    ((PFOUR_BYTE)&dataOffset)->Byte2 = pCdb->CDB10.LogicalBlockByte1;
                    ((PFOUR_BYTE)&dataOffset)->Byte1 = pCdb->CDB10.LogicalBlockByte2;
                    ((PFOUR_BYTE)&dataOffset)->Byte0 = pCdb->CDB10.LogicalBlockByte3;
#ifdef _WIN64
                    dataOffset *= bytesPerSector;
#else
                    dataOffset = pGlobalBlock->pCommonBlock->fn_allmul(dataOffset, (uint64_t)bytesPerSector);
#endif // _WIN64
                }
                else {
                    uint8_t* pCommandBlock = (uint8_t*)pSrb->Cdb;

                    if (pCommandBlock[5] & 0x40) {
                        // Режим адресации LBA
                        ((PFOUR_BYTE)&dataOffset)->Byte3 = pCommandBlock[5] & 0x0F;
                        ((PFOUR_BYTE)&dataOffset)->Byte2 = pCommandBlock[4];
                        ((PFOUR_BYTE)&dataOffset)->Byte1 = pCommandBlock[3];
                        ((PFOUR_BYTE)&dataOffset)->Byte0 = pCommandBlock[2];
#ifdef _WIN64
                        dataOffset *= bytesPerSector;
#else
                        dataOffset = pGlobalBlock->pCommonBlock->fn_allmul(dataOffset, (uint64_t)bytesPerSector);
#endif // _WIN64
                    }
                    else {

                    }
                }

                dataSize = pSrb->DataTransferLength;

                if (pCdb->CDB10.OperationCode == SCSIOP_READ || pSrb->Function == 0xC9) {
                    // Устанавливаем наш CompletionRoutine.
                    if (fnprotector_set_completion_routine(pIrp, pIrpStack, NULL, dataSize, dataOffset)) {

                    }
                }
                else if (pCdb->CDB10.OperationCode == SCSIOP_WRITE) {
                    if (pMdl == NULL) { 
                        pMdl = pGlobalBlock->pCommonBlock->fnIoAllocateMdl(pIrp->UserBuffer, dataSize, FALSE, FALSE, NULL);
                        if (pMdl == NULL) {
                            break;
                        }
                        pGlobalBlock->pCommonBlock->fnMmProbeAndLockPages(pMdl, KernelMode, IoWriteAccess);
                        isMdlAllocated = TRUE;
                    }

                    pData = mmGetSystemAddressForMdl(pMdl);

                    ntStatus = fnprotector_verify_write_sectors((psectors_region_t)pHeadRegion->Flink, pData, dataSize, dataOffset);

                    if (isMdlAllocated) {
                        pGlobalBlock->pCommonBlock->fnMmUnlockPages(pMdl);
                    }
                }
            } while (0);
        }
    }
    else if (pIrpStack->MajorFunction == IRP_MJ_DEVICE_CONTROL) {
        bool_t needRevertCompletion = FALSE;

        do {
            if (pHeadRegion->Flink == (PLIST_ENTRY)pHeadRegion) {
                break;
            }

            if (pIrpStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_SCSI_PASS_THROUGH || pIrpStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_SCSI_PASS_THROUGH_DIRECT) {
                PSCSI_PASS_THROUGH_DIRECT pPassThru = NULL;
                PCDB pCdb;

                if (pIrp->UserBuffer != NULL) {
                    pPassThru = pIrp->UserBuffer;
                }

                if (pPassThru != NULL) {
                    pCdb = (PCDB)pPassThru->Cdb;

                    ((PFOUR_BYTE)&dataOffset)->Byte3 = pCdb->CDB10.LogicalBlockByte0;
                    ((PFOUR_BYTE)&dataOffset)->Byte2 = pCdb->CDB10.LogicalBlockByte1;
                    ((PFOUR_BYTE)&dataOffset)->Byte1 = pCdb->CDB10.LogicalBlockByte2;
                    ((PFOUR_BYTE)&dataOffset)->Byte0 = pCdb->CDB10.LogicalBlockByte3;
#ifdef _WIN64
                    dataOffset *= bytesPerSector;
#else
                    dataOffset = pGlobalBlock->pCommonBlock->fn_allmul(dataOffset, (uint64_t)bytesPerSector);
#endif // _WIN64
                    pData = pPassThru->DataBuffer;
                    dataSize = pPassThru->DataTransferLength;

                    if (pCdb->CDB10.OperationCode == SCSIOP_READ) {
                        needRevertCompletion = TRUE;
                    }
                    else if (pCdb->CDB10.OperationCode == SCSIOP_WRITE) {
                        ntStatus = fnprotector_verify_write_sectors((psectors_region_t)pHeadRegion->Flink, pData, dataSize, dataOffset);
                    }
                }
            }
            else if (pIrpStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_ATA_PASS_THROUGH || pIrpStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_ATA_PASS_THROUGH_DIRECT) {
                uint8_t* pAtaBuffer;

                if (pIrp->UserBuffer != NULL) {
                    PATA_PASS_THROUGH_DIRECT pPassThru = pIrp->UserBuffer;
                    uint8_t* pCommandBlock;

                    pAtaBuffer = pIrpStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_ATA_PASS_THROUGH ? (uint8_t*)pPassThru + ((PATA_PASS_THROUGH_EX)pPassThru)->DataBufferOffset : pPassThru->DataBuffer;

                    if (pAtaBuffer != NULL) {
                        pCommandBlock = pPassThru->CurrentTaskFile;
#ifdef _WIN64
checkAta:
#endif // _WIN64
                        if (pCommandBlock[5] & 0x40) {
                            // Режим адресации LBA
                            ((PFOUR_BYTE)&dataOffset)->Byte3 = pCommandBlock[5] & 0x0F;
                            ((PFOUR_BYTE)&dataOffset)->Byte2 = pCommandBlock[4];
                            ((PFOUR_BYTE)&dataOffset)->Byte1 = pCommandBlock[3];
                            ((PFOUR_BYTE)&dataOffset)->Byte0 = pCommandBlock[2];
#ifdef _WIN64
                            dataOffset *= bytesPerSector;
#else
                            dataOffset = pGlobalBlock->pCommonBlock->fn_allmul(dataOffset, (uint64_t)bytesPerSector);
#endif // _WIN64

                        }
                        //                             else {
                        //                                 uint32_t secCount = pPassThru->CurrentTaskFile[1] == 0 ? 256 : pPassThru->CurrentTaskFile[1];
                        //                                 // Режим адресации CHS.
                        //                                 
                        //                                 // Проверяем некоторые частные баговые случаи.
                        //                                 if ((pPassThru->DataTransferLength / pDiskInfo->bytesPerSector) != secCount) {
                        //                                     pContext->offset = 0;
                        //                                 }                                
                        //                             }
                        //                         ((PFOUR_BYTE)&pContext->offset)->Byte3 = pCdb->CDB10.LogicalBlockByte0;
                        //                         ((PFOUR_BYTE)&pContext->offset)->Byte2 = pCdb->CDB10.LogicalBlockByte1;
                        //                         ((PFOUR_BYTE)&pContext->offset)->Byte1 = pCdb->CDB10.LogicalBlockByte2;
                        //                         ((PFOUR_BYTE)&pContext->offset)->Byte0 = pCdb->CDB10.LogicalBlockByte3;
                        // #ifdef _WIN64
                        //                         pContext->offset *= pRegion->pDiskInfo->bytesPerSector;
                        // #else
                        //                         pContext->offset = pGlobalBlock->pCommonBlock->fn_allmul(pContext->offset, (uint64_t)pRegion->pDiskInfo->bytesPerSector);
                        // #endif // _WIN64
                        pData = pAtaBuffer;
                        dataSize = pPassThru->DataTransferLength;
                        if (pPassThru->AtaFlags & ATA_FLAGS_DATA_IN) {
                            needRevertCompletion = TRUE;
                        }
                        else if (pPassThru->AtaFlags & ATA_FLAGS_DATA_OUT) {
                            ntStatus = fnprotector_verify_write_sectors((psectors_region_t)pHeadRegion->Flink, pData, dataSize, dataOffset);
                        }
                    }
#ifdef _WIN64
                    else {
                        PATA_PASS_THROUGH_DIRECT32 pPassThru32 = pIrp->UserBuffer;

                        pAtaBuffer = (uint8_t*)(pIrpStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_ATA_PASS_THROUGH ? (uint8_t*)pPassThru32 + ((PATA_PASS_THROUGH_EX32)pPassThru32)->DataBufferOffset : pPassThru32->DataBuffer);
                        if (pAtaBuffer != NULL) {
                            pCommandBlock = pPassThru32->CurrentTaskFile;
                            goto checkAta;
                        }
                    }
#endif // _WIN64
                }
            }

            if (needRevertCompletion) {
                // Устанавливаем наш CompletionRoutine.
                fnprotector_set_completion_routine(pIrp, pIrpStack, pData, dataSize, dataOffset);
            }
        } while (0);
    }

    // Вызываем оригинальный обработчик.
    if (ntStatus == STATUS_SUCCESS) {
        for (dataSize = 0; dataSize <= IRP_MJ_MAXIMUM_FUNCTION; ++dataSize) {
            if (dataSize == pIrpStack->MajorFunction) {
                ntStatus = pGlobalBlock->pProtectorBlock->majorFunctions[dataSize](pDeviceObject, pIrp);
                break;
            }
        }
    }
    else {
        pIrp->IoStatus.Status = ntStatus;
        pGlobalBlock->pCommonBlock->fnIofCompleteRequest(pIrp, 0);
    }

    return ntStatus;
}

/* Обработчик IRP_MJ_INTERNAL_DEVICE_CONTROL/IRP_MJ_SCSI. */
NTSTATUS protector_irp_internal_devctl_hook(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
    USE_GLOBAL_BLOCK
    return pGlobalBlock->pProtectorBlock->fnprotector_common(pDeviceObject, pIrp);
}

NTSTATUS protector_irp_devctl_hook(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
    USE_GLOBAL_BLOCK
    return pGlobalBlock->pProtectorBlock->fnprotector_common(pDeviceObject, pIrp);
}

void protector_hooks_end()
{

}
