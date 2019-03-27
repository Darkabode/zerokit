#ifndef __SHARED_NATIVE_H_
#define __SHARED_NATIVE_H_

#ifndef SYS_ALLOCATOR
#define SYS_ALLOCATOR(sz) malloc(sz)
#endif // SYS_ALLOCATOR

#ifndef SYS_DEALLOCATOR
#define SYS_DEALLOCATOR(ptr) free(ptr)
#endif // SYS_DEALLOCATOR

#if defined(_MSC_VER)

#ifndef MEMCPY
#define MEMCPY(dest, src, size) __movsb((unsigned char*)dest, (unsigned char const*)src, (size_t)size)
#endif // MEMCPY

#ifndef MEMSET
#define MEMSET(dest, val, size) __stosb((unsigned char*)dest, (unsigned char)val, (size_t)size);
#endif // MEMSET

#ifndef MEMCMP
#define MEMCMP(dest, src, size) memcmp(dest, src, size) == 0
#endif // MEMCMP

#ifndef STRLEN
#define STRLEN strlen
#endif // STRLEN

#ifndef STRCMP
#define STRCMP strcmp
#endif // STRCMP

#else

#ifndef MEMCPY
#define MEMCPY(dest, src, size) memcpy(dest, src, size)
#endif // MEMCPY

#ifndef MEMSET
#define MEMSET(dest, val, size) memset(dest, val, size);
#endif // MEMSET

#ifndef MEMCMP
#define MEMCMP(dest, src, size) memcmp(dest, src, size) == 0
#endif // MEMCMP

#endif // _MSC_VER

#ifndef STRCPY
#define STRCPY strcpy
#endif // STRCPY

#ifndef FN_STRICMP
#define FN_STRICMP _stricmp
#endif // FN_STRICMP

#ifndef FN_STRCPY_S
#define FN_STRCPY_S strcpy_s
#endif // FN_STRCPY_S

// #ifndef FN_ZEROMEM
// #define FN_ZEROMEM __stosb
// #endif // FN_ZEROMEM

#endif // __SHARED_NATIVE_H_
