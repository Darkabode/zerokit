#ifndef _CONSOLE
#include <ntifs.h>
#include <wdm.h>
#include <ntstrsafe.h>
#else
typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
#ifdef MIDL_PASS
    [size_is(MaximumLength / 2), length_is((Length) / 2) ] USHORT * Buffer;
#else // MIDL_PASS
    __field_bcount_part(MaximumLength, Length) PWCH   Buffer;
#endif // MIDL_PASS
} UNICODE_STRING;
typedef UNICODE_STRING *PUNICODE_STRING;
typedef const UNICODE_STRING *PCUNICODE_STRING;
#endif // _CONSOLE

#include "../../../../../../shared_code/types.h"

pvoid_t __stdcall shellcode_ntdll_finder(uint32_t dllNameHash)
{
    PLIST_ENTRY pDllListHead = NULL;
    PLIST_ENTRY pDllListEntry = NULL;
    PUNICODE_STRING dllName;
    uint8_t* pebBaseAddress;
    pvoid_t dllBase = NULL;

#ifdef _WIN64
#define LDR_OFFSET 0x018
#define INMEMORYORDERMODULELIST_OFFSET 0x020
#define FULLDLLNAME_OFFSET 0x048
#define DLLBASE_OFFSET 0x020
    pebBaseAddress = (pvoid_t)__readgsqword(0x60);
#else
#define LDR_OFFSET 0x00C
#define INMEMORYORDERMODULELIST_OFFSET 0x014
#define FULLDLLNAME_OFFSET 0x024
#define DLLBASE_OFFSET 0x010
	pebBaseAddress = (pvoid_t)__readfsdword(0x30);
#endif

    pDllListEntry = pDllListHead = *(void**)(*(uint8_t**)(pebBaseAddress + LDR_OFFSET) + INMEMORYORDERMODULELIST_OFFSET);
    if (pDllListHead != NULL) {
        do {        
            dllName = (PUNICODE_STRING)((uint8_t*)pDllListEntry + FULLDLLNAME_OFFSET);

            if (dllName != NULL) {
                uint32_t hashVal = 0;
                uint16_t len = dllName->Length;
                uint8_t* ptr = (uint8_t*)dllName->Buffer;
                
                for ( ; len > 0; --len, ++ptr) {
                    hashVal = ((hashVal >> 11) | (hashVal << (32 - 11)));
                    hashVal += *ptr | 0x20;
                }
                
                if (hashVal == dllNameHash) {
                    dllBase = *(pvoid_t*)((uint8_t*)pDllListEntry + DLLBASE_OFFSET);
                    break;
                }
            }
            pDllListEntry = pDllListEntry->Flink;
        } while (pDllListEntry != pDllListHead);
    }

    return dllBase;
}
