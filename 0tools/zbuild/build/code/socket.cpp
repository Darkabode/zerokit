#ifndef AI_NUMERICSERV  // (missing in older Mac SDKs)
    #define AI_NUMERICSERV 0x1000
#endif

namespace SocketHelpers
{
    typedef int (WSAAPI *FnWSAStartup)( IN WORD wVersionRequired, OUT LPWSADATA lpWSAData);
    typedef int (WSAAPI *FnWSACleanup)(void);
    typedef int (WSAAPI *FnWSAGetLastError)(void);
    // typedef SOCKET (*FnWSASocketA)( IN int af, IN int type, IN int protocol, IN LPWSAPROTOCOL_INFOA lpProtocolInfo, IN GROUP g, IN DWORD dwFlags );
    typedef SOCKET (WSAAPI *Fnsocket)( IN int af, IN int type, IN int protocol);
    // typedef int (*Fnbind)( IN SOCKET s, IN const struct sockaddr FAR *addr, IN int namelen);
    typedef struct hostent FAR* (WSAAPI *Fngethostbyname)(__in const char FAR * name);
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


    static FnWSAStartup fnWSAStartup;
    static FnWSACleanup fnWSACleanup;
    static FnWSAGetLastError fnWSAGetLastError;
    // FnWSASocketA fnWSASocketA;
    static Fnsocket fnsocket;
    // Fnbind fnbind;
    static Fngethostbyname fngethostbyname;
    static Fngetaddrinfo fngetaddrinfo;
    static Fnfreeaddrinfo fnfreeaddrinfo;
    static Fnsetsockopt fnsetsockopt;
    static Fngetsockopt fngetsockopt;
    // Fngetsockname fngetsockname;
    // Fnntohs fnntohs;
    static Fnhtons fnhtons;
    static Fnselect fnselect;
    // Fnhtonl fnhtonl;
    static Fnntohl fnntohl;
    static Fnconnect fnconnect;
    static Fnioctlsocket fnioctlsocket;
    // Fngetpeername fngetpeername;
    static Fnclosesocket fnclosesocket;
    static Fnshutdown fnshutdown;
    // Fngethostname fngethostname;
    static Fnsend fnsend;
    static Fnrecv fnrecv;
    // Fnsendto fnsendto;
    // Fnrecvfrom fnrecvfrom;
    // Fnaccept fnaccept;
    // Fnlisten fnlisten;
    static Fn__WSAFDIsSet fn__WSAFDIsSet;
    static Fninet_addr fninet_addr;

    static bool resetSocketOptions(const int handle, const bool isDatagram, const bool allowBroadcast) throw()
    {
        const int sndBufSize = 65536;
        const int rcvBufSize = 65536;
        const int one = 1;

        return handle > 0
                && fnsetsockopt (handle, SOL_SOCKET, SO_RCVBUF, (const char*) &rcvBufSize, sizeof (rcvBufSize)) == 0
                && fnsetsockopt(handle, SOL_SOCKET, SO_SNDBUF, (const char*) &sndBufSize, sizeof (sndBufSize)) == 0
                && (isDatagram ? ((! allowBroadcast) || fnsetsockopt (handle, SOL_SOCKET, SO_BROADCAST, (const char*) &one, sizeof (one)) == 0)
                               : (fnsetsockopt(handle, IPPROTO_TCP, TCP_NODELAY, (const char*) &one, sizeof (one)) == 0));
    }

    static int readSocket (const int handle,
                           void* const destBuffer, const int maxBytesToRead,
                           bool volatile& connected,
                           const bool blockUntilSpecifiedAmountHasArrived) throw()
    {
        int bytesRead = 0;

        while (bytesRead < maxBytesToRead)
        {
            int bytesThisTime;

            bytesThisTime = fnrecv(handle, static_cast<char*> (destBuffer) + bytesRead, maxBytesToRead - bytesRead, 0);

            if (bytesThisTime <= 0 || ! connected)
            {
                if (bytesRead == 0)
                    bytesRead = -1;

                break;
            }

            bytesRead += bytesThisTime;

            if (! blockUntilSpecifiedAmountHasArrived)
                break;
        }

        return bytesRead;
    }

    static int waitForReadiness(const int handle, const bool forReading, const int timeoutMsecs) throw()
    {
        struct timeval timeout;
        struct timeval* timeoutp;

        if (timeoutMsecs >= 0) {
            timeout.tv_sec = timeoutMsecs / 1000;
            timeout.tv_usec = (timeoutMsecs % 1000) * 1000;
            timeoutp = &timeout;
        }
        else {
            timeoutp = 0;
        }

        fd_set rset, wset;
        FD_ZERO (&rset);
        FD_SET (handle, &rset);
        FD_ZERO (&wset);
        FD_SET (handle, &wset);

        fd_set* const prset = forReading ? &rset : 0;
        fd_set* const pwset = forReading ? 0 : &wset;

        if (fnselect(handle + 1, prset, pwset, 0, timeoutp) < 0) {
            return -1;
        }

        {
            int opt;
            int len = sizeof (opt);

            if (SocketHelpers::fngetsockopt(handle, SOL_SOCKET, SO_ERROR, (char*) &opt, &len) < 0 || opt != 0) {
                return -1;
            }
        }

        return fn__WSAFDIsSet(handle, forReading ? &rset : &wset) ? 1 : 0;
    }

    static bool setSocketBlockingState (const int handle, const bool shouldBlock) throw()
    {
        u_long nonBlocking = shouldBlock ? 0 : (u_long) 1;
        return fnioctlsocket(handle, FIONBIO, &nonBlocking) == 0;
    }

    static bool connectSocket(int volatile& handle, const zgui::String& hostName, const int portNumber, const int timeOutMillisecs) throw()
    {
        struct addrinfo hints = { 0 };
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_NUMERICSERV;

        struct addrinfo* info = 0;
        if (fngetaddrinfo(hostName.toUTF8(), zgui::String (portNumber).toUTF8(), &hints, &info) != 0 || info == 0) {
            return false;
        }

        if (handle < 0) {
            handle = (int) fnsocket(info->ai_family, info->ai_socktype, 0);
        }

        if (handle < 0) {
            fnfreeaddrinfo (info);
            return false;
        }

        setSocketBlockingState (handle, false);
        const int result = fnconnect (handle, info->ai_addr, (int) info->ai_addrlen);
        fnfreeaddrinfo (info);

        if (result < 0) {
            if (result == SOCKET_ERROR && fnWSAGetLastError() == WSAEWOULDBLOCK) {
                if (waitForReadiness(handle, false, timeOutMillisecs) != 1) {
                    setSocketBlockingState (handle, true);
                    return false;
                }
            }
        }

        setSocketBlockingState (handle, true);
        resetSocketOptions (handle, false, false);

        return true;
    }
}

StreamingSocket::StreamingSocket() :
portNumber(0),
handle(-1),
connected(false)
{
}

StreamingSocket::StreamingSocket (const zgui::String& hostName_, const int portNumber_, const int handle_) :
hostName(hostName_),
portNumber(portNumber_),
handle(handle_),
connected(true)
{
    SocketHelpers::resetSocketOptions(handle_, false, false);
}

StreamingSocket::~StreamingSocket()
{
    close();
}

bool StreamingSocket::socketsStarted = false;

bool StreamingSocket::initSockets()
{
    bool ret = true;
    if (!socketsStarted) {
        socketsStarted = true;

        HMODULE hSockLib = LoadLibraryA("ws2_32.dll");
        if (hSockLib != NULL) {
            SocketHelpers::fnWSAStartup = (SocketHelpers::FnWSAStartup)GetProcAddress(hSockLib, "WSAStartup");
            SocketHelpers::fnWSACleanup = (SocketHelpers::FnWSACleanup)GetProcAddress(hSockLib, "WSACleanup");
            SocketHelpers::fnWSAGetLastError = (SocketHelpers::FnWSAGetLastError)GetProcAddress(hSockLib, "WSAGetLastError");
            //     fnWSASocketA = (FnWSASocketA)GetProcAddress(hSockLib, "WSASocketA");
            SocketHelpers::fnsocket = (SocketHelpers::Fnsocket)GetProcAddress(hSockLib, "socket");
            //     fnbind = (Fnbind)GetProcAddress(hSockLib, "bind");
            SocketHelpers::fngethostbyname = (SocketHelpers::Fngethostbyname)GetProcAddress(hSockLib, "gethostbyname");
            SocketHelpers::fngetaddrinfo = (SocketHelpers::Fngetaddrinfo)GetProcAddress(hSockLib, "getaddrinfo");
            SocketHelpers::fnfreeaddrinfo = (SocketHelpers::Fnfreeaddrinfo)GetProcAddress(hSockLib, "freeaddrinfo");
            SocketHelpers::fnsetsockopt = (SocketHelpers::Fnsetsockopt)GetProcAddress(hSockLib, "setsockopt");
            SocketHelpers::fngetsockopt = (SocketHelpers::Fngetsockopt)GetProcAddress(hSockLib, "getsockopt");
            //     fngetsockname = (Fngetsockname)GetProcAddress(hSockLib, "getsockname");
            //     fnntohs = (Fnntohs)GetProcAddress(hSockLib, "ntohs");
            SocketHelpers::fnhtons = (SocketHelpers::Fnhtons)GetProcAddress(hSockLib, "htons");
            SocketHelpers::fnselect = (SocketHelpers::Fnselect)GetProcAddress(hSockLib, "select");
            //     fnhtonl = (Fnhtonl)GetProcAddress(hSockLib, "htonl");
            SocketHelpers::fnntohl = (SocketHelpers::Fnntohl)GetProcAddress(hSockLib, "ntohl");
            SocketHelpers::fnconnect = (SocketHelpers::Fnconnect)GetProcAddress(hSockLib, "connect");
            SocketHelpers::fnioctlsocket = (SocketHelpers::Fnioctlsocket)GetProcAddress(hSockLib, "ioctlsocket");
            //     fngetpeername = (Fngetpeername)GetProcAddress(hSockLib, "getpeername");
            SocketHelpers::fnclosesocket = (SocketHelpers::Fnclosesocket)GetProcAddress(hSockLib, "closesocket");
            SocketHelpers::fnshutdown = (SocketHelpers::Fnshutdown)GetProcAddress(hSockLib, "shutdown");
            //     fngethostname = (Fngethostname)GetProcAddress(hSockLib, "gethostname");
            SocketHelpers::fnsend = (SocketHelpers::Fnsend)GetProcAddress(hSockLib, "send");
            SocketHelpers::fnrecv = (SocketHelpers::Fnrecv)GetProcAddress(hSockLib, "recv");
            //     fnsendto = (Fnsendto)GetProcAddress(hSockLib, "sendto");
            //     fnrecvfrom = (Fnrecvfrom)GetProcAddress(hSockLib, "recvfrom");
            SocketHelpers::fn__WSAFDIsSet = (SocketHelpers::Fn__WSAFDIsSet)GetProcAddress(hSockLib, "__WSAFDIsSet");
            SocketHelpers::fninet_addr = (SocketHelpers::Fninet_addr)GetProcAddress(hSockLib, "inet_addr");

            WSADATA wsaData;
            const WORD wVersionRequested = MAKEWORD (1, 1);

            ret = (SocketHelpers::fnWSAStartup(wVersionRequested, &wsaData) == 0);
        }
    }
    return ret;
}

void StreamingSocket::doneSockets()
{
    if (socketsStarted) {
        SocketHelpers::fnWSACleanup();
    }
}

int StreamingSocket::read(void* destBuffer, const int maxBytesToRead, const bool blockUntilSpecifiedAmountHasArrived)
{
    return connected ? SocketHelpers::readSocket(handle, destBuffer, maxBytesToRead, connected, blockUntilSpecifiedAmountHasArrived) : -1;
}

int StreamingSocket::write(const void* sourceBuffer, const int numBytesToWrite)
{
    if (!connected) {
        return -1;
    }

    return SocketHelpers::fnsend(handle, (const char*) sourceBuffer, numBytesToWrite, 0);
}

int StreamingSocket::waitUntilReady (const bool readyForReading, const int timeoutMsecs) const
{
    return connected ? SocketHelpers::waitForReadiness (handle, readyForReading, timeoutMsecs) : -1;
}

bool StreamingSocket::connect(const zgui::String& remoteHostName, const int remotePortNumber, const int timeOutMillisecs)
{
    if (connected) {
        close();
    }

    hostName = remoteHostName;
    portNumber = remotePortNumber;

    connected = SocketHelpers::connectSocket(handle, remoteHostName, remotePortNumber, timeOutMillisecs);

    if (! (connected && SocketHelpers::resetSocketOptions (handle, false, false))) {
        close();
        return false;
    }

    return true;
}

void StreamingSocket::close()
{
    if (handle != SOCKET_ERROR || connected) {
        SocketHelpers::fnclosesocket(handle);
    }

    connected = false;

    hostName = zgui::String::empty;
    portNumber = 0;
    handle = -1;
}

#pragma warning (pop)
