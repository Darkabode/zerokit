#include <Windows.h>
#include "libct.h"
#include <stdio.h>
#include <errno.h>

#pragma comment(linker, "/nodefaultlib:libc.lib")
#pragma comment(linker, "/nodefaultlib:libcmt.lib")
#pragma comment(linker, "/nodefaultlib:libcmtd.lib")


static int ErrnoNoMem = ENOMEM;

int * __cdecl _errno()
{
    return &ErrnoNoMem;
}

int __cdecl _tmain(int, TCHAR **, TCHAR **);    // In user's code

#ifdef UNICODE
 void wmainCRTStartup()
#else
 void mainCRTStartup()
#endif
{
    int ret;
    int argc = _init_args();
    _init_atexit();
	_init_file();
    _initterm(__xc_a, __xc_z);

    ret = _tmain(argc, _argv, 0);

	_doexit();
	_term_args();
    ExitProcess(ret);
}
