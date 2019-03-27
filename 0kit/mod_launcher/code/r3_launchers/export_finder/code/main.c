#ifndef _CONSOLE
#include <ntifs.h>
#include <wdm.h>
#include <ntstrsafe.h>
#else
#include <Windows.h>
#endif // _CONSOLE

#include "../../../../../../shared_code/types.h"
#ifndef _CONSOLE
#include "../../../../../../shared_code/pe.h"
#endif // _CONSOLE

pvoid_t shellcode_export_finder(uint8_t* moduleBase, uint32_t hashVal)
{
    PIMAGE_DOS_HEADER dosHdr = (PIMAGE_DOS_HEADER)moduleBase;
    PIMAGE_NT_HEADERS ntHdr = (PIMAGE_NT_HEADERS)(moduleBase + dosHdr->e_lfanew);
    PIMAGE_EXPORT_DIRECTORY pExports;
    uint32_t i, NumberOfFuncNames;
    uint32_t* AddressOfNames;
    uint32_t* AddressOfFunctions;
    uint16_t index;
    pvoid_t apiVA = NULL;
    USE_GLOBAL_BLOCK;

    pExports = (PIMAGE_EXPORT_DIRECTORY)(moduleBase + ntHdr->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);

    if (pExports != NULL) {
        NumberOfFuncNames = pExports->NumberOfNames;
        AddressOfNames = (uint32_t* )(moduleBase + pExports->AddressOfNames);

        for (i = 0; i < NumberOfFuncNames; ++i) {
            char* pThunkRVAtemp = (char*)(moduleBase + *AddressOfNames);
            if (pThunkRVAtemp != NULL) {
                uint32_t cHashVal = 0;
                uint8_t* ptr = (uint8_t*)pThunkRVAtemp;

                for ( ; *ptr != '\0'; ++ptr) {
                    cHashVal = ((cHashVal >> 11) | (cHashVal << (32 - 11)));
                    cHashVal += *ptr;
                }

                if (cHashVal == hashVal) {
                    uint16_t* AddressOfNameOrdinals = (uint16_t*)(moduleBase + pExports->AddressOfNameOrdinals);
                    AddressOfNameOrdinals += (uint16_t)i;
                    index = *AddressOfNameOrdinals;
                    AddressOfFunctions = (uint32_t*)(moduleBase +  pExports->AddressOfFunctions);
                    AddressOfFunctions += index;
                    apiVA = (pvoid_t)(moduleBase + *AddressOfFunctions);
                    break;
                }
            }
            AddressOfNames++;
        }
    }

    return apiVA;
}
