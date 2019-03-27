#ifndef __PLATFORM_H_
#define __PLATFORM_H_

#define _OS_FREE_BSD      0x0001
#define _OS_LINUX         0x0002
#define _OS_NET_BSD       0x0003
#define _OS_OPEN_BSD      0x0004
#define _OS_SOLARIS       0x0005
#define _OS_QNX           0x0006
#define _OS_UNKNOWN_UNIX  0x00ff
#define _OS_WINDOWS_NT    0x1001

#if defined(__FreeBSD__)
    #define _OS _OS_FREE_BSD
#elif defined(linux) || defined(__linux) || defined(__linux__) || defined(__TOS_LINUX__)
    #define _OS _OS_LINUX
#elif defined(__NetBSD__)
    #define _OS _OS_NET_BSD
#elif defined(__OpenBSD__)
    #define _OS _OS_OPEN_BSD
#elif defined(sun) || defined(__sun)
    #define _OS _OS_SOLARIS
#elif defined(__QNX__)
    #define _OS _OS_QNX
#elif defined(unix) || defined(__unix) || defined(__unix__)
    #define _OS _OS_UNKNOWN_UNIX
#elif defined(_WIN32) || defined(_WIN64)
    #define _OS _OS_WINDOWS_NT
#endif

#endif // __PLATFORM_H_
