/**
 * @file
 * Sequential API Main thread module
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

#if !NO_SYS /* don't build if not configured for use in lwipopts.h */

#include "lwip/sys.h"
#include "lwip/memp.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/tcpip.h"
#include "lwip/init.h"
#include "netif/etharp.h"
#include "netif/ppp_oe.h"

#if 0
/* global variables */
static tcpip_init_done_fn tcpip_init_done;
static void *tcpip_init_done_arg;
static sys_mbox_t mbox;
#endif // #if 0

#if LWIP_TCPIP_CORE_LOCKING
/** The global semaphore to lock the stack. */
sys_mutex_t lock_tcpip_core;
#endif /* LWIP_TCPIP_CORE_LOCKING */


/**
 * The main lwIP thread. This thread has exclusive access to lwIP core functions
 * (unless access to them is not locked). Other threads communicate with this
 * thread using message boxes.
 *
 * It also starts all the timers to make sure they are running in the right
 * thread context.
 *
 * @param arg unused argument
 */
void
tcpip_thread(void *arg)
{
    struct tcpip_msg *msg;
    UINT8 msgType;
    //LARGE_INTEGER delay;
    //pndis_adapter_t pAdapter;
    USE_GLOBAL_BLOCK

//     delay.QuadPart = -10000000I64;  // 1 секунда

#if 0
    if (tcpip_init_done != NULL) {
        tcpip_init_done(tcpip_init_done_arg);
    }
#endif // #if 0
    LOCK_TCPIP_CORE();
    while (1) { /* MAIN Loop */
        UNLOCK_TCPIP_CORE();
        LWIP_TCPIP_THREAD_ALIVE();

        // Ждём появление сетевого адаптера...
//         while (pNetworkBlock->needReconfigure) {
//             if (pTcpipBlock->tcpipThreadMustBeStopped) {
//                 break;
//             }
//             pGlobalBlock->pCommonBlock->fnKeDelayExecutionThread(KernelMode, FALSE, &delay);
//         }

        if (pGlobalBlock->pTcpipBlock->tcpipThreadMustBeStopped) {
            break;
        }

//         pAdapter = pNetworkBlock->pActiveAdapter;
// 
//         if (pAdapter->miniportPaused) {
//             // Ждём активации сетевого интерфейса одну минуту...
//             pGlobalBlock->pCommonBlock->fnKeDelayExecutionThread(KernelMode, FALSE, &delay);
// 
//             // На случай, если опять реинициализация... :)
//             if (pNetworkBlock->needReconfigure) {
//                 continue;
//             }
//         }

        /* wait for a message, timeouts are processed while waiting */
#if 0
        sys_timeouts_mbox_fetch(&pTcpipBlock->mbox, (void **)&msg);
#else
        pGlobalBlock->pTcpipBlock->fnsys_timeouts_mbox_fetch((sys_mbox_t*)&pGlobalBlock->pTcpipBlock->mbox, (void**)&msg);
        if (msg == NULL) {
            continue;
        }
#endif
        LOCK_TCPIP_CORE();
#if 0
//      switch (msg->type) {
#else
        msgType = msg->type;
#endif // #if 0
#if LWIP_NETCONN
#if 0
        case TCPIP_MSG_API:
#else
        if (msgType == TCPIP_MSG_API) {
#endif
            LWIP_DEBUGF(TCPIP_DEBUG, ("tcpip_thread: API message %p\n", (void *)msg));
            msg->msg.apimsg->function(&(msg->msg.apimsg->msg));
#if 0
        break;
#else
        }
#endif // #if 0
#endif /* LWIP_NETCONN */

#if !LWIP_TCPIP_CORE_LOCKING_INPUT
#if 0
        case TCPIP_MSG_INPKT:
#else
        else if (msgType == TCPIP_MSG_INPKT) {
#endif // #if 0
            LWIP_DEBUGF(TCPIP_DEBUG, ("tcpip_thread: PACKET %p\n", (void *)msg));
#if LWIP_ETHERNET
            if (msg->msg.inp.netif->flags & (NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET)) {
                pGlobalBlock->pTcpipBlock->fnethernet_input(msg->msg.inp.p, msg->msg.inp.netif);
            } else
#endif /* LWIP_ETHERNET */
#if LWIP_IPV6
            if ((*((unsigned char *)(msg->msg.inp.p->payload)) & 0xf0) == 0x60) {
                pGlobalBlock->pTcpipBlock->fnip6_input(msg->msg.inp.p, msg->msg.inp.netif);
            } else
#endif /* LWIP_IPV6 */
            {
                pGlobalBlock->pTcpipBlock->fnip_input(msg->msg.inp.p, msg->msg.inp.netif);
            }
            memp_free(MEMP_TCPIP_MSG_INPKT, msg);
#if 0
        break;
#else
        }
#endif // #if 0
#endif /* LWIP_TCPIP_CORE_LOCKING_INPUT */

#if LWIP_NETIF_API
    case TCPIP_MSG_NETIFAPI:
      LWIP_DEBUGF(TCPIP_DEBUG, ("tcpip_thread: Netif API message %p\n", (void *)msg));
      msg->msg.netifapimsg->function(&(msg->msg.netifapimsg->msg));
      break;
#endif /* LWIP_NETIF_API */

#if LWIP_TCPIP_TIMEOUT
#if 0
    case TCPIP_MSG_TIMEOUT:
#else
        else if (msgType == TCPIP_MSG_TIMEOUT) {
#endif // #if 0
            LWIP_DEBUGF(TCPIP_DEBUG, ("tcpip_thread: TIMEOUT %p\n", (void *)msg));
            pGlobalBlock->pTcpipBlock->fnsys_timeout(msg->msg.tmo.msecs, msg->msg.tmo.h, msg->msg.tmo.arg);
            memp_free(MEMP_TCPIP_MSG_API, msg);
#if 0
        break;
#else
        }
#endif // #if 0

#if 0
        case TCPIP_MSG_UNTIMEOUT:
#else
        else if (msgType == TCPIP_MSG_UNTIMEOUT) {
#endif // #if 0
            LWIP_DEBUGF(TCPIP_DEBUG, ("tcpip_thread: UNTIMEOUT %p\n", (void *)msg));
            pGlobalBlock->pTcpipBlock->fnsys_untimeout(msg->msg.tmo.h, msg->msg.tmo.arg);
            memp_free(MEMP_TCPIP_MSG_API, msg);
#if 0
        break;
#else
        }
#endif
#endif /* LWIP_TCPIP_TIMEOUT */
#if 0
        case TCPIP_MSG_CALLBACK:
#else
        else if (msgType == TCPIP_MSG_CALLBACK) {
#endif // #if 0
            LWIP_DEBUGF(TCPIP_DEBUG, ("tcpip_thread: CALLBACK %p\n", (void *)msg));
            msg->msg.cb.function(msg->msg.cb.ctx);
            memp_free(MEMP_TCPIP_MSG_API, msg);
#if 0
        break;
#else
        }
#endif
#if 0
        case TCPIP_MSG_CALLBACK_STATIC:
#else
        else if (msgType == TCPIP_MSG_CALLBACK_STATIC) {
#endif // #if 0
            LWIP_DEBUGF(TCPIP_DEBUG, ("tcpip_thread: CALLBACK_STATIC %p\n", (void *)msg));
            msg->msg.cb.function(msg->msg.cb.ctx);
#if 0
        break;
#else
        }
#endif
#if 0
    default:
#else
        else {
#endif // #if 0
            LWIP_DEBUGF(TCPIP_DEBUG, ("tcpip_thread: invalid message: %d\n", msg->type));
            LWIP_ASSERT("tcpip_thread: invalid message", 0);
#if 0
        break;
#else
        }
#endif
    }
    pGlobalBlock->pCommonBlock->fnPsTerminateSystemThread(STATUS_SUCCESS);
}

#if 0
/**
 * Pass a received packet to tcpip_thread for input processing
 *
 * @param p the received packet, p->payload pointing to the Ethernet header or
 *          to an IP header (if inp doesn't have NETIF_FLAG_ETHARP or
 *          NETIF_FLAG_ETHERNET flags)
 * @param inp the network interface on which the packet was received
 */
err_t
tcpip_input(struct pbuf *p, struct netif *inp)
{
#if LWIP_TCPIP_CORE_LOCKING_INPUT
  err_t ret;
  LWIP_DEBUGF(TCPIP_DEBUG, ("tcpip_input: PACKET %p/%p\n", (void *)p, (void *)inp));
  LOCK_TCPIP_CORE();
#if LWIP_ETHERNET
  if (inp->flags & (NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET)) {
    ret = ethernet_input(p, inp);
  } else
#endif /* LWIP_ETHERNET */
  {
    ret = ip_input(p, inp);
  }
  UNLOCK_TCPIP_CORE();
  return ret;
#else /* LWIP_TCPIP_CORE_LOCKING_INPUT */
  struct tcpip_msg *msg;

  if (!sys_mbox_valid(&mbox)) {
    return ERR_VAL;
  }
  msg = (struct tcpip_msg *)memp_malloc(MEMP_TCPIP_MSG_INPKT);
  if (msg == NULL) {
    return ERR_MEM;
  }

  msg->type = TCPIP_MSG_INPKT;
  msg->msg.inp.p = p;
  msg->msg.inp.netif = inp;
  if (sys_mbox_trypost(&mbox, msg) != ERR_OK) {
    memp_free(MEMP_TCPIP_MSG_INPKT, msg);
    return ERR_MEM;
  }
  return ERR_OK;
#endif /* LWIP_TCPIP_CORE_LOCKING_INPUT */
}
#endif // #if 0
/**
 * Call a specific function in the thread context of
 * tcpip_thread for easy access synchronization.
 * A function called in that way may access lwIP core code
 * without fearing concurrent access.
 *
 * @param f the function to call
 * @param ctx parameter passed to f
 * @param block 1 to block until the request is posted, 0 to non-blocking mode
 * @return ERR_OK if the function was called, another err_t if not
 */
err_t
tcpip_callback_with_block(tcpip_callback_fn function, void *ctx, UINT8 block)
{
  struct tcpip_msg *msg;
  pmod_tcpip_block_t pTcpipBlock;
  USE_GLOBAL_BLOCK
  pTcpipBlock = pGlobalBlock->pTcpipBlock;

#if 0
  if (pGLobalData->fnsys_mbox_valid(&mbox)) {
#endif // #if 0
    msg = (struct tcpip_msg *)memp_malloc(MEMP_TCPIP_MSG_API);
    if (msg == NULL) {
      return ERR_MEM;
    }

    msg->type = TCPIP_MSG_CALLBACK;
    msg->msg.cb.function = function;
    msg->msg.cb.ctx = ctx;
    if (block) {
      pTcpipBlock->fnsys_mbox_trypost(&pTcpipBlock->mbox, msg);
    } else {
      if (pTcpipBlock->fnsys_mbox_trypost(&pTcpipBlock->mbox, msg) != ERR_OK) {
        memp_free(MEMP_TCPIP_MSG_API, msg);
        return ERR_MEM;
      }
    }
    return ERR_OK;
#if 0
  }
  return ERR_VAL;
#endif
}

#if LWIP_TCPIP_TIMEOUT
/**
 * call sys_timeout in tcpip_thread
 *
 * @param msec time in milliseconds for timeout
 * @param h function to be called on timeout
 * @param arg argument to pass to timeout function h
 * @return ERR_MEM on memory error, ERR_OK otherwise
 */
err_t
tcpip_timeout(uint32_t msecs, sys_timeout_handler h, void *arg)
{
  struct tcpip_msg *msg;
  pmod_tcpip_block_t pTcpipBlock;
  USE_GLOBAL_BLOCK

  pTcpipBlock = pGlobalBlock->pTcpipBlock;

#if 0
  if (pTcpipBlock->fnsys_mbox_valid(&mbox)) {
#endif // #if 0
    msg = (struct tcpip_msg *)memp_malloc(MEMP_TCPIP_MSG_API);
    if (msg == NULL) {
      return ERR_MEM;
    }

    msg->type = TCPIP_MSG_TIMEOUT;
    msg->msg.tmo.msecs = msecs;
    msg->msg.tmo.h = h;
    msg->msg.tmo.arg = arg;
    pTcpipBlock->fnsys_mbox_trypost(&pTcpipBlock->mbox, msg);
    return ERR_OK;
#if 0
  }
  return ERR_VAL;
#endif // #if 0
}

/**
 * call sys_untimeout in tcpip_thread
 *
 * @param msec time in milliseconds for timeout
 * @param h function to be called on timeout
 * @param arg argument to pass to timeout function h
 * @return ERR_MEM on memory error, ERR_OK otherwise
 */
err_t
tcpip_untimeout(sys_timeout_handler h, void *arg)
{
  struct tcpip_msg *msg;
  pmod_tcpip_block_t pTcpipBlock;
  USE_GLOBAL_BLOCK

  pTcpipBlock = pGlobalBlock->pTcpipBlock;

#if 0
  if (pTcpipBlock->fnsys_mbox_valid(&mbox)) {
#endif // #if 0
    msg = (struct tcpip_msg *)memp_malloc(MEMP_TCPIP_MSG_API);
    if (msg == NULL) {
      return ERR_MEM;
    }

    msg->type = TCPIP_MSG_UNTIMEOUT;
    msg->msg.tmo.h = h;
    msg->msg.tmo.arg = arg;
    pTcpipBlock->fnsys_mbox_trypost(&pTcpipBlock->mbox, msg);
    return ERR_OK;
#if 0
  }
  return ERR_VAL;
#endif // #if 0
}
#endif /* LWIP_TCPIP_TIMEOUT */

#if LWIP_NETCONN
/**
 * Call the lower part of a netconn_* function
 * This function is then running in the thread context
 * of tcpip_thread and has exclusive access to lwIP core code.
 *
 * @param apimsg a struct containing the function to call and its parameters
 * @return ERR_OK if the function was called, another err_t if not
 */
err_t
tcpip_apimsg(struct api_msg *apimsg)
{
  struct tcpip_msg msg;
  pmod_tcpip_block_t pTcpipBlock;
  USE_GLOBAL_BLOCK

  pTcpipBlock = pGlobalBlock->pTcpipBlock;

#ifdef LWIP_DEBUG
  /* catch functions that don't set err */
  apimsg->msg.err = ERR_VAL;
#endif

#if 0  
  if (pTcpipBlock->fnsys_mbox_valid(&mbox)) {
#endif // #if 0
    msg.type = TCPIP_MSG_API;
    msg.msg.apimsg = apimsg;
    pTcpipBlock->fnsys_mbox_trypost(&pTcpipBlock->mbox, &msg);
    pTcpipBlock->fnsys_arch_sem_wait(&apimsg->msg.conn->op_completed, 0);
    return apimsg->msg.err;
#if 0
  }
  return ERR_VAL;
#endif // #if 0
}

#endif /* LWIP_NETCONN */

#if LWIP_NETIF_API
#if !LWIP_TCPIP_CORE_LOCKING
/**
 * Much like tcpip_apimsg, but calls the lower part of a netifapi_*
 * function.
 *
 * @param netifapimsg a struct containing the function to call and its parameters
 * @return error code given back by the function that was called
 */
err_t
tcpip_netifapi(struct netifapi_msg* netifapimsg)
{
  struct tcpip_msg msg;
  
  if (pGLobalData->fnsys_mbox_valid(&mbox)) {
    err_t err = pTcpipBlock->fnsys_sem_new(&netifapimsg->msg.sem, 0);
    if (err != ERR_OK) {
      netifapimsg->msg.err = err;
      return err;
    }
    
    msg.type = TCPIP_MSG_NETIFAPI;
    msg.msg.netifapimsg = netifapimsg;
    pTcpipBlock->fnsys_mbox_trypost(&mbox, &msg);
    sys_sem_wait(&netifapimsg->msg.sem);
    sys_sem_free(&netifapimsg->msg.sem);
    return netifapimsg->msg.err;
  }
  return ERR_VAL;
}
#else /* !LWIP_TCPIP_CORE_LOCKING */
/**
 * Call the lower part of a netifapi_* function
 * This function has exclusive access to lwIP core code by locking it
 * before the function is called.
 *
 * @param netifapimsg a struct containing the function to call and its parameters
 * @return ERR_OK (only for compatibility fo tcpip_netifapi())
 */
err_t
tcpip_netifapi_lock(struct netifapi_msg* netifapimsg)
{
  LOCK_TCPIP_CORE();  
  netifapimsg->function(&(netifapimsg->msg));
  UNLOCK_TCPIP_CORE();
  return netifapimsg->msg.err;
}
#endif /* !LWIP_TCPIP_CORE_LOCKING */
#endif /* LWIP_NETIF_API */

/**
 * Allocate a structure for a static callback message and initialize it.
 * This is intended to be used to send "static" messages from interrupt context.
 *
 * @param function the function to call
 * @param ctx parameter passed to function
 * @return a struct pointer to pass to tcpip_trycallback().
 */
struct tcpip_callback_msg* tcpip_callbackmsg_new(tcpip_callback_fn function, void *ctx)
{
  struct tcpip_msg *msg;
  pmod_tcpip_block_t pTcpipBlock;
  USE_GLOBAL_BLOCK

  pTcpipBlock = pGlobalBlock->pTcpipBlock;

  msg = (struct tcpip_msg *)memp_malloc(MEMP_TCPIP_MSG_API);
  if (msg == NULL) {
    return NULL;
  }
  msg->type = TCPIP_MSG_CALLBACK_STATIC;
  msg->msg.cb.function = function;
  msg->msg.cb.ctx = ctx;
  return (struct tcpip_callback_msg*)msg;
}

/**
 * Free a callback message allocated by tcpip_callbackmsg_new().
 *
 * @param msg the message to free
 */
void tcpip_callbackmsg_delete(struct tcpip_callback_msg* msg)
{
  USE_GLOBAL_BLOCK

  memp_free(MEMP_TCPIP_MSG_API, msg);
}

/**
 * Try to post a callback-message to the tcpip_thread mbox
 * This is intended to be used to send "static" messages from interrupt context.
 *
 * @param msg pointer to the message to post
 * @return sys_mbox_trypost() return code
 */
err_t
tcpip_trycallback(struct tcpip_callback_msg* msg)
{
  pmod_tcpip_block_t pTcpipBlock;
  USE_GLOBAL_BLOCK
  pTcpipBlock = pGlobalBlock->pTcpipBlock;

#if 0
  if (!sys_mbox_valid(&pTcpipBlock->mbox)) {
    return ERR_VAL;
  }
#endif // #if 0
  return pTcpipBlock->fnsys_mbox_trypost(&pTcpipBlock->mbox, msg);
}

#if 0
/**
 * Initialize this module:
 * - initialize all sub modules
 * - start the tcpip_thread
 *
 * @param initfunc a function to call when tcpip_thread is running and finished initializing
 * @param arg argument to pass to initfunc
 */
void
tcpip_init(tcpip_init_done_fn initfunc, void *arg)
{
  lwip_init();

  tcpip_init_done = initfunc;
  tcpip_init_done_arg = arg;
  pTcpipBlock->fnsys_mbox_new(&mbox, TCPIP_MBOX_SIZE);
//   if(pTcpipBlock->fnsys_mbox_new(&mbox, TCPIP_MBOX_SIZE) != ERR_OK) {
//     LWIP_ASSERT("failed to create tcpip_thread mbox", 0);
//   }
#if LWIP_TCPIP_CORE_LOCKING
  if(sys_mutex_new(&lock_tcpip_core) != ERR_OK) {
    LWIP_ASSERT("failed to create lock_tcpip_core", 0);
  }
#endif /* LWIP_TCPIP_CORE_LOCKING */

  sys_thread_new(TCPIP_THREAD_NAME, tcpip_thread, NULL, TCPIP_THREAD_STACKSIZE, TCPIP_THREAD_PRIO);
}
#endif // #if 0
/**
 * Simple callback function used with tcpip_callback to free a pbuf
 * (pbuf_free has a wrong signature for tcpip_callback)
 *
 * @param p The pbuf (chain) to be dereferenced.
 */
void
pbuf_free_int(void *p)
{
  struct pbuf *q = (struct pbuf *)p;
  pmod_tcpip_block_t pTcpipBlock;
  USE_GLOBAL_BLOCK
  pTcpipBlock = pGlobalBlock->pTcpipBlock;

  pTcpipBlock->fnpbuf_free(q);
}

/**
 * A simple wrapper function that allows you to free a pbuf from interrupt context.
 *
 * @param p The pbuf (chain) to be dereferenced.
 * @return ERR_OK if callback could be enqueued, an err_t if not
 */
err_t
pbuf_free_callback(struct pbuf *p)
{
  pmod_tcpip_block_t pTcpipBlock;
  USE_GLOBAL_BLOCK
  pTcpipBlock = pGlobalBlock->pTcpipBlock;

  return pTcpipBlock->fntcpip_callback_with_block(pTcpipBlock->fnpbuf_free_int, p, 0);
}

#if 0
/**
 * A simple wrapper function that allows you to free heap memory from
 * interrupt context.
 *
 * @param m the heap memory to free
 * @return ERR_OK if callback could be enqueued, an err_t if not
 */
err_t
mem_free_callback(void *m)
{
  USE_GLOBAL_BLOCK

  return pTcpipBlock->fntcpip_callback_with_block(/*pTcpipBlock->fn*/mem_free, m, 0);
}

#endif // #if 0

#endif /* !NO_SYS */
