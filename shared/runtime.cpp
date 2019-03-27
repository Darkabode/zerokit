//extern HANDLE gHeap;

typedef void (__cdecl *_PVFV)(void);
typedef int  (__cdecl *_PIFV)(void);
//typedef void (__cdecl *_PVFI)(int);

#pragma comment(linker, "/merge:.CRT=.data")

#pragma section(".CRTMP$XCA",long,read)
#pragma section(".CRTMP$XCZ",long,read)
#pragma section(".CRTMP$XIA",long,read)
#pragma section(".CRTMP$XIZ",long,read)

#pragma section(".CRTMA$XCA",long,read)
#pragma section(".CRTMA$XCZ",long,read)
#pragma section(".CRTMA$XIA",long,read)
#pragma section(".CRTMA$XIZ",long,read)

#pragma section(".CRTVT$XCA",long,read)
#pragma section(".CRTVT$XCZ",long,read)

#pragma section(".CRT$XCA",long,read)
#pragma section(".CRT$XCAA",long,read)
#pragma section(".CRT$XCC",long,read)
#pragma section(".CRT$XCZ",long,read)
#pragma section(".CRT$XDA",long,read)
#pragma section(".CRT$XDC",long,read)
#pragma section(".CRT$XDZ",long,read)
#pragma section(".CRT$XIA",long,read)
#pragma section(".CRT$XIAA",long,read)
#pragma section(".CRT$XIC",long,read)
#pragma section(".CRT$XID",long,read)
#pragma section(".CRT$XIY",long,read)
#pragma section(".CRT$XIZ",long,read)
#pragma section(".CRT$XLA",long,read)
#pragma section(".CRT$XLC",long,read)
#pragma section(".CRT$XLD",long,read)
#pragma section(".CRT$XLZ",long,read)
#pragma section(".CRT$XPA",long,read)
#pragma section(".CRT$XPX",long,read)
#pragma section(".CRT$XPXA",long,read)
#pragma section(".CRT$XPZ",long,read)
#pragma section(".CRT$XTA",long,read)
#pragma section(".CRT$XTB",long,read)
#pragma section(".CRT$XTX",long,read)
#pragma section(".CRT$XTZ",long,read)

#pragma section(".rdata$T",long,read)
#pragma section(".rtc$IAA",long,read)
#pragma section(".rtc$IZZ",long,read)
#pragma section(".rtc$TAA",long,read)
#pragma section(".rtc$TZZ",long,read)

#define _CRTALLOC(x) __declspec(allocate(x))

extern "C" {

int _fltused;

_CRTALLOC(".CRT$XCA") _PVFV __xc_a[] = { 0 };

_CRTALLOC(".CRT$XCZ") _PVFV __xc_z[] = { 0 };

_CRTALLOC(".CRT$XIA") _PIFV __xi_a[] = { 0 };

_CRTALLOC(".CRT$XIZ") _PIFV __xi_z[] = { 0 };

_CRTALLOC(".CRT$XPA") _PVFV __xp_a[] = { 0 };

_CRTALLOC(".CRT$XPZ") _PVFV __xp_z[] = { 0 };

_CRTALLOC(".CRT$XTA") _PVFV __xt_a[] = { 0 };

_CRTALLOC(".CRT$XTZ") _PVFV __xt_z[] = { 0 };

int __cdecl _purecall(void)
{
    return 0;
}

int __cdecl _initterm(void *pfbegin, void *pfend, int is_PIFV)
{
    _PIFV *pifbegin = (_PIFV *)pfbegin;
    _PIFV *pifend = (_PIFV *)pfend;

    while (pifbegin < pifend) {
        if (*pifbegin) {
            int res = (**pifbegin)();
            if (is_PIFV && res) {
                return res;
            }
        }

        ++pifbegin;
    }

    return 0;
}

void __cdecl _invokedtors(_PVFV pfn[], UINT c)
{
    if (c > 0) { // reverse order
        do {
            (*pfn[--c])();
        } while(c);
    }
}

static _PVFV* ppfnTerm = NULL;
static UINT cTerms = 0;

int __cdecl atexit(_PVFV pfn)
{
    static UINT cAlloc = 0;

    if (cTerms >= cAlloc) {
        cAlloc += 16;
        ppfnTerm = (_PVFV*)fn_memrealloc(ppfnTerm, sizeof(_PVFV)*cAlloc);
        
        if (ppfnTerm == NULL) {
            return 1;
            //fn_ExitProcess(0);
        }
    }

    ppfnTerm[cTerms++] = pfn;
    return 0;
}

}

void* __cdecl operator new(size_t size)/* _THROW1(_STD bad_alloc)*/
{
    return fn_memalloc(size);
}

void* __cdecl operator new(size_t, void* p)
{
    return p;
}

void __cdecl operator delete(void* p)
{
    fn_memfree(p);
}

void cpp_startup()
{
    // call c initialisers
    if (_initterm(__xi_a, __xi_z, 1)) {
        fn_ExitProcess((UINT)-3);
    }

    // call c++ initialisers
    _initterm(__xc_a, __xc_z, 0);
}

void cpp_shutdown()
{
    // invoke terminators
    _invokedtors(ppfnTerm, cTerms);
    fn_memfree(ppfnTerm);
    // call C pre-termination functions
    _initterm(__xp_a, __xp_z, 0);

    // call C termination functions
    _initterm(__xt_a, __xt_z, 0);
}