uint32_t userio_allocate_handle(uintptr_t zfsHandle)
{
    KIRQL x;
    uint32_t i;
    uint32_t size;
    uintptr_t* pHandles;
    USE_GLOBAL_BLOCK

        x = pGlobalBlock->pCommonBlock->fnKfAcquireSpinLock(&pGlobalBlock->pUserioBlock->slHandles);
    pHandles = pGlobalBlock->pUserioBlock->pHandles;
    size = (uint32_t)pHandles[0];

    for (i = 1; i < size; ++i) {
        if (pHandles[i] == 0) {
            pHandles[i] = zfsHandle;
            break;
        }
    }

    if (i == size) {
        uintptr_t* pNewHandles;
        // Выделяем дополнительное место для описателей.
        pGlobalBlock->pCommonBlock->fncommon_allocate_memory(pGlobalBlock->pCommonBlock, &pNewHandles, (48 + size) * sizeof(uintptr_t), NonPagedPool);
        __movsb((unsigned char*)pNewHandles, (unsigned char*)pHandles, size * sizeof(uintptr_t));
        pNewHandles[0] += 48;
        pNewHandles[i] = zfsHandle;
        pGlobalBlock->pUserioBlock->pHandles = pNewHandles;

        // Удаляем старый буфер.
        pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(pHandles, LOADER_TAG);
    }

    pGlobalBlock->pCommonBlock->fnKfReleaseSpinLock(&pGlobalBlock->pUserioBlock->slHandles, x);

    return i;
}

uintptr_t userio_get_zfs_handle(uint32_t handle)
{
    KIRQL x;
    uintptr_t zfsHandle = 0;
    USE_GLOBAL_BLOCK

        x = pGlobalBlock->pCommonBlock->fnKfAcquireSpinLock(&pGlobalBlock->pUserioBlock->slHandles);
    do {
        if (handle < 1 && handle >= (uint32_t)pGlobalBlock->pUserioBlock->pHandles[0]) {
            break;
        }
        zfsHandle = pGlobalBlock->pUserioBlock->pHandles[handle];
    } while (0);

    pGlobalBlock->pCommonBlock->fnKfReleaseSpinLock(&pGlobalBlock->pUserioBlock->slHandles, x);

    return zfsHandle;
}

void userio_free_zfs_handle(uint32_t handle)
{
    KIRQL x;
    USE_GLOBAL_BLOCK

    x = pGlobalBlock->pCommonBlock->fnKfAcquireSpinLock(&pGlobalBlock->pUserioBlock->slHandles);
    do {
        if (handle < 1 && handle >= (uint32_t)pGlobalBlock->pUserioBlock->pHandles[0]) {
            break;
        }
        pGlobalBlock->pUserioBlock->pHandles[handle] = 0;
    } while (0);

    pGlobalBlock->pCommonBlock->fnKfReleaseSpinLock(&pGlobalBlock->pUserioBlock->slHandles, x);
}

pclient_entry_t userio_find_client_id(pfile_packet_t pFilePacket)
{
    pclient_entry_t pHead, pEntry;
    USE_GLOBAL_BLOCK

    // Ищем начальный путь по client id.
    pHead = &pGlobalBlock->pUserioBlock->clientsHead;
    pEntry = (pclient_entry_t)pHead->Flink;

    // Ищем запись о клиенте среди уже созданных.
    while (pEntry != pHead) {
        if (MEMCMP(pEntry->clientId, pFilePacket->clientId, sizeof(pFilePacket->clientId))) {
            break;
        }

        pEntry = (pclient_entry_t)pEntry->Flink;
    }

    if (pEntry == pHead) {
        return NULL;
    }
    return pEntry;
}

#include "userio_funcs.c"

NTSTATUS userio_null_irp_ioctl_hook(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PIO_STACK_LOCATION pIrpSt;
    USE_GLOBAL_BLOCK

    pIrpSt = pIrp->Tail.Overlay.CurrentStackLocation;

    do {
        if (pIrpSt->MajorFunction == IRP_MJ_DEVICE_CONTROL) {
            uint32_t i;

            for (i = 0; i < sizeof(pGlobalBlock->pUserioBlock->zfs_wrapper_IDs) / sizeof(uint32_t); ++i) {
                if (pGlobalBlock->pUserioBlock->zfs_wrapper_CtrlCodes[i] == pIrpSt->Parameters.DeviceIoControl.IoControlCode) {
                    uint32_t bufferSize = pIrpSt->Parameters.DeviceIoControl.OutputBufferLength;
                    void* pUserBuffer;
                    pfile_packet_t pFilePacket;

                    pUserBuffer = pIrp->UserBuffer;
                    if (pUserBuffer != NULL) {
                        pGlobalBlock->pCommonBlock->fnarc4_crypt_self((uint8_t*)pUserBuffer, bufferSize, pGlobalBlock->pCommonBlock->fsKey, sizeof(pGlobalBlock->pCommonBlock->fsKey));
                        pFilePacket = (pfile_packet_t)pUserBuffer;
                        if (pFilePacket->signature == REQUEST_SIGNATURE && pFilePacket->operation == pGlobalBlock->pUserioBlock->zfs_wrapper_IDs[i]) {
                            pGlobalBlock->pUserioBlock->zfs_wrapper_FNs[i](pFilePacket);
                            pGlobalBlock->pCommonBlock->fnarc4_crypt_self((uint8_t*)pUserBuffer, bufferSize, pGlobalBlock->pCommonBlock->fsKey, sizeof(pGlobalBlock->pCommonBlock->fsKey));
                        }
                    }
                    break;
                }
            }

            if (i == sizeof(pGlobalBlock->pUserioBlock->zfs_wrapper_IDs) / sizeof(uint32_t)) {
                break;
            }

            ntStatus = STATUS_CANCELLED;
            pGlobalBlock->pCommonBlock->fnIoCancelIrp(pIrp);   
        }
    } while (0);

    pIrp->IoStatus.Status = ntStatus;
    pIrp->IoStatus.Information = 0;
    pGlobalBlock->pCommonBlock->fnIofCompleteRequest(pIrp, IO_NO_INCREMENT);
    return ntStatus;
    //return ((FnMajorFunction)(pGlobalBlock->pProtectorBlock->MajorTrampolineIOCTL->pOriginalFunc))(pDeviceObject, pIrp);
}
