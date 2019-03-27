#ifndef __LWIP_OPTS_H_
#define __LWIP_OPTS_H_

#define LWIP_PROVIDE_ERRNO
//#define ICMP_DEBUG LWIP_DBG_ON
//#define ETHARP_DEBUG LWIP_DBG_ON
//#define TCPIP_DEBUG LWIP_DBG_ON
//#define RAW_DEBUG LWIP_DBG_ON
//#define TCP_DEBUG	LWIP_DBG_ON
//#define TCP_INPUT_DEBUG                 LWIP_DBG_ON
//#define TCP_OUTPUT_DEBUG                LWIP_DBG_ON
//#define IP_DEBUG LWIP_DBG_ON

//#define LWIP_DEBUG
#define LWIP_PLATFORM_ASSERT(x) FnKdPrint(("Assertion \"%s\" failed at line %d in %s\n", x, __LINE__, __FILE__))//ASSERT

#define LWIP_NOASSERT 1

//PVOID pLwipRamMemory = NULL;

#define LWIP_RAM_HEAP_POINTER 1//pLwipRamMemory

#endif // __LWIP_OPTS_H_