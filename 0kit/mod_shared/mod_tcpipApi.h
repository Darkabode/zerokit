#ifndef __MODSHARED_TCPIPAPI_H_
#define __MODSHARED_TCPIPAPI_H_

#include "lwip/ip4_addr.h"
#include "lwip/tcp_impl.h"
#include "netif/etharp.h"
#include "lwip/udp.h"
#include "lwip/dns.h"
#include "lwip/timers.h"
#include "lwip/tcpip.h"

// Из dns.c
/** DNS table entry */
struct dns_table_entry
{
    uint8_t  state;
    uint8_t  numdns;
    uint8_t  tmr;
    uint8_t  retries;
    uint8_t  seqno;
    uint8_t  err;
    uint32_t ttl;
    char name[256/*DNS_MAX_NAME_LENGTH*/];
    struct ip_addr ipaddr;
    /* pointer to callback on DNS query done */
    dns_found_callback found;
    void *arg;
};
#if LWIP_ARP
// Из etharp.c
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
#if LWIP_SNMP
    struct netif *netif;
#endif /* LWIP_SNMP */
    uint8_t state;
    uint8_t ctime;
#if ETHARP_SUPPORT_STATIC_ENTRIES
    uint8_t static_entry;
#endif /* ETHARP_SUPPORT_STATIC_ENTRIES */
};

#else

struct etharp_entry
{
    ip_addr_t ipaddr;
    struct eth_addr ethaddr;
};

#endif // LWIP_ARP

typedef struct queue_node
{
    PVOID msg;
    struct queue_node* next;
} queue_node_t;

typedef struct queue
{
    struct queue_node* head;
    struct queue_node* tail;
    uint32_t enqueue;            // enqueue counter
    uint32_t dequeue;            // dequeue counter
} queue_t;


typedef struct _ADAPTER ADAPTER, *PADAPTER;

typedef struct _dns_spoof_entry
{
    LIST_ENTRY;
    int type;
    uint32_t ipAddr;
    char name[1];
} dns_spoof_entry_t, *pdns_spoof_entry_t;

///////////////////////////////////////////////////////////////////////////////////////////////////
// Внутренние функции для работы с сетью.

typedef struct pbuf* (*Fnpbuf_alloc)(pbuf_layer layer, uint16_t length, pbuf_type type);
typedef uint8_t (*Fnpbuf_free)(struct pbuf *p);
typedef uint16_t (*Fnpbuf_copy_partial)(struct pbuf *buf, void *dataptr, uint16_t len, uint16_t offset);
typedef void (*Fnpbuf_cat)(struct pbuf *h, struct pbuf *t);
typedef uint8_t (*Fnpbuf_clen)(struct pbuf *p);
typedef uint8_t (*Fnpbuf_header)(struct pbuf *p, INT16 header_size_increment);
typedef uint16_t (*Fnpbuf_memcmp)(struct pbuf* p, uint16_t offset, const void* s2, uint16_t n);

typedef void (*Fnethernetif_init)(struct netif *netif);

typedef void (*Fnnetbuf_delete)(struct netbuf *buf);

typedef struct netif* (*Fnnetif_add)(struct netif *netif, ip_addr_t *ipaddr, ip_addr_t *netmask, ip_addr_t *gw, void *state, Fnethernetif_init init/*, netif_input_fn input*/);
typedef void (*Fnnetif_set_default)(struct netif *netif);
typedef void (*Fnnetif_set_up)(struct netif *netif);
typedef void (*Fnnetif_set_addr)(struct netif *netif, ip_addr_t *ipaddr, ip_addr_t *netmask, ip_addr_t *gw);
typedef void (*Fnnetif_remove)(struct netif *netif);
typedef void (*Fnnetif_set_down)(struct netif *netif);
typedef void (*Fnnetif_set_gw)(struct netif *netif, ip_addr_t *gw);
typedef void (*Fnnetif_set_ipaddr)(struct netif *netif, ip_addr_t *ipaddr);
typedef void (*Fnnetif_set_netmask)(struct netif *netif, ip_addr_t *netmask);


typedef void (*Fnpbuf_chain)(struct pbuf *h, struct pbuf *t);
typedef err_t (*Fnpbuf_copy)(struct pbuf *p_to, struct pbuf *p_from);
typedef void (*Fnpbuf_free_int)(void *p);
typedef uint8_t (*Fnpbuf_get_at)(struct pbuf* p, uint16_t offset);
typedef uint16_t (*Fnpbuf_memfind)(struct pbuf* p, const void* mem, uint16_t mem_len, uint16_t start_offset);
typedef void (*Fnpbuf_realloc)(struct pbuf *p, uint16_t new_len);
typedef void (*Fnpbuf_ref)(struct pbuf *p);

typedef void (*Fnsys_timeout)(uint32_t msecs, sys_timeout_handler handler, void *arg);


typedef err_t (*Fnsys_sem_new)(sys_sem_t *sem, uint8_t count);
typedef void (*Fnsys_sem_set_invalid)(sys_sem_t *sem);
typedef uint32_t (*Fnsys_arch_sem_wait)(sys_sem_t* sem, uint32_t timeout);

typedef void (*Fnsys_mbox_new)(sys_mbox_t* gMBox, int size);
typedef err_t (*Fnsys_mbox_trypost)(sys_mbox_t* gMBox, VOID *msg);

typedef VOID (*Fntcpip_thread)(PVOID arg);
typedef err_t (*Fntcpip_callback_with_block)(tcpip_callback_fn function, void *ctx, uint8_t block);
typedef err_t (*Fntcpip_apimsg)(struct api_msg *apimsg);

typedef void (*Fnlwip_netconn_do_connect)(struct api_msg_msg *msg);
typedef void (*Fnlwip_netconn_do_newconn)(struct api_msg_msg *msg);
typedef void (*Fnlwip_netconn_do_write)(struct api_msg_msg *msg);
typedef void (*Fnlwip_netconn_do_delconn)(struct api_msg_msg *msg);
typedef void (*Fnlwip_netconn_do_close_internal)(struct netconn *conn);
typedef void (*Fnlwip_netconn_do_disconnect)(struct api_msg_msg *msg);
typedef void (*Fnlwip_netconn_do_recv)(struct api_msg_msg *msg);
typedef void (*Fnlwip_netconn_do_send)(struct api_msg_msg *msg);
typedef void (*Fnlwip_netconn_do_close)(struct api_msg_msg *msg);
typedef void (*Fnlwip_netconn_do_gethostbyname)(void *arg);
typedef err_t (*Fnlwip_netconn_do_connected)(void *arg, struct tcp_pcb *pcb, err_t err);
typedef void (*Fnlwip_netconn_do_dns_found)(const char *name, ip_addr_t *ipaddr, void *arg);

typedef uint16_t (*Fninet_chksum)(void *dataptr, uint16_t len);
typedef uint16_t (*Fnlwip_standard_chksum)(void *dataptr, int len);
typedef uint16_t (*Fninet_cksum_pseudo_base)(struct pbuf *p, uint8_t proto, uint16_t proto_len, uint32_t acc);
typedef uint16_t (*Fninet_chksum_pseudo)(struct pbuf *p, uint8_t proto, uint16_t proto_len, ip_addr_t *src, ip_addr_t *dest);
typedef uint16_t (*Fninet_cksum_pseudo_partial_base)(struct pbuf *p, uint8_t proto, uint16_t proto_len, uint16_t chksum_len, uint32_t acc);
typedef uint16_t (*Fninet_chksum_pseudo_partial)(struct pbuf *p, uint8_t proto, uint16_t proto_len, uint16_t chksum_len, ip_addr_t *src, ip_addr_t *dest);

typedef err_t (*Fnip_input)(struct pbuf *p, struct netif *inp);
typedef err_t (*Fnip_output)(struct pbuf *p, ip_addr_t *src, ip_addr_t *dest, uint8_t ttl, uint8_t tos, uint8_t proto);
typedef err_t (*Fnip_output_if)(struct pbuf *p, ip_addr_t *src, ip_addr_t *dest, uint8_t ttl, uint8_t tos, uint8_t proto, struct netif *netif);
typedef struct netif* (*Fnip_route)(ip_addr_t *dest);
typedef uint8_t (*Fnip4_addr_isbroadcast)(uint32_t addr, const struct netif *netif);
//typedef uint8_t (*Fnip4_addr_netmask_valid)(uint32_t netmask);
typedef uint32_t (*Fnipaddr_addr)(const char *cp);
typedef int (*Fnipaddr_aton)(const char *cp, ip_addr_t *addr);
typedef char* (*Fnipaddr_ntoa)(const ip_addr_t *addr);
typedef char* (*Fnipaddr_ntoa_r)(const ip_addr_t *addr, char *buf, int buflen);

typedef queue_t* (*Fnqueue_create)();
typedef void (*Fnqueue_free)(queue_t* q);
typedef void* (*Fnqueue_pop)(queue_t* q, uint32_t timeout, PLARGE_INTEGER tmWaited);
typedef BOOLEAN (*Fnqueue_push)(queue_t* q, VOID* msg);

typedef uint32_t (*Fnsys_arch_mbox_fetch)(sys_mbox_t* mbox, VOID **msg, uint32_t timeout);
typedef void (*Fnsys_mbox_free)(sys_mbox_t* gMBox);
typedef void (*Fnsys_mbox_set_invalid)(sys_mbox_t *mbox);
typedef int (*Fnsys_mbox_valid)(sys_mbox_t *mbox);
typedef void (*Fnsys_sem_signal)(sys_sem_t* sem);
typedef int (*Fnsys_sem_valid)(sys_sem_t *sem);


typedef void (*Fnsys_timeouts_mbox_fetch)(sys_mbox_t *mbox, void **msg);

typedef void (*Fntcp_recved)(struct tcp_pcb *pcb, uint16_t len);
typedef uint32_t (*Fntcp_update_rcv_ann_wnd)(struct tcp_pcb *pcb);

typedef err_t (*Fntcp_output)(struct tcp_pcb *pcb);
typedef void (*Fntcp_abort)(struct tcp_pcb *pcb);

typedef err_t (*Fnlwip_netconn_do_writemore)(struct netconn *conn);

typedef err_t (*Fndns_gethostbyname)(const char *hostname, ip_addr_t *addr, dns_found_callback found, void *callback_arg);

typedef struct netconn* (*Fnnetconn_alloc)(enum netconn_type t, netconn_callback callback);
#if LWIP_DNS
//typedef uint8_t (*Fnraw_input)(struct pbuf *p, struct netif *inp);
typedef uint32_t (*Fndns_lookup)(const char *name);
typedef err_t (*Fndns_enqueue)(const char *name, dns_found_callback found, void *callback_arg);
typedef err_t (*Fndns_send)(uint8_t numdns, const char* name, uint8_t id);
typedef void (*Fndns_check_entry)(uint8_t i);
typedef void (*Fndns_recv)(void* arg, struct udp_pcb* pcb, struct pbuf* p, ip_addr_t* addr, uint16_t port);
typedef unsigned char* (*Fndns_parse_name)(unsigned char *query);

#endif // LWIP_DNS

#if LWIP_UDP
typedef struct udp_pcb* (*Fnudp_new)();
typedef err_t (*Fnudp_bind)(struct udp_pcb *pcb, ip_addr_t *ipaddr, uint16_t port);
typedef void (*Fnudp_recv)(struct udp_pcb *pcb, udp_recv_fn recv, void *recv_arg);
typedef err_t (*Fnudp_connect)(struct udp_pcb *pcb, ip_addr_t *ipaddr, uint16_t port);
typedef err_t (*Fnudp_sendto)(struct udp_pcb *pcb, struct pbuf *p, ip_addr_t *dst_ip, uint16_t dst_port);
#endif // LWIP_UDP

#if LWIP_SNTP
typedef void (*Fnsntp_recv)(void* arg, struct udp_pcb* pcb, struct pbuf* p, ip_addr_t* addr, uint16_t port);
typedef void (*Fnsntp_request)(void* arg);
typedef int (*Fntcpip_get_ntp_time)(uint32_t* pTimeVal);
#endif // LWIP_SNTp

typedef void (*Fntcp_abandon)(struct tcp_pcb *pcb, int reset);
typedef struct tcp_pcb* (*Fntcp_alloc)(uint8_t prio);
typedef err_t (*Fntcp_send_empty_ack)(struct tcp_pcb *pcb);
typedef void (*Fntcp_rexmit)(struct tcp_pcb *pcb);
typedef void (*Fntcp_rexmit_rto)(struct tcp_pcb *pcb);
typedef void (*Fntcp_rexmit_fast)(struct tcp_pcb *pcb);
typedef void (*Fntcp_tmr)();
typedef void (*Fntcp_slowtmr)();
typedef void (*Fntcp_fasttmr)();
typedef void (*Fntcp_input)(struct pbuf *p, struct netif *inp);
typedef void (*Fntcp_segs_free)(struct tcp_seg *seg);
typedef err_t (*Fntcp_process_refused_data)(struct tcp_pcb *pcb);
typedef void (*Fntcp_seg_free)(struct tcp_seg *seg);
typedef err_t (*Fntcp_timewait_input)(struct tcp_pcb *pcb);
typedef err_t (*Fntcp_process)(struct tcp_pcb *pcb);
typedef void (*Fntcp_parseopt)(struct tcp_pcb *pcb);
typedef void (*Fntcp_rst_impl)(uint32_t seqno, uint32_t ackno, ipX_addr_t *local_ip, ipX_addr_t *remote_ip, uint16_t local_port, uint16_t remote_port
#if LWIP_IPV6
        , u8_t isipv6
#endif /* LWIP_IPV6 */
        );
typedef void (*Fntcp_receive)(struct tcp_pcb *pcb);
typedef void (*Fntcp_pcb_purge)(struct tcp_pcb *pcb);
typedef struct tcp_seg* (*Fntcp_seg_copy)(struct tcp_seg *seg);
typedef err_t (*Fntcp_enqueue_flags)(struct tcp_pcb *pcb, uint8_t flags);
typedef err_t (*Fntcp_write_checks)(struct tcp_pcb *pcb, uint16_t len);
typedef struct tcp_seg* (*Fntcp_create_segment)(struct tcp_pcb *pcb, struct pbuf *p, uint8_t flags, uint32_t seqno, uint8_t optflags);
typedef struct pbuf* (*Fntcp_output_alloc_header)(struct tcp_pcb *pcb, uint16_t optlen, uint16_t datalen, uint32_t seqno_be /* already in network byte order */);
typedef void (*Fntcp_output_segment)(struct tcp_seg *seg, struct tcp_pcb *pcb);
typedef err_t (*Fntcp_send_fin)(struct tcp_pcb *pcb);
typedef struct pbuf* (*Fntcp_pbuf_prealloc)(pbuf_layer layer, uint16_t length, uint16_t max_length, uint16_t *oversize, struct tcp_pcb *pcb, uint8_t apiflags, uint8_t first_seg);
typedef err_t (*Fntcp_write)(struct tcp_pcb *pcb, const void *arg, uint16_t len, uint8_t apiflags);
typedef void (*Fntcp_zero_window_probe)(struct tcp_pcb *pcb);
typedef void (*Fntcp_keepalive)(struct tcp_pcb *pcb);
typedef void (*Fntcp_pcb_remove)(struct tcp_pcb **pcblist, struct tcp_pcb *pcb);
typedef uint32_t (*Fntcp_next_iss)();
typedef uint16_t (*Fntcp_new_port)();
typedef err_t (*Fntcp_connect)(struct tcp_pcb *pcb, ip_addr_t *ipaddr, uint16_t port, tcp_connected_fn connected);
typedef err_t (*Fntcp_close)(struct tcp_pcb *pcb);
typedef err_t (*Fntcp_close_shutdown)(struct tcp_pcb *pcb, uint8_t rst_on_unacked_data);
typedef err_t (*Fntcp_shutdown)(struct tcp_pcb *pcb, int shut_rx, int shut_tx);
typedef void (*Fntcp_kill_timewait)();
typedef void (*Fntcp_kill_prio)(uint8_t prio);
typedef void (*Fntcp_oos_insert_segment)(struct tcp_seg *cseg, struct tcp_seg *next);

typedef void (*Fntcpip_tcp_timer)(void *arg);
typedef void (*Fntcp_timer_needed)();

typedef uint16_t (*Fnudp_new_port)(void);
typedef void (*Fnudp_input)(struct pbuf *p, struct netif *inp);
typedef err_t (*Fnudp_sendto_if)(struct udp_pcb *pcb, struct pbuf *p, ip_addr_t *dst_ip, uint16_t dst_port, struct netif *netif);
typedef err_t (*Fnudp_send)(struct udp_pcb *pcb, struct pbuf *p);

#if LWIP_ARP
typedef void (*Fnarp_timer)(void *arg);
typedef void (*Fnetharp_tmr)();
typedef void (*Fnetharp_free_entry)(int i);
typedef void (*Fnetharp_arp_input)(struct netif *netif, struct eth_addr *ethaddr, struct pbuf *p);
typedef err_t (*Fnetharp_update_arp_entry)(struct netif *netif, ip_addr_t *ipaddr, struct eth_addr *ethaddr, uint8_t flags);
typedef err_t (*Fnetharp_output_to_arp_index)(struct netif *netif, struct pbuf *q, uint8_t arp_idx);
typedef err_t (*Fnetharp_send_ip)(struct netif *netif, struct pbuf *p, struct eth_addr *src, struct eth_addr *dst);
typedef err_t (*Fnetharp_request)(struct netif *netif, ip_addr_t *ipaddr);
typedef err_t (*Fnetharp_raw)(struct netif *netif, const struct eth_addr *ethsrc_addr, const struct eth_addr *ethdst_addr, const struct eth_addr *hwsrc_addr, const ip_addr_t *ipsrc_addr, const struct eth_addr *hwdst_addr, const ip_addr_t *ipdst_addr, const uint16_t opcode);
typedef err_t (*Fnetharp_query)(struct netif *netif, ip_addr_t *ipaddr, struct pbuf *q);
typedef INT8 (*Fnetharp_find_entry)(ip_addr_t *ipaddr, uint8_t flags);
typedef void (*Fnfree_etharp_q)(struct etharp_q_entry *q);
#endif // LWIP_ARP
typedef err_t (*Fnetharp_output)(struct netif *netif, struct pbuf *q, ip_addr_t *ipaddr);

typedef struct pbuf* (*Fnip_reass)(struct pbuf *p);
typedef void (*Fnip_reass_timer)(void *arg);
typedef void (*Fnip_reass_tmr)();
typedef int (*Fnip_reass_free_complete_datagram)(struct ip_reassdata *ipr, struct ip_reassdata *prev);
typedef void (*Fnip_reass_dequeue_datagram)(struct ip_reassdata *ipr, struct ip_reassdata *prev);
typedef int (*Fnip_reass_remove_oldest_datagram)(struct ip_hdr *fraghdr, int pbufs_needed);
typedef struct ip_reassdata* (*Fnip_reass_enqueue_new_datagram)(struct ip_hdr *fraghdr, int clen);
typedef void (*Fnip_frag_free_pbuf_custom_ref)(struct pbuf_custom_ref* p);

typedef struct pbuf* (*Fnpbuf_alloced_custom)(pbuf_layer l, uint16_t length, pbuf_type type, struct pbuf_custom *p, void *payload_mem, uint16_t payload_mem_len);
typedef void (*Fndns_timer)(void *arg);
typedef void (*Fndns_tmr)();

typedef void (*Fnpbuf_free_ooseq)();
typedef void (*Fnpbuf_pool_is_empty)();
typedef void (*Fndns_check_entries)();
typedef int (*Fnip_reass_chain_frag_into_datagram_and_validate)(struct ip_reassdata *ipr, struct pbuf *new_p);
typedef void (*Fnipfrag_free_pbuf_custom)(struct pbuf *p);
typedef err_t (*Fnip_frag)(struct pbuf *p, struct netif *netif, ip_addr_t *dest);

typedef int (*Fnethernet_input)(struct pbuf *p, struct netif *netif);
typedef void (*Fnsys_untimeout)(sys_timeout_handler handler, void *arg);
typedef void (*Fnudp_disconnect)(struct udp_pcb *pcb);

typedef err_t (*Fnpoll_tcp)(void *arg, struct tcp_pcb *pcb);
typedef err_t (*Fnrecv_tcp)(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err);
typedef err_t (*Fnsent_tcp)(void *arg, struct tcp_pcb *pcb, uint16_t len);
typedef void (*Fnerr_tcp)(void *arg, err_t err);
typedef void (*Fnsetup_tcp)(struct netconn *conn);

typedef void (*Fntcp_arg)(struct tcp_pcb *pcb, void *arg);
//typedef void (*Fntcp_accept)(struct tcp_pcb *pcb, tcp_accept_fn accept);
typedef void (*Fntcp_recv)(struct tcp_pcb *pcb, tcp_recv_fn recv);
typedef void (*Fntcp_sent)(struct tcp_pcb *pcb, tcp_sent_fn sent);
typedef void (*Fntcp_poll)(struct tcp_pcb *pcb, tcp_poll_fn poll, uint8_t interval);
typedef void (*Fntcp_err)(struct tcp_pcb *pcb, tcp_err_fn err);
typedef void (*Fnudp_remove)(struct udp_pcb *pcb);
typedef void (*Fnpcb_new)(struct api_msg_msg *msg);
typedef void (*Fnrecv_udp)(void *arg, struct udp_pcb *pcb, struct pbuf *p, ip_addr_t *addr, uint16_t port);
typedef struct tcp_pcb* (*Fntcp_new)(void);
typedef uint8_t (*Fndns_compare_name)(unsigned char *query, unsigned char *response);
typedef struct pbuf_custom_ref* (*Fnip_frag_alloc_pbuf_custom_ref)(void);
typedef err_t (*Fntcp_recv_null)(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err);
typedef uint16_t (*Fntcp_eff_send_mss_impl)(uint16_t sendmss, ipX_addr_t *dest
#if LWIP_IPV6
                                     , ipX_addr_t *src, u8_t isipv6
#endif /* LWIP_IPV6 */
               );

///////////////////////////////////////////////////////////////////////////////////////////////////
// Публичные функции для работы с сетью.

typedef void (*Fndns_setserver)(uint8_t numdns, ip_addr_t *dnsserver);

typedef err_t (*Fnnetconn_connect)(struct netconn *conn, ip_addr_t *addr, uint16_t port);
typedef err_t (*Fnnetconn_gethostbyname)(const char *name, ip_addr_t *addr);
typedef err_t (*Fnnetconn_write_partly)(struct netconn *conn, const void *dataptr, size_t size, uint8_t apiflags, size_t *bytes_written);
typedef err_t (*Fnnetconn_recv)(struct netconn *conn, struct netbuf **new_buf);
typedef err_t (*Fnnetconn_close)(struct netconn *conn);
typedef err_t (*Fnnetconn_shutdown)(struct netconn *conn, uint8_t shut_rx, uint8_t shut_tx);
typedef err_t (*Fnnetconn_close_shutdown)(struct netconn *conn, uint8_t how);
typedef err_t (*Fnnetconn_delete)(struct netconn *conn);
typedef void (*Fnnetconn_free)(struct netconn *conn);
typedef err_t (*Fnnetconn_recv_data)(struct netconn *conn, void **new_buf);
typedef struct netconn* (*Fnnetconn_new_with_proto_and_callback)(enum netconn_type t, uint8_t proto, netconn_callback callback);
typedef void (*Fnnetconn_drain)(struct netconn *conn);


typedef int (*Fntcpip_receive)(PUCHAR pBuffer, uint32_t size);
typedef int (*Fntcpip_network_metrics_detector)(uint8_t* pBuffer, uint32_t size);
typedef err_t (*Fntcpip_send)(struct netif * netif, struct pbuf *p);

// Функция должна вернуть TRUE если пакет не должен быть пропущен в стек Windows.
typedef int (*Fnmatch_pattern)(const char* s, const char* pattern);
typedef int (*Fndns_expand)(const uint8_t* msg, const uint8_t* eom_orig, const uint8_t* comp_dn, char* exp_dn, int length);
typedef int (*Fntcpip_dns_spoofed_a)(const char* aName, uint32_t* pIpAddr);
typedef int (*Fntcpip_udp_decoder)(struct ip_hdr* pIpHdr, struct udp_hdr* pUdpHdr, uint8_t* pBuffer, uint32_t size);


typedef struct _mod_tcpip_private
{
    Fnnetif_add fnnetif_add;
    Fnnetif_set_default fnnetif_set_default;
    Fnnetif_set_up fnnetif_set_up;
    Fnnetif_remove fnnetif_remove;
    Fnnetif_set_addr fnnetif_set_addr;
    Fnnetif_set_down fnnetif_set_down;
    Fnnetif_set_gw fnnetif_set_gw;
    Fnnetif_set_ipaddr fnnetif_set_ipaddr;
    Fnnetif_set_netmask fnnetif_set_netmask;

    Fnnetbuf_delete fnnetbuf_delete;
    Fnpbuf_cat fnpbuf_cat;
    Fnpbuf_chain fnpbuf_chain;
    Fnpbuf_clen fnpbuf_clen;
    Fnpbuf_copy fnpbuf_copy;
    Fnpbuf_free_int fnpbuf_free_int;
    Fnpbuf_get_at fnpbuf_get_at;
    Fnpbuf_header fnpbuf_header;
    Fnpbuf_memcmp fnpbuf_memcmp;
    Fnpbuf_memfind fnpbuf_memfind;
    Fnpbuf_realloc fnpbuf_realloc;
    Fnpbuf_ref fnpbuf_ref;

    Fnethernetif_init fnethernetif_init;

    Fnpbuf_alloc fnpbuf_alloc;
    Fnpbuf_free fnpbuf_free;
    Fnpbuf_copy_partial fnpbuf_copy_partial;

    Fnsys_timeout fnsys_timeout;

    Fnsys_mbox_new fnsys_mbox_new;
    Fnsys_mbox_trypost fnsys_mbox_trypost;

    Fnsys_sem_new fnsys_sem_new;
    Fnsys_sem_set_invalid fnsys_sem_set_invalid;

    Fntcpip_thread fntcpip_thread; // offset 0x2E0 - 184
    Fntcpip_callback_with_block fntcpip_callback_with_block;
    Fntcpip_apimsg fntcpip_apimsg;

    Fnlwip_netconn_do_connect fnlwip_netconn_do_connect;
    Fnlwip_netconn_do_newconn fnlwip_netconn_do_newconn;
    Fnlwip_netconn_do_write fnlwip_netconn_do_write;
    Fnlwip_netconn_do_delconn fnlwip_netconn_do_delconn;
    Fnlwip_netconn_do_close_internal fnlwip_netconn_do_close_internal;
    Fnlwip_netconn_do_disconnect fnlwip_netconn_do_disconnect;
    Fnlwip_netconn_do_recv fnlwip_netconn_do_recv;
    Fnlwip_netconn_do_send fnlwip_netconn_do_send;
    Fnlwip_netconn_do_close fnlwip_netconn_do_close;
    Fnlwip_netconn_do_gethostbyname fnlwip_netconn_do_gethostbyname;
    Fnlwip_netconn_do_connected fnlwip_netconn_do_connected;
    Fnlwip_netconn_do_dns_found fnlwip_netconn_do_dns_found;

    Fninet_chksum fninet_chksum;
    Fnlwip_standard_chksum fnlwip_standard_chksum;
    Fninet_cksum_pseudo_base fninet_cksum_pseudo_base;
    Fninet_chksum_pseudo fninet_chksum_pseudo;
    Fninet_cksum_pseudo_partial_base fninet_cksum_pseudo_partial_base;
    Fninet_chksum_pseudo_partial fninet_chksum_pseudo_partial;

    Fnip_input fnip_input;
    Fnip_output fnip_output;
    Fnip_output_if fnip_output_if;
    Fnip_route fnip_route;
    Fnip4_addr_isbroadcast fnip4_addr_isbroadcast;
    //    Fnip4_addr_netmask_valid fnip4_addr_netmask_valid;
    Fnipaddr_addr fnipaddr_addr;
    Fnipaddr_aton fnipaddr_aton;
    Fnipaddr_ntoa fnipaddr_ntoa;
    Fnipaddr_ntoa_r fnipaddr_ntoa_r;    

    Fnqueue_create fnqueue_create;
    Fnqueue_free fnqueue_free;
    Fnqueue_pop fnqueue_pop;
    Fnqueue_push fnqueue_push;

    Fnsys_arch_mbox_fetch fnsys_arch_mbox_fetch;
    Fnsys_arch_sem_wait fnsys_arch_sem_wait; // offset 0x378 - 222
    Fnsys_mbox_free fnsys_mbox_free;
    Fnsys_mbox_set_invalid fnsys_mbox_set_invalid;
    Fnsys_mbox_valid fnsys_mbox_valid;
    Fnsys_sem_signal fnsys_sem_signal;
    Fnsys_sem_valid fnsys_sem_valid;

    Fnsys_timeouts_mbox_fetch fnsys_timeouts_mbox_fetch;

    Fntcp_recved fntcp_recved;
    Fntcp_update_rcv_ann_wnd fntcp_update_rcv_ann_wnd;

    Fntcp_output fntcp_output;
    Fntcp_abort fntcp_abort;
    Fnlwip_netconn_do_writemore fnlwip_netconn_do_writemore;

    Fndns_gethostbyname fndns_gethostbyname;
    Fnnetconn_alloc fnnetconn_alloc;

#if LWIP_DNS
    //    Fnraw_input fnraw_input;
    Fndns_lookup fndns_lookup;
    Fndns_enqueue fndns_enqueue;
    Fndns_send fndns_send;
    Fndns_check_entry fndns_check_entry;
    Fndns_recv fndns_recv;
    Fndns_parse_name fndns_parse_name;
#endif // LWIP_DNS

#if LWIP_UDP
    Fnudp_connect fnudp_connect;
    Fnudp_sendto fnudp_sendto;
    Fnudp_new fnudp_new;
    Fnudp_bind fnudp_bind;
    Fnudp_recv fnudp_recv;
    Fnudp_new_port fnudp_new_port;
    Fnudp_input fnudp_input;
    Fnudp_sendto_if fnudp_sendto_if;
    Fnudp_send fnudp_send;
#endif // LWIP_UDP

#if LWIP_SNTP
    Fnsntp_recv fnsntp_recv;
    Fnsntp_request fnsntp_request;
    Fntcpip_get_ntp_time fntcpip_get_ntp_time;
#endif // LWIP_SNTP

    Fntcp_abandon fntcp_abandon;
    Fntcp_alloc fntcp_alloc;
    Fntcp_send_empty_ack fntcp_send_empty_ack;
    Fntcp_rexmit fntcp_rexmit;
    Fntcp_rexmit_rto fntcp_rexmit_rto;
    Fntcp_rexmit_fast fntcp_rexmit_fast;
    Fntcp_tmr fntcp_tmr;
    Fntcp_slowtmr fntcp_slowtmr;
    Fntcp_fasttmr fntcp_fasttmr;
    Fntcp_input fntcp_input;
    Fntcp_segs_free fntcp_segs_free;
    Fntcp_process_refused_data fntcp_process_refused_data;
    Fntcp_seg_free fntcp_seg_free;
    Fntcp_timewait_input fntcp_timewait_input;
    Fntcp_process fntcp_process;
    Fntcp_parseopt fntcp_parseopt;
    Fntcp_rst_impl fntcp_rst_impl;
    Fntcp_receive fntcp_receive;
    Fntcp_pcb_purge fntcp_pcb_purge;
    Fntcp_seg_copy fntcp_seg_copy;
    Fntcp_enqueue_flags fntcp_enqueue_flags;
    Fntcp_write_checks fntcp_write_checks;
    Fntcp_create_segment fntcp_create_segment;
    Fntcp_output_alloc_header fntcp_output_alloc_header;
    Fntcp_output_segment fntcp_output_segment;
    Fntcp_send_fin fntcp_send_fin;
    Fntcp_pbuf_prealloc fntcp_pbuf_prealloc;
    Fntcp_write fntcp_write;
    Fntcp_zero_window_probe fntcp_zero_window_probe;
    Fntcp_keepalive fntcp_keepalive;
    Fntcp_pcb_remove fntcp_pcb_remove;
    Fntcp_next_iss fntcp_next_iss;
    Fntcp_new_port fntcp_new_port;
    Fntcp_connect fntcp_connect;
    Fntcp_close fntcp_close;
    Fntcp_close_shutdown fntcp_close_shutdown;
    Fntcp_shutdown fntcp_shutdown;
    Fntcp_kill_timewait fntcp_kill_timewait;
    Fntcp_kill_prio fntcp_kill_prio;
    Fntcp_oos_insert_segment fntcp_oos_insert_segment;

    Fntcpip_tcp_timer fntcpip_tcp_timer;
    Fntcp_timer_needed fntcp_timer_needed;
#if LWIP_ARP
    Fnarp_timer fnarp_timer;
    Fnetharp_tmr fnetharp_tmr;
    Fnetharp_free_entry fnetharp_free_entry;
    Fnetharp_arp_input fnetharp_arp_input;
    Fnetharp_update_arp_entry fnetharp_update_arp_entry;
    Fnetharp_output_to_arp_index fnetharp_output_to_arp_index;
    Fnetharp_send_ip fnetharp_send_ip;
    Fnetharp_request fnetharp_request;
    Fnetharp_raw fnetharp_raw;
    Fnetharp_query fnetharp_query;
    Fnetharp_find_entry fnetharp_find_entry;
    Fnfree_etharp_q fnfree_etharp_q;
#endif // LWIP_ARP
    Fnetharp_output fnetharp_output;

    Fnip_reass fnip_reass;
    Fnip_reass_timer fnip_reass_timer;
    Fnip_reass_tmr fnip_reass_tmr;
    Fnip_reass_free_complete_datagram fnip_reass_free_complete_datagram;
    Fnip_reass_dequeue_datagram fnip_reass_dequeue_datagram;
    Fnip_reass_remove_oldest_datagram fnip_reass_remove_oldest_datagram;
    Fnip_reass_enqueue_new_datagram fnip_reass_enqueue_new_datagram;
    Fnip_frag_free_pbuf_custom_ref fnip_frag_free_pbuf_custom_ref;

    Fnpbuf_alloced_custom fnpbuf_alloced_custom;
    Fndns_timer fndns_timer;
    Fndns_tmr fndns_tmr;

    Fnpbuf_free_ooseq fnpbuf_free_ooseq;
    Fnpbuf_pool_is_empty fnpbuf_pool_is_empty;
    Fndns_check_entries fndns_check_entries;
    Fnip_reass_chain_frag_into_datagram_and_validate fnip_reass_chain_frag_into_datagram_and_validate;
    Fnipfrag_free_pbuf_custom fnipfrag_free_pbuf_custom;
    Fnip_frag fnip_frag;
    Fnethernet_input fnethernet_input;
    Fnsys_untimeout fnsys_untimeout;
    Fnudp_disconnect fnudp_disconnect;
    Fnpoll_tcp fnpoll_tcp;
    Fnrecv_tcp fnrecv_tcp;
    Fnsent_tcp fnsent_tcp;
    Fnerr_tcp fnerr_tcp;
    Fnsetup_tcp fnsetup_tcp;

    Fntcp_arg fntcp_arg;
    Fntcp_recv fntcp_recv;
    Fntcp_sent fntcp_sent;
    Fntcp_poll fntcp_poll;
    Fntcp_err fntcp_err;

    Fnudp_remove fnudp_remove;
    Fnpcb_new fnpcb_new;
    Fnrecv_udp fnrecv_udp;
    Fntcp_new fntcp_new;
    Fndns_compare_name fndns_compare_name;
    Fnip_frag_alloc_pbuf_custom_ref fnip_frag_alloc_pbuf_custom_ref;
    Fntcp_recv_null fntcp_recv_null;
    Fntcp_eff_send_mss_impl fntcp_eff_send_mss_impl;

    Fntcpip_send fntcpip_send;

    Fnmatch_pattern fnmatch_pattern;
    Fndns_expand fndns_expand;
    Fntcpip_dns_spoofed_a fntcpip_dns_spoofed_a;
    Fntcpip_udp_decoder fntcpip_udp_decoder;

    uint8_t* pModBase;

    KSPIN_LOCK spinLock;
    pvoid_t tcpipThreadObject;
    bool_t tcpipThreadMustBeStopped;

    struct netif *netif_list;
    struct netif *netif_default;
    uint8_t netif_num;

    uint16_t localPort;

    sys_mbox_t mbox;

    /* Incremented every coarse grained timer shot (typically every 500 ms). */
    uint32_t tcp_ticks;
    uint8_t tcp_active_pcbs_changed;
    uint8_t tcp_backoff[13 + 3/*padding*/];// = { 1, 2, 3, 4, 5, 6, 7, 7, 7, 7, 7, 7, 7};
     /* Times per slowtmr hits */
    uint8_t tcp_persist_backoff[7 + 1/*padding*/];// = { 3, 6, 12, 24, 48, 96, 120 };

    /* The TCP PCB lists. */
#if 0
    /** List of all TCP PCBs bound but not yet (connected || listening) */
    struct tcp_pcb *tcp_bound_pcbs;
    /** List of all TCP PCBs in LISTEN state */
    union tcp_listen_pcbs_t tcp_listen_pcbs;
#endif // #if 0
    /** List of all TCP PCBs that are in a state in which
     * they accept or send data. */
    struct tcp_pcb *tcp_active_pcbs;
    /** List of all TCP PCBs in TIME-WAIT state */
    struct tcp_pcb *tcp_tw_pcbs;

    /** An array with all (non-temporary) PCB lists, mainly used for smaller code size */
    struct tcp_pcb** tcp_pcb_lists[2/*4*/];// = {&tcp_listen_pcbs.pcbs, &tcp_bound_pcbs, &tcp_active_pcbs, &tcp_tw_pcbs};

    /** Only used for temporary storage. */
    struct tcp_pcb *tcp_tmp_pcb;

    struct tcp_seg inseg;
    struct tcp_hdr *tcphdr;
    struct ip_hdr *iphdr;
    uint32_t seqno, ackno;
    uint16_t tcplen;
    uint8_t flags;
    uint8_t recv_flags;
    
    //uint16_t static_port;
    uint16_t tcp_port;
    uint16_t udp_port;
    /** Timer counter to handle calling slow-timer from tcp_tmr() */ 
    uint8_t tcp_timer;
    uint8_t tcp_timer_ctr;
    uint8_t pbuf_free_ooseq_pending;
    struct pbuf *recv_data;

    struct tcp_pcb *tcp_input_pcb;
    uint32_t iss;

    ip_addr_t ip_addr_any; // = { IPADDR_ANY };
    ip_addr_t ip_addr_broadcast; // = { IPADDR_BROADCAST };

    struct udp_pcb *udp_pcbs;

    /**
     * The interface that provided the packet for the current callback
     * invocation.
     */
//    struct netif *current_netif;

    
    uint8_t mem_free_count;
    uint8_t dns_seqno;
#if LWIP_ARP
    struct etharp_entry arp_table[32/*ARP_TABLE_SIZE*/];
    uint8_t etharp_cached_entry;
#else

    struct etharp_entry arp_table[3];

#endif // LWIP_ARP

    /**
     * Header of the input packet currently being processed.
     */
    const struct ip_hdr *current_header;
    /** Source IP address of current_header */
//    ip_addr_t current_iphdr_src;
    /** Destination IP address of current_header */
//    ip_addr_t current_iphdr_dest;

    char ip_addr_str[16];

    struct ip_reassdata *reassdatagrams;
    uint16_t ip_reass_pbufcount;

    struct ip_globals ip_data;

    /** The IP header ID of the next outgoing IP packet */
    uint16_t ip_id;

    uint16_t memp_sizes[14]; // Количество элементов в данном масиве зависит от количества активных пулов перечисленных в файле memp_std.h
                          // При добавлении/удалении нового пула необходимо сихнронизироват файл memp_std.h с этим массивом. Также очень
                          // важно соблюдать порядок.


    struct sys_timeo *next_timeout;
    int tcpip_tcp_timer_active;

    // DNS переменные.
    struct udp_pcb* dns_pcb;
    struct dns_table_entry dns_table[DNS_TABLE_SIZE];
    struct ip_addr dns_servers[DNS_MAX_SERVERS];
    /** Contiguous buffer for processing responses */
    uint8_t dns_payload_buffer[LWIP_MEM_ALIGN_BUFFER(DNS_MSG_SIZE)];
    uint8_t* dns_payload;

    int dnsSpooferPaused;
    dns_spoof_entry_t dnsSpoofHead;

    // SNTP переменные.
    struct udp_pcb* sntp_pcb;
#if LWIP_ETHERNET
    struct eth_addr ethbroadcast; // = {{0xff,0xff,0xff,0xff,0xff,0xff}};
    struct eth_addr ethzero; // = {{0,0,0,0,0,0}};
#endif // LWIP_ETHERNET
} mod_tcpip_private_t, *pmod_tcpip_private_t;

// Интерфейсные функции
typedef void (*Fntcpip_shutdown_routine)();
typedef NTSTATUS (*Fntcpip_start_thread)(LONG baseThreadPrioIncrement);
typedef void (*Fntcpip_reinit_stack_for_adapter)(pndis_adapter_t pAdapter);

typedef void (*Fntcpip_udp_protocol_handler)(struct ip_hdr* pIpHdr, struct udp_hdr* pUdpHdr, uint8_t* pBuffer, uint32_t size);

typedef void (*Fntcpip_remove_all_dnss_entries)();
typedef void (*Fntcpip_add_or_modify_dnss_entry)(const char* sUrl, uint32_t ipAddr);

typedef struct _mod_tcpip_block
{
    Fntcpip_shutdown_routine fntcpip_shutdown_routine;
    Fntcpip_start_thread fntcpip_start_thread;
    Fntcpip_reinit_stack_for_adapter fntcpip_reinit_stack_for_adapter;

    Fntcpip_receive fntcpip_receive;
    Fntcpip_network_metrics_detector fntcpip_network_metrics_detector;
    Fntcpip_udp_protocol_handler fntcpip_udp_protocol_handler;

    Fndns_setserver fndns_setserver;

    Fnnetconn_connect fnnetconn_connect;
    Fnnetconn_write_partly fnnetconn_write_partly;
    Fnnetconn_recv fnnetconn_recv;
    Fnnetconn_gethostbyname fnnetconn_gethostbyname;
    Fnnetconn_close fnnetconn_close;
    Fnnetconn_shutdown fnnetconn_shutdown;
    Fnnetconn_close_shutdown fnnetconn_close_shutdown;
    Fnnetconn_free fnnetconn_free;
    Fnnetconn_delete fnnetconn_delete;
    Fnnetconn_recv_data fnnetconn_recv_data;
    Fnnetconn_new_with_proto_and_callback fnnetconn_new_with_proto_and_callback;
    Fnnetconn_drain fnnetconn_drain;

    Fntcpip_remove_all_dnss_entries fntcpip_remove_all_dnss_entries;
    Fntcpip_add_or_modify_dnss_entry fntcpip_add_or_modify_dnss_entry;

//     // DNS spoofer
//     char domainName[32];
//     // DNS spoofer


    mod_tcpip_private_t;
} mod_tcpip_block_t, *pmod_tcpip_block_t;

#endif // __MODSHARED_TCPIPAPI_H_
