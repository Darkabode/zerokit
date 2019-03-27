/**
 * @file
 * Address Resolution Protocol module for IP over Ethernet
 *
 * Functionally, ARP is divided into two parts. The first maps an IP address
 * to a physical address when sending a packet, and the second part answers
 * requests from other machines for our physical address.
 *
 * This implementation complies with RFC 826 (Ethernet ARP). It supports
 * Gratuitious ARP from RFC3220 (IP Mobility Support for IPv4) section 4.6
 * if an interface calls etharp_gratuitous(our_netif) upon address change.
 */

/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
 * Copyright (c) 2003-2004 Leon Woestenberg <leon.woestenberg@axon.tv>
 * Copyright (c) 2003-2004 Axon Digital Design B.V., The Netherlands.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 */
 
#include "lwip/opt.h"

#if LWIP_ARP || LWIP_ETHERNET

#include "lwip/ip_addr.h"
//#include "lwip/def.h"
#include "lwip/ip.h"
#include "lwip/stats.h"
#include "lwip/snmp.h"
#include "lwip/dhcp.h"
#include "lwip/autoip.h"
#include "netif/etharp.h"
#include "lwip/ip6.h"

#if PPPOE_SUPPORT
#include "netif/ppp_oe.h"
#endif /* PPPOE_SUPPORT */
#if 0
#include <string.h>
#endif // #if 0
#if 0
const struct eth_addr ethbroadcast = {{0xff,0xff,0xff,0xff,0xff,0xff}};
const struct eth_addr ethzero = {{0,0,0,0,0,0}};
#endif // #if 0

/** The 24-bit IANA multicast OUI is 01-00-5e: */
#define LL_MULTICAST_ADDR_0 0x01
#define LL_MULTICAST_ADDR_1 0x00
#define LL_MULTICAST_ADDR_2 0x5e

#if LWIP_ARP /* don't build if not configured for use in lwipopts.h */

/** the time an ARP entry stays valid after its last update,
 *  for ARP_TMR_INTERVAL = 5000, this is
 *  (240 * 5) seconds = 20 minutes.
 */
#define ARP_MAXAGE              240
/** Re-request a used ARP entry 1 minute before it would expire to prevent
 *  breaking a steadily used connection because the ARP entry timed out. */
#define ARP_AGE_REREQUEST_USED  (ARP_MAXAGE - 12)

/** the time an ARP entry stays pending after first request,
 *  for ARP_TMR_INTERVAL = 5000, this is
 *  (2 * 5) seconds = 10 seconds.
 * 
 *  @internal Keep this number at least 2, otherwise it might
 *  run out instantly if the timeout occurs directly after a request.
 */
#define ARP_MAXPENDING 2

#define HWTYPE_ETHERNET 1

enum etharp_state {
  ETHARP_STATE_EMPTY = 0,
  ETHARP_STATE_PENDING,
  ETHARP_STATE_STABLE,
  ETHARP_STATE_STABLE_REREQUESTING
#if ETHARP_SUPPORT_STATIC_ENTRIES
  ,ETHARP_STATE_STATIC
#endif /* ETHARP_SUPPORT_STATIC_ENTRIES */
};
#if 0
struct etharp_entry {
#if ARP_QUEUEING
  /** Pointer to queue of pending outgoing packets on this ARP entry. */
  struct etharp_q_entry *q;
#else /* ARP_QUEUEING */
  /** Pointer to a single pending outgoing packet on this ARP entry. */
  struct pbuf *q;
#endif /* ARP_QUEUEING */
  ip_addr_t ipaddr;
  struct netif *netif;
  struct eth_addr ethaddr;
  UINT8 state;
  UINT8 ctime;
};

static struct etharp_entry arp_table[ARP_TABLE_SIZE];

#endif // #if 0

#if !LWIP_NETIF_HWADDRHINT
#if 0
static UINT8 etharp_cached_entry;
#endif // #if 0
#endif /* !LWIP_NETIF_HWADDRHINT */

/** Try hard to create a new entry - we want the IP address to appear in
    the cache (even if this means removing an active entry or so). */
#define ETHARP_FLAG_TRY_HARD     1
#define ETHARP_FLAG_FIND_ONLY    2
#if ETHARP_SUPPORT_STATIC_ENTRIES
#define ETHARP_FLAG_STATIC_ENTRY 4
#endif /* ETHARP_SUPPORT_STATIC_ENTRIES */

#if LWIP_NETIF_HWADDRHINT
#define ETHARP_SET_HINT(netif, hint)  if (((netif) != NULL) && ((netif)->addr_hint != NULL))  \
                                      *((netif)->addr_hint) = (hint);
#else /* LWIP_NETIF_HWADDRHINT */
#define ETHARP_SET_HINT(netif, hint)  (pGlobalBlock->pTcpipBlock->etharp_cached_entry = (hint))
#endif /* LWIP_NETIF_HWADDRHINT */


/* Some checks, instead of etharp_init(): */
#if (LWIP_ARP && (ARP_TABLE_SIZE > 0x7f))
  #error "ARP_TABLE_SIZE must fit in an INT8, you have to reduce it in your lwipopts.h"
#endif


#if ARP_QUEUEING
/**
 * Free a complete queue of etharp entries
 *
 * @param q a qeueue of etharp_q_entry's to free
 */
void
free_etharp_q(struct etharp_q_entry *q)
{
  struct etharp_q_entry *r;
  USE_GLOBAL_BLOCK

  LWIP_ASSERT("q != NULL", q != NULL);
  LWIP_ASSERT("q->p != NULL", q->p != NULL);
  while (q) {
    r = q;
    q = q->next;
    LWIP_ASSERT("r->p != NULL", (r->p != NULL));
    pGlobalBlock->pTcpipBlock->fnpbuf_free(r->p);
    memp_free(MEMP_ARP_QUEUE, r);
  }
}
#else /* ARP_QUEUEING */

/** Compatibility define: free the queued pbuf */
#define free_etharp_q(q) pTcpipBlock->fnpbuf_free(q)

#endif /* ARP_QUEUEING */

/** Clean up ARP table entries */
void
etharp_free_entry(int i)
{
  USE_GLOBAL_BLOCK

  /* remove from SNMP ARP index tree */
  snmp_delete_arpidx_tree(arp_table[i].netif, &arp_table[i].ipaddr);
  /* and empty packet queue */
  if (pGlobalBlock->pTcpipBlock->arp_table[i].q != NULL) {
    /* remove all queued packets */
	  LWIP_DEBUGF(ETHARP_DEBUG, ("etharp_free_entry: freeing entry %"U16_F", packet queue %p.\n", (uint16_t)i, (void *)(arp_table[i].q)));
    pGlobalBlock->pTcpipBlock->fnfree_etharp_q(pGlobalBlock->pTcpipBlock->arp_table[i].q);
    pGlobalBlock->pTcpipBlock->arp_table[i].q = NULL;
  }
  /* recycle entry for re-use */
  pGlobalBlock->pTcpipBlock->arp_table[i].state = ETHARP_STATE_EMPTY;
#ifdef LWIP_DEBUG
  /* for debugging, clean out the complete entry */
  arp_table[i].ctime = 0;
#if LWIP_SNMP
  arp_table[i].netif = NULL;
#endif /* LWIP_SNMP */
  ip_addr_set_zero(&arp_table[i].ipaddr);
  arp_table[i].ethaddr = ethzero;
#endif /* LWIP_DEBUG */
}

/**
 * Clears expired entries in the ARP table.
 *
 * This function should be called every ETHARP_TMR_INTERVAL milliseconds (5 seconds),
 * in order to expire entries in the ARP table.
 */
void
etharp_tmr(void)
{
  UINT8 i;
  USE_GLOBAL_BLOCK

  LWIP_DEBUGF(ETHARP_DEBUG, ("etharp_timer\n"));
  /* remove expired entries from the ARP table */
  for (i = 0; i < ARP_TABLE_SIZE; ++i) {
    UINT8 state = pGlobalBlock->pTcpipBlock->arp_table[i].state;
    if (state != ETHARP_STATE_EMPTY
#if ETHARP_SUPPORT_STATIC_ENTRIES
      && (state != ETHARP_STATE_STATIC)
#endif /* ETHARP_SUPPORT_STATIC_ENTRIES */
      ) {
      pGlobalBlock->pTcpipBlock->arp_table[i].ctime++;
      if ((pGlobalBlock->pTcpipBlock->arp_table[i].ctime >= ARP_MAXAGE) ||
          ((pGlobalBlock->pTcpipBlock->arp_table[i].state == ETHARP_STATE_PENDING)  &&
           (pGlobalBlock->pTcpipBlock->arp_table[i].ctime >= ARP_MAXPENDING))) {
        /* pending or stable entry has become old! */
        LWIP_DEBUGF(ETHARP_DEBUG, ("etharp_timer: expired %s entry %"U16_F".\n",
             pTcpipBlock->arp_table[i].state >= ETHARP_STATE_STABLE ? "stable" : "pending", (uint16_t)i));
        /* clean up entries that have just been expired */
        pGlobalBlock->pTcpipBlock->fnetharp_free_entry(i);
      }
	  else if (pGlobalBlock->pTcpipBlock->arp_table[i].state == ETHARP_STATE_STABLE_REREQUESTING) {
        /* Reset state to stable, so that the next transmitted packet will
           re-send an ARP request. */
        pGlobalBlock->pTcpipBlock->arp_table[i].state = ETHARP_STATE_STABLE;
      }
#if ARP_QUEUEING
      /* still pending entry? (not expired) */
      if (pGlobalBlock->pTcpipBlock->arp_table[i].state == ETHARP_STATE_PENDING) {
        /* resend an ARP query here? */
      }
#endif /* ARP_QUEUEING */
    }
  }
}

/**
 * Search the ARP table for a matching or new entry.
 * 
 * If an IP address is given, return a pending or stable ARP entry that matches
 * the address. If no match is found, create a new entry with this address set,
 * but in state ETHARP_EMPTY. The caller must check and possibly change the
 * state of the returned entry.
 * 
 * If ipaddr is NULL, return a initialized new entry in state ETHARP_EMPTY.
 * 
 * In all cases, attempt to create new entries from an empty entry. If no
 * empty entries are available and ETHARP_FLAG_TRY_HARD flag is set, recycle
 * old entries. Heuristic choose the least important entry for recycling.
 *
 * @param ipaddr IP address to find in ARP cache, or to add if not found.
 * @param flags @see definition of ETHARP_FLAG_*
 * @param netif netif related to this address (used for NETIF_HWADDRHINT)
 *  
 * @return The ARP entry index that matched or is created, ERR_MEM if no
 * entry is found or could be recycled.
 */
INT8
etharp_find_entry(ip_addr_t *ipaddr, UINT8 flags)
{
  INT8 old_pending = ARP_TABLE_SIZE, old_stable = ARP_TABLE_SIZE;
  INT8 empty = ARP_TABLE_SIZE;
  UINT8 i = 0, age_pending = 0, age_stable = 0;
  /* oldest entry with packets on queue */
  INT8 old_queue = ARP_TABLE_SIZE;
  /* its age */
  UINT8 age_queue = 0;
  USE_GLOBAL_BLOCK

  /**
   * a) do a search through the cache, remember candidates
   * b) select candidate entry
   * c) create new entry
   */

  /* a) in a single search sweep, do all of this
   * 1) remember the first empty entry (if any)
   * 2) remember the oldest stable entry (if any)
   * 3) remember the oldest pending entry without queued packets (if any)
   * 4) remember the oldest pending entry with queued packets (if any)
   * 5) search for a matching IP entry, either pending or stable
   *    until 5 matches, or all entries are searched for.
   */

  for (i = 0; i < ARP_TABLE_SIZE; ++i) {
    UINT8 state = pGlobalBlock->pTcpipBlock->arp_table[i].state;
    /* no empty entry found yet and now we do find one? */
    if ((empty == ARP_TABLE_SIZE) && (state == ETHARP_STATE_EMPTY)) {
      LWIP_DEBUGF(ETHARP_DEBUG, ("find_entry: found empty entry %"U16_F"\n", (uint16_t)i));
      /* remember first empty entry */
      empty = i;
    } else if (state != ETHARP_STATE_EMPTY) {
      LWIP_ASSERT("state == ETHARP_STATE_PENDING || state >= ETHARP_STATE_STABLE",
        state == ETHARP_STATE_PENDING || state >= ETHARP_STATE_STABLE);
      /* if given, does IP address match IP address in ARP entry? */
      if (ipaddr && ip_addr_cmp(ipaddr, &pGlobalBlock->pTcpipBlock->arp_table[i].ipaddr)) {
        LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("find_entry: found matching entry %"U16_F"\n", (uint16_t)i));
        /* found exact IP address match, simply bail out */
        return i;
      }
      /* pending entry? */
      if (state == ETHARP_STATE_PENDING) {
        /* pending with queued packets? */
        if (pGlobalBlock->pTcpipBlock->arp_table[i].q != NULL) {
          if (pGlobalBlock->pTcpipBlock->arp_table[i].ctime >= age_queue) {
            old_queue = i;
            age_queue = pGlobalBlock->pTcpipBlock->arp_table[i].ctime;
          }
        } else
        /* pending without queued packets? */
        {
          if (pGlobalBlock->pTcpipBlock->arp_table[i].ctime >= age_pending) {
            old_pending = i;
            age_pending = pGlobalBlock->pTcpipBlock->arp_table[i].ctime;
          }
        }
      /* stable entry? */
      } else if (state >= ETHARP_STATE_STABLE) {
#if ETHARP_SUPPORT_STATIC_ENTRIES
        /* don't record old_stable for static entries since they never expire */
        if (state < ETHARP_STATE_STATIC)
#endif /* ETHARP_SUPPORT_STATIC_ENTRIES */
        {
          /* remember entry with oldest stable entry in oldest, its age in maxtime */
          if (pGlobalBlock->pTcpipBlock->arp_table[i].ctime >= age_stable) {
            old_stable = i;
            age_stable = pGlobalBlock->pTcpipBlock->arp_table[i].ctime;
          }
        }
      }
    }
  }
  /* { we have no match } => try to create a new entry */
   
  /* don't create new entry, only search? */
  if (((flags & ETHARP_FLAG_FIND_ONLY) != 0) ||
      /* or no empty entry found and not allowed to recycle? */
      ((empty == ARP_TABLE_SIZE) && ((flags & ETHARP_FLAG_TRY_HARD) == 0))) {
    LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("find_entry: no empty entry found and not allowed to recycle\n"));
    return (INT8)ERR_MEM;
  }
  
  /* b) choose the least destructive entry to recycle:
   * 1) empty entry
   * 2) oldest stable entry
   * 3) oldest pending entry without queued packets
   * 4) oldest pending entry with queued packets
   * 
   * { ETHARP_FLAG_TRY_HARD is set at this point }
   */ 

  /* 1) empty entry available? */
  if (empty < ARP_TABLE_SIZE) {
    i = empty;
    LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("find_entry: selecting empty entry %"U16_F"\n", (uint16_t)i));
  } else {
    /* 2) found recyclable stable entry? */
    if (old_stable < ARP_TABLE_SIZE) {
      /* recycle oldest stable*/
      i = old_stable;
      LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("find_entry: selecting oldest stable entry %"U16_F"\n", (uint16_t)i));
      /* no queued packets should exist on stable entries */
      LWIP_ASSERT("arp_table[i].q == NULL", arp_table[i].q == NULL);
    /* 3) found recyclable pending entry without queued packets? */
    } else if (old_pending < ARP_TABLE_SIZE) {
      /* recycle oldest pending */
      i = old_pending;
      LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("find_entry: selecting oldest pending entry %"U16_F" (without queue)\n", (uint16_t)i));
    /* 4) found recyclable pending entry with queued packets? */
    } else if (old_queue < ARP_TABLE_SIZE) {
      /* recycle oldest pending (queued packets are free in free_entry) */
      i = old_queue;
      LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("find_entry: selecting oldest pending entry %"U16_F", freeing packet queue %p\n", (uint16_t)i, (void *)(arp_table[i].q)));
      /* no empty or recyclable entries found */
    } else {
      LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("find_entry: no empty or recyclable entries found\n"));
      return (INT8)ERR_MEM;
    }

    /* { empty or recyclable entry found } */
    LWIP_ASSERT("i < ARP_TABLE_SIZE", i < ARP_TABLE_SIZE);
    pGlobalBlock->pTcpipBlock->fnetharp_free_entry(i);
  }

  LWIP_ASSERT("i < ARP_TABLE_SIZE", i < ARP_TABLE_SIZE);
  LWIP_ASSERT("arp_table[i].state == ETHARP_STATE_EMPTY",
    arp_table[i].state == ETHARP_STATE_EMPTY);

  /* IP address given? */
  if (ipaddr != NULL) {
    /* set IP address */
    ip_addr_copy(pGlobalBlock->pTcpipBlock->arp_table[i].ipaddr, *ipaddr);
  }
  pGlobalBlock->pTcpipBlock->arp_table[i].ctime = 0;
  return (err_t)i;
}

/**
 * Send an IP packet on the network using netif->linkoutput
 * The ethernet header is filled in before sending.
 *
 * @params netif the lwIP network interface on which to send the packet
 * @params p the packet to send, p->payload pointing to the (uninitialized) ethernet header
 * @params src the source MAC address to be copied into the ethernet header
 * @params dst the destination MAC address to be copied into the ethernet header
 * @return ERR_OK if the packet was sent, any other err_t on failure
 */
err_t
etharp_send_ip(struct netif *netif, struct pbuf *p, struct eth_addr *src, struct eth_addr *dst)
{
  struct eth_hdr *ethhdr = (struct eth_hdr *)p->payload;
  USE_GLOBAL_BLOCK

  LWIP_ASSERT("netif->hwaddr_len must be the same as ETHARP_HWADDR_LEN for etharp!",
              (netif->hwaddr_len == ETHARP_HWADDR_LEN));
  ETHADDR32_COPY(&ethhdr->dest, dst);
  ETHADDR16_COPY(&ethhdr->src, src);
  ethhdr->type = PP_HTONS(ETHTYPE_IP);
  LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_send_ip: sending packet %p\n", (void *)p));
  /* send the packet */
  return netif->linkoutput(netif, p);
}

/**
 * Update (or insert) a IP/MAC address pair in the ARP cache.
 *
 * If a pending entry is resolved, any queued packets will be sent
 * at this point.
 * 
 * @param netif netif related to this entry (used for NETIF_ADDRHINT)
 * @param ipaddr IP address of the inserted ARP entry.
 * @param ethaddr Ethernet address of the inserted ARP entry.
 * @param flags @see definition of ETHARP_FLAG_*
 *
 * @return
 * - ERR_OK Succesfully updated ARP cache.
 * - ERR_MEM If we could not add a new ARP entry when ETHARP_FLAG_TRY_HARD was set.
 * - ERR_ARG Non-unicast address given, those will not appear in ARP cache.
 *
 * @see pbuf_free()
 */
err_t
etharp_update_arp_entry(struct netif *netif, ip_addr_t *ipaddr, struct eth_addr *ethaddr, UINT8 flags)
{
  INT8 i;
  USE_GLOBAL_BLOCK

  LWIP_ASSERT("netif->hwaddr_len == ETHARP_HWADDR_LEN", netif->hwaddr_len == ETHARP_HWADDR_LEN);
  LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("update_arp_entry: %"U16_F".%"U16_F".%"U16_F".%"U16_F" - %02"X16_F":%02"X16_F":%02"X16_F":%02"X16_F":%02"X16_F":%02"X16_F"\n",
    ip4_addr1_16(ipaddr), ip4_addr2_16(ipaddr), ip4_addr3_16(ipaddr), ip4_addr4_16(ipaddr),
    ethaddr->addr[0], ethaddr->addr[1], ethaddr->addr[2],
    ethaddr->addr[3], ethaddr->addr[4], ethaddr->addr[5]));
  /* non-unicast address? */
  if (ip_addr_isany(ipaddr) ||
      ip_addr_isbroadcast(ipaddr, netif) ||
      ip_addr_ismulticast(ipaddr)) {
    LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("update_arp_entry: will not add non-unicast IP address to ARP cache\n"));
    return ERR_ARG;
  }
  /* find or create ARP entry */
  i = pGlobalBlock->pTcpipBlock->fnetharp_find_entry(ipaddr, flags);
  /* bail out if no entry could be found */
  if (i < 0) {
    return (err_t)i;
  }

#if ETHARP_SUPPORT_STATIC_ENTRIES
  if (flags & ETHARP_FLAG_STATIC_ENTRY) {
    /* record static type */
    arp_table[i].state = ETHARP_STATE_STATIC;
  } else
#endif /* ETHARP_SUPPORT_STATIC_ENTRIES */
  {
	  /* mark it stable */
	  pGlobalBlock->pTcpipBlock->arp_table[i].state = ETHARP_STATE_STABLE;
  }

  /* record network interface */
#if LWIP_SNMP
  pGlobalBlock->pTcpipBlock->arp_table[i].netif = netif;
#endif /* LWIP_SNMP */
  /* insert in SNMP ARP index tree */
  snmp_insert_arpidx_tree(netif, &arp_table[i].ipaddr);

  LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("update_arp_entry: updating stable entry %"S16_F"\n", (INT16)i));
  /* update address */
  ETHADDR32_COPY(&pGlobalBlock->pTcpipBlock->arp_table[i].ethaddr, ethaddr);
  /* reset time stamp */
  pGlobalBlock->pTcpipBlock->arp_table[i].ctime = 0;
  /* this is where we will send out queued packets! */
#if ARP_QUEUEING
  while (pGlobalBlock->pTcpipBlock->arp_table[i].q != NULL) {
    struct pbuf *p;
    /* remember remainder of queue */
    struct etharp_q_entry *q = pGlobalBlock->pTcpipBlock->arp_table[i].q;
    /* pop first item off the queue */
    pGlobalBlock->pTcpipBlock->arp_table[i].q = q->next;
    /* get the packet pointer */
    p = q->p;
    /* now queue entry can be freed */
    memp_free(MEMP_ARP_QUEUE, q);
#else /* ARP_QUEUEING */
  if (arp_table[i].q != NULL) {
    struct pbuf *p = arp_table[i].q;
    arp_table[i].q = NULL;
#endif /* ARP_QUEUEING */
    /* send the queued IP packet */
    pGlobalBlock->pTcpipBlock->fnetharp_send_ip(netif, p, (struct eth_addr*)(netif->hwaddr), ethaddr);
    /* free the queued IP packet */
    pGlobalBlock->pTcpipBlock->fnpbuf_free(p);
  }
  return ERR_OK;
}

#if ETHARP_SUPPORT_STATIC_ENTRIES
/** Add a new static entry to the ARP table. If an entry exists for the
 * specified IP address, this entry is overwritten.
 * If packets are queued for the specified IP address, they are sent out.
 *
 * @param ipaddr IP address for the new static entry
 * @param ethaddr ethernet address for the new static entry
 * @return @see return values of etharp_add_static_entry
 */
err_t
etharp_add_static_entry(ip_addr_t *ipaddr, struct eth_addr *ethaddr)
{
  struct netif *netif;
  LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_add_static_entry: %"U16_F".%"U16_F".%"U16_F".%"U16_F" - %02"X16_F":%02"X16_F":%02"X16_F":%02"X16_F":%02"X16_F":%02"X16_F"\n",
    ip4_addr1_16(ipaddr), ip4_addr2_16(ipaddr), ip4_addr3_16(ipaddr), ip4_addr4_16(ipaddr),
    ethaddr->addr[0], ethaddr->addr[1], ethaddr->addr[2],
    ethaddr->addr[3], ethaddr->addr[4], ethaddr->addr[5]));

  netif = pTcpipBlock->fnip_route(ipaddr);
  if (netif == NULL) {
    return ERR_RTE;
  }

  return pTcpipBlock->fnupdate_arp_entry(netif, ipaddr, ethaddr, ETHARP_FLAG_TRY_HARD | ETHARP_FLAG_STATIC_ENTRY);
}

/** Remove a static entry from the ARP table previously added with a call to
 * etharp_add_static_entry.
 *
 * @param ipaddr IP address of the static entry to remove
 * @return ERR_OK: entry removed
 *         ERR_MEM: entry wasn't found
 *         ERR_ARG: entry wasn't a static entry but a dynamic one
 */
err_t
etharp_remove_static_entry(ip_addr_t *ipaddr)
{
  INT8 i;
  LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_remove_static_entry: %"U16_F".%"U16_F".%"U16_F".%"U16_F"\n",
    ip4_addr1_16(ipaddr), ip4_addr2_16(ipaddr), ip4_addr3_16(ipaddr), ip4_addr4_16(ipaddr)));

  /* find or create ARP entry */
  i = pTcpipBlock->fnetharp_find_entry(ipaddr, ETHARP_FLAG_FIND_ONLY);
  /* bail out if no entry could be found */
  if (i < 0) {
    return (err_t)i;
  }

  if (arp_table[i].state != ETHARP_STATE_STATIC) {
    /* entry wasn't a static entry, cannot remove it */
    return ERR_ARG;
  }
  /* entry found, free it */
  pTcpipBlock->fnetharp_free_entry(i);
  return ERR_OK;
}
#endif /* ETHARP_SUPPORT_STATIC_ENTRIES */

/**
 * Remove all ARP table entries of the specified netif.
 *
 * @param netif points to a network interface
 */
void etharp_cleanup_netif(struct netif *netif)
{
  u8_t i;

  for (i = 0; i < ARP_TABLE_SIZE; ++i) {
    u8_t state = arp_table[i].state;
    if ((state != ETHARP_STATE_EMPTY) && (arp_table[i].netif == netif)) {
      etharp_free_entry(i);
    }
  }
}

/**
 * Finds (stable) ethernet/IP address pair from ARP table
 * using interface and IP address index.
 * @note the addresses in the ARP table are in network order!
 *
 * @param netif points to interface index
 * @param ipaddr points to the (network order) IP address index
 * @param eth_ret points to return pointer
 * @param ip_ret points to return pointer
 * @return table index if found, -1 otherwise
 */
INT8
etharp_find_addr(struct netif *netif, ip_addr_t *ipaddr,
         struct eth_addr **eth_ret, ip_addr_t **ip_ret)
{
  INT8 i;
  USE_GLOBAL_BLOCK

  LWIP_ASSERT("eth_ret != NULL && ip_ret != NULL",
    eth_ret != NULL && ip_ret != NULL);

  i = pGlobalBlock->pTcpipBlock->fnetharp_find_entry(ipaddr, ETHARP_FLAG_FIND_ONLY);
  if((i >= 0) && (pGlobalBlock->pTcpipBlock->arp_table[i].state >= ETHARP_STATE_STABLE)) {
      *eth_ret = &pGlobalBlock->pTcpipBlock->arp_table[i].ethaddr;
      *ip_ret = &pGlobalBlock->pTcpipBlock->arp_table[i].ipaddr;
      return i;
  }
  return -1;
}

#if ETHARP_TRUST_IP_MAC
/**
 * Updates the ARP table using the given IP packet.
 *
 * Uses the incoming IP packet's source address to update the
 * ARP cache for the local network. The function does not alter
 * or free the packet. This function must be called before the
 * packet p is passed to the IP layer.
 *
 * @param netif The lwIP network interface on which the IP packet pbuf arrived.
 * @param p The IP packet that arrived on netif.
 *
 * @return NULL
 *
 * @see pbuf_free()
 */
void
etharp_ip_input(struct netif *netif, struct pbuf *p)
{
  struct eth_hdr *ethhdr;
  struct ip_hdr *iphdr;
  ip_addr_t iphdr_src;
  USE_GLOBAL_BLOCK

  LWIP_ERROR("netif != NULL", (netif != NULL), return;);

  /* Only insert an entry if the source IP address of the
     incoming IP packet comes from a host on the local network. */
  ethhdr = (struct eth_hdr *)p->payload;
  iphdr = (struct ip_hdr *)((UINT8*)ethhdr + SIZEOF_ETH_HDR);
#if ETHARP_SUPPORT_VLAN
  if (ethhdr->type == PP_HTONS(ETHTYPE_VLAN)) {
    iphdr = (struct ip_hdr *)((UINT8*)ethhdr + SIZEOF_ETH_HDR + SIZEOF_VLAN_HDR);
  }
#endif /* ETHARP_SUPPORT_VLAN */

  ip_addr_copy(iphdr_src, iphdr->src);

  /* source is not on the local network? */
  if (!ip_addr_netcmp(&iphdr_src, &(netif->ip_addr), &(netif->netmask))) {
    /* do nothing */
    return;
  }

  LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_ip_input: updating ETHARP table.\n"));
  /* update the source IP address in the cache, if present */
  /* @todo We could use ETHARP_FLAG_TRY_HARD if we think we are going to talk
   * back soon (for example, if the destination IP address is ours. */
  pTcpipBlock->fnupdate_arp_entry(netif, &iphdr_src, &(ethhdr->src), ETHARP_FLAG_FIND_ONLY);
}
#endif /* ETHARP_TRUST_IP_MAC */

/**
 * Responds to ARP requests to us. Upon ARP replies to us, add entry to cache  
 * send out queued IP packets. Updates cache with snooped address pairs.
 *
 * Should be called for incoming ARP packets. The pbuf in the argument
 * is freed by this function.
 *
 * @param netif The lwIP network interface on which the ARP packet pbuf arrived.
 * @param ethaddr Ethernet address of netif.
 * @param p The ARP packet that arrived on netif. Is freed by this function.
 *
 * @return NULL
 *
 * @see pbuf_free()
 */
void
etharp_arp_input(struct netif *netif, struct eth_addr *ethaddr, struct pbuf *p)
{
  uint16_t opcode;
  struct etharp_hdr *hdr;
  struct eth_hdr *ethhdr;
  /* these are aligned properly, whereas the ARP header fields might not be */
  ip_addr_t sipaddr, dipaddr;
  UINT8 for_us;
#if LWIP_AUTOIP
  const UINT8 * ethdst_hwaddr;
#endif /* LWIP_AUTOIP */
  USE_GLOBAL_BLOCK

  LWIP_ERROR("netif != NULL", (netif != NULL), return;);

  /* drop short ARP packets: we have to check for p->len instead of p->tot_len here
     since a struct etharp_hdr is pointed to p->payload, so it musn't be chained! */
  if (p->len < SIZEOF_ETHARP_PACKET) {
    LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_WARNING,
      ("etharp_arp_input: packet dropped, too short (%"S16_F"/%"S16_F")\n", p->tot_len,
      (INT16)SIZEOF_ETHARP_PACKET));
    ETHARP_STATS_INC(etharp.lenerr);
    ETHARP_STATS_INC(etharp.drop);
    pGlobalBlock->pTcpipBlock->fnpbuf_free(p);
    return;
  }

  ethhdr = (struct eth_hdr *)p->payload;
  hdr = (struct etharp_hdr *)((UINT8*)ethhdr + SIZEOF_ETH_HDR);
#if ETHARP_SUPPORT_VLAN
  if (ethhdr->type == PP_HTONS(ETHTYPE_VLAN)) {
    hdr = (struct etharp_hdr *)(((UINT8*)ethhdr) + SIZEOF_ETH_HDR + SIZEOF_VLAN_HDR);
  }
#endif /* ETHARP_SUPPORT_VLAN */

  /* RFC 826 "Packet Reception": */
  if ((hdr->hwtype != PP_HTONS(HWTYPE_ETHERNET)) ||
      (hdr->hwlen != ETHARP_HWADDR_LEN) ||
      (hdr->protolen != sizeof(ip_addr_t)) ||
      (hdr->proto != PP_HTONS(ETHTYPE_IP)))  {
    LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_WARNING,
      ("etharp_arp_input: packet dropped, wrong hw type, hwlen, proto, protolen or ethernet type (%"U16_F"/%"U16_F"/%"U16_F"/%"U16_F")\n",
      hdr->hwtype, hdr->hwlen, hdr->proto, hdr->protolen, ethhdr->type));
    ETHARP_STATS_INC(etharp.proterr);
    ETHARP_STATS_INC(etharp.drop);
    pGlobalBlock->pTcpipBlock->fnpbuf_free(p);
    return;
  }
  ETHARP_STATS_INC(etharp.recv);

#if LWIP_AUTOIP
  /* We have to check if a host already has configured our random
   * created link local address and continously check if there is
   * a host with this IP-address so we can detect collisions */
  autoip_arp_reply(netif, hdr);
#endif /* LWIP_AUTOIP */

  /* Copy struct ip_addr2 to aligned ip_addr, to support compilers without
   * structure packing (not using structure copy which breaks strict-aliasing rules). */
  IPADDR2_COPY(&sipaddr, &hdr->sipaddr);
  IPADDR2_COPY(&dipaddr, &hdr->dipaddr);

  /* this interface is not configured? */
  if (ip_addr_isany(&netif->ip_addr)) {
    for_us = 0;
  } else {
    /* ARP packet directed to us? */
    for_us = (UINT8)ip_addr_cmp(&dipaddr, &(netif->ip_addr));
  }

  /* ARP message directed to us?
      -> add IP address in ARP cache; assume requester wants to talk to us,
         can result in directly sending the queued packets for this host.
     ARP message not directed to us?
      ->  update the source IP address in the cache, if present */
  pGlobalBlock->pTcpipBlock->fnetharp_update_arp_entry(netif, &sipaddr, &(hdr->shwaddr),
                   for_us ? ETHARP_FLAG_TRY_HARD : ETHARP_FLAG_FIND_ONLY);

  /* now act on the message itself */
#if 0
//  switch (hdr->opcode) {
  /* ARP request? */
  case PP_HTONS(ARP_REQUEST):
#else
  opcode = hdr->opcode;
  if (opcode == PP_HTONS(ARP_REQUEST)) {
#endif // #if 0
    /* ARP request. If it asked for our address, we send out a
     * reply. In any case, we time-stamp any existing ARP entry,
     * and possiby send out an IP packet that was queued on it. */

    LWIP_DEBUGF (ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_arp_input: incoming ARP request\n"));
    /* ARP request for our address? */
    if (for_us) {

      LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_arp_input: replying to ARP request for our IP address\n"));
      /* Re-use pbuf to send ARP reply.
         Since we are re-using an existing pbuf, we can't call etharp_raw since
         that would allocate a new pbuf. */
      hdr->opcode = HTONS(ARP_REPLY);

      IPADDR2_COPY(&hdr->dipaddr, &hdr->sipaddr);
      IPADDR2_COPY(&hdr->sipaddr, &netif->ip_addr);

      LWIP_ASSERT("netif->hwaddr_len must be the same as ETHARP_HWADDR_LEN for etharp!",
                  (netif->hwaddr_len == ETHARP_HWADDR_LEN));
#if LWIP_AUTOIP
      /* If we are using Link-Local, all ARP packets that contain a Link-Local
       * 'sender IP address' MUST be sent using link-layer broadcast instead of
       * link-layer unicast. (See RFC3927 Section 2.5, last paragraph) */
      ethdst_hwaddr = ip_addr_islinklocal(&netif->ip_addr) ? (UINT8*)(pTcpipBlock->ethbroadcast.addr) : hdr->shwaddr.addr;
#endif /* LWIP_AUTOIP */

      ETHADDR16_COPY(&hdr->dhwaddr, &hdr->shwaddr);
#if LWIP_AUTOIP
      ETHADDR16_COPY(&ethhdr->dest, ethdst_hwaddr);
#else  /* LWIP_AUTOIP */
      ETHADDR16_COPY(&ethhdr->dest, &hdr->shwaddr);
#endif /* LWIP_AUTOIP */
      ETHADDR16_COPY(&hdr->shwaddr, ethaddr);
      ETHADDR16_COPY(&ethhdr->src, ethaddr);

      /* hwtype, hwaddr_len, proto, protolen and the type in the ethernet header
         are already correct, we tested that before */

      /* return ARP reply */
      netif->linkoutput(netif, p);
    /* we are not configured? */
    } else if (ip_addr_isany(&netif->ip_addr)) {
      /* { for_us == 0 and netif->ip_addr.addr == 0 } */
      LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_arp_input: we are unconfigured, ARP request ignored.\n"));
    /* request was not directed to us */
    } else {
      /* { for_us == 0 and netif->ip_addr.addr != 0 } */
      LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_arp_input: ARP request was not for us.\n"));
    }
#if 0
    break;
  case PP_HTONS(ARP_REPLY):
#else
  }
  else if (opcode == PP_HTONS(ARP_REPLY)) {
#endif // #if 0
    /* ARP reply. We already updated the ARP cache earlier. */
    LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_arp_input: incoming ARP reply\n"));
#if (LWIP_DHCP && DHCP_DOES_ARP_CHECK)
    /* DHCP wants to know about ARP replies from any host with an
     * IP address also offered to us by the DHCP server. We do not
     * want to take a duplicate IP address on a single network.
     * @todo How should we handle redundant (fail-over) interfaces? */
    dhcp_arp_reply(netif, &sipaddr);
#endif /* (LWIP_DHCP && DHCP_DOES_ARP_CHECK) */
#if 0
    break;
  default:
    LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_arp_input: ARP unknown opcode type %"S16_F"\n", htons(hdr->opcode)));
    ETHARP_STATS_INC(etharp.err);
    break;
#endif // #if 0
  }
  /* free ARP packet */
  pGlobalBlock->pTcpipBlock->fnpbuf_free(p);
}

/** Just a small helper function that sends a pbuf to an ethernet address
 * in the arp_table specified by the index 'arp_idx'.
 */
err_t
etharp_output_to_arp_index(struct netif *netif, struct pbuf *q, UINT8 arp_idx)
{
  USE_GLOBAL_BLOCK

  LWIP_ASSERT("arp_table[arp_idx].state >= ETHARP_STATE_STABLE",
              pGlobalBlock->pTcpipBlock->arp_table[arp_idx].state >= ETHARP_STATE_STABLE);
  /* if arp table entry is about to expire: re-request it,
     but only if its state is ETHARP_STATE_STABLE to prevent flooding the
     network with ARP requests if this address is used frequently. */
  if ((pGlobalBlock->pTcpipBlock->arp_table[arp_idx].state == ETHARP_STATE_STABLE) && 
      (pGlobalBlock->pTcpipBlock->arp_table[arp_idx].ctime >= ARP_AGE_REREQUEST_USED)) {
    if (pGlobalBlock->pTcpipBlock->fnetharp_request(netif, &pGlobalBlock->pTcpipBlock->arp_table[arp_idx].ipaddr) == ERR_OK) {
      pGlobalBlock->pTcpipBlock->arp_table[arp_idx].state = ETHARP_STATE_STABLE_REREQUESTING;
    }
  }
  
  return pGlobalBlock->pTcpipBlock->fnetharp_send_ip(netif, q, (struct eth_addr*)(netif->hwaddr),
    &pGlobalBlock->pTcpipBlock->arp_table[arp_idx].ethaddr);
}

/**
 * Resolve and fill-in Ethernet address header for outgoing IP packet.
 *
 * For IP multicast and broadcast, corresponding Ethernet addresses
 * are selected and the packet is transmitted on the link.
 *
 * For unicast addresses, the packet is submitted to etharp_query(). In
 * case the IP address is outside the local network, the IP address of
 * the gateway is used.
 *
 * @param netif The lwIP network interface which the IP packet will be sent on.
 * @param q The pbuf(s) containing the IP packet to be sent.
 * @param ipaddr The IP address of the packet destination.
 *
 * @return
 * - ERR_RTE No route to destination (no gateway to external networks),
 * or the return type of either etharp_query() or etharp_send_ip().
 */
err_t
etharp_output(struct netif *netif, struct pbuf *q, ip_addr_t *ipaddr)
{
  struct eth_addr *dest;
  struct eth_addr mcastaddr;
  ip_addr_t *dst_addr = ipaddr;
  USE_GLOBAL_BLOCK

  LWIP_ASSERT("netif != NULL", netif != NULL);
  LWIP_ASSERT("q != NULL", q != NULL);
  LWIP_ASSERT("ipaddr != NULL", ipaddr != NULL);

  /* make room for Ethernet header - should not fail */
  if (pGlobalBlock->pTcpipBlock->fnpbuf_header(q, sizeof(struct eth_hdr)) != 0) {
    /* bail out */
    LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_SERIOUS,
      ("etharp_output: could not allocate room for header.\n"));
    LINK_STATS_INC(link.lenerr);
    return ERR_BUF;
  }

    /* outside local network? if so, this can neither be a global broadcast nor
     a subnet broadcast. */
  if (!ip_addr_netcmp(ipaddr, &(netif->ip_addr), &(netif->netmask)) &&
      !ip_addr_islinklocal(ipaddr)) {
#if LWIP_AUTOIP
    struct ip_hdr *iphdr = (struct ip_hdr*)((UINT8*)q->payload +
      sizeof(struct eth_hdr));
    /* According to RFC 3297, chapter 2.6.2 (Forwarding Rules), a packet with
       a link-local source address must always be "directly to its destination
       on the same physical link. The host MUST NOT send the packet to any
       router for forwarding". */
    if (!ip_addr_islinklocal(&iphdr->src))
#endif /* LWIP_AUTOIP */
    {
      /* interface has default gateway? */
      if (!ip_addr_isany(&netif->gw)) {
        /* send to hardware address of default gateway IP address */
        dst_addr = &(netif->gw);
      /* no default gateway available */
      } else {
        /* no route to destination error (default gateway missing) */
        return ERR_RTE;
      }
    }
  }
#if LWIP_NETIF_HWADDRHINT
  if (netif->addr_hint != NULL) {
    /* per-pcb cached entry was given */
    UINT8 etharp_cached_entry = *(netif->addr_hint);
    if (etharp_cached_entry < ARP_TABLE_SIZE) {
#endif /* LWIP_NETIF_HWADDRHINT */
      if ((pGlobalBlock->pTcpipBlock->arp_table[pGlobalBlock->pTcpipBlock->etharp_cached_entry].state >= ETHARP_STATE_STABLE) &&
          (ip_addr_cmp(dst_addr, &pGlobalBlock->pTcpipBlock->arp_table[pGlobalBlock->pTcpipBlock->etharp_cached_entry].ipaddr))) {
        /* the per-pcb-cached entry is stable and the right one! */
        ETHARP_STATS_INC(etharp.cachehit);
        return pGlobalBlock->pTcpipBlock->fnetharp_output_to_arp_index(netif, q, pGlobalBlock->pTcpipBlock->etharp_cached_entry);
      }
#if LWIP_NETIF_HWADDRHINT
    }
  }
#endif /* LWIP_NETIF_HWADDRHINT */

  /* Determine on destination hardware address. Broadcasts and multicasts
   * are special, other IP addresses are looked up in the ARP table. */

  /* broadcast destination IP address? */
  if (ip_addr_isbroadcast(ipaddr, netif)) {
    /* broadcast on Ethernet also */
    dest = (struct eth_addr *)&pGlobalBlock->pTcpipBlock->ethbroadcast;
  /* multicast destination IP address? */
  } else if (ip_addr_ismulticast(ipaddr)) {
    /* Hash IP multicast address to MAC address.*/
    mcastaddr.addr[0] = LL_MULTICAST_ADDR_0;
    mcastaddr.addr[1] = LL_MULTICAST_ADDR_1;
    mcastaddr.addr[2] = LL_MULTICAST_ADDR_2;
    mcastaddr.addr[3] = ip4_addr2(ipaddr) & 0x7f;
    mcastaddr.addr[4] = ip4_addr3(ipaddr);
    mcastaddr.addr[5] = ip4_addr4(ipaddr);
    /* destination Ethernet address is multicast */
    dest = &mcastaddr;
  /* unicast destination IP address? */
  } else {
    INT8 i;
    /* find stable entry: do this here since this is a critical path for
       throughput and find_entry() is kind of slow */
    for (i = 0; i < ARP_TABLE_SIZE; i++) {
      if ((pGlobalBlock->pTcpipBlock->arp_table[i].state >= ETHARP_STATE_STABLE) &&
          (ip_addr_cmp(dst_addr, &pGlobalBlock->pTcpipBlock->arp_table[i].ipaddr))) {
        /* found an existing, stable entry */
        ETHARP_SET_HINT(netif, i);
        return pGlobalBlock->pTcpipBlock->fnetharp_output_to_arp_index(netif, q, i);
      }
    }
    /* no stable entry found, use the (slower) query function:
       queue on destination Ethernet address belonging to ipaddr */
    return pGlobalBlock->pTcpipBlock->fnetharp_query(netif, dst_addr, q);
  }

  /* continuation for multicast/broadcast destinations */
  /* obtain source Ethernet address of the given interface */
  /* send packet directly on the link */
  return pGlobalBlock->pTcpipBlock->fnetharp_send_ip(netif, q, (struct eth_addr*)(netif->hwaddr), dest);
}

/**
 * Send an ARP request for the given IP address and/or queue a packet.
 *
 * If the IP address was not yet in the cache, a pending ARP cache entry
 * is added and an ARP request is sent for the given address. The packet
 * is queued on this entry.
 *
 * If the IP address was already pending in the cache, a new ARP request
 * is sent for the given address. The packet is queued on this entry.
 *
 * If the IP address was already stable in the cache, and a packet is
 * given, it is directly sent and no ARP request is sent out. 
 * 
 * If the IP address was already stable in the cache, and no packet is
 * given, an ARP request is sent out.
 * 
 * @param netif The lwIP network interface on which ipaddr
 * must be queried for.
 * @param ipaddr The IP address to be resolved.
 * @param q If non-NULL, a pbuf that must be delivered to the IP address.
 * q is not freed by this function.
 *
 * @note q must only be ONE packet, not a packet queue!
 *
 * @return
 * - ERR_BUF Could not make room for Ethernet header.
 * - ERR_MEM Hardware address unknown, and no more ARP entries available
 *   to query for address or queue the packet.
 * - ERR_MEM Could not queue packet due to memory shortage.
 * - ERR_RTE No route to destination (no gateway to external networks).
 * - ERR_ARG Non-unicast address given, those will not appear in ARP cache.
 *
 */
err_t
etharp_query(struct netif *netif, ip_addr_t *ipaddr, struct pbuf *q)
{
  struct eth_addr * srcaddr = (struct eth_addr *)netif->hwaddr;
  err_t result = ERR_MEM;
  INT8 i; /* ARP entry index */
  USE_GLOBAL_BLOCK

  /* non-unicast address? */
  if (ip_addr_isbroadcast(ipaddr, netif) ||
      ip_addr_ismulticast(ipaddr) ||
      ip_addr_isany(ipaddr)) {
    LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_query: will not add non-unicast IP address to ARP cache\n"));
    return ERR_ARG;
  }

  /* find entry in ARP cache, ask to create entry if queueing packet */
  i = pGlobalBlock->pTcpipBlock->fnetharp_find_entry(ipaddr, ETHARP_FLAG_TRY_HARD);

  /* could not find or create entry? */
  if (i < 0) {
    LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_query: could not create ARP entry\n"));
    if (q) {
      LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_query: packet dropped\n"));
      ETHARP_STATS_INC(etharp.memerr);
    }
    return (err_t)i;
  }

  /* mark a fresh entry as pending (we just sent a request) */
  if (pGlobalBlock->pTcpipBlock->arp_table[i].state == ETHARP_STATE_EMPTY) {
    pGlobalBlock->pTcpipBlock->arp_table[i].state = ETHARP_STATE_PENDING;
  }

  /* { i is either a STABLE or (new or existing) PENDING entry } */
  LWIP_ASSERT("arp_table[i].state == PENDING or STABLE",
  ((pGlobalBlock->pTcpipBlock->arp_table[i].state == ETHARP_STATE_PENDING) ||
   (pGlobalBlock->pTcpipBlock->arp_table[i].state >= ETHARP_STATE_STABLE)));

  /* do we have a pending entry? or an implicit query request? */
  if ((pGlobalBlock->pTcpipBlock->arp_table[i].state == ETHARP_STATE_PENDING) || (q == NULL)) {
    /* try to resolve it; send out ARP request */
    result = pGlobalBlock->pTcpipBlock->fnetharp_request(netif, ipaddr);
    if (result != ERR_OK) {
      /* ARP request couldn't be sent */
      /* We don't re-send arp request in etharp_tmr, but we still queue packets,
         since this failure could be temporary, and the next packet calling
         etharp_query again could lead to sending the queued packets. */
    }
    if (q == NULL) {
      return result;
    }
  }

  /* packet given? */
  LWIP_ASSERT("q != NULL", q != NULL);
  /* stable entry? */
  if (pGlobalBlock->pTcpipBlock->arp_table[i].state >= ETHARP_STATE_STABLE) {
    /* we have a valid IP->Ethernet address mapping */
    ETHARP_SET_HINT(netif, i);
    /* send the packet */
    result = pGlobalBlock->pTcpipBlock->fnetharp_send_ip(netif, q, srcaddr, &(pGlobalBlock->pTcpipBlock->arp_table[i].ethaddr));
  /* pending entry? (either just created or already pending */
  } else if (pGlobalBlock->pTcpipBlock->arp_table[i].state == ETHARP_STATE_PENDING) {
    /* entry is still pending, queue the given packet 'q' */
    struct pbuf *p;
    int copy_needed = 0;
    /* IF q includes a PBUF_REF, PBUF_POOL or PBUF_RAM, we have no choice but
     * to copy the whole queue into a new PBUF_RAM (see bug #11400) 
     * PBUF_ROMs can be left as they are, since ROM must not get changed. */
    p = q;
    while (p) {
      LWIP_ASSERT("no packet queues allowed!", (p->len != p->tot_len) || (p->next == 0));
      if(p->type != PBUF_ROM) {
        copy_needed = 1;
        break;
      }
      p = p->next;
    }
    if(copy_needed) {
      /* copy the whole packet into new pbufs */
      p = pGlobalBlock->pTcpipBlock->fnpbuf_alloc(PBUF_RAW, p->tot_len, PBUF_RAM);
      if(p != NULL) {
        if (pGlobalBlock->pTcpipBlock->fnpbuf_copy(p, q) != ERR_OK) {
          pGlobalBlock->pTcpipBlock->fnpbuf_free(p);
          p = NULL;
        }
      }
    } else {
      /* referencing the old pbuf is enough */
      p = q;
      pGlobalBlock->pTcpipBlock->fnpbuf_ref(p);
    }
    /* packet could be taken over? */
    if (p != NULL) {
      /* queue packet ... */
#if ARP_QUEUEING
      struct etharp_q_entry *new_entry;
      /* allocate a new arp queue entry */
      new_entry = (struct etharp_q_entry *)memp_malloc(MEMP_ARP_QUEUE);
      if (new_entry != NULL) {
        new_entry->next = 0;
        new_entry->p = p;
        if(pGlobalBlock->pTcpipBlock->arp_table[i].q != NULL) {
          /* queue was already existent, append the new entry to the end */
          struct etharp_q_entry *r;
          r = pGlobalBlock->pTcpipBlock->arp_table[i].q;
          while (r->next != NULL) {
            r = r->next;
          }
          r->next = new_entry;
        } else {
          /* queue did not exist, first item in queue */
          pGlobalBlock->pTcpipBlock->arp_table[i].q = new_entry;
        }
        LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_query: queued packet %p on ARP entry %"S16_F"\n", (void *)q, (INT16)i));
        result = ERR_OK;
      } else {
        /* the pool MEMP_ARP_QUEUE is empty */
        pGlobalBlock->pTcpipBlock->fnpbuf_free(p);
        LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_query: could not queue a copy of PBUF_REF packet %p (out of memory)\n", (void *)q));
        result = ERR_MEM;
      }
#else /* ARP_QUEUEING */
      /* always queue one packet per ARP request only, freeing a previously queued packet */
      if (arp_table[i].q != NULL) {
        LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_query: dropped previously queued packet %p for ARP entry %"S16_F"\n", (void *)q, (INT16)i));
        pGlobalBlock->pTcpipBlock->fnpbuf_free(arp_table[i].q);
      }
      arp_table[i].q = p;
      result = ERR_OK;
      LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_query: queued packet %p on ARP entry %"S16_F"\n", (void *)q, (INT16)i));
#endif /* ARP_QUEUEING */
    } else {
      ETHARP_STATS_INC(etharp.memerr);
      LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_query: could not queue a copy of PBUF_REF packet %p (out of memory)\n", (void *)q));
      result = ERR_MEM;
    }
  }
  return result;
}

/**
 * Send a raw ARP packet (opcode and all addresses can be modified)
 *
 * @param netif the lwip network interface on which to send the ARP packet
 * @param ethsrc_addr the source MAC address for the ethernet header
 * @param ethdst_addr the destination MAC address for the ethernet header
 * @param hwsrc_addr the source MAC address for the ARP protocol header
 * @param ipsrc_addr the source IP address for the ARP protocol header
 * @param hwdst_addr the destination MAC address for the ARP protocol header
 * @param ipdst_addr the destination IP address for the ARP protocol header
 * @param opcode the type of the ARP packet
 * @return ERR_OK if the ARP packet has been sent
 *         ERR_MEM if the ARP packet couldn't be allocated
 *         any other err_t on failure
 */
#if !LWIP_AUTOIP
#endif /* LWIP_AUTOIP */
err_t
etharp_raw(struct netif *netif, const struct eth_addr *ethsrc_addr,
           const struct eth_addr *ethdst_addr,
           const struct eth_addr *hwsrc_addr, const ip_addr_t *ipsrc_addr,
           const struct eth_addr *hwdst_addr, const ip_addr_t *ipdst_addr,
           const uint16_t opcode)
{
  struct pbuf *p;
  err_t result = ERR_OK;
  struct eth_hdr *ethhdr;
  struct etharp_hdr *hdr;
#if LWIP_AUTOIP
  const UINT8 * ethdst_hwaddr;
#endif /* LWIP_AUTOIP */
  USE_GLOBAL_BLOCK

  LWIP_ASSERT("netif != NULL", netif != NULL);

  /* allocate a pbuf for the outgoing ARP request packet */
  p = pGlobalBlock->pTcpipBlock->fnpbuf_alloc(PBUF_RAW, SIZEOF_ETHARP_PACKET, PBUF_RAM);
  /* could allocate a pbuf for an ARP request? */
  if (p == NULL) {
    LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_SERIOUS,
      ("etharp_raw: could not allocate pbuf for ARP request.\n"));
    ETHARP_STATS_INC(etharp.memerr);
    return ERR_MEM;
  }
  LWIP_ASSERT("check that first pbuf can hold struct etharp_hdr",
              (p->len >= SIZEOF_ETHARP_PACKET));

  ethhdr = (struct eth_hdr *)p->payload;
  hdr = (struct etharp_hdr *)((UINT8*)ethhdr + SIZEOF_ETH_HDR);
  LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_raw: sending raw ARP packet.\n"));
  hdr->opcode = HTONS(opcode);

  LWIP_ASSERT("netif->hwaddr_len must be the same as ETHARP_HWADDR_LEN for etharp!",
              (netif->hwaddr_len == ETHARP_HWADDR_LEN));
#if LWIP_AUTOIP
  /* If we are using Link-Local, all ARP packets that contain a Link-Local
   * 'sender IP address' MUST be sent using link-layer broadcast instead of
   * link-layer unicast. (See RFC3927 Section 2.5, last paragraph) */
  ethdst_hwaddr = ip_addr_islinklocal(ipsrc_addr) ? (UINT8*)(pGlobalBlock->pTcpipBlock->ethbroadcast.addr) : ethdst_addr->addr;
#endif /* LWIP_AUTOIP */
  /* Write the ARP MAC-Addresses */
  ETHADDR16_COPY(&hdr->shwaddr, hwsrc_addr);
  ETHADDR16_COPY(&hdr->dhwaddr, hwdst_addr);
  /* Write the Ethernet MAC-Addresses */
#if LWIP_AUTOIP
  ETHADDR16_COPY(&ethhdr->dest, ethdst_hwaddr);
#else  /* LWIP_AUTOIP */
  ETHADDR16_COPY(&ethhdr->dest, ethdst_addr);
#endif /* LWIP_AUTOIP */
  ETHADDR16_COPY(&ethhdr->src, ethsrc_addr);
  /* Copy struct ip_addr2 to aligned ip_addr, to support compilers without
   * structure packing. */ 
  IPADDR2_COPY(&hdr->sipaddr, ipsrc_addr);
  IPADDR2_COPY(&hdr->dipaddr, ipdst_addr);

  hdr->hwtype = PP_HTONS(HWTYPE_ETHERNET);
  hdr->proto = PP_HTONS(ETHTYPE_IP);
  /* set hwlen and protolen */
  hdr->hwlen = ETHARP_HWADDR_LEN;
  hdr->protolen = sizeof(ip_addr_t);

  ethhdr->type = PP_HTONS(ETHTYPE_ARP);
  /* send ARP query */
  result = netif->linkoutput(netif, p);
  ETHARP_STATS_INC(etharp.xmit);
  /* free ARP query packet */
  pGlobalBlock->pTcpipBlock->fnpbuf_free(p);
  p = NULL;
  /* could not allocate pbuf for ARP request */

  return result;
}

/**
 * Send an ARP request packet asking for ipaddr.
 *
 * @param netif the lwip network interface on which to send the request
 * @param ipaddr the IP address for which to ask
 * @return ERR_OK if the request has been sent
 *         ERR_MEM if the ARP packet couldn't be allocated
 *         any other err_t on failure
 */
err_t
etharp_request(struct netif *netif, ip_addr_t *ipaddr)
{
  USE_GLOBAL_BLOCK

  LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_request: sending ARP request.\n"));
  return pGlobalBlock->pTcpipBlock->fnetharp_raw(netif, (struct eth_addr *)netif->hwaddr, &pGlobalBlock->pTcpipBlock->ethbroadcast,
                    (struct eth_addr *)netif->hwaddr, &netif->ip_addr, &pGlobalBlock->pTcpipBlock->ethzero,
                    ipaddr, ARP_REQUEST);
}

#else 

err_t etharp_output(struct netif *netif, struct pbuf *q, ip_addr_t *ipaddr)
{
    uint32_t i;
    ip_addr_t *dst_addr = ipaddr;
    struct eth_hdr* ethhdr;
    USE_GLOBAL_BLOCK

    /* make room for Ethernet header - should not fail */
    if (pGlobalBlock->pTcpipBlock->fnpbuf_header(q, sizeof(struct eth_hdr)) != 0) {
        /* bail out */
        LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_SERIOUS,
          ("etharp_output: could not allocate room for header.\n"));
        LINK_STATS_INC(link.lenerr);
        return ERR_BUF;
    }

    /* outside local network? if so, this can neither be a global broadcast nor a subnet broadcast. */
    if (!ip_addr_netcmp(ipaddr, &(netif->ip_addr), &(netif->netmask)) && !ip_addr_islinklocal(ipaddr)) {
        /* interface has default gateway? */
        if (!ip_addr_isany(&netif->gw)) {
            /* send to hardware address of default gateway IP address */
            dst_addr = &(netif->gw);
            /* no default gateway available */
        }
        else {
            /* no route to destination error (default gateway missing) */
            return ERR_RTE;
        }
    }

    ethhdr = (struct eth_hdr *)q->payload;

    ETHADDR32_COPY(&ethhdr->dest, &pGlobalBlock->pTcpipBlock->arp_table[0].ethaddr);

    for (i = 1; i < 3; ++i) {
        if (pGlobalBlock->pTcpipBlock->arp_table[i].ipaddr.addr == ipaddr->addr) {
            ETHADDR32_COPY(&ethhdr->dest, &pGlobalBlock->pTcpipBlock->arp_table[i].ethaddr);
            break;
        }
    }

    ETHADDR16_COPY(&ethhdr->src, (struct eth_addr*)(netif->hwaddr));
    ethhdr->type = PP_HTONS(ETHTYPE_IP);
    /* send the packet */
    return netif->linkoutput(netif, q);
}

#endif /* LWIP_ARP */

/**
 * Process received ethernet frames. Using this function instead of directly
 * calling ip_input and passing ARP frames through etharp in ethernetif_input,
 * the ARP cache is protected from concurrent access.
 *
 * @param p the recevied packet, p->payload pointing to the ethernet header
 * @param netif the network interface on which the packet was received
 */
err_t
ethernet_input(struct pbuf *p, struct netif *netif)
{
  struct eth_hdr* ethhdr;
  uint16_t type;
  INT16 ip_hdr_offset = SIZEOF_ETH_HDR;
  USE_GLOBAL_BLOCK

  if (p->len <= SIZEOF_ETH_HDR) {
	  /* a packet with only an ethernet header (or less) is not valid for us */
	  ETHARP_STATS_INC(etharp.proterr);
	  ETHARP_STATS_INC(etharp.drop);
	  goto free_and_return;
  }

  /* points to packet payload, which starts with an Ethernet header */
  ethhdr = (struct eth_hdr *)p->payload;
  LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE,
    ("ethernet_input: dest:%"X8_F":%"X8_F":%"X8_F":%"X8_F":%"X8_F":%"X8_F", src:%"X8_F":%"X8_F":%"X8_F":%"X8_F":%"X8_F":%"X8_F", type:%"X16_F"\n",
     (unsigned)ethhdr->dest.addr[0], (unsigned)ethhdr->dest.addr[1], (unsigned)ethhdr->dest.addr[2],
     (unsigned)ethhdr->dest.addr[3], (unsigned)ethhdr->dest.addr[4], (unsigned)ethhdr->dest.addr[5],
     (unsigned)ethhdr->src.addr[0], (unsigned)ethhdr->src.addr[1], (unsigned)ethhdr->src.addr[2],
     (unsigned)ethhdr->src.addr[3], (unsigned)ethhdr->src.addr[4], (unsigned)ethhdr->src.addr[5],
     (unsigned)htons(ethhdr->type)));

  type = ethhdr->type;
#if ETHARP_SUPPORT_VLAN
  if (type == PP_HTONS(ETHTYPE_VLAN)) {
    struct eth_vlan_hdr *vlan = (struct eth_vlan_hdr*)(((char*)ethhdr) + SIZEOF_ETH_HDR);
	if (p->len <= SIZEOF_ETH_HDR + SIZEOF_VLAN_HDR) {
		/* a packet with only an ethernet/vlan header (or less) is not valid for us */
		ETHARP_STATS_INC(etharp.proterr);
		ETHARP_STATS_INC(etharp.drop);
		goto free_and_return;
	}
#if defined(ETHARP_VLAN_CHECK) || defined(ETHARP_VLAN_CHECK_FN) /* if not, allow all VLANs */
#ifdef ETHARP_VLAN_CHECK_FN
	if (!ETHARP_VLAN_CHECK_FN(ethhdr, vlan)) {
#elif defined(ETHARP_VLAN_CHECK)
    if (VLAN_ID(vlan) != ETHARP_VLAN_CHECK) {
#endif
      /* silently ignore this packet: not for our VLAN */
      pGlobalBlock->pTcpipBlock->fnpbuf_free(p);
      return ERR_OK;
    }
#endif /* defined(ETHARP_VLAN_CHECK) || defined(ETHARP_VLAN_CHECK_FN) */
    type = vlan->tpid;
	ip_hdr_offset = SIZEOF_ETH_HDR + SIZEOF_VLAN_HDR;
  }
#endif /* ETHARP_SUPPORT_VLAN */

#if LWIP_ARP_FILTER_NETIF
  netif = LWIP_ARP_FILTER_NETIF_FN(p, netif, htons(type));
#endif /* LWIP_ARP_FILTER_NETIF*/

  if (ethhdr->dest.addr[0] & 1) {
	  /* this might be a multicast or broadcast packet */
	  if (ethhdr->dest.addr[0] == LL_MULTICAST_ADDR_0) {
		  if ((ethhdr->dest.addr[1] == LL_MULTICAST_ADDR_1) &&
			  (ethhdr->dest.addr[2] == LL_MULTICAST_ADDR_2)) {
				  /* mark the pbuf as link-layer multicast */
				  p->flags |= PBUF_FLAG_LLMCAST;
		  }
	  } else if (eth_addr_cmp(&ethhdr->dest, &pGlobalBlock->pTcpipBlock->ethbroadcast)) {
		  /* mark the pbuf as link-layer broadcast */
		  p->flags |= PBUF_FLAG_LLBCAST;
	  }
  }

#if 0
//  switch (type) {
#endif // #if 0
    /* IP packet? */
#if 0
    case PP_HTONS(ETHTYPE_IP):
#else
    if (type == PP_HTONS(ETHTYPE_IP)) {
#endif // #if0
      if (!(netif->flags & NETIF_FLAG_ETHARP)) {
        goto free_and_return;
      }
#if ETHARP_TRUST_IP_MAC
      /* update ARP table */
      etharp_ip_input(netif, p);
#endif /* ETHARP_TRUST_IP_MAC */
      /* skip Ethernet header */
      if(pGlobalBlock->pTcpipBlock->fnpbuf_header(p, -ip_hdr_offset)) {
        LWIP_ASSERT("Can't move over header in packet", 0);
        goto free_and_return;
      } else {
        /* pass to IP layer */
        pGlobalBlock->pTcpipBlock->fnip_input(p, netif);
      }
#if LWIP_ARP
#if 0
      break;
    case PP_HTONS(ETHTYPE_ARP):
#else
    }
	else if (type == PP_HTONS(ETHTYPE_ARP)) {
#endif // #if 0
      if (!(netif->flags & NETIF_FLAG_ETHARP)) {
        goto free_and_return;
      }
      /* pass p to ARP module */
      pGlobalBlock->pTcpipBlock->fnetharp_arp_input(netif, (struct eth_addr*)(netif->hwaddr), p);
#if 0
      break;
#endif // #if 0
#endif /* LWIP_ARP */
#if PPPOE_SUPPORT
    case PP_HTONS(ETHTYPE_PPPOEDISC): /* PPP Over Ethernet Discovery Stage */
      pppoe_disc_input(netif, p);
      break;

    case PP_HTONS(ETHTYPE_PPPOE): /* PPP Over Ethernet Session Stage */
      pppoe_data_input(netif, p);
      break;
#endif /* PPPOE_SUPPORT */

#if LWIP_IPV6
    case PP_HTONS(ETHTYPE_IPV6): /* IPv6 */
      /* skip Ethernet header */
      if(pbuf_header(p, -(s16_t)SIZEOF_ETH_HDR)) {
        LWIP_ASSERT("Can't move over header in packet", 0);
        goto free_and_return;
      } else {
        /* pass to IPv6 layer */
        ip6_input(p, netif);
      }
      break;
#endif /* LWIP_IPV6 */
#if 0
    default:
      ETHARP_STATS_INC(etharp.proterr);
      ETHARP_STATS_INC(etharp.drop);
#else
    }
    else {
#endif
      goto free_and_return;
    }

  /* This means the pbuf is freed or consumed,
     so the caller doesn't have to free it again */
  return ERR_OK;

free_and_return:
  pGlobalBlock->pTcpipBlock->fnpbuf_free(p);
  return ERR_OK;
}

#endif /* LWIP_ARP || LWIP_ETHERNET */