#pragma once

#include <tchar.h>

#ifdef EXPORT_LIBC
    #define LIBC_API_EXTERN __declspec(dllexport)
#else
    #define LIBC_API_EXTERN
#endif // EXPORT_LIBC

extern TCHAR *_argv[];
int _init_args();
void _term_args();

typedef void (__cdecl *_PVFV)();
LIBC_API_EXTERN _PVFV __xc_a[], __xc_z[];    /* C++ initializers */

LIBC_API_EXTERN void _initterm(_PVFV *pfbegin, _PVFV *pfend);
LIBC_API_EXTERN void _init_atexit();
LIBC_API_EXTERN void _doexit();

LIBC_API_EXTERN void _init_file();
