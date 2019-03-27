
#include "lwip/opt.h"

#if LWIP_SNTP

/** SNTP server port */
#define SNTP_PORT 123

#define SNTP_ERR_KOD                1

/* SNTP protocol defines */
#define SNTP_MSG_LEN                48

#define SNTP_OFFSET_LI_VN_MODE      0
#define SNTP_LI_MASK                0xC0
#define SNTP_LI_NO_WARNING          0x00
#define SNTP_LI_LAST_MINUTE_61_SEC  0x01
#define SNTP_LI_LAST_MINUTE_59_SEC  0x02
#define SNTP_LI_ALARM_CONDITION     0x03 /* (clock not synchronized) */

#define SNTP_VERSION_MASK           0x38
#define SNTP_VERSION                (4/* NTP Version 4*/<<3) 

#define SNTP_MODE_MASK              0x07
#define SNTP_MODE_CLIENT            0x03
#define SNTP_MODE_SERVER            0x04
#define SNTP_MODE_BROADCAST         0x05

#define SNTP_OFFSET_STRATUM         1
#define SNTP_STRATUM_KOD            0x00

#define SNTP_OFFSET_ORIGINATE_TIME  24
#define SNTP_OFFSET_RECEIVE_TIME    32
#define SNTP_OFFSET_TRANSMIT_TIME   40

/* number of seconds between 1900 and 1970 */
#define DIFF_SEC_1900_1970         (2208988800)

/**
 * SNTP packet format (without optional fields)
 * Timestamps are coded as 64 bits:
 * - 32 bits seconds since Jan 01, 1970, 00:00
 * - 32 bits seconds fraction (0-padded)
 * For future use, if the MSB in the seconds part is set, seconds are based
 * on Feb 07, 2036, 06:28:16.
 */
typedef struct _sntp_msg
{
    uint8_t li_vn_mode;
    uint8_t stratum;
    uint8_t poll;
    uint8_t precision;
    uint32_t root_delay;
    uint32_t root_dispersion;
    uint32_t reference_identifier;
    uint32_t reference_timestamp[2];
    uint32_t originate_timestamp[2];
    uint32_t receive_timestamp[2];
    uint32_t transmit_timestamp[2];
} sntp_msg_t;

struct sntp_api_msg
{
    ip_addr_t* pIpAddr;
    // Значение времени, полученное от NTP-сервера.
    uint32_t* pTimeVal;
    /** This semaphore is posted when the name is resolved, the application thread should wait on it. */
    sys_sem_t *sem;
    /** Errors are given back here */
    int* err;
};

/**
 * Send out an sntp request via raw API.
 *
 * @param arg is unused (only necessary to conform to sys_timeout)
 */
void sntp_request(void* arg)
{
    struct sntp_api_msg* msg = (struct sntp_api_msg*)arg;
    struct pbuf* p;
    USE_GLOBAL_BLOCK

    p = pGlobalBlock->pTcpipBlock->fnpbuf_alloc(PBUF_TRANSPORT, SNTP_MSG_LEN, PBUF_RAM);
    if (p != NULL) {
        sntp_msg_t* sntpmsg = (sntp_msg_t*)p->payload;

        /* initialize request message */
        __stosb((uint8_t*)sntpmsg, 0, SNTP_MSG_LEN);
        sntpmsg->li_vn_mode = SNTP_LI_NO_WARNING | SNTP_VERSION | SNTP_MODE_CLIENT;

        /* send request */

        pGlobalBlock->pTcpipBlock->sntp_pcb->recv_arg = arg;
        pGlobalBlock->pTcpipBlock->fnudp_sendto(pGlobalBlock->pTcpipBlock->sntp_pcb, p, msg->pIpAddr, SNTP_PORT);
        pGlobalBlock->pTcpipBlock->fnpbuf_free(p);
    }
    else {
        *msg->err = ERR_MEM;
    }
}

// Функция обратного вызова для входящих UDP пакетов SNTP pcb.
void sntp_recv(void* arg, struct udp_pcb* pcb, struct pbuf* p, ip_addr_t* addr, uint16_t port)
{
    struct sntp_api_msg* msg = (struct sntp_api_msg*)arg;
    uint8_t mode;
    uint8_t stratum;
    uint32_t receive_timestamp;
    err_t err;
    USE_GLOBAL_BLOCK

    err = ERR_ARG;
    /* process the response */
    if (p->tot_len == SNTP_MSG_LEN) {
        pGlobalBlock->pTcpipBlock->fnpbuf_copy_partial(p, &mode, 1, SNTP_OFFSET_LI_VN_MODE);
        mode &= SNTP_MODE_MASK;
        /* if this is a SNTP response... */
        if ((mode == SNTP_MODE_SERVER) || (mode == SNTP_MODE_BROADCAST)) {
                pGlobalBlock->pTcpipBlock->fnpbuf_copy_partial(p, &stratum, 1, SNTP_OFFSET_STRATUM);
                if (stratum == SNTP_STRATUM_KOD) {
                    /* Kiss-of-death packet. Use another server or increase UPDATE_DELAY. */
                    err = SNTP_ERR_KOD;
                }
                else {
                    /* @todo: add code for SNTP_CHECK_RESPONSE >= 3 and >= 4 here */
                    /* correct answer */
                    err = ERR_OK;
                    pGlobalBlock->pTcpipBlock->fnpbuf_copy_partial(p, &receive_timestamp, 4, SNTP_OFFSET_RECEIVE_TIME);
                }
        }
    }

    pGlobalBlock->pTcpipBlock->fnpbuf_free(p);
    if (err == ERR_OK) {
        *msg->pTimeVal = (NTOHL(receive_timestamp) - DIFF_SEC_1900_1970);
    }
    else if (err == SNTP_ERR_KOD) {
        uint32_t activeNtpServer;

        // Меняем сервер, чтобы при следующей попытке, попробовать получить время с другого сервера.

        activeNtpServer = pGlobalBlock->pCommonBlock->pConfig->activeNtpServer;

        if (++activeNtpServer > 3) {
            activeNtpServer = 0;
        }
#if DBG
        pGlobalBlock->pCommonBlock->fncommon_disable_wp();
#endif // DBG
        pGlobalBlock->pCommonBlock->pConfig->activeNtpServer = activeNtpServer;
#if DBG
        pGlobalBlock->pCommonBlock->fncommon_enable_wp();
#endif // DBG
    }

    *msg->err = err;

    /* wake up the application task waiting in netconn_gethostbyname */
    pGlobalBlock->pTcpipBlock->fnsys_sem_signal(msg->sem);
}

// Интерфейсная функция, которую следует вызывать для получения времени с NTP-сервера.
int tcpip_get_ntp_time(uint32_t* pTimeVal)
{
    struct sntp_api_msg msg;
    int err;
    sys_sem_t sem;
    ip_addr_t sntpServerAddr;
    uint32_t sNum, i, currServer;
    char* ntpServer;
    USE_GLOBAL_BLOCK

    LWIP_ERROR("tcpip_get_ntp_time: invalid addr", (pTimeVal != NULL), return ERR_ARG;);

// #ifdef _WIN64
//     __debugbreak();
// #else
//     __asm int 3
// #endif // _WIN64

    for (sNum = 0; sNum < 3; ++sNum) {
        ntpServer = pGlobalBlock->pCommonBlock->pConfig->ntpServers;
        currServer = pGlobalBlock->pCommonBlock->pConfig->activeNtpServer;

        if (currServer > 0) {
            i = 0;
            for ( ; ; ++ntpServer) {
                if (*ntpServer == '\0') {
                    ++i;
                    if (i == currServer) {
                        ++ntpServer;
                        break;
                    }
                }
            }
        }

        err = pGlobalBlock->pTcpipBlock->fnnetconn_gethostbyname(ntpServer, &sntpServerAddr);
        if (err == ERR_OK) {
            err = pGlobalBlock->pTcpipBlock->fnsys_sem_new(&sem, 0);
            if (err != ERR_OK) {
                return err;
            }

            msg.pIpAddr = &sntpServerAddr;
            msg.pTimeVal = pTimeVal;
            msg.err = &err;
            msg.sem = &sem;

            tcpip_callback(pGlobalBlock->pTcpipBlock->fnsntp_request, &msg);
            err = sys_sem_wait(&sem);
            sys_sem_free(&sem);

            if (err == ERR_OK) {
                break;
            }
        }

        if (err != ERR_OK) {
            // Не получилось разрешить имя NTP-сервера, продуем другой сервер.
            if ((++currServer) >= 3) {
                currServer = 0;
            }
#if DBG
            pGlobalBlock->pCommonBlock->fncommon_disable_wp();
#endif // DBG
            pGlobalBlock->pCommonBlock->pConfig->activeNtpServer = currServer;
#if DBG
            pGlobalBlock->pCommonBlock->fncommon_enable_wp();
#endif // DBG
        }
    }

    return err;
}

#endif // LWIP_SNTP
