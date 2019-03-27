/**
 * @file
 * Sequential API Internal module
 *
 */

/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
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
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

#include "lwip/opt.h"

//#if LWIP_NETCONN /* don't build if not configured for use in lwipopts.h */

#include "lwip/api_msg.h"

#include "lwip/ip.h"
#include "lwip/udp.h"
#include "lwip/tcp.h"
#include "lwip/raw.h"

#include "lwip/memp.h"
#include "lwip/tcpip.h"
#include "lwip/igmp.h"
#include "lwip/dns.h"
#include "lwip/mld6.h"
#if 0
#include <string.h>
#endif // #if 0
#define SET_NONBLOCKING_CONNECT(conn, val)  do { if(val) { \
  (conn)->flags |= NETCONN_FLAG_IN_NONBLOCKING_CONNECT; \
} else { \
  (conn)->flags &= ~ NETCONN_FLAG_IN_NONBLOCKING_CONNECT; }} while(0)
#define IN_NONBLOCKING_CONNECT(conn) (((conn)->flags & NETCONN_FLAG_IN_NONBLOCKING_CONNECT) != 0)

/* forward declarations */
// #if LWIP_TCP
// err_t do_writemore(struct netconn *conn);
// void do_close_internal(struct netconn *conn);
// #endif

#if LWIP_RAW
/**
 * Receive callback function for RAW netconns.
 * Doesn't 'eat' the packet, only references it and sends it to
 * conn->recvmbox
 *
 * @see raw.h (struct raw_pcb.recv) for parameters and return value
 */
UINT8
recv_raw(void *arg, struct raw_pcb *pcb, struct pbuf *p,
    ip_addr_t *addr)
{
  struct pbuf *q;
  struct netbuf *buf;
  struct netconn *conn;

  LWIP_UNUSED_ARG(addr);
  conn = (struct netconn *)arg;

  if ((conn != NULL) && pGLobalData->fnsys_mbox_valid(&conn->recvmbox)) {
#if LWIP_SO_RCVBUF
    int recv_avail;
    SYS_ARCH_GET(conn->recv_avail, recv_avail);
    if ((recv_avail + (int)(p->tot_len)) > conn->recv_bufsize) {
      return 0;
    }
#endif /* LWIP_SO_RCVBUF */
    /* copy the whole packet into new pbufs */
    q = pTcpipBlock->fnpbuf_alloc(PBUF_RAW, p->tot_len, PBUF_RAM);
    if(q != NULL) {
      if (pTcpipBlock->fnpbuf_copy(q, p) != ERR_OK) {
        pTcpipBlock->fnpbuf_free(q);
        q = NULL;
      }
    }

    if (q != NULL) {
      uint16_t len;
      buf = (struct netbuf *)memp_malloc(MEMP_NETBUF);
      if (buf == NULL) {
        pTcpipBlock->fnpbuf_free(q);
        return 0;
      }

      buf->p = q;
      buf->ptr = q;
      ipX_addr_copy(PCB_ISIPV6(pcb), buf->addr, *ipX_current_src_addr());
      buf->port = pcb->protocol;

      len = q->tot_len;
      if (pTcpipBlock->fnsys_mbox_trypost(&conn->recvmbox, buf) != ERR_OK) {
        pTcpipBlock->fnnetbuf_delete(buf);
        return 0;
      } else {
#if LWIP_SO_RCVBUF
        SYS_ARCH_INC(conn->recv_avail, len);
#endif /* LWIP_SO_RCVBUF */
        /* Register event with callback */
        API_EVENT(conn, NETCONN_EVT_RCVPLUS, len);
      }
    }
  }

  return 0; /* do not eat the packet */
}
#endif /* LWIP_RAW*/

#if LWIP_UDP
/**
 * Receive callback function for UDP netconns.
 * Posts the packet to conn->recvmbox or deletes it on memory error.
 *
 * @see udp.h (struct udp_pcb.recv) for parameters
 */
void
recv_udp(void *arg, struct udp_pcb *pcb, struct pbuf *p,
   ip_addr_t *addr, uint16_t port)
{
  struct netbuf *buf;
  struct netconn *conn;
  uint16_t len;
#if LWIP_SO_RCVBUF
  int recv_avail;
#endif /* LWIP_SO_RCVBUF */
  USE_GLOBAL_BLOCK

  LWIP_ASSERT("recv_udp must have a pcb argument", pcb != NULL);
  LWIP_ASSERT("recv_udp must have an argument", arg != NULL);
  conn = (struct netconn *)arg;
  LWIP_ASSERT("recv_udp: recv for wrong pcb!", conn->pcb.udp == pcb);

#if LWIP_SO_RCVBUF
  SYS_ARCH_GET(conn->recv_avail, recv_avail);
  if ((conn == NULL) || !pGlobalBlock->pTcpipBlock->fnsys_mbox_valid(&conn->recvmbox) ||
      ((recv_avail + (int)(p->tot_len)) > conn->recv_bufsize)) {
#else  /* LWIP_SO_RCVBUF */
  if ((conn == NULL) || !pGlobalBlock->pTcpipBlock->fnsys_mbox_valid(&conn->recvmbox)) {
#endif /* LWIP_SO_RCVBUF */
    pGlobalBlock->pTcpipBlock->fnpbuf_free(p);
    return;
  }

  buf = (struct netbuf *)memp_malloc(MEMP_NETBUF);
  if (buf == NULL) {
    pGlobalBlock->pTcpipBlock->fnpbuf_free(p);
    return;
  } else {
    buf->p = p;
    buf->ptr = p;
    ipX_addr_set_ipaddr(ip_current_is_v6(), &buf->addr, addr);
    buf->port = port;
#if LWIP_NETBUF_RECVINFO
    {
      /* get the UDP header - always in the first pbuf, ensured by udp_input */
      const struct udp_hdr* udphdr = ipX_next_header_ptr();
#if LWIP_CHECKSUM_ON_COPY
      buf->flags = NETBUF_FLAG_DESTADDR;
#endif /* LWIP_CHECKSUM_ON_COPY */
      ipX_addr_set(ip_current_is_v6(), &buf->toaddr, ipX_current_dest_addr());
      buf->toport_chksum = udphdr->dest;
    }
#endif /* LWIP_NETBUF_RECVINFO */
  }

  len = p->tot_len;
  if (pGlobalBlock->pTcpipBlock->fnsys_mbox_trypost(&conn->recvmbox, buf) != ERR_OK) {
    pGlobalBlock->pTcpipBlock->fnnetbuf_delete(buf);
    return;
  } else {
#if LWIP_SO_RCVBUF
    SYS_ARCH_INC(conn->recv_avail, len);
#endif /* LWIP_SO_RCVBUF */
    /* Register event with callback */
    API_EVENT(conn, NETCONN_EVT_RCVPLUS, len);
  }
}
#endif /* LWIP_UDP */

#if LWIP_TCP
/**
 * Receive callback function for TCP netconns.
 * Posts the packet to conn->recvmbox, but doesn't delete it on errors.
 *
 * @see tcp.h (struct tcp_pcb.recv) for parameters and return value
 */
err_t
recv_tcp(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err)
{
  struct netconn *conn;
  uint16_t len;
  USE_GLOBAL_BLOCK

  LWIP_ASSERT("recv_tcp must have a pcb argument", pcb != NULL);
  LWIP_ASSERT("recv_tcp must have an argument", arg != NULL);
  conn = (struct netconn *)arg;
  LWIP_ASSERT("recv_tcp: recv for wrong pcb!", conn->pcb.tcp == pcb);

  if (conn == NULL) {
    return ERR_VAL;
  }

  if (!pGlobalBlock->pTcpipBlock->fnsys_mbox_valid(&conn->recvmbox)) {
    /* recvmbox already deleted */
    if (p != NULL) {
      pGlobalBlock->pTcpipBlock->fntcp_recved(pcb, p->tot_len);
      pGlobalBlock->pTcpipBlock->fnpbuf_free(p);
    }
    return ERR_OK;
  }
  /* Unlike for UDP or RAW pcbs, don't check for available space
     using recv_avail since that could break the connection
     (data is already ACKed) */

  /* don't overwrite fatal errors! */
  NETCONN_SET_SAFE_ERR(conn, err);

  if (p != NULL) {
    len = p->tot_len;
  } else {
    len = 0;
  }

  if (pGlobalBlock->pTcpipBlock->fnsys_mbox_trypost(&conn->recvmbox, p) != ERR_OK) {
    /* don't deallocate p: it is presented to us later again frotcp_fasttmr! */
    return ERR_MEM;
  } else {
#if LWIP_SO_RCVBUF
    SYS_ARCH_INC(conn->recv_avail, len);
#endif /* LWIP_SO_RCVBUF */
    /* Register event with callback */
    API_EVENT(conn, NETCONN_EVT_RCVPLUS, len);
  }

  return ERR_OK;
}

/**
 * Poll callback function for TCP netconns.
 * Wakes up an application thread that waits for a connection to close
 * or data to be sent. The application thread then takes the
 * appropriate action to go on.
 *
 * Signals the conn->sem.
 * netconn_close waits for conn->sem if closing failed.
 *
 * @see tcp.h (struct tcp_pcb.poll) for parameters and return value
 */
err_t
poll_tcp(void *arg, struct tcp_pcb *pcb)
{
  struct netconn *conn = (struct netconn *)arg;
  USE_GLOBAL_BLOCK

  LWIP_ASSERT("conn != NULL", (conn != NULL));

  if (conn->state == NETCONN_WRITE) {
    pGlobalBlock->pTcpipBlock->fnlwip_netconn_do_writemore(conn);
  } else if (conn->state == NETCONN_CLOSE) {
    pGlobalBlock->pTcpipBlock->fnlwip_netconn_do_close_internal(conn);
  }
  /* @todo: implement connect timeout here? */

  /* Did a nonblocking write fail before? Then check available write-space. */
  if (conn->flags & NETCONN_FLAG_CHECK_WRITESPACE) {
    /* If the queued byte- or pbuf-count drops below the configured low-water limit,
       let select mark this pcb as writable again. */
    if ((conn->pcb.tcp != NULL) && (tcp_sndbuf(conn->pcb.tcp) > TCP_SNDLOWAT) &&
      (tcp_sndqueuelen(conn->pcb.tcp) < TCP_SNDQUEUELOWAT)) {
      conn->flags &= ~NETCONN_FLAG_CHECK_WRITESPACE;
      API_EVENT(conn, NETCONN_EVT_SENDPLUS, 0);
    }
  }

  return ERR_OK;
}

/**
 * Sent callback function for TCP netconns.
 * Signals the conn->sem and calls API_EVENT.
 * netconn_write waits for conn->sem if send buffer is low.
 *
 * @see tcp.h (struct tcp_pcb.sent) for parameters and return value
 */
err_t
sent_tcp(void *arg, struct tcp_pcb *pcb, uint16_t len)
{
  struct netconn *conn = (struct netconn *)arg;
  USE_GLOBAL_BLOCK

  LWIP_ASSERT("conn != NULL", (conn != NULL));

  if (conn->state == NETCONN_WRITE) {
    pGlobalBlock->pTcpipBlock->fnlwip_netconn_do_writemore(conn);
  } else if (conn->state == NETCONN_CLOSE) {
    pGlobalBlock->pTcpipBlock->fnlwip_netconn_do_close_internal(conn);
  }

  if (conn) {
    /* If the queued byte- or pbuf-count drops below the configured low-water limit,
       let select mark this pcb as writable again. */
    if ((conn->pcb.tcp != NULL) && (tcp_sndbuf(conn->pcb.tcp) > TCP_SNDLOWAT) &&
      (tcp_sndqueuelen(conn->pcb.tcp) < TCP_SNDQUEUELOWAT)) {
      conn->flags &= ~NETCONN_FLAG_CHECK_WRITESPACE;
      API_EVENT(conn, NETCONN_EVT_SENDPLUS, len);
    }
  }
  
  return ERR_OK;
}

/**
 * Error callback function for TCP netconns.
 * Signals conn->sem, posts to all conn mboxes and calls API_EVENT.
 * The application thread has then to decide what to do.
 *
 * @see tcp.h (struct tcp_pcb.err) for parameters
 */
void
err_tcp(void *arg, err_t err)
{
  struct netconn *conn;
  enum netconn_state old_state;
  SYS_ARCH_DECL_PROTECT(lev);
  USE_GLOBAL_BLOCK

  conn = (struct netconn *)arg;
  LWIP_ASSERT("conn != NULL", (conn != NULL));

  conn->pcb.tcp = NULL;

  /* no check since this is always fatal! */
  SYS_ARCH_PROTECT(lev);
  conn->last_err = err;
  SYS_ARCH_UNPROTECT(lev);

  /* reset conn->state now before waking up other threads */
  old_state = conn->state;
  conn->state = NETCONN_NONE;

  /* Notify the user layer about a connection error. Used to signal
     select. */
  API_EVENT(conn, NETCONN_EVT_ERROR, 0);
  /* Try to release selects pending on 'read' or 'write', too.
     They will get an error if they actually try to read or write. */
  API_EVENT(conn, NETCONN_EVT_RCVPLUS, 0);
  API_EVENT(conn, NETCONN_EVT_SENDPLUS, 0);

  /* pass NULL-message to recvmbox to wake up pending recv */
  if (pGlobalBlock->pTcpipBlock->fnsys_mbox_valid(&conn->recvmbox)) {
    /* use trypost to prevent deadlock */
    pGlobalBlock->pTcpipBlock->fnsys_mbox_trypost(&conn->recvmbox, NULL);
  }
//   /* pass NULL-message to acceptmbox to wake up pending accept */
//   if (pGlobalBlock->pTcpipBlock->fnsys_mbox_valid(&conn->acceptmbox)) {
//     /* use trypost to preven deadlock */
//     pGlobalBlock->pTcpipBlock->fnsys_mbox_trypost(&conn->acceptmbox, NULL);
//   }

  if ((old_state == NETCONN_WRITE) || (old_state == NETCONN_CLOSE) ||
      (old_state == NETCONN_CONNECT)) {
    /* calling lwip_netconn_do_writemore/lwip_netconn_do_close_internal is not necessary
       since the pcb has already been deleted! */
    int was_nonblocking_connect = IN_NONBLOCKING_CONNECT(conn);
    SET_NONBLOCKING_CONNECT(conn, 0);

    if (!was_nonblocking_connect) {
      /* set error return code */
      LWIP_ASSERT("conn->current_msg != NULL", conn->current_msg != NULL);
      conn->current_msg->err = err;
      conn->current_msg = NULL;
      /* wake up the waiting task */
      pGlobalBlock->pTcpipBlock->fnsys_sem_signal(&conn->op_completed);
    }
  } else {
    LWIP_ASSERT("conn->current_msg == NULL", conn->current_msg == NULL);
  }
}

/**
 * Setup a tcp_pcb with the correct callback function pointers
 * and their arguments.
 *
 * @param conn the TCP netconn to setup
 */
void
setup_tcp(struct netconn *conn)
{
  struct tcp_pcb *pcb;
  USE_GLOBAL_BLOCK

  pcb = conn->pcb.tcp;
  pGlobalBlock->pTcpipBlock->fntcp_arg(pcb, conn);
  pGlobalBlock->pTcpipBlock->fntcp_recv(pcb, pGlobalBlock->pTcpipBlock->fnrecv_tcp);
  pGlobalBlock->pTcpipBlock->fntcp_sent(pcb, pGlobalBlock->pTcpipBlock->fnsent_tcp);
  pGlobalBlock->pTcpipBlock->fntcp_poll(pcb, pGlobalBlock->pTcpipBlock->fnpoll_tcp, 4);
  pGlobalBlock->pTcpipBlock->fntcp_err(pcb, pGlobalBlock->pTcpipBlock->fnerr_tcp);
}
#if 0
/**
 * Accept callback function for TCP netconns.
 * Allocates a new netconn and posts that to conn->acceptmbox.
 *
 * @see tcp.h (struct tcp_pcb_listen.accept) for parameters and return value
 */
err_t
accept_function(void *arg, struct tcp_pcb *newpcb, err_t err)
{
  struct netconn *newconn;
  struct netconn *conn = (struct netconn *)arg;
  pmod_tcpip_block_t pTcpipBlock;
  USE_GLOBAL_BLOCK

  pTcpipBlock = pGlobalBlock->pTcpipBlock;

  LWIP_DEBUGF(API_MSG_DEBUG, ("accept_function: newpcb->tate: %s\n", tcp_debug_state_str(newpcb->state)));

  if (!pTcpipBlock->fnsys_mbox_valid(&conn->acceptmbox)) {
    LWIP_DEBUGF(API_MSG_DEBUG, ("accept_function: acceptmbox already deleted\n"));
    return ERR_VAL;
  }

  /* We have to set the callback here even though
   * the new socket is unknown. conn->socket is marked as -1. */
  newconn = pTcpipBlock->fnnetconn_alloc(conn->type, conn->callback);
  if (newconn == NULL) {
    return ERR_MEM;
  }
  newconn->pcb.tcp = newpcb;
  pTcpipBlock->fnsetup_tcp(newconn);
  /* no protection: when creating the pcb, the netconn is not yet known
     to the application thread */
  newconn->last_err = err;

  if (pTcpipBlock->fnsys_mbox_trypost(&conn->acceptmbox, newconn) != ERR_OK) {
    /* When returning != ERR_OK, the pcb is aborted in tcp_process(),
       so do nothing here! */
    newconn->pcb.tcp = NULL;
    /* no need to drain since we know the recvmbox is empty. */
    pTcpipBlock->fnsys_mbox_free(&newconn->recvmbox);
    //pTcpipBlock->fnsys_mbox_set_invalid(&newconn->recvmbox);
    pTcpipBlock->fnnetconn_free(newconn);
    return ERR_MEM;
  } else {
    /* Register event with callback */
    API_EVENT(conn, NETCONN_EVT_RCVPLUS, 0);
  }

  return ERR_OK;
}
#endif // 0
#endif /* LWIP_TCP */

/**
 * Create a new pcb of a specific type.
 * Called from lwip_netconn_do_newconn().
 *
 * @param msg the api_msg_msg describing the connection type
 * @return msg->conn->err, but the return value is currently ignored
 */
void
pcb_new(struct api_msg_msg *msg)
{
  UINT8 ncType;
  USE_GLOBAL_BLOCK

  LWIP_ASSERT("pcb_new: pcb already allocated", msg->conn->pcb.tcp == NULL);

  /* Allocate a PCB for this connection */
#if 0
//  switch(NETCONNTYPE_GROUP(msg->conn->type)) {
#else
    ncType = NETCONNTYPE_GROUP(msg->conn->type);
#endif // #if 0
#if LWIP_RAW
  case NETCONN_RAW:
    msg->conn->pcb.raw = raw_new(msg->msg.n.proto);
    if(msg->conn->pcb.raw != NULL) {
      raw_recv(msg->conn->pcb.raw, recv_raw, msg->conn);
    }
    break;
#endif /* LWIP_RAW */
#if LWIP_UDP
#if 0
  case NETCONN_UDP:
#else
  if (ncType == NETCONN_UDP) {
#endif
    msg->conn->pcb.udp = pGlobalBlock->pTcpipBlock->fnudp_new();
    if(msg->conn->pcb.udp != NULL) {
#if LWIP_UDPLITE
        if (NETCONNTYPE_ISUDPLITE(msg->conn->type)) {
          udp_setflags(msg->conn->pcb.udp, UDP_FLAGS_UDPLITE);
        }
#endif /* LWIP_UDPLITE */
        if (NETCONNTYPE_ISUDPNOCHKSUM(msg->conn->type)) {
          udp_setflags(msg->conn->pcb.udp, UDP_FLAGS_NOCHKSUM);
        }
        pGlobalBlock->pTcpipBlock->fnudp_recv(msg->conn->pcb.udp, pGlobalBlock->pTcpipBlock->fnrecv_udp, msg->conn);
    }
#if 0
    break;
#else
  }
  else 
#endif
#endif /* LWIP_UDP */
#if LWIP_TCP
#if 0
  case NETCONN_TCP:
#else
  if (ncType == NETCONN_TCP) {
#endif
    msg->conn->pcb.tcp = pGlobalBlock->pTcpipBlock->fntcp_new();
    if(msg->conn->pcb.tcp != NULL) {
      pGlobalBlock->pTcpipBlock->fnsetup_tcp(msg->conn);
    }
#if 0
    break;
#else
  }
#endif
#endif /* LWIP_TCP */
#if 0
  default:
#else
  else {
#endif
    /* Unsupported netconn type, e.g. protocol disabled */
    msg->err = ERR_VAL;
    return;
  }
  if (msg->conn->pcb.ip == NULL) {
    msg->err = ERR_MEM;
  }
#if LWIP_IPV6
  else {
    if (NETCONNTYPE_ISIPV6(msg->conn->type)) {
      ip_set_v6(msg->conn->pcb.ip, 1);
    }
  }
#endif /* LWIP_IPV6 */
}

/**
 * Create a new pcb of a specific type inside a netconn.
 * Called from netconn_new_with_proto_and_callback.
 *
 * @param msg the api_msg_msg describing the connection type
 */
void
lwip_netconn_do_newconn(struct api_msg_msg *msg)
{
  USE_GLOBAL_BLOCK

  msg->err = ERR_OK;
  if(msg->conn->pcb.tcp == NULL) {
    pGlobalBlock->pTcpipBlock->fnpcb_new(msg);
  }
  /* Else? This "new" connection already has a PCB allocated. */
  /* Is this an error condition? Should it be deleted? */
  /* We currently just are happy and return. */

  TCPIP_APIMSG_ACK(msg);
}

/**
 * Create a new netconn (of a specific type) that has a callback function.
 * The corresponding pcb is NOT created!
 *
 * @param t the type of 'connection' to create (@see enum netconn_type)
 * @param proto the IP protocol for RAW IP pcbs
 * @param callback a function to call on status changes (RX available, TX'ed)
 * @return a newly allocated struct netconn or
 *         NULL on memory error
 */
struct netconn*
netconn_alloc(enum netconn_type t, netconn_callback callback)
{
  struct netconn *conn;
  int size;
  UINT8 ncType;
  USE_GLOBAL_BLOCK

  conn = (struct netconn *)memp_malloc(MEMP_NETCONN);
  if (conn == NULL) {
    return NULL;
  }

  conn->last_err = ERR_OK;
  conn->type = t;
  conn->pcb.tcp = NULL;

  /* If all sizes are the same, every compiler should optimize this switch to nothing, */#if 0
//  switch(NETCONNTYPE_GROUP(t)) {
#else
  ncType = NETCONNTYPE_GROUP(t);
#endif // #if 0
#if LWIP_RAW
  case NETCONN_RAW:
    size = DEFAULT_RAW_RECVMBOX_SIZE;
    break;
#endif /* LWIP_RAW */
#if LWIP_UDP
#if 0
  case NETCONN_UDP:
#else // #if 0
  if (ncType == NETCONN_UDP)  {
#endif // #if 0
    size = DEFAULT_UDP_RECVMBOX_SIZE;
#if 0
    break;
#else
  }
#endif // #if 0
#endif /* LWIP_UDP */
#if LWIP_TCP
#if 0
  case NETCONN_TCP:
#else
  else if (ncType == NETCONN_TCP) {
#endif
    size = DEFAULT_TCP_RECVMBOX_SIZE;
#if 0
    break;
#else
  }
#endif // #if 0
#endif /* LWIP_TCP */
#if 0
  default:
#endif
  else {
    LWIP_ASSERT("netconn_alloc: undefined netconn_type", 0);
    goto free_and_return;
//    break;
  }
//#endif // #if 0

  if (pGlobalBlock->pTcpipBlock->fnsys_sem_new(&conn->op_completed, 0) != ERR_OK) {
      goto free_and_return;
  }
#if 0
  if (pTcpipBlock->fnsys_mbox_new(&conn->recvmbox, size) != ERR_OK) {
    sys_sem_free(&conn->op_completed);
    memp_free(MEMP_NETCONN, conn);
    return NULL;
  }
#else
    pGlobalBlock->pTcpipBlock->fnsys_mbox_new(&conn->recvmbox, size);
#endif

// #if LWIP_TCP
//   pGlobalBlock->pTcpipBlock->fnsys_mbox_set_invalid(&conn->acceptmbox);
// #endif
  conn->state        = NETCONN_NONE;
#if LWIP_SOCKET
  /* initialize socket to -1 since 0 is a valid socket */
  conn->socket       = -1;
#endif /* LWIP_SOCKET */
  conn->callback     = callback;
#if LWIP_TCP
  conn->current_msg  = NULL;
  conn->write_offset = 0;
#endif /* LWIP_TCP */
#if LWIP_SO_SNDTIMEO
  conn->send_timeout = 11000;//0;
#endif /* LWIP_SO_SNDTIMEO */
#if LWIP_SO_RCVTIMEO
  conn->recv_timeout = 11000;
#endif /* LWIP_SO_RCVTIMEO */
#if LWIP_SO_RCVBUF
  conn->recv_bufsize = RECV_BUFSIZE_DEFAULT;
  conn->recv_avail   = 0;
#endif /* LWIP_SO_RCVBUF */
  conn->flags = 0;
  return conn;
free_and_return:
  memp_free(MEMP_NETCONN, conn);
  return NULL;
}

/**
 * Delete a netconn and all its resources.
 * The pcb is NOT freed (since we might not be in the right thread context do this).
 *
 * @param conn the netconn to free
 */
void
netconn_free(struct netconn *conn)
{
  USE_GLOBAL_BLOCK

  LWIP_ASSERT("PCB must be deallocated outside this function", conn->pcb.tcp == NULL);
  LWIP_ASSERT("recvmbox must be deallocated before calling this function",
    !pGLobalData->fnsys_mbox_valid(&conn->recvmbox));
#if LWIP_TCP
  LWIP_ASSERT("acceptmbox must be deallocated before calling this function",
    !pGLobalData->fnsys_mbox_valid(&conn->acceptmbox));
#endif /* LWIP_TCP */

  sys_sem_free(&conn->op_completed);
  pGlobalBlock->pTcpipBlock->fnsys_sem_set_invalid(&conn->op_completed);

  memp_free(MEMP_NETCONN, conn);
}

/**
 * Delete rcvmbox and acceptmbox of a netconn and free the left-over data in
 * these mboxes
 *
 * @param conn the netconn to free
 * @bytes_drained bytes drained from recvmbox
 * @accepts_drained pending connections drained from acceptmbox
 */
void
netconn_drain(struct netconn *conn)
{
  void *mem;
#if LWIP_TCP
  struct pbuf *p;
#endif /* LWIP_TCP */
  USE_GLOBAL_BLOCK

  /* This runs in tcpip_thread, so we don't need to lock against rx packets */

  /* Delete and drain the recvmbox. */
  if (pGlobalBlock->pTcpipBlock->fnsys_mbox_valid(&conn->recvmbox)) {
    while (sys_mbox_tryfetch(&conn->recvmbox, &mem) != SYS_MBOX_EMPTY) {
#if LWIP_TCP
      if (NETCONNTYPE_GROUP(conn->type) == NETCONN_TCP) {
        if(mem != NULL) {
          p = (struct pbuf*)mem;
          /* pcb might be set to NULL already by err_tcp() */
          if (conn->pcb.tcp != NULL) {
            pGlobalBlock->pTcpipBlock->fntcp_recved(conn->pcb.tcp, p->tot_len);
          }
          pGlobalBlock->pTcpipBlock->fnpbuf_free(p);
        }
      } else
#endif /* LWIP_TCP */
      {
        pGlobalBlock->pTcpipBlock->fnnetbuf_delete((struct netbuf *)mem);
      }
    }
    pGlobalBlock->pTcpipBlock->fnsys_mbox_free(&conn->recvmbox);
    pGlobalBlock->pTcpipBlock->fnsys_mbox_set_invalid(&conn->recvmbox);
  }
// 
//   /* Delete and drain the acceptmbox. */
// #if LWIP_TCP
//   if (pGlobalBlock->pTcpipBlock->fnsys_mbox_valid(&conn->acceptmbox)) {
//     while (sys_mbox_tryfetch(&conn->acceptmbox, &mem) != SYS_MBOX_EMPTY) {
//       struct netconn *newconn = (struct netconn *)mem;
//       /* Only tcp pcbs have an acceptmbox, so no need to check conn->type */
//       /* pcb might be set to NULL already by err_tcp() */
//       if (conn->pcb.tcp != NULL) {
//         tcp_accepted(conn->pcb.tcp);
//       }
//       /* drain recvmbox */
//       pGlobalBlock->pTcpipBlock->fnnetconn_drain(newconn);
//       if (newconn->pcb.tcp != NULL) {
//         pGlobalBlock->pTcpipBlock->fntcp_abort(newconn->pcb.tcp);
//         newconn->pcb.tcp = NULL;
//       }
//       pGlobalBlock->pTcpipBlock->fnnetconn_free(newconn);
//     }
//     pGlobalBlock->pTcpipBlock->fnsys_mbox_free(&conn->acceptmbox);
//     pGlobalBlock->pTcpipBlock->fnsys_mbox_set_invalid(&conn->acceptmbox);
//   }
// #endif /* LWIP_TCP */
}

#if LWIP_TCP
/**
 * Internal helper function to close a TCP netconn: since this sometimes
 * doesn't work at the first attempt, this function is called from multiple
 * places.
 *
 * @param conn the TCP netconn to close
 */
void
lwip_netconn_do_close_internal(struct netconn *conn)
{
  err_t err;
  UINT8 shut, shut_rx, shut_tx, close;
  USE_GLOBAL_BLOCK

  LWIP_ASSERT("invalid conn", (conn != NULL));
  LWIP_ASSERT("this is for tcp netconns only", (NETCONNTYPE_GROUP(conn->type) == NETCONN_TCP));
  LWIP_ASSERT("conn must be in state NETCONN_CLOSE", (conn->state == NETCONN_CLOSE));
  LWIP_ASSERT("pcb already closed", (conn->pcb.tcp != NULL));
  LWIP_ASSERT("conn->current_msg != NULL", conn->current_msg != NULL);

  shut = conn->current_msg->msg.sd.shut;
  shut_rx = shut & NETCONN_SHUT_RD;
  shut_tx = shut & NETCONN_SHUT_WR;
  /* shutting down both ends is the same as closing */
  close = shut == NETCONN_SHUT_RDWR;

  /* Set back some callback pointers */
  if (close) {
    pGlobalBlock->pTcpipBlock->fntcp_arg(conn->pcb.tcp, NULL);
  }
#if 0
  if (conn->pcb.tcp->state == LISTEN) {
    pGlobalBlock->pTcpipBlock->fntcp_accept(conn->pcb.tcp, NULL);
  } else {
#endif // #if 0
    /* some callbacks have to be reset if tcp_close is not successful */
    if (shut_rx) {
      pGlobalBlock->pTcpipBlock->fntcp_recv(conn->pcb.tcp, NULL);
#if 0
      pTcpipBlock->fntcp_accept(conn->pcb.tcp, NULL);
#endif // #if 0
    }
    if (shut_tx) {
      pGlobalBlock->pTcpipBlock->fntcp_sent(conn->pcb.tcp, NULL);
    }
    if (close) {
      pGlobalBlock->pTcpipBlock->fntcp_poll(conn->pcb.tcp, NULL, 4);
      pGlobalBlock->pTcpipBlock->fntcp_err(conn->pcb.tcp, NULL);
    }
#if 0
  }
#endif // #if 0
  /* Try to close the connection */
  if (close) {
    err = pGlobalBlock->pTcpipBlock->fntcp_close(conn->pcb.tcp);
  } else {
    err = pGlobalBlock->pTcpipBlock->fntcp_shutdown(conn->pcb.tcp, shut_rx, shut_tx);
  }
  if (err == ERR_OK) {
    /* Closing succeeded */
    conn->current_msg->err = ERR_OK;
    conn->current_msg = NULL;
    conn->state = NETCONN_NONE;
    if (close) {
      /* Set back some callback pointers as conn is going away */
      conn->pcb.tcp = NULL;
      /* Trigger select() in socket layer. Make sure everybody notices activity
       on the connection, error first! */
      API_EVENT(conn, NETCONN_EVT_ERROR, 0);
    }
    if (shut_rx) {
      API_EVENT(conn, NETCONN_EVT_RCVPLUS, 0);
    }
    if (shut_tx) {
      API_EVENT(conn, NETCONN_EVT_SENDPLUS, 0);
    }
    /* wake up the application task */
    pGlobalBlock->pTcpipBlock->fnsys_sem_signal(&conn->op_completed);
  } else {
    /* Closing failed, restore some of the callbacks */
    /* Closing of listen pcb will never fail! */
    LWIP_ASSERT("Closing a listen pcb may not fail!", (conn->pcb.tcp->state != LISTEN));
    pGlobalBlock->pTcpipBlock->fntcp_sent(conn->pcb.tcp, pGlobalBlock->pTcpipBlock->fnsent_tcp);
    pGlobalBlock->pTcpipBlock->fntcp_poll(conn->pcb.tcp, pGlobalBlock->pTcpipBlock->fnpoll_tcp, 4);
    pGlobalBlock->pTcpipBlock->fntcp_err(conn->pcb.tcp, pGlobalBlock->pTcpipBlock->fnerr_tcp);
    pGlobalBlock->pTcpipBlock->fntcp_arg(conn->pcb.tcp, conn);
    /* don't restore recv callback: we don't want to receive any more data */
  }
  /* If closing didn't succeed, we get called again either
     from poll_tcp or from sent_tcp */
}
#endif /* LWIP_TCP */

/**
 * Delete the pcb inside a netconn.
 * Called from netconn_delete.
 *
 * @param msg the api_msg_msg pointing to the connection
 */
void
lwip_netconn_do_delconn(struct api_msg_msg *msg)
{
  UINT8 ncType;
  USE_GLOBAL_BLOCK

  /* @todo TCP: abort running write/connect? */
  if ((msg->conn->state != NETCONN_NONE) &&
     (msg->conn->state != NETCONN_LISTEN) &&
     (msg->conn->state != NETCONN_CONNECT)) {
    /* this only happens for TCP netconns */
    LWIP_ASSERT("NETCONNTYPE_GROUP(msg->conn->type) == NETCONN_TCP", 
                NETCONNTYPE_GROUP(msg->conn->type) == NETCONN_TCP);
    msg->err = ERR_INPROGRESS;
  } else {
    LWIP_ASSERT("blocking connect in progress",
      (msg->conn->state != NETCONN_CONNECT) || IN_NONBLOCKING_CONNECT(msg->conn));
    /* Drain and delete mboxes */
    pGlobalBlock->pTcpipBlock->fnnetconn_drain(msg->conn);

    if (msg->conn->pcb.tcp != NULL) {
#if 0
//      switch (NETCONNTYPE_GROUP(msg->conn->type)) {
#else
      ncType = NETCONNTYPE_GROUP(msg->conn->type);
#endif
#if LWIP_RAW
      case NETCONN_RAW:
        raw_remove(msg->conn->pcb.raw);
        break;
#endif /* LWIP_RAW */
#if LWIP_UDP
#if 0
      case NETCONN_UDP:
#else
      if (ncType == NETCONN_UDP) {
#endif 
        msg->conn->pcb.udp->recv_arg = NULL;
        pGlobalBlock->pTcpipBlock->fnudp_remove(msg->conn->pcb.udp);
#if 0
        break;
#else
      }
      else 
#endif // #if 0
#endif /* LWIP_UDP */
#if LWIP_TCP
#if 0
      case NETCONN_TCP:
#else
      if (ncType == NETCONN_TCP) {
#endif
        LWIP_ASSERT("already writing or closing", msg->conn->current_msg == NULL &&
          msg->conn->write_offset == 0);
        msg->conn->state = NETCONN_CLOSE;
        msg->msg.sd.shut = NETCONN_SHUT_RDWR;
        msg->conn->current_msg = msg;
        pGlobalBlock->pTcpipBlock->fnlwip_netconn_do_close_internal(msg->conn);
        /* API_EVENT is called inside lwip_netconn_do_close_internal, before releasing
           the application thread, so we can return at this point! */
        return;
      } // added in case of reconstruction from switch to if statements
#endif /* LWIP_TCP */
#if 0
      default:
        break;
      }
#endif // #if 0
      msg->conn->pcb.tcp = NULL;
    }
    /* tcp netconns don't come here! */

    /* @todo: this lets select make the socket readable and writable,
       which is wrong! errfd instead? */
    API_EVENT(msg->conn, NETCONN_EVT_RCVPLUS, 0);
    API_EVENT(msg->conn, NETCONN_EVT_SENDPLUS, 0);
  }
  if (pGlobalBlock->pTcpipBlock->fnsys_sem_valid(&msg->conn->op_completed)) {
    pGlobalBlock->pTcpipBlock->fnsys_sem_signal(&msg->conn->op_completed);
  }
}

#if 0
/**
 * Bind a pcb contained in a netconn
 * Called from netconn_bind.
 *
 * @param msg the api_msg_msg pointing to the connection and containing
 *            the IP address and port to bind to
 */
void
do_bind(struct api_msg_msg *msg)
{
    UINT8 ncType;
  if (ERR_IS_FATAL(msg->conn->last_err)) {
    msg->err = msg->conn->last_err;
  } else {
    msg->err = ERR_VAL;
    if (msg->conn->pcb.tcp != NULL) {
#if 0
//      switch (NETCONNTYPE_GROUP(msg->conn->type)) {
#else
      ncType = NETCONNTYPE_GROUP(msg->conn->type);
#endif
#if LWIP_RAW
      case NETCONN_RAW:
        msg->err = raw_bind(msg->conn->pcb.raw, msg->msg.bc.ipaddr);
        break;
#endif /* LWIP_RAW */
#if LWIP_UDP
#if 0
      case NETCONN_UDP:
#else
      if (ncType == NETCONN_UDP) {
#endif // #if 0
        msg->err = pTcpipBlock->fnudp_bind(msg->conn->pcb.udp, msg->msg.bc.ipaddr, msg->msg.bc.port);
#if 0
        break;
#else
      }
#endif // #if 0
#endif /* LWIP_UDP */
#if LWIP_TCP
#if 0
      case NETCONN_TCP:
#else
      if (ncType == NETCONN_TCP) {
#endif // #if 0
        msg->err = tcp_bind(msg->conn->pcb.tcp, msg->msg.bc.ipaddr, msg->msg.bc.port);
#if 0
        break;
#else
      }
#endif // #if 0
#endif /* LWIP_TCP */
#if 0
      default:
        break;
      }
#endif // #if 0
    }
  }
  TCPIP_APIMSG_ACK(msg);
}
#endif // #if 0

#if LWIP_TCP
/**
 * TCP callback function if a connection (opened by tcp_connect/lwip_netconn_do_connect) has
 * been established (or reset by the remote host).
 *
 * @see tcp.h (struct tcp_pcb.connected) for parameters and return values
 */
err_t
lwip_netconn_do_connected(void *arg, struct tcp_pcb *pcb, err_t err)
{
  struct netconn *conn;
  int was_blocking;
  USE_GLOBAL_BLOCK

  conn = (struct netconn *)arg;

  if (conn == NULL) {
    return ERR_VAL;
  }

  LWIP_ASSERT("conn->state == NETCONN_CONNECT", conn->state == NETCONN_CONNECT);
  LWIP_ASSERT("(conn->current_msg != NULL) || conn->in_non_blocking_connect",
    (conn->current_msg != NULL) || IN_NONBLOCKING_CONNECT(conn));

  if (conn->current_msg != NULL) {
    conn->current_msg->err = err;
  }
  if ((NETCONNTYPE_GROUP(conn->type) == NETCONN_TCP) && (err == ERR_OK)) {
    pGlobalBlock->pTcpipBlock->fnsetup_tcp(conn);
  }
  was_blocking = !IN_NONBLOCKING_CONNECT(conn);
  SET_NONBLOCKING_CONNECT(conn, 0);
  conn->current_msg = NULL;
  conn->state = NETCONN_NONE;
  if (!was_blocking) {
    NETCONN_SET_SAFE_ERR(conn, ERR_OK);
  }
  API_EVENT(conn, NETCONN_EVT_SENDPLUS, 0);

  if (was_blocking) {
    pGlobalBlock->pTcpipBlock->fnsys_sem_signal(&conn->op_completed);
  }
  return ERR_OK;
}
#endif /* LWIP_TCP */

/**
 * Connect a pcb contained inside a netconn
 * Called from netconn_connect.
 *
 * @param msg the api_msg_msg pointing to the connection and containing
 *            the IP address and port to connect to
 */
void
lwip_netconn_do_connect(struct api_msg_msg *msg)
{
  UINT8 ncType;
  USE_GLOBAL_BLOCK

  if (msg->conn->pcb.tcp == NULL) {
    /* This may happen when calling netconn_connect() a second time */
    msg->err = ERR_CLSD;
    if (NETCONNTYPE_GROUP(msg->conn->type) == NETCONN_TCP) {
        /* For TCP, netconn_connect() calls tcpip_apimsg(), so signal op_completed here. */
        pGlobalBlock->pTcpipBlock->fnsys_sem_signal(&msg->conn->op_completed);
        return;
    }
  } else {
#if 0
//    switch (NETCONNTYPE_GROUP(msg->conn->type)) {
#else
    ncType = NETCONNTYPE_GROUP(msg->conn->type);
#endif // #if 0
#if LWIP_RAW
  case NETCONN_RAW:
    msg->err = raw_connect(msg->conn->pcb.raw, msg->msg.bc.ipaddr);
    break;
#endif /* LWIP_RAW */
#if LWIP_UDP
#if 0
  case NETCONN_UDP:
#else
  if (ncType == NETCONN_UDP) {
#endif // #if 0
    msg->err = pGlobalBlock->pTcpipBlock->fnudp_connect(msg->conn->pcb.udp, msg->msg.bc.ipaddr, msg->msg.bc.port);
#if 0
    break;
#else
  }
  else 
#endif // #if 0
#endif /* LWIP_UDP */
#if LWIP_TCP
#if 0
  case NETCONN_TCP:
#else
  if (ncType == NETCONN_TCP) {
#endif // #if 0
    /* Prevent connect while doing any other action. */
    if (msg->conn->state != NETCONN_NONE) {
      msg->err = ERR_ISCONN;
    } else {
      pGlobalBlock->pTcpipBlock->fnsetup_tcp(msg->conn);
      msg->err = pGlobalBlock->pTcpipBlock->fntcp_connect(msg->conn->pcb.tcp, msg->msg.bc.ipaddr,
        msg->msg.bc.port, pGlobalBlock->pTcpipBlock->fnlwip_netconn_do_connected);
      if (msg->err == ERR_OK) {
        UINT8 non_blocking = netconn_is_nonblocking(msg->conn);
        msg->conn->state = NETCONN_CONNECT;
        SET_NONBLOCKING_CONNECT(msg->conn, non_blocking);
        if (non_blocking) {
          msg->err = ERR_INPROGRESS;
        } else {
          msg->conn->current_msg = msg;
          /* sys_sem_signal() is called from lwip_netconn_do_connected (or err_tcp()),
          * when the connection is established! */
          return;
        }
      }
    }
    /* For TCP, netconn_connect() calls tcpip_apimsg(), so signal op_completed here. */
    pGlobalBlock->pTcpipBlock->fnsys_sem_signal(&msg->conn->op_completed);
    return;
#if 0
    break;
#else
  }
#endif // #if 0
#endif /* LWIP_TCP */
#if 0
  default:
#else
  else {
#endif // #if 0
    LWIP_ERROR("Invalid netconn type", 0, do{ msg->err = ERR_VAL; }while(0));
#if 0
    break;
#endif // #if 0
    }
  }
  /* For all other protocols, netconn_connect() calls TCPIP_APIMSG(),
     so use TCPIP_APIMSG_ACK() here. */
  TCPIP_APIMSG_ACK(msg);
}

/**
 * Connect a pcb contained inside a netconn
 * Only used for UDP netconns.
 * Called from netconn_disconnect.
 *
 * @param msg the api_msg_msg pointing to the connection to disconnect
 */
void
lwip_netconn_do_disconnect(struct api_msg_msg *msg)
{
  USE_GLOBAL_BLOCK

#if LWIP_UDP
  if (NETCONNTYPE_GROUP(msg->conn->type) == NETCONN_UDP) {
    pGlobalBlock->pTcpipBlock->fnudp_disconnect(msg->conn->pcb.udp);
    msg->err = ERR_OK;
  } else
#endif /* LWIP_UDP */
  {
    msg->err = ERR_VAL;
  }
  TCPIP_APIMSG_ACK(msg);
}

#if LWIP_TCP

#if 0
/**
 * Set a TCP pcb contained in a netconn into listen mode
 * Called from netconn_listen.
 *
 * @param msg the api_msg_msg pointing to the connection
 */
void do_listen(struct api_msg_msg *msg)
{
  if (ERR_IS_FATAL(msg->conn->last_err)) {
    msg->err = msg->conn->last_err;
  } else {
    msg->err = ERR_CONN;
    if (msg->conn->pcb.tcp != NULL) {
      if (NETCONNTYPE_GROUP(msg->conn->type) == NETCONN_TCP) {
        if (msg->conn->state == NETCONN_NONE) {
#if TCP_LISTEN_BACKLOG
          struct tcp_pcb* lpcb = tcp_listen_with_backlog(msg->conn->pcb.tcp, msg->msg.lb.backlog);
#else  /* TCP_LISTEN_BACKLOG */
          struct tcp_pcb* lpcb = tcp_listen(msg->conn->pcb.tcp);
#endif /* TCP_LISTEN_BACKLOG */
          if (lpcb == NULL) {
            /* in this case, the old pcb is still allocated */
            msg->err = ERR_MEM;
          } else {
            /* delete the recvmbox and allocate the acceptmbox */
            if (pGLobalData->fnsys_mbox_valid(&msg->conn->recvmbox)) {
              /** @todo: should we drain the recvmbox here? */
              pTcpipBlock->fnsys_mbox_free(&msg->conn->recvmbox);
              pTcpipBlock->fnsys_mbox_set_invalid(&msg->conn->recvmbox);
            }
            msg->err = ERR_OK;
            if (!pGLobalData->fnsys_mbox_valid(&msg->conn->acceptmbox)) {
              msg->err = pTcpipBlock->fnsys_mbox_new(&msg->conn->acceptmbox, DEFAULT_ACCEPTMBOX_SIZE);
            }
            if (msg->err == ERR_OK) {
              msg->conn->state = NETCONN_LISTEN;
              msg->conn->pcb.tcp = lpcb;
              pTcpipBlock->fntcp_arg(msg->conn->pcb.tcp, msg->conn);
              pTcpipBlock->fntcp_accept(msg->conn->pcb.tcp, accept_function);
            } else {
              /* since the old pcb is already deallocated, free lpcb now */
              pTcpipBlock->fntcp_close(lpcb);
              msg->conn->pcb.tcp = NULL;
            }
          }
        }
      }else {
          msg->err = ERR_ARG;
      }
    }
  }
  TCPIP_APIMSG_ACK(msg);
}
#endif // #if 0
#endif /* LWIP_TCP */

/**
 * Send some data on a RAW or UDP pcb contained in a netconn
 * Called from netconn_send
 *
 * @param msg the api_msg_msg pointing to the connection
 */
void
lwip_netconn_do_send(struct api_msg_msg *msg)
{
  UINT8 ncType;
  USE_GLOBAL_BLOCK

  if (ERR_IS_FATAL(msg->conn->last_err)) {
    msg->err = msg->conn->last_err;
  } else {
    msg->err = ERR_CONN;
    if (msg->conn->pcb.tcp != NULL) {
#if 0
//      switch (NETCONNTYPE_GROUP(msg->conn->type)) {
#else
      ncType = NETCONNTYPE_GROUP(msg->conn->type);
#endif // #if 0
#if LWIP_RAW
      case NETCONN_RAW:
        if (ipX_addr_isany(PCB_ISIPV6(msg->conn->pcb.ip), &msg->msg.b->addr)) {
          msg->err = raw_send(msg->conn->pcb.raw, msg->msg.b->p);
        } else {
          msg->err = raw_sendto(msg->conn->pcb.raw, msg->msg.b->p, ipX_2_ip(&msg->msg.b->addr));
        }
        break;
#endif
#if LWIP_UDP
#if 0
      case NETCONN_UDP:
#else
      if (ncType == NETCONN_UDP) {
#endif // #if 0
#if LWIP_CHECKSUM_ON_COPY
        if (ipX_addr_isany(PCB_ISIPV6(msg->conn->pcb.ip), &msg->msg.b->addr)) {
          msg->err = udp_send_chksum(msg->conn->pcb.udp, msg->msg.b->p,
            msg->msg.b->flags & NETBUF_FLAG_CHKSUM, msg->msg.b->toport_chksum);
        } else {
          msg->err = udp_sendto_chksum(msg->conn->pcb.udp, msg->msg.b->p,
            ipX_2_ip(&msg->msg.b->addr), msg->msg.b->port,
            msg->msg.b->flags & NETBUF_FLAG_CHKSUM, msg->msg.b->toport_chksum);
        }
#else /* LWIP_CHECKSUM_ON_COPY */
        if (ipX_addr_isany(PCB_ISIPV6(msg->conn->pcb.ip), &msg->msg.b->addr)) {
          msg->err = pGlobalBlock->pTcpipBlock->fnudp_send(msg->conn->pcb.udp, msg->msg.b->p);
        } else {
          msg->err = pGlobalBlock->pTcpipBlock->fnudp_sendto(msg->conn->pcb.udp, msg->msg.b->p, ipX_2_ip(&msg->msg.b->addr), msg->msg.b->port);
        }
#endif /* LWIP_CHECKSUM_ON_COPY */
#if 0
        break;
#else
      }
#endif // #if 0
#endif /* LWIP_UDP */
#if 0
      default:
        break;
      }
#endif
    }
  }
  TCPIP_APIMSG_ACK(msg);
}

#if LWIP_TCP
/**
 * Indicate data has been received from a TCP pcb contained in a netconn
 * Called from netconn_recv
 *
 * @param msg the api_msg_msg pointing to the connection
 */
void
lwip_netconn_do_recv(struct api_msg_msg *msg)
{
  USE_GLOBAL_BLOCK

  msg->err = ERR_OK;
  if (msg->conn->pcb.tcp != NULL) {
    if (NETCONNTYPE_GROUP(msg->conn->type) == NETCONN_TCP) {
#if TCP_LISTEN_BACKLOG
      if (msg->conn->pcb.tcp->state == LISTEN) {
        tcp_accepted(msg->conn->pcb.tcp);
      } else
#endif /* TCP_LISTEN_BACKLOG */
      {
        uint32_t remaining = msg->msg.r.len;
        do {
          uint16_t recved = (remaining > 0xffff) ? 0xffff : (uint16_t)remaining;
          pGlobalBlock->pTcpipBlock->fntcp_recved(msg->conn->pcb.tcp, recved);
          remaining -= recved;
        }while(remaining != 0);
      }
    }
  }
  TCPIP_APIMSG_ACK(msg);
}

/**
 * See if more data needs to be written from a previous call to netconn_write.
 * Called initially from lwip_netconn_do_write. If the first call can't send all data
 * (because of low memory or empty send-buffer), this function is called again
 * from sent_tcp() or poll_tcp() to send more data. If all data is sent, the
 * blocking application thread (waiting in netconn_write) is released.
 *
 * @param conn netconn (that is currently in state NETCONN_WRITE) to process
 * @return ERR_OK
 *         ERR_MEM if LWIP_TCPIP_CORE_LOCKING=1 and sending hasn't yet finished
 */
err_t
lwip_netconn_do_writemore(struct netconn *conn)
{
  err_t err;
  void *dataptr;
  uint16_t len, available;
  UINT8 write_finished = 0;
  size_t diff;
  UINT8 dontblock = netconn_is_nonblocking(conn) ||
       (conn->current_msg->msg.w.apiflags & NETCONN_DONTBLOCK);
  UINT8 apiflags = conn->current_msg->msg.w.apiflags;
  USE_GLOBAL_BLOCK

  LWIP_ASSERT("conn != NULL", conn != NULL);
  LWIP_ASSERT("conn->state == NETCONN_WRITE", (conn->state == NETCONN_WRITE));
  LWIP_ASSERT("conn->current_msg != NULL", conn->current_msg != NULL);
  LWIP_ASSERT("conn->pcb.tcp != NULL", conn->pcb.tcp != NULL);
  LWIP_ASSERT("conn->write_offset < conn->current_msg->msg.w.len",
    conn->write_offset < conn->current_msg->msg.w.len);

#if LWIP_SO_SNDTIMEO
  if ((conn->send_timeout != 0) &&
      ((int32_t)(pGlobalBlock->pTcpipBlock->fnsys_now() - conn->current_msg->msg.w.time_started) >= conn->send_timeout)) {
      write_finished = 1;
    if (conn->write_offset == 0) {
        /* nothing has been written */
        err = ERR_WOULDBLOCK;
        conn->current_msg->msg.w.len = 0;
    } else {
        /* partial write */
        err = ERR_OK;
        conn->current_msg->msg.w.len = conn->write_offset;
    }
  } else
#endif /* LWIP_SO_SNDTIMEO */
  {
      dataptr = (uint8_t*)conn->current_msg->msg.w.dataptr + conn->write_offset;
      diff = conn->current_msg->msg.w.len - conn->write_offset;
      if (diff > 0xffffUL) { /* max_u16_t */
          len = 0xffff;
#if LWIP_TCPIP_CORE_LOCKING
      conn->flags |= NETCONN_FLAG_WRITE_DELAYED;
#endif
      apiflags |= TCP_WRITE_FLAG_MORE;
    } else {
        len = (uint16_t)diff;
    }
      available = tcp_sndbuf(conn->pcb.tcp);
      if (available < len) {
          /* don't try to write more than sendbuf */
          len = available;
          if (dontblock){ 
              if (!len) {
                  err = ERR_WOULDBLOCK;
                  goto err_mem;
              }
          } else {
#if LWIP_TCPIP_CORE_LOCKING
              conn->flags |= NETCONN_FLAG_WRITE_DELAYED;
#endif
              apiflags |= TCP_WRITE_FLAG_MORE;
          }
      }
      LWIP_ASSERT("lwip_netconn_do_writemore: invalid length!", ((conn->write_offset + len) <= conn->current_msg->msg.w.len));
      err = pGlobalBlock->pTcpipBlock->fntcp_write(conn->pcb.tcp, dataptr, len, apiflags);
      /* if OK or memory error, check available space */
      if ((err == ERR_OK) || (err == ERR_MEM)) {
err_mem:
    if (dontblock && (len < conn->current_msg->msg.w.len)) {
      /* non-blocking write did not write everything: mark the pcb non-writable
         and let poll_tcp check writable space to mark the pcb writable again */
      API_EVENT(conn, NETCONN_EVT_SENDMINUS, len);
      conn->flags |= NETCONN_FLAG_CHECK_WRITESPACE;
    } else if ((tcp_sndbuf(conn->pcb.tcp) <= TCP_SNDLOWAT) ||
               (tcp_sndqueuelen(conn->pcb.tcp) >= TCP_SNDQUEUELOWAT)) {
      /* The queued byte- or pbuf-count exceeds the configured low-water limit,
         let select mark this pcb as non-writable. */
      API_EVENT(conn, NETCONN_EVT_SENDMINUS, len);
    }
  }
  
  if (err == ERR_OK) {
    conn->write_offset += len;
    if ((conn->write_offset == conn->current_msg->msg.w.len) || dontblock) {
      /* return sent length */
      conn->current_msg->msg.w.len = conn->write_offset;
      /* everything was written */
      write_finished = 1;
      conn->write_offset = 0;
    }
    pGlobalBlock->pTcpipBlock->fntcp_output(conn->pcb.tcp);
  } else if ((err == ERR_MEM) && !dontblock) {
    /* If ERR_MEM, we wait for sent_tcp or poll_tcp to be called
       we do NOT return to the application thread, since ERR_MEM is
       only a temporary error! */

    /* tcp_write returned ERR_MEM, try tcp_output anyway */
    pGlobalBlock->pTcpipBlock->fntcp_output(conn->pcb.tcp);

#if LWIP_TCPIP_CORE_LOCKING
    conn->flags |= NETCONN_FLAG_WRITE_DELAYED;
#endif
  } else {
    /* On errors != ERR_MEM, we don't try writing any more but return
       the error to the application thread. */
    write_finished = 1;
    conn->current_msg->msg.w.len = 0;
  }
  }
  if (write_finished) {
    /* everything was written: set back connection state
       and back to application task */
    conn->current_msg->err = err;
    conn->current_msg = NULL;
    conn->state = NETCONN_NONE;
#if LWIP_TCPIP_CORE_LOCKING
    if ((conn->flags & NETCONN_FLAG_WRITE_DELAYED) != 0)
#endif
    {
      pGlobalBlock->pTcpipBlock->fnsys_sem_signal(&conn->op_completed);
    }
  }
#if LWIP_TCPIP_CORE_LOCKING
  else
    return ERR_MEM;
#endif
  return ERR_OK;
}
#endif /* LWIP_TCP */

/**
 * Send some data on a TCP pcb contained in a netconn
 * Called from netconn_write
 *
 * @param msg the api_msg_msg pointing to the connection
 */
void
lwip_netconn_do_write(struct api_msg_msg *msg)
{
  USE_GLOBAL_BLOCK

  if (ERR_IS_FATAL(msg->conn->last_err)) {
    msg->err = msg->conn->last_err;
  } else {
    if (NETCONNTYPE_GROUP(msg->conn->type) == NETCONN_TCP) {
#if LWIP_TCP
      if (msg->conn->state != NETCONN_NONE) {
        /* netconn is connecting, closing or in blocking write */
        msg->err = ERR_INPROGRESS;
      } else if (msg->conn->pcb.tcp != NULL) {
        msg->conn->state = NETCONN_WRITE;
        /* set all the variables used by lwip_netconn_do_writemore */
        LWIP_ASSERT("already writing or closing", msg->conn->current_msg == NULL &&
          msg->conn->write_offset == 0);
        LWIP_ASSERT("msg->msg.w.len != 0", msg->msg.w.len != 0);
        msg->conn->current_msg = msg;
        msg->conn->write_offset = 0;
#if LWIP_TCPIP_CORE_LOCKING
        msg->conn->flags &= ~NETCONN_FLAG_WRITE_DELAYED;
        if (pGlobalBlock->pTcpipBlock->fnlwip_netconn_do_writemore(msg->conn) != ERR_OK) {
          LWIP_ASSERT("state!", msg->conn->state == NETCONN_WRITE);
          UNLOCK_TCPIP_CORE();
          pGlobalBlock->pTcpipBlock->fnsys_arch_sem_wait(&msg->conn->op_completed, 0);
          LOCK_TCPIP_CORE();
          LWIP_ASSERT("state!", msg->conn->state == NETCONN_NONE);
        }
#else /* LWIP_TCPIP_CORE_LOCKING */
        pGlobalBlock->pTcpipBlock->fnlwip_netconn_do_writemore(msg->conn);
#endif /* LWIP_TCPIP_CORE_LOCKING */
        /* for both cases: if lwip_netconn_do_writemore was called, don't ACK the APIMSG
           since lwip_netconn_do_writemore ACKs it! */
        return;
      } else {
        msg->err = ERR_CONN;
      }
#else /* LWIP_TCP */
      msg->err = ERR_VAL;
#endif /* LWIP_TCP */
#if (LWIP_UDP || LWIP_RAW)
    } else {
      msg->err = ERR_VAL;
#endif /* (LWIP_UDP || LWIP_RAW) */
    }
  }
  TCPIP_APIMSG_ACK(msg);
}

/**
 * Return a connection's local or remote address
 * Called from netconn_getaddr
 *
 * @param msg the api_msg_msg pointing to the connection
 */
void
lwip_netconn_do_getaddr(struct api_msg_msg *msg)
{
  UINT8 ncType;
  USE_GLOBAL_BLOCK

  if (msg->conn->pcb.ip != NULL) {
    if (msg->msg.ad.local) {
      ipX_addr_copy(PCB_ISIPV6(msg->conn->pcb.ip), *(msg->msg.ad.ipaddr),
        msg->conn->pcb.ip->local_ip);
    } else {
      ipX_addr_copy(PCB_ISIPV6(msg->conn->pcb.ip), *(msg->msg.ad.ipaddr),
        msg->conn->pcb.ip->remote_ip);
    }
    msg->err = ERR_OK;
#if 0
//    switch (NETCONNTYPE_GROUP(msg->conn->type)) {
#else
    ncType = NETCONNTYPE_GROUP(msg->conn->type);
#endif // #if 0
#if LWIP_RAW
    case NETCONN_RAW:
      if (msg->msg.ad.local) {
        *(msg->msg.ad.port) = msg->conn->pcb.raw->protocol;
      } else {
        /* return an error as connecting is only a helper for upper layers */
        msg->err = ERR_CONN;
      }
      break;
#endif /* LWIP_RAW */
#if LWIP_UDP
#if 0
    case NETCONN_UDP:
#else
    if (ncType == NETCONN_UDP) {
#endif // #if 0
      if (msg->msg.ad.local) {
        *(msg->msg.ad.port) = msg->conn->pcb.udp->local_port;
      } else {
        if ((msg->conn->pcb.udp->flags & UDP_FLAGS_CONNECTED) == 0) {
          msg->err = ERR_CONN;
        } else {
          *(msg->msg.ad.port) = msg->conn->pcb.udp->remote_port;
        }
      }
#if 0
      break;
#else
    }
    else 
#endif // #if 0
#endif /* LWIP_UDP */
#if LWIP_TCP
#if 0
    case NETCONN_TCP:
#else
    if (ncType == NETCONN_TCP) {
#endif // #if 0
      *(msg->msg.ad.port) = (msg->msg.ad.local?msg->conn->pcb.tcp->local_port:msg->conn->pcb.tcp->remote_port);
#if 0
      break;
#else
    }
#endif // #if 0
#endif /* LWIP_TCP */
#if 0
    default:
      LWIP_ASSERT("invalid netconn_type", 0);
      break;
    }
#endif // #if 0
  } else {
    msg->err = ERR_CONN;
  }
  TCPIP_APIMSG_ACK(msg);
}

/**
 * Close a TCP pcb contained in a netconn
 * Called from netconn_close
 *
 * @param msg the api_msg_msg pointing to the connection
 */
void
lwip_netconn_do_close(struct api_msg_msg *msg)
{
  USE_GLOBAL_BLOCK

#if LWIP_TCP
  /* @todo: abort running write/connect? */
  if ((msg->conn->state != NETCONN_NONE) && (msg->conn->state != NETCONN_LISTEN)) {
    /* this only happens for TCP netconns */
    LWIP_ASSERT("NETCONNTYPE_GROUP(msg->conn->type) == NETCONN_TCP",
                NETCONNTYPE_GROUP(msg->conn->type) == NETCONN_TCP);
    msg->err = ERR_INPROGRESS;
  } else if ((msg->conn->pcb.tcp != NULL) && (NETCONNTYPE_GROUP(msg->conn->type) == NETCONN_TCP)) {
    if ((msg->msg.sd.shut != NETCONN_SHUT_RDWR) && (msg->conn->state == NETCONN_LISTEN)) {
      /* LISTEN doesn't support half shutdown */
      msg->err = ERR_CONN;
    } else {
      if (msg->msg.sd.shut & NETCONN_SHUT_RD) {
        /* Drain and delete mboxes */
        pGlobalBlock->pTcpipBlock->fnnetconn_drain(msg->conn);
      }
      LWIP_ASSERT("already writing or closing", msg->conn->current_msg == NULL &&
        msg->conn->write_offset == 0);
      msg->conn->state = NETCONN_CLOSE;
      msg->conn->current_msg = msg;
      pGlobalBlock->pTcpipBlock->fnlwip_netconn_do_close_internal(msg->conn);
      /* for tcp netconns, lwip_netconn_do_close_internal ACKs the message */
      return;
    }
  } else
#endif /* LWIP_TCP */
  {
    msg->err = ERR_VAL;
  }
  pGlobalBlock->pTcpipBlock->fnsys_sem_signal(&msg->conn->op_completed);
}

#if LWIP_IGMP || (LWIP_IPV6 && LWIP_IPV6_MLD)
/**
 * Join multicast groups for UDP netconns.
 * Called from netconn_join_leave_group
 *
 * @param msg the api_msg_msg pointing to the connection
 */
void
do_join_leave_group(struct api_msg_msg *msg)
{ 
  if (ERR_IS_FATAL(msg->conn->last_err)) {
    msg->err = msg->conn->last_err;
  } else {
    if (msg->conn->pcb.tcp != NULL) {
      if (NETCONNTYPE_GROUP(msg->conn->type) == NETCONN_UDP) {
#if LWIP_UDP
#if LWIP_IPV6 && LWIP_IPV6_MLD
          if (PCB_ISIPV6(msg->conn->pcb.udp)) {
              if (msg->msg.jl.join_or_leave == NETCONN_JOIN) {
                  msg->err = mld6_joingroup(ipX_2_ip6(msg->msg.jl.netif_addr),
                      ipX_2_ip6(msg->msg.jl.multiaddr));
              } else {
                  msg->err = mld6_leavegroup(ipX_2_ip6(msg->msg.jl.netif_addr),
                      ipX_2_ip6(msg->msg.jl.multiaddr));
              }
          }
          else
#endif /* LWIP_IPV6 && LWIP_IPV6_MLD */
          {
#if LWIP_IGMP
              if (msg->msg.jl.join_or_leave == NETCONN_JOIN) {
                  msg->err = igmp_joingroup(ipX_2_ip(msg->msg.jl.netif_addr),
                      ipX_2_ip(msg->msg.jl.multiaddr));
              } else {
                  msg->err = igmp_leavegroup(ipX_2_ip(msg->msg.jl.netif_addr),
                      ipX_2_ip(msg->msg.jl.multiaddr));
              }
#endif /* LWIP_IGMP */
        }
#endif /* LWIP_UDP */
#if (LWIP_TCP || LWIP_RAW)
      } else {
        msg->err = ERR_VAL;
#endif /* (LWIP_TCP || LWIP_RAW) */
      }
    } else {
      msg->err = ERR_CONN;
    }
  }
  TCPIP_APIMSG_ACK(msg);
}
#endif /* LWIP_IGMP || (LWIP_IPV6 && LWIP_IPV6_MLD) */

#if LWIP_DNS
/**
 * Callback function that is called when DNS name is resolved
 * (or on timeout). A waiting application thread is waked up by
 * signaling the semaphore.
 */
void
lwip_netconn_do_dns_found(const char *name, ip_addr_t *ipaddr, void *arg)
{
  struct dns_api_msg *msg = (struct dns_api_msg*)arg;
  USE_GLOBAL_BLOCK

  LWIP_ASSERT("DNS response for wrong host name", strcmp(msg->name, name) == 0);

  if (ipaddr == NULL) {
    /* timeout or memory error */
    *msg->err = ERR_VAL;
  } else {
    /* address was resolved */
    *msg->err = ERR_OK;
    *msg->addr = *ipaddr;
  }
  /* wake up the application task waiting in netconn_gethostbyname */
  pGlobalBlock->pTcpipBlock->fnsys_sem_signal(msg->sem);
}

/**
 * Execute a DNS query
 * Called from netconn_gethostbyname
 *
 * @param arg the dns_api_msg pointing to the query
 */
void
lwip_netconn_do_gethostbyname(void *arg)
{
  struct dns_api_msg *msg = (struct dns_api_msg*)arg;
  USE_GLOBAL_BLOCK

  *msg->err = pGlobalBlock->pTcpipBlock->fndns_gethostbyname(msg->name, msg->addr, pGlobalBlock->pTcpipBlock->fnlwip_netconn_do_dns_found, msg);
  if (*msg->err != ERR_INPROGRESS) {
    /* on error or immediate success, wake up the application
     * task waiting in netconn_gethostbyname */
    pGlobalBlock->pTcpipBlock->fnsys_sem_signal(msg->sem);
  }
}
#endif /* LWIP_DNS */

//#endif /* LWIP_NETCONN */
