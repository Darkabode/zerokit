#ifndef __OVERLORD_H_
#define __OVERLORD_H_

#define _WIN32_WINNT 0x0501
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>
#include <Winsock2.h>
#include <WS2tcpip.h>

typedef struct _CLIENT_ID {
    HANDLE UniqueProcess;
    HANDLE UniqueThread;
} CLIENT_ID;

#include "..\..\..\shared\platform.h"
#include "..\..\..\shared\types.h"
#include "..\..\..\shared\native.h"
#include "../../../shared/debug.h"
#include "../../../shared/utils.h"

#include "..\..\userio_api\zuserio\code\zuserio.h"
#include "..\..\mod_launcher\code\zshellcode.h"
#include "..\..\mod_shared\userio.h"
#include "..\..\mod_shared\overlord_ext.h"


#ifndef _CONSOLE
pfile_data_t zfs_prepare_request_data(pfile_packet_t* ppFilePacket, uint32_t inSize, uint32_t opID);
#endif // _CONSOLE

extern HANDLE gHeap;

typedef int (WSAAPI *FnWSAStartup)( IN WORD wVersionRequired, OUT LPWSADATA lpWSAData);
typedef int (WSAAPI *FnWSACleanup)(void);
typedef int (WSAAPI *FnWSAGetLastError)(void);
// typedef SOCKET (*FnWSASocketA)( IN int af, IN int type, IN int protocol, IN LPWSAPROTOCOL_INFOA lpProtocolInfo, IN GROUP g, IN DWORD dwFlags );
typedef SOCKET (WSAAPI *Fnsocket)( IN int af, IN int type, IN int protocol);
// typedef int (*Fnbind)( IN SOCKET s, IN const struct sockaddr FAR *addr, IN int namelen);
typedef struct hostent FAR * (WSAAPI *Fngethostbyname)(__in const char FAR * name);
typedef INT (WSAAPI *Fngetaddrinfo)(PCSTR pNodeName, PCSTR pServiceName, const ADDRINFOA* pHints, PADDRINFOA* ppResult);
typedef VOID (WSAAPI *Fnfreeaddrinfo)(PADDRINFOA pAddrInfo);
typedef int (WSAAPI FAR *Fnsetsockopt)( IN SOCKET s, IN int level, IN int optname, IN const char FAR * optval, IN int optlen);
typedef int (WSAAPI FAR *Fngetsockopt)(IN SOCKET s, IN int level, IN int optname, char FAR * optval, IN OUT int FAR *optlen);
// typedef int (*Fngetsockname)( IN SOCKET s, OUT struct sockaddr FAR *name, IN OUT int FAR * namelen);
// typedef u_short (*Fnntohs)(IN u_short netshort);
typedef u_short (WSAAPI *Fnhtons)(IN u_short hostshort);
typedef int (WSAAPI *Fnselect)( IN int nfds, IN OUT fd_set FAR *readfds, IN OUT fd_set FAR *writefds, IN OUT fd_set FAR *exceptfds, IN const struct timeval FAR *timeout);
// typedef u_long (*Fnhtonl)( IN u_long hostlong);
typedef u_long (WSAAPI *Fnntohl)(IN u_long netlong);
typedef int (WSAAPI *Fnconnect)( IN SOCKET s, IN const struct sockaddr FAR *name, IN int namelen);
typedef int (WSAAPI *Fnioctlsocket)( IN SOCKET s, IN long cmd, IN OUT u_long FAR *argp);
// typedef int (*Fngetpeername)( IN SOCKET s, OUT struct sockaddr FAR *name, IN OUT int FAR * namelen);
typedef int (WSAAPI *Fnclosesocket)( IN SOCKET s);
typedef int (WSAAPI *Fnshutdown)( IN SOCKET s, IN int how);
// typedef int (*Fngethostname)( __out_bcount_part(namelen, return) char FAR * name, IN int namelen);
typedef int (WSAAPI *Fnsend)( IN SOCKET s, IN const char FAR * buf, IN int len, IN int flags);
typedef int (WSAAPI *Fnrecv)(IN SOCKET s, __out_bcount_part(len, return) __out_data_source(NETWORK) char FAR * buf, IN int len, IN int flags);
// typedef int (*Fnsendto)( IN SOCKET s, IN const char FAR * buf, IN int len, IN int flags, IN const struct sockaddr FAR *to, IN int tolen);
// typedef int (*Fnrecvfrom)( IN SOCKET s, __out_bcount_part(len, return) __out_data_source(NETWORK) char FAR * buf, IN int len, IN int flags, __out_bcount(*fromlen) struct sockaddr FAR * from, IN OUT int FAR * fromlen);
// typedef SOCKET (*Fnaccept)( IN SOCKET s, OUT struct sockaddr FAR *addr, IN OUT int FAR *addrlen);
// typedef int (*Fnlisten)( IN SOCKET s, IN int backlog);
typedef int (PASCAL *Fn__WSAFDIsSet)(SOCKET fd, fd_set FAR *);
typedef unsigned long (WSAAPI *Fninet_addr)(__in IN const char FAR * cp);

extern FnWSAStartup fnWSAStartup;
extern FnWSACleanup fnWSACleanup;
extern FnWSAGetLastError fnWSAGetLastError;
// extern FnWSASocketA fnWSASocketA;
extern Fnsocket fnsocket;
// extern Fnbind fnbind;
extern Fngethostbyname fngethostbyname;
extern Fngetaddrinfo fngetaddrinfo;
extern Fnfreeaddrinfo fnfreeaddrinfo;
extern Fnsetsockopt fnsetsockopt;
extern Fngetsockopt fngetsockopt;
// extern Fngetsockname fngetsockname;
// extern Fnntohs fnntohs;
extern Fnhtons fnhtons;
extern Fnselect fnselect;
// extern Fnhtonl fnhtonl;
extern Fnntohl fnntohl;
extern Fnconnect fnconnect;
extern Fnioctlsocket fnioctlsocket;
// extern Fngetpeername fngetpeername;
extern Fnclosesocket fnclosesocket;
extern Fnshutdown fnshutdown;
// extern Fngethostname fngethostname;
extern Fnsend fnsend;
extern Fnrecv fnrecv;
// extern Fnsendto fnsendto;
// extern Fnrecvfrom fnrecvfrom;
// extern Fnaccept fnaccept;
// extern Fnlisten fnlisten;
extern Fn__WSAFDIsSet fn__WSAFDIsSet;
extern Fninet_addr fninet_addr;

#endif // __OVERLORD_H_
