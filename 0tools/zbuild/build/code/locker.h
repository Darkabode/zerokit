#ifndef __LOCKERDLL_H_
#define __LOCKERDLL_H_

#define _WIN32_WINNT 0x0501
//#undef WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
#include <Windows.h>
#include <gdiplus.h>
#include <windowsx.h>
#include <WS2tcpip.h>
#include <Tlhelp32.h>
#include <stdlib.h>
#include <commctrl.h>
#include <richedit.h>
#include <tchar.h>
#include <assert.h>
#include <crtdbg.h>

#if defined(__cplusplus)
extern "C" {
#endif

    /*
    ** __MACHINE             : all compilers
    ** __MACHINEI            : Intel (32 bit x86) and X64
    ** __MACHINEX64          : X64 compiler only
    ** __MACHINEIA32         : 32 bit x86 arch only
    ** __MACHINEX86X_X64     : X86 Extended intrinsics supported on X64
    ** __MACHINEX86X_IA64    : X86 Extended intrinsics supported on IA64
    ** __MACHINEIA64         : IA64 compiler only
    ** __MACHINEW64          : WIN64(tm), 64 bit compilers only
    ** __MACHINEIW64         : IA32 + Win64 compilers only (__MACHINEI + __MACHINEW64)
    ** __MACHINESA           : ARM (StrongARM) only
    ** __MACHINEARMX         : ARM XSCALE intrinsics
    ** __MACHINECC           : Intel XSCALE Concan
    ** __MACHINECE           : common intrinsic functions for Windows CE
    ** __MACHINEZ            : nothing
    */

#define __MACHINEI            __MACHINE
#define __MACHINEX64          __MACHINE
#define __MACHINEIA32         __MACHINE
#define __MACHINEX86X_X64     __MACHINE
#define __MACHINEX86X_IA64    __MACHINE
#define __MACHINEIA64         __MACHINE
#define __MACHINEW64          __MACHINE
#define __MACHINEIW64         __MACHINE
#define __MACHINESA           __MACHINE
#define __MACHINEARMX         __MACHINE
#define __MACHINECC           __MACHINE
#define __MACHINECE           __MACHINE

    /* No intrinsics available to pure managed code */
#if defined(_M_CEE_PURE)
#define __MACHINE(X)          __MACHINEZ(X)
#else
#define __MACHINE(X)          X;
#endif

#define __MACHINEZ(X)         /* NOTHING */

#if !(_M_IX86)
#undef __MACHINEIA32
#define __MACHINEIA32         __MACHINEZ
#endif

#if !(_M_IA64)
#undef __MACHINEIA64
#define __MACHINEIA64         __MACHINEZ
#endif

#if !(_M_AMD64)
#undef __MACHINEX64
#define __MACHINEX64          __MACHINEZ
#endif

#if !(_M_IA64 || _M_AMD64)
#undef __MACHINEW64
#define __MACHINEW64          __MACHINEZ
#endif

#if !(_M_IX86 || _M_AMD64 || _M_IA64)
#undef __MACHINEIW64
#define __MACHINEIW64         __MACHINEZ
#endif

#if !(_M_IX86 || _M_IA64)
#undef __MACHINEX86X_IA64
#define __MACHINEX86X_IA64    __MACHINEZ
#endif

#if !(_M_IX86 || _M_AMD64)
#undef __MACHINEX86X_X64
#define __MACHINEX86X_X64     __MACHINEZ
#endif

#if !(_M_ARM)
#undef  __MACHINESA
#undef  __MACHINEARMX
#undef  __MACHINECC
#define __MACHINESA           __MACHINEZ
#define __MACHINEARMX         __MACHINEZ
#define __MACHINECC           __MACHINEZ
#endif

#if !defined(_WIN32_WCE)
#undef __MACHINECE
#define __MACHINECE           __MACHINEZ
#endif

LONGLONG __cdecl InterlockedExchange64(__inout  LONGLONG volatile *Target, __in     LONGLONG Value);

__MACHINEI(long _InterlockedExchange(long volatile *, long))
__MACHINEIA64(long _InterlockedExchange(long volatile *, long))
__MACHINESA(long __stdcall _InterlockedExchange(long volatile *, long))
__MACHINEI(long _InterlockedExchangeAdd(long volatile *, long))
__MACHINEIA64(long _InterlockedExchangeAdd(long volatile *, long))
__MACHINEI(long __cdecl _InterlockedIncrement(long volatile *))
__MACHINEIA64(long _InterlockedIncrement(long volatile *))
__MACHINEI(long __cdecl _InterlockedDecrement(long volatile *))
__MACHINEIA64(long _InterlockedDecrement(long volatile *))
__MACHINEI(long _InterlockedCompareExchange (long volatile *, long, long))
__MACHINEIA64(long _InterlockedCompareExchange (long volatile *, long, long))
__MACHINEIA64(__int64 _InterlockedCompareExchange64(__int64 volatile *, __int64, __int64))
__MACHINEI(__int64 _InterlockedCompareExchange64(__int64 volatile *, __int64, __int64))

#if defined(__cplusplus)
}
#endif


// long _InterlockedExchange(long volatile *, long);
// long _InterlockedExchangeAdd(long volatile *, long);
// long __cdecl _InterlockedIncrement(long volatile *);
// long __cdecl _InterlockedDecrement(long volatile *);
// long _InterlockedCompareExchange(long volatile *, long, long);
// __int64 _InterlockedCompareExchange64(__int64 volatile *, __int64, __int64);

#include "..\..\..\..\shared_code\platform.h"
#include "..\..\..\..\shared_code\types.h"
#include "..\..\..\..\shared_code\native.h"

#include "..\..\..\..\loader\userio_api\zuserio\code\zuserio.h"
#include "..\..\..\..\loader\mod_launcher\code\zshellcode.h"
#include "..\..\..\..\loader\mod_shared\userio.h"

#include "..\..\..\shared\debug.h"
#include "..\..\..\shared\utils.h"

#include "..\..\..\shared\vector.h"

extern uint32_t gAffId;
extern uint32_t gSubId;

#endif // __LOCKERDLL_H_
