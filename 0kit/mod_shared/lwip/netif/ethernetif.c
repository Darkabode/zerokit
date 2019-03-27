#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/stats.h"
#include "lwip/snmp.h"
#include "lwip/ethip6.h"
#include "netif/etharp.h"
#include "netif/ppp_oe.h"

#define IFNAME0 'e'
#define IFNAME1 'n'

// int LoaderSendPacketFromStack(char** buf, uint16_t len);
// int LoaderReadPacketForStack(unsigned int* len, char** pkt_data);

struct ethernetif {
  struct eth_addr *ethaddr;
  /* Add whatever per-interface state that is needed here. */
};
//extern void etharp_arp_input(struct netif *netif, struct eth_addr *ethaddr, struct pbuf *p);
extern void etharp_ip_input(struct netif *netif, struct pbuf *p);

// void low_level_init(struct netif *netif)
// {
// 	
// // 	}
// }

// 
// static struct pbuf* low_level_input(struct netif *netif)
// {
// //	struct ethernetif* ethernetif = netif->state;
// 	struct pbuf *p, *q;
// 	uint32_t len;
// 	char* pkt_data;
// 	char* data;
//   
// 	LoaderReadPacketForStack(&len, &pkt_data);
//   
// #if ETH_PAD_SIZE
// 	len += ETH_PAD_SIZE; /* allow room for Ethernet padding */
// #endif
//   
// 	/* We allocate a pbuf chain of pbufs from the pool. */
// 	p = pGlobalBlock->fnpbuf_alloc(PBUF_RAW, (uint16_t)len, PBUF_POOL);
// 
// 	if (p != NULL) {
//     
// #if ETH_PAD_SIZE
// 		pGlobalBlock->fnpbuf_header(p, -ETH_PAD_SIZE); /* drop the padding word */
// #endif
//     
// 		for(q = p, data = pkt_data; q != NULL; q = q->next) {
//  		  RtlCopyMemory(q->payload, data, q->len);
// 		  data += q->len;
// 		}
//     
// #if ETH_PAD_SIZE
// 		pGlobalBlock->fnpbuf_header(p, ETH_PAD_SIZE); /* reclaim the padding word */
// #endif
//     
// 		LINK_STATS_INC(link.recv);
// 	}
// 	else {
// 		LINK_STATS_INC(link.memerr);
// 		LINK_STATS_INC(link.drop);
// 	}
//   
// 	return p;  
// }

void ethernetif_init(struct netif *netif)
{
	struct ethernetif* ethernetif;
	LARGE_INTEGER delay;
	ULONG i;
	pmod_common_block_t pCommonBlock;
	pmod_network_block_t pNetworkBlock;
	pmod_tcpip_block_t pTcpipBlock;
	USE_GLOBAL_BLOCK

	pCommonBlock = pGlobalBlock->pCommonBlock;
	pNetworkBlock = pGlobalBlock->pNetworkBlock;
	pTcpipBlock = pGlobalBlock->pTcpipBlock;

	LWIP_ASSERT("netif != NULL", (netif != NULL));

	//ethernetif = mem_malloc(sizeof(struct ethernetif));
	pCommonBlock->fncommon_allocate_memory(pCommonBlock, &ethernetif, sizeof(struct ethernetif), NonPagedPool);

#if LWIP_NETIF_HOSTNAME
	/* Initialize interface hostname */
	netif->hostname = "lwip";
#endif /* LWIP_NETIF_HOSTNAME */
  
	/*
	* Initialize the snmp variables and counters inside the struct netif.
	* The last argument should be replaced with your link speed, in units
	* of bits per second.
	*/
	NETIF_INIT_SNMP(netif, snmp_ifType_ethernet_csmacd, 100 * 1024 * 1024);

	netif->state = ethernetif;
	netif->name[0] = IFNAME0;
	netif->name[1] = IFNAME1;
	/* We directly use etharp_output() here to save a function call.
	* You can instead declare your own function an call etharp_output()
	* from it if you have to do some checks before sending (e.g. if link
	* is available...) */
	netif->output = pTcpipBlock->fnetharp_output;
#if LWIP_IPV6
	netif->output_ip6 = ethip6_output;
#endif /* LWIP_IPV6 */
	netif->linkoutput = pTcpipBlock->fntcpip_send;

	ethernetif->ethaddr = (struct eth_addr *)&(netif->hwaddr[0]);

	// Этот код перенесён в reinitStack
// 	/* initialize the hardware */
// 	netif->hwaddr_len = ETHARP_HWADDR_LEN;
// 	netif->hwaddr[0] = pAdapter->macAddr[0];
// 	netif->hwaddr[1] = pAdapter->macAddr[1];
// 	netif->hwaddr[2] = pAdapter->macAddr[2];
// 	netif->hwaddr[3] = pAdapter->macAddr[3];
// 	netif->hwaddr[4] = pAdapter->macAddr[4];
// 	netif->hwaddr[5] = pAdapter->macAddr[5];
// 	netif->mtu = 1500;

	/* don't set NETIF_FLAG_ETHARP if this device is not an ethernet one */
	netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;

	//low_level_init(netif);
}
