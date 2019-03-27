#include <malloc.h>
#include "libct.h"

__declspec(restrict, noalias) void *malloc(size_t size)
{
    return HeapAlloc(GetProcessHeap(), 0, size);
}

__declspec(noalias) void free(void *p)
{
    HeapFree(GetProcessHeap(), 0, p);
}

__declspec(restrict, noalias) void *realloc(void *p, size_t size)
{
    if (p)
        return HeapReAlloc(GetProcessHeap(), 0, p, size);
    else
        return HeapAlloc(GetProcessHeap(), 0, size);
}

__declspec(restrict, noalias) void *calloc(size_t nitems, size_t size)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, nitems * size);
}

void *_calloc_crt(size_t nitems, size_t size)
{
	return calloc(nitems, size);
}

void *_nh_malloc(size_t size, int nhFlag)
{
	nhFlag;
    return HeapAlloc(GetProcessHeap(), 0, size);
}

size_t _msize(void *p)
{
    return HeapSize(GetProcessHeap(), 0, p);
}
