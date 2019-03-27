#include "overlord.h"

#include "netcomm.h"

int wsaInited = FALSE;
WSADATA gWsaData;

const char google_req[] = "GET / HTTP/1.1\r\nHost: www.update.microsoft.com\r\nConnection: close\r\n\r\n";
#define BUFF_SIZE 4096
char gBuffer[BUFF_SIZE];

int netcomm_check()
{
    int ret;
    uint8_t* data = NULL;
    uint32_t dataSize;

    ret = netcomm_make_transaction("www.update.microsoft.com", 80, google_req, sizeof(google_req) - 1, &data, &dataSize);

// #ifdef _WIN64
//     __debugbreak();
// #else
//     __asm int 3
// #endif // _WIN64
    if (data != NULL) {
        DBG_PRINTF(("netcomm_check() received %u bytes\n", dataSize));
        VirtualFree(data, dataSize, MEM_RELEASE);
    }

    DBG_PRINTF(("netcomm_check() returned %d\n", ret));

    return ret;
}


int netcomm_reset_socket_options(const SOCKET handle)
{
    const int sndBufSize = 65536;
    const int rcvBufSize = 65536;
    const int one = 1;

    return handle > 0
        && fnsetsockopt(handle, SOL_SOCKET, SO_RCVBUF, (const char*) &rcvBufSize, sizeof (rcvBufSize)) == 0
        && fnsetsockopt(handle, SOL_SOCKET, SO_SNDBUF, (const char*) &sndBufSize, sizeof (sndBufSize)) == 0
        && (fnsetsockopt(handle, IPPROTO_TCP, TCP_NODELAY, (const char*) &one, sizeof (one)) == 0);
}
// 
// int netcomm_wait_for_readiness(const SOCKET handle, const int forReading, const int timeoutMsecs)
// {
//     struct timeval timeout;
//     struct timeval* timeoutp;
//     fd_set rset, wset;
//     fd_set* prset;
//     fd_set* pwset;
// 
//     if (timeoutMsecs >= 0) {
//         timeout.tv_sec = timeoutMsecs / 1000;
//         timeout.tv_usec = (timeoutMsecs % 1000) * 1000;
//         timeoutp = &timeout;
//     }
//     else {
//         timeoutp = 0;
//     }
// 
//     FD_ZERO (&rset);
//     FD_SET (handle, &rset);
//     FD_ZERO (&wset);
//     FD_SET (handle, &wset);
// 
//     prset = forReading ? &rset : NULL;
//     pwset = forReading ? NULL : &wset;
// 
//     DBG_PRINTF(("netcomm_wait_for_readiness() prepared data\n"));
//     if (fnselect((int)handle + 1, prset, pwset, 0, timeoutp) < 0) {
//         DBG_PRINTF(("select() failed (error: %d)\n", fnWSAGetLastError()));
//         return -1;
//     }
// 
//     DBG_PRINTF(("select() OK\n"));
// 
//     {
//         int opt;
//         int len = sizeof (opt);
// 
//         if (fngetsockopt(handle, SOL_SOCKET, SO_ERROR, (char*) &opt, &len) < 0 || opt != 0) {
//             DBG_PRINTF(("getsockopt() failed (error: %d)\n", fnWSAGetLastError()));
//             return -1;
//         }
//     }
// 
//     return fn__WSAFDIsSet(handle, forReading ? &rset : &wset) ? 1 : 0;
// }

int netcomm_make_transaction(const char* server, uint16_t port, const char* httpRequest, uint32_t httpRequestLen, uint8_t** pData, uint32_t* pDataSize)
{
    SOCKET sock = INVALID_SOCKET;
    uint32_t dataSize = 0;
    //struct hostent* srvEnt;
    //struct sockaddr_in srvAddr;
    uint8_t* data = NULL;
    int ret = 0, result;
    uint8_t* newBuff;
    int recvdSize = -1;
    struct addrinfo hints;
    struct addrinfo* info = NULL;
    char servname[32];
    u_long nonBlocking;
    int timeOutMillisecs = 7000;

    if (!wsaInited) {
        wsaInited = TRUE;
        if (fnWSAStartup(MAKEWORD(1, 1), &gWsaData) != 0) {
            DBG_PRINTF(("WSAStartup() failed (error: %d)\n", fnWSAGetLastError()));
            return 0;
        }
    }

    do {
        __stosb((uint8_t*)&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_NUMERICSERV;

        wsprintf(servname, "%u", port);
        DBG_PRINTF(("connecting to %s:%s OK\n", server, servname));

        if (fngetaddrinfo(server, servname, &hints, &info) != 0 || info == NULL) {
            DBG_PRINTF(("getaddrinfo() failed (error: %d)\n", fnWSAGetLastError()));
            break;
        }

        DBG_PRINTF(("getaddrinfo() OK\n"));

        if ((sock = (int)fnsocket(info->ai_family, info->ai_socktype, 0)) < 0) {
            DBG_PRINTF(("socket() failed (error: %d)\n", fnWSAGetLastError()));
            break;
        }

        DBG_PRINTF(("socket() OK\n"));

//         nonBlocking = 1;
//         fnioctlsocket(sock, FIONBIO, &nonBlocking);
// 
//         DBG_PRINTF(("ioctlsocket() OK\n"));

        result = fnconnect(sock, info->ai_addr, (int)info->ai_addrlen);
        DBG_PRINTF(("connect() OK\n"));

        if (result < 0) {
//             if (result == SOCKET_ERROR && fnWSAGetLastError() == WSAEWOULDBLOCK) {
//                 if (netcomm_wait_for_readiness(sock, 0, timeOutMillisecs) != 1) {
//                     nonBlocking = 0;
//                     fnioctlsocket(sock, FIONBIO, &nonBlocking);
                    break;
//                }
//             }
        }

//         nonBlocking = 0;
//         fnioctlsocket(sock, FIONBIO, &nonBlocking);
// 
//         DBG_PRINTF(("ioctlsocket() OK\n"));

        netcomm_reset_socket_options(sock);
        DBG_PRINTF(("netcomm_reset_socket_options() OK\n"));

        if (fnsend(sock, httpRequest, httpRequestLen, 0) < (int)httpRequestLen) {
            DBG_PRINTF(("send() failed (error: %d)\n", fnWSAGetLastError()));
            break;
        }

        DBG_PRINTF(("send() OK\n"));
        
        while ((recvdSize = fnrecv(sock, gBuffer, BUFF_SIZE, 0)) > 0) {
            DBG_PRINTF(("successfully received %d bytes!\n", recvdSize));
            newBuff = VirtualAlloc(NULL, dataSize + recvdSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
            if (dataSize > 0) {
                __movsb(newBuff, data, dataSize);
                VirtualFree(data, dataSize, MEM_RELEASE);
            }
            data = newBuff;
            __movsb(data + dataSize, gBuffer, recvdSize);
            dataSize += recvdSize;
        }

        if (recvdSize >= 0) { // all right!
            DBG_PRINTF(("Total size of received data from %s = %u bytes!\n", server, dataSize));

            *pData = data;
            *pDataSize = dataSize;
            ret = 1;
        }
        else {
            DBG_PRINTF(("recv() failed (error: %d)\n", fnWSAGetLastError()));
            if (data != NULL) {
                VirtualFree(data, dataSize, MEM_RELEASE);                        
            }
        }
    } while (0);

    if (info != NULL) {
        fnfreeaddrinfo(info);
    }

    if (sock != INVALID_SOCKET) {
        fnclosesocket(sock);
    }

// 
//     if ((srvEnt = fngethostbyname(server)) != NULL) {
//         DBG_PRINTF(("%s resolved!\n", server));
//         if ((sock = fnsocket(srvEnt->h_addrtype, SOCK_STREAM, IPPROTO_TCP)) != INVALID_SOCKET) {
//             DBG_PRINTF(("socket successfully initialized!\n"));
//             srvAddr.sin_family = srvEnt->h_addrtype;
//             srvAddr.sin_addr.S_un.S_addr = *(uint32_t*)srvEnt->h_addr_list[0];
//             srvAddr.sin_port = fnhtons(port);
//             DBG_PRINTF(("socket successfully configured!\n"));
//             if (fnconnect(sock, (struct sockaddr*)&srvAddr, sizeof(srvAddr)) == 0) {
//                 DBG_PRINTF(("successfully connected to %s!\n", server));
//                 if (fnsend(sock, httpRequest, httpRequestLen, 0) >= (int)httpRequestLen) {
//                     fd_set fds;
//                     struct timeval timeout;
//                     timeout.tv_sec = 7;
//                     timeout.tv_usec = 0;
// 
//                     DBG_PRINTF(("successfully sended %u bytes\n", httpRequestLen));
// 
//                     FD_ZERO(&fds);
//                     FD_SET(sock, &fds);
// 
//                     DBG_PRINTF(("before fnselect\n"));
//                     while (fnselect(0, &fds, NULL, NULL, &timeout) >= 1) {
//                         DBG_PRINTF(("after sfnselect\n"));
//                         recvdSize = fnrecv(sock, gBuffer, BUFF_SIZE, 0);
//                         if (recvdSize <= 0) {
//                             break;
//                         }
//                         DBG_PRINTF(("successfully received %d bytes!\n", recvdSize));
//                         newBuff = VirtualAlloc(NULL, dataSize + recvdSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
//                         if (dataSize > 0) {
//                             __movsb(newBuff, data, dataSize);
//                             VirtualFree(data, dataSize, MEM_RELEASE);
//                         }
//                         data = newBuff;
//                         __movsb(data + dataSize, gBuffer, recvdSize);
//                         dataSize += recvdSize;
// 
//                         FD_ZERO(&fds);
//                         FD_SET(sock, &fds);
//                     }
// 
//                     if (recvdSize != -1) {// all right!
//                         DBG_PRINTF(("Total size of received data from %s = %u bytes!\n", server, dataSize));
// 
//                         *pData = data;
//                         *pDataSize = dataSize;
//                         ret = 1;
//                     }
//                     else {
//                         DBG_PRINTF(("recv() failed (error: %d)\n", fnWSAGetLastError()));
//                         if (data != NULL) {
//                             VirtualFree(data, dataSize, MEM_RELEASE);                        
//                         }
//                     }
//                 }
//                 else {
//                     DBG_PRINTF(("send() failed (error: %d)\n", fnWSAGetLastError()));
//                 }
//             }
//             else {
//                 DBG_PRINTF(("connect() failed (error: %d)\n", fnWSAGetLastError()));
//             }
//         }
//         else {
//             DBG_PRINTF(("socket() failed (error: %d)\n", fnWSAGetLastError()));
//         }
//     }
//     else {
//         DBG_PRINTF(("Cannot resolve IP for %s (error: %d)\n", server, fnWSAGetLastError()));
//     }
// 
//     if (sock != INVALID_SOCKET) {
//         fnclosesocket(sock);
//     }
    if (ret == 0) {
        fnWSACleanup();
        wsaInited = FALSE;
    }

    return ret;
}


#define SNTP_MSG_LEN                48
#define SNTP_PORT 123

#define SNTP_LI_NO_WARNING          0x00

#define SNTP_VERSION                (4/* NTP Version 4*/<<3) 

#define SNTP_MODE_MASK              0x07
#define SNTP_MODE_CLIENT            0x03
#define SNTP_MODE_SERVER            0x04
#define SNTP_MODE_BROADCAST         0x05

#define SNTP_STRATUM_KOD            0x00

/* number of seconds between 1900 and 1970 */
#define DIFF_SEC_1900_1970         (2208988800)

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

int netcomm_get_ntp_time(uint32_t* pNtpTime)
{
    SOCKET sock = INVALID_SOCKET;
    sntp_msg_t sntpmsg;
    struct hostent* ntpEnt = NULL;
    struct sockaddr_in ntpAddr;
    int ret = 0;

    if ((ntpEnt = fngethostbyname("time.windows.com")) != NULL) {
        __stosb((uint8_t*)&sntpmsg, 0, SNTP_MSG_LEN);
        sntpmsg.li_vn_mode = SNTP_LI_NO_WARNING | SNTP_VERSION | SNTP_MODE_CLIENT;

        if ((sock = fnsocket(ntpEnt->h_addrtype, SOCK_DGRAM, 0)) != -1) {
            ntpAddr.sin_family = ntpEnt->h_addrtype;
            __movsb((uint8_t*)&ntpAddr.sin_addr, (uint8_t*)ntpEnt->h_addr, ntpEnt->h_length);
            ntpAddr.sin_port = fnhtons(SNTP_PORT);
            if (fnconnect(sock, (struct sockaddr*)&ntpAddr, sizeof(ntpAddr)) == 0) {
                if (fnsend(sock, (const char*)&sntpmsg, sizeof(sntpmsg), 0) >= sizeof(sntpmsg)) {
                    fd_set fds;
                    struct timeval timeout;
                    timeout.tv_sec = 7;
                    timeout.tv_usec = 0;
                    
                    FD_ZERO(&fds);
                    FD_SET(sock, &fds);

                    if (fnselect(0, &fds, NULL, NULL, &timeout) >= 1) {
                        if (fnrecv(sock, gBuffer, BUFF_SIZE, 0) == SNTP_MSG_LEN) {
                            sntp_msg_t* pSntpResponse = (sntp_msg_t*)gBuffer;
                            uint8_t mode = pSntpResponse->li_vn_mode;
                            mode &= SNTP_MODE_MASK;
                            if ((mode == SNTP_MODE_SERVER) || (mode == SNTP_MODE_BROADCAST)) {
                                if (pSntpResponse->stratum != SNTP_STRATUM_KOD) {
                                    *pNtpTime = fnntohl(pSntpResponse->receive_timestamp[0]) - DIFF_SEC_1900_1970;
                                    ret = 1;
                                }
                            }
                        }
                    }
                }
                else {
                    DBG_PRINTF(("send() failed (error: %d)\n", fnWSAGetLastError()));
                }
            }
            else {
                DBG_PRINTF(("connect() failed (error: %d)\n", fnWSAGetLastError()));
            }
        }
        else {
            DBG_PRINTF(("socket() failed (error: %d)\n", fnWSAGetLastError()));
        }
    }
    else {
        DBG_PRINTF(("Cannot resolve IP for time.windows.com (error: %d)\n", fnWSAGetLastError()));
    }

    if (sock != INVALID_SOCKET) {
        fnclosesocket(sock);
    }

    return ret;
}

int netcomm_gethostbyname(const char* server, uint32_t* pAddr)
{
    struct hostent* servEnt = NULL;

    if ((servEnt = fngethostbyname(server)) != NULL) {
        __movsb((uint8_t*)pAddr, (uint8_t*)servEnt->h_addr, servEnt->h_length);
        return 1;
    }
    else {
        DBG_PRINTF(("Cannot resolve IP for %s (error: %d)\n", server, fnWSAGetLastError()));
    }

    return 0;
}
