#include "platform.h"
#include "types.h"
#include "net.h"

#if defined(_WIN32)

#include <winsock2.h>
#include <windows.h>

#pragma comment( lib, "ws2_32.lib" )

#define read(fd,buf,len)        recv(fd,(char*)buf,(int) len,0)
#define write(fd,buf,len)       send(fd,(char*)buf,(int) len,0)
#define close(fd)               closesocket(fd)

static int wsa_init_done = 0;

#else

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <netdb.h>
#include <errno.h>

#if defined(__FreeBSD__) || defined(__OpenBSD__)
#include <sys/endian.h>
#elif defined(__APPLE__)
#include <machine/endian.h>
#else
#include <endian.h>
#endif

#endif

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

/*
 * htons() is not always available.
 * By default go for LITTLE_ENDIAN variant. Otherwise hope for _BYTE_ORDER and __BIG_ENDIAN
 * to help determine endianess.
 */
#if defined(__BYTE_ORDER) && defined(__BIG_ENDIAN) && __BYTE_ORDER == __BIG_ENDIAN
#define POLARSSL_HTONS(n) (n)
#else
#define POLARSSL_HTONS(n) (((((unsigned short)(n) & 0xFF)) << 8) | (((unsigned short)(n) & 0xFF00) >> 8))
#endif

unsigned short net_htons(unsigned short n);
#define net_htons(n) POLARSSL_HTONS(n)

/*
 * Initiate a TCP connection with host:port
 */
int net_connect( int *fd, const char *host, int port )
{
    struct sockaddr_in server_addr;
    struct hostent *server_host;

#if defined(_WIN32)
    WSADATA wsaData;

    if( wsa_init_done == 0 )
    {
        if( WSAStartup( MAKEWORD(2,0), &wsaData ) == SOCKET_ERROR )
            return( POLARSSL_ERR_NET_SOCKET_FAILED );

        wsa_init_done = 1;
    }
#else
    signal( SIGPIPE, SIG_IGN );
#endif

    if( ( server_host = gethostbyname( host ) ) == NULL )
        return( POLARSSL_ERR_NET_UNKNOWN_HOST );

    if( ( *fd = socket( AF_INET, SOCK_STREAM, IPPROTO_IP ) ) < 0 )
        return( POLARSSL_ERR_NET_SOCKET_FAILED );

    memcpy( (void *) &server_addr.sin_addr,
            (void *) server_host->h_addr,
                     server_host->h_length );

    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = net_htons( port );

    if( connect( *fd, (struct sockaddr *) &server_addr,
                 sizeof( server_addr ) ) < 0 )
    {
        close( *fd );
        return( POLARSSL_ERR_NET_CONNECT_FAILED );
    }

    return( 0 );
}

/*
 * Create a listening socket on bind_ip:port
 */
int net_bind( int *fd, const char *bind_ip, int port )
{
    int n, c[4];
    struct sockaddr_in server_addr;

#if defined(_WIN32)
    WSADATA wsaData;

    if( wsa_init_done == 0 )
    {
        if( WSAStartup( MAKEWORD(2,0), &wsaData ) == SOCKET_ERROR )
            return( POLARSSL_ERR_NET_SOCKET_FAILED );

        wsa_init_done = 1;
    }
#else
    signal( SIGPIPE, SIG_IGN );
#endif

    if( ( *fd = socket( AF_INET, SOCK_STREAM, IPPROTO_IP ) ) < 0 )
        return( POLARSSL_ERR_NET_SOCKET_FAILED );

    n = 1;
    setsockopt( *fd, SOL_SOCKET, SO_REUSEADDR,
                (const char *) &n, sizeof( n ) );

    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = net_htons( port );

    if( bind_ip != NULL )
    {
        memset( c, 0, sizeof( c ) );
        sscanf( bind_ip, "%d.%d.%d.%d", &c[0], &c[1], &c[2], &c[3] );

        for( n = 0; n < 4; n++ )
            if( c[n] < 0 || c[n] > 255 )
                break;

        if( n == 4 )
            server_addr.sin_addr.s_addr =
                ( (unsigned long) c[0] << 24 ) |
                ( (unsigned long) c[1] << 16 ) |
                ( (unsigned long) c[2] <<  8 ) |
                ( (unsigned long) c[3]       );
    }

    if( bind( *fd, (struct sockaddr *) &server_addr,
              sizeof( server_addr ) ) < 0 )
    {
        close( *fd );
        return( POLARSSL_ERR_NET_BIND_FAILED );
    }

    if( listen( *fd, POLARSSL_NET_LISTEN_BACKLOG ) != 0 )
    {
        close( *fd );
        return( POLARSSL_ERR_NET_LISTEN_FAILED );
    }

    return( 0 );
}

/*
 * Check if the current operation is blocking
 */
static int net_is_blocking( void )
{
#if defined(_WIN32)
    return( WSAGetLastError() == WSAEWOULDBLOCK );
#else
    switch( errno )
    {
#if defined EAGAIN
        case EAGAIN:
#endif
#if defined EWOULDBLOCK && EWOULDBLOCK != EAGAIN
        case EWOULDBLOCK:
#endif
            return( 1 );
    }
    return( 0 );
#endif
}

/*
 * Accept a connection from a remote client
 */
int net_accept( int bind_fd, int *client_fd, void *client_ip )
{
    struct sockaddr_in client_addr;

#if defined(__socklen_t_defined) || defined(_SOCKLEN_T)
    socklen_t n = (socklen_t) sizeof( client_addr );
#else
    int n = (int) sizeof( client_addr );
#endif

    *client_fd = accept( bind_fd, (struct sockaddr *)
                         &client_addr, &n );

    if( *client_fd < 0 )
    {
        if( net_is_blocking() != 0 )
            return( POLARSSL_ERR_NET_WANT_READ );

        return( POLARSSL_ERR_NET_ACCEPT_FAILED );
    }

    if( client_ip != NULL )
        memcpy( client_ip, &client_addr.sin_addr.s_addr,
                    sizeof( client_addr.sin_addr.s_addr ) );

    return( 0 );
}

/*
 * Set the socket blocking or non-blocking
 */
int net_set_block( int fd )
{
#if defined(_WIN32)
    u_long n = 0;
    return( ioctlsocket( fd, FIONBIO, &n ) );
#else
    return( fcntl( fd, F_SETFL, fcntl( fd, F_GETFL ) & ~O_NONBLOCK ) );
#endif
}

int net_set_nonblock( int fd )
{
#if defined(_WIN32)
    u_long n = 1;
    return( ioctlsocket( fd, FIONBIO, &n ) );
#else
    return( fcntl( fd, F_SETFL, fcntl( fd, F_GETFL ) | O_NONBLOCK ) );
#endif
}

/*
 * Portable usleep helper
 */
void net_usleep( unsigned long usec )
{
    struct timeval tv;
    tv.tv_sec  = 0;
    tv.tv_usec = usec;
    select( 0, NULL, NULL, NULL, &tv );
}

/*
 * Read at most 'len' characters
 */
int net_recv( void *ctx, uint8_t *buf, size_t len )
{ 
    int ret = read( *((int *) ctx), buf, len );

    if( ret < 0 )
    {
        if( net_is_blocking() != 0 )
            return( POLARSSL_ERR_NET_WANT_READ );

#if defined(_WIN32)
        if( WSAGetLastError() == WSAECONNRESET )
            return( POLARSSL_ERR_NET_CONN_RESET );
#else
        if( errno == EPIPE || errno == ECONNRESET )
            return( POLARSSL_ERR_NET_CONN_RESET );

        if( errno == EINTR )
            return( POLARSSL_ERR_NET_WANT_READ );
#endif

        return( POLARSSL_ERR_NET_RECV_FAILED );
    }

    return( ret );
}

/*
 * Write at most 'len' characters
 */
int net_send( void *ctx, const uint8_t *buf, size_t len )
{
    int ret = write( *((int *) ctx), buf, len );

    if( ret < 0 )
    {
        if( net_is_blocking() != 0 )
            return( POLARSSL_ERR_NET_WANT_WRITE );

#if defined(_WIN32)
        if( WSAGetLastError() == WSAECONNRESET )
            return( POLARSSL_ERR_NET_CONN_RESET );
#else
        if( errno == EPIPE || errno == ECONNRESET )
            return( POLARSSL_ERR_NET_CONN_RESET );

        if( errno == EINTR )
            return( POLARSSL_ERR_NET_WANT_WRITE );
#endif

        return( POLARSSL_ERR_NET_SEND_FAILED );
    }

    return( ret );
}

/*
 * Gracefully close the connection
 */
void net_close( int fd )
{
    shutdown( fd, 2 );
    close( fd );
}
