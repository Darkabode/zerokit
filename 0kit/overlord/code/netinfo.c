#include "overlord.h"

#include <Iphlpapi.h>

#include "netinfo.h"

padapter_entry_t gpAdapters = NULL;
uint32_t gAdaptersCount = 0;

void netinfo_clear()
{
    if (gpAdapters != NULL) {
        HeapFree(gHeap, 0, gpAdapters);
        gpAdapters = NULL;
        gAdaptersCount = 0;
    }
}

void netinfo_enum_dns_servers(padapter_entry_t pAdapterEntry)
{
    int i = 0;
    IP_PER_ADAPTER_INFO* pPerAdapterInfo = NULL;
    ULONG ulBufLen = 0;
    DWORD err = ERROR_SUCCESS;
    IP_ADDR_STRING* dnsServerList;

    err = GetPerAdapterInfo(pAdapterEntry->index, pPerAdapterInfo, &ulBufLen);
    if (err == ERROR_BUFFER_OVERFLOW) {
        pPerAdapterInfo = (IP_PER_ADAPTER_INFO*)HeapAlloc(gHeap, HEAP_ZERO_MEMORY, ulBufLen);
        if (pPerAdapterInfo == NULL) {
            DBG_PRINTF(("Can't allocate memory on heap for DNS server list (%d bytes): EnumDnsServers\n", ulBufLen));
            return;
        }
        err = GetPerAdapterInfo(pAdapterEntry->index, pPerAdapterInfo, &ulBufLen);
    }
    do {
        if (err != ERROR_SUCCESS) {
            DBG_PRINTF(("netinfo_enum_dns_servers error: %08X\n", err));
            break;
        }

        // clear the adapterEntry dns server list
        __stosb((uint8_t*)&pAdapterEntry->dnsServers, 0, sizeof(pAdapterEntry->dnsServers));

        DBG_PRINTF((" DNS servers:\n"));
        dnsServerList = &pPerAdapterInfo->DnsServerList;
        while (dnsServerList != NULL && lstrlenA(dnsServerList->IpAddress.String) > 0){
            pAdapterEntry->dnsServers[i] = fninet_addr(dnsServerList->IpAddress.String);
            DBG_PRINTF(("  Server %d: %u.%u.%u.%u\n", i + 1, ((uint8_t*)(&pAdapterEntry->dnsServers[i]))[0], ((uint8_t*)(&pAdapterEntry->dnsServers[i]))[1], ((uint8_t*)(&pAdapterEntry->dnsServers[i]))[2], ((uint8_t*)(&pAdapterEntry->dnsServers[i]))[3]));
            dnsServerList = dnsServerList->Next;
            ++i;
        }
    } while (0);

    HeapFree(gHeap, 0, pPerAdapterInfo);
}

int netinfo_update()
{
    IP_ADAPTER_INFO* pAdapterList = NULL;
    IP_ADAPTER_INFO* pAdapter = NULL;
    ULONG ulBufLen = 0;
    DWORD err = ERROR_SUCCESS;
    MIB_IPNETTABLE* pIpNetTable = NULL;
    unsigned int i, j, arpNum = 0;

    netinfo_clear();

    err = GetAdaptersInfo(pAdapterList, &ulBufLen);
    DBG_PRINTF(("netinfo_update(): GetAdaptersInfo() needed buffer %u and returned %u\n", ulBufLen, err));
    if (err == ERROR_BUFFER_OVERFLOW) {
        // allocate memory for the linked adapters of IP_ADAPTER_INFO structures
        pAdapterList = (IP_ADAPTER_INFO*)HeapAlloc(gHeap, HEAP_ZERO_MEMORY, ulBufLen);
        if (pAdapterList == NULL) {
            DBG_PRINTF(("Can't allocate memory on heap (%d bytes) for GetAdaptersInfo\n", ulBufLen));
            return 0;
        }
        err = GetAdaptersInfo(pAdapterList, &ulBufLen);
        DBG_PRINTF(("netinfo_update(): GetAdaptersInfo() needed buffer %u and returned %u\n", ulBufLen, err));
    }

    if (err != ERROR_SUCCESS) {
        DBG_PRINTF(("GetAdaptersInfo error: %08X\n", err));
        HeapFree(gHeap, 0, pAdapterList);
        return 0;
    }

    // enumerate the network adapters 
    pAdapter = pAdapterList;
    while (pAdapter != NULL) {
        padapter_entry_t pAdapterEntry;

        if (gpAdapters == NULL) {
            gpAdapters = HeapAlloc(gHeap, HEAP_ZERO_MEMORY, sizeof(adapter_entry_t));
        }
        else {
            gpAdapters = HeapReAlloc(gHeap, HEAP_ZERO_MEMORY, gpAdapters, (gAdaptersCount + 1) * sizeof(adapter_entry_t));
        }

        pAdapterEntry = gpAdapters + (gAdaptersCount++);

        pAdapterEntry->index = pAdapter->Index;

        lstrcpyA(pAdapterEntry->name, pAdapter->AdapterName);
        lstrcpyA(pAdapterEntry->description, pAdapter->Description);
        __movsb(pAdapterEntry->mac, pAdapter->Address, pAdapter->AddressLength);

        pAdapterEntry->ip = fninet_addr(pAdapter->IpAddressList.IpAddress.String);
        pAdapterEntry->subnet = fninet_addr(pAdapter->IpAddressList.IpMask.String);

        // check if gateway is present
        if (lstrlenA(pAdapter->GatewayList.IpAddress.String) > 0) {
            pAdapterEntry->gateway = fninet_addr(pAdapter->GatewayList.IpAddress.String);
        }

        // dhcp
        pAdapterEntry->dhcpEnabled = pAdapter->DhcpEnabled ? TRUE : FALSE;
        if (pAdapterEntry->dhcpEnabled) {
            pAdapterEntry->dhcpServer = fninet_addr(pAdapter->DhcpServer.IpAddress.String);
        }

        DBG_PRINTF(("Adapter %u\n", pAdapterEntry->index));
        DBG_PRINTF((" Name: %s\n", pAdapterEntry->name));
        DBG_PRINTF((" Description: %s\n", pAdapterEntry->description));
        DBG_PRINTF_ARR(" MAC", pAdapterEntry->mac, MAX_ADAPTER_ADDRESS_LENGTH);
        DBG_PRINTF((" IP: %u.%u.%u.%u\n", ((uint8_t*)(&pAdapterEntry->ip))[0], ((uint8_t*)(&pAdapterEntry->ip))[1], ((uint8_t*)(&pAdapterEntry->ip))[2], ((uint8_t*)(&pAdapterEntry->ip))[3]));
        DBG_PRINTF((" Subnet: %u.%u.%u.%u\n", ((uint8_t*)(&pAdapterEntry->subnet))[0], ((uint8_t*)(&pAdapterEntry->subnet))[1], ((uint8_t*)(&pAdapterEntry->subnet))[2], ((uint8_t*)(&pAdapterEntry->subnet))[3]));
        DBG_PRINTF((" Gateway: %u.%u.%u.%u\n", ((uint8_t*)(&pAdapterEntry->gateway))[0], ((uint8_t*)(&pAdapterEntry->gateway))[1], ((uint8_t*)(&pAdapterEntry->gateway))[2], ((uint8_t*)(&pAdapterEntry->gateway))[3]));
        DBG_PRINTF((" DHCP enabled: %s\n", pAdapterEntry->dhcpEnabled ? "YES" : "NO"));
#ifdef USE_DBGPRINT
        if (pAdapterEntry->dhcpEnabled) {
            DBG_PRINTF((" DHCP server: %u.%u.%u.%u\n", ((uint8_t*)(&pAdapterEntry->dhcpServer))[0], ((uint8_t*)(&pAdapterEntry->dhcpServer))[1], ((uint8_t*)(&pAdapterEntry->dhcpServer))[2], ((uint8_t*)(&pAdapterEntry->dhcpServer))[3]));
        }
#endif // USE_DBGPRINT

        // try to get the dns server list
        netinfo_enum_dns_servers(pAdapterEntry);

        // go on to the next adapter item
        pAdapter = pAdapter->Next;
    }

    HeapFree(gHeap, 0, pAdapterList);

    DBG_PRINTF(("ARP tables:\n"));
    ulBufLen = 0;
    err = GetIpNetTable(pIpNetTable, &ulBufLen, TRUE);
    if (err == ERROR_INSUFFICIENT_BUFFER) {
        pIpNetTable = (MIB_IPNETTABLE*)HeapAlloc(gHeap, HEAP_ZERO_MEMORY, ulBufLen);
        if (pIpNetTable == NULL) {
            // error HeapAlloc returned NULL
            DBG_PRINTF(("Can't allocate memory on heap (%d bytes) for ARP data for GetIpNetTable\n", ulBufLen));
            return 0;
        }
        err = GetIpNetTable(pIpNetTable, &ulBufLen, TRUE);
    }
    if (err != ERROR_SUCCESS) {
        DBG_PRINTF(("GetIpNetTable error: %08X\n", err));
        return 0;
    }

    // enumerate the arp table
    for (i = 0; i < pIpNetTable->dwNumEntries; ++i) {
        PMIB_IPNETROW pArpRow = &pIpNetTable->table[i]; // information from an ARP entry
        padapter_entry_t pAdapterEntry = NULL;
        parp_entry_t pArpEntry;

        // »щем адаптер дл€ которого предназначен данный ARP-элемент.
        for (j = 0; j < gAdaptersCount; ++j) {
            if (gpAdapters[j].index == (uint32_t)pArpRow->dwIndex) {
                break;
            }
        }

        if (j < gAdaptersCount) {
            gpAdapters = HeapReAlloc(gHeap, HEAP_ZERO_MEMORY, gpAdapters, gAdaptersCount * sizeof(adapter_entry_t) + (arpNum + 1) * sizeof(arp_entry_t));

            pAdapterEntry = &gpAdapters[j];
            DBG_PRINTF((" Adapter %s:\n", pAdapterEntry->description));
            ++pAdapterEntry->arpEntriesCount;
            pArpEntry = ((parp_entry_t)((uint8_t*)gpAdapters + gAdaptersCount * sizeof(adapter_entry_t))) + (arpNum++);

            if (pAdapterEntry->arpEntryOffset == 0) {
                pAdapterEntry->arpEntryOffset = (uint32_t)((uint8_t*)pArpEntry - (uint8_t*)gpAdapters);
                DBG_PRINTF(("  ARP entry offset: %u\n", pAdapterEntry->arpEntryOffset));
            }
            // get ARP entry informations
            pArpEntry->ip = pArpRow->dwAddr;
            __movsb(pArpEntry->mac, pArpRow->bPhysAddr, sizeof(pArpEntry->mac));
            pArpEntry->entryType = (arp_entry_type_e)pArpRow->dwType;

            DBG_PRINTF(("  ARP entry:\n"));
            DBG_PRINTF(("   IP: %u.%u.%u.%u\n", ((uint8_t*)(&pArpEntry->ip))[0], ((uint8_t*)(&pArpEntry->ip))[1], ((uint8_t*)(&pArpEntry->ip))[2], ((uint8_t*)(&pArpEntry->ip))[3]));
            DBG_PRINTF_ARR("   MAC", pArpEntry->mac, sizeof(pArpEntry->mac));
            DBG_PRINTF(("   Type: %s\n", pArpEntry->entryType == Other ? "Other" : (pArpEntry->entryType == Invalid ? "Invalid" :(pArpEntry->entryType == Dynamic ? "Dynamic" : "Static") )));
        }
    }

    HeapFree(gHeap, 0, pIpNetTable);
    return 1;
}
