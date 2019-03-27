#include "zgui.h"

namespace zgui
{

void* memalloc(size_t sz)
{
    return ::HeapAlloc(::GetProcessHeap()/*gHeap*/, 0, sz);
}

void* memcalloc(size_t sz)
{
    return ::HeapAlloc(::GetProcessHeap()/*gHeap*/, HEAP_ZERO_MEMORY, sz);
}

void* memrealloc(void* pBuffer, size_t newSize)
{
    return ::HeapReAlloc(::GetProcessHeap()/*gHeap*/, 0, pBuffer, newSize);
}

void memfree(void* pBuffer)
{
    ::HeapFree(::GetProcessHeap()/*gHeap*/, 0, pBuffer);
}

}
