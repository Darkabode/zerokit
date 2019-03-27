#include <Windows.h>
#include <malloc.h>
#include "libct.h"

extern HANDLE gHeap;

#undef malloc
#define malloc(sz) HeapAlloc(gHeap, 0, sz)

#undef free
#define free(ptr) HeapFree(gHeap, 0, ptr)


#pragma section(".CRT$XCA", read, write)
#pragma data_seg(".CRT$XCA")		// start of ctor section
_PVFV __xc_a[] = {0};

#pragma section(".CRT$XCZ", read, write)
#pragma data_seg(".CRT$XCZ")		// end of ctor section
_PVFV __xc_z[] = {0};

#pragma data_seg()
#pragma comment(linker, "/merge:.CRT=.rdata")

void _initterm(_PVFV *pfbegin, _PVFV *pfend)
{
	// walk the table of function pointers from the bottom up, until
	// the end is encountered.  Do not skip the first entry.  The initial
	// value of pfbegin points to the first valid entry.  Do not try to
	// execute what pfend points to.  Only entries before pfend are valid.
	while (pfbegin < pfend)
	{
		if (*pfbegin != 0)
			(**pfbegin)();
		++pfbegin;
	}
}


static _PVFV *atexitlist = 0;
static unsigned int max_atexit_entries = 0;
static unsigned int cur_atexit_entries = 0;

void _init_atexit()
{
	max_atexit_entries = 128;
	atexitlist = (_PVFV*)VirtualAlloc(NULL, max_atexit_entries * sizeof(_PVFV*), MEM_COMMIT, PAGE_READWRITE);
    __stosb(atexitlist, 0, max_atexit_entries * sizeof(_PVFV*));
}

int __cdecl atexit(_PVFV func)
{
	if (cur_atexit_entries < max_atexit_entries)
	{
		atexitlist[cur_atexit_entries++] = func; 
		return 0;
	}

	return -1;
}

void _doexit()
{
	if (cur_atexit_entries)
	{
		// Use ptr math to find the end of the array
		_initterm(atexitlist, atexitlist + cur_atexit_entries);
	}
}

void __cdecl exit(int code)
{
	_doexit();
	ExitProcess(code);
}

int _purecall()
{
	ExitProcess((UINT)-1);
}
