#ifndef __ZGUI_MEMORY_H_
#define __ZGUI_MEMORY_H_

namespace zgui
{

void* memalloc(size_t sz);
void* memcalloc(size_t sz);
void* memrealloc(void* pBuffer, size_t newSize);
void memfree(void* pBuffer);

}

#endif // __ZGUI_MEMORY_H_
