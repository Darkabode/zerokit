#ifndef __COMMON_H_
#define __COMMON_H_



#include "types.h"
#include "zfs_proxy.h"
#include "zshellcode.h"
#include "zfs_wrapper.h"
#include "debug.h"
#include "utils.h"


typedef int (*FnWSAStartup)( IN WORD wVersionRequired, OUT LPWSADATA lpWSAData);
typedef int (*FnWSACleanup)(void);
typedef int (*FnWSAGetLastError)(void);
// typedef SOCKET (*FnWSASocketA)( IN int af, IN int type, IN int protocol, IN LPWSAPROTOCOL_INFOA lpProtocolInfo, IN GROUP g, IN DWORD dwFlags );
typedef SOCKET (*Fnsocket)( IN int af, IN int type, IN int protocol);
// typedef int (*Fnbind)( IN SOCKET s, IN const struct sockaddr FAR *addr, IN int namelen);
typedef struct hostent FAR * (*Fngethostbyname)(__in const char FAR * name);
typedef int (*Fnsetsockopt)( IN SOCKET s, IN int level, IN int optname, IN const char FAR * optval, IN int optlen);
// typedef int (*Fngetsockname)( IN SOCKET s, OUT struct sockaddr FAR *name, IN OUT int FAR * namelen);
// typedef u_short (*Fnntohs)(IN u_short netshort);
typedef u_short (*Fnhtons)(IN u_short hostshort);
typedef int (*Fnselect)( IN int nfds, IN OUT fd_set FAR *readfds, IN OUT fd_set FAR *writefds, IN OUT fd_set FAR *exceptfds, IN const struct timeval FAR *timeout);
// typedef u_long (*Fnhtonl)( IN u_long hostlong);
typedef u_long (*Fnntohl)(IN u_long netlong);
typedef int (*Fnconnect)( IN SOCKET s, IN const struct sockaddr FAR *name, IN int namelen);
// typedef int (*Fnioctlsocket)( IN SOCKET s, IN long cmd, IN OUT u_long FAR *argp);
// typedef int (*Fngetpeername)( IN SOCKET s, OUT struct sockaddr FAR *name, IN OUT int FAR * namelen);
typedef int (*Fnclosesocket)( IN SOCKET s);
typedef int (*Fnshutdown)( IN SOCKET s, IN int how);
// typedef int (*Fngethostname)( __out_bcount_part(namelen, return) char FAR * name, IN int namelen);
typedef int (*Fnsend)( IN SOCKET s, IN const char FAR * buf, IN int len, IN int flags);
typedef int (*Fnrecv)(IN SOCKET s, __out_bcount_part(len, return) __out_data_source(NETWORK) char FAR * buf, IN int len, IN int flags);
// typedef int (*Fnsendto)( IN SOCKET s, IN const char FAR * buf, IN int len, IN int flags, IN const struct sockaddr FAR *to, IN int tolen);
// typedef int (*Fnrecvfrom)( IN SOCKET s, __out_bcount_part(len, return) __out_data_source(NETWORK) char FAR * buf, IN int len, IN int flags, __out_bcount(*fromlen) struct sockaddr FAR * from, IN OUT int FAR * fromlen);
// typedef SOCKET (*Fnaccept)( IN SOCKET s, OUT struct sockaddr FAR *addr, IN OUT int FAR *addrlen);
// typedef int (*Fnlisten)( IN SOCKET s, IN int backlog);
// typedef int (*Fn__WSAFDIsSet)(SOCKET fd, fd_set FAR *);
typedef unsigned long (*Fninet_addr)(__in IN const char FAR * cp);

extern FnWSAStartup fnWSAStartup;
extern FnWSACleanup fnWSACleanup;
extern FnWSAGetLastError fnWSAGetLastError;
// extern FnWSASocketA fnWSASocketA;
extern Fnsocket fnsocket;
// extern Fnbind fnbind;
extern Fngethostbyname fngethostbyname;
extern Fnsetsockopt fnsetsockopt;
// extern Fngetsockname fngetsockname;
// extern Fnntohs fnntohs;
extern Fnhtons fnhtons;
extern Fnselect fnselect;
// extern Fnhtonl fnhtonl;
extern Fnntohl fnntohl;
extern Fnconnect fnconnect;
// extern Fnioctlsocket fnioctlsocket;
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
// extern Fn__WSAFDIsSet fn__WSAFDIsSet;
extern Fninet_addr fninet_addr;

#endif // __COMMON_H_
