#include <Windows.h>
#include "internal.h"
#include "wow64ext.h"

DWORD64 __cdecl X64Call(DWORD func, int argC, ...)
{
    DWORD back_esp;
    DWORD64 restArgs;
    DWORD64 _rcx, _rdx, _r8, _r9;
    DWORD64 _argC;
    DWORD64 _func;
    reg64 _rax;
	va_list args;

    va_start(args, argC);
	_rcx = (argC > 0) ? argC--, va_arg(args, DWORD64) : 0;
	_rdx = (argC > 0) ? argC--, va_arg(args, DWORD64) : 0;
	_r8 = (argC > 0) ? argC--, va_arg(args, DWORD64) : 0;
	_r9 = (argC > 0) ? argC--, va_arg(args, DWORD64) : 0;

	_rax.v = 0;

	restArgs = (DWORD64)&va_arg(args, DWORD64);

	//conversion to QWORD for easier use in inline assembly
    _argC = argC;
    _func = func;

    back_esp = 0;
	__asm {
		;//keep original esp in back_esp variable
		mov    back_esp, esp
		
		;//align esp to 8, without aligned stack some syscalls may return errors !
		and    esp, 0xFFFFFFF8

		X64_Start();

		;//fill first four arguments
		push   _rcx
		X64_Pop(_RCX);
		push   _rdx
		X64_Pop(_RDX);
		push   _r8
		X64_Pop(_R8);
		push   _r9
		X64_Pop(_R9);

		push   edi

		push   restArgs
		X64_Pop(_RDI);

		push   _argC
		X64_Pop(_RAX);

		;//put rest of arguments on the stack
		test   eax, eax
		jz     _ls_e
		lea    edi, dword ptr [edi + 8*eax - 8]

		_ls:
		test   eax, eax
		jz     _ls_e
		push   dword ptr [edi]
		sub    edi, 8
		sub    eax, 1
		jmp    _ls
		_ls_e:

		;//create stack space for spilling registers
		sub    esp, 0x20

		call   _func

		;//cleanup stack
		push   _argC
		X64_Pop(_RCX);
		lea    esp, dword ptr [esp + 8*ecx + 0x20]

		pop    edi

		//set return value
		X64_Push(_RAX);
		pop    _rax.dw[0]

		X64_End();

		mov    esp, back_esp
	}
	return _rax.v;
}

TEB64* getTEB64()
{
	reg64 reg;
	reg.v = 0;

    X64_Start();
    //R12 register should always contain pointer to TEB64 in WoW64 processes
    X64_Push(_R12);
    //below pop will pop QWORD from stack, as we're in x64 mode now
    __asm pop reg.dw[0]
    X64_End();

	//upper 32 bits should be always 0 in WoW64 processes
    if (reg.dw[1] != 0) {
		return 0;
    }
	return (TEB64*)reg.dw[0];
}

DWORD GetModuleHandle64(wchar_t* lpModuleName)
{
	DWORD module = 0;

	TEB64* teb64 = getTEB64();
	PEB64* peb64 = (PEB64*)teb64->ProcessEnvironmentBlock;
	PEB_LDR_DATA64* ldr = (PEB_LDR_DATA64*)peb64->Ldr;
	LDR_DATA_TABLE_ENTRY64* head = (LDR_DATA_TABLE_ENTRY64*)ldr->InLoadOrderModuleList.Flink;

	do {
        if (memcmp((void*)head->BaseDllName.Buffer, lpModuleName, head->BaseDllName.Length) == 0) {
			module = (DWORD)head->DllBase;
            break;
        }
		head = (LDR_DATA_TABLE_ENTRY64*)head->InLoadOrderLinks.Flink;
	} while (head != (LDR_DATA_TABLE_ENTRY64*)&ldr->InLoadOrderModuleList);

	return module;
}

DWORD getNTDLL64()
{
	static DWORD ntdll64 = 0;

    if (ntdll64 != 0) {
		return ntdll64;
    }

	ntdll64 = GetModuleHandle64(L"ntdll.dll");
	return ntdll64;
}

DWORD getLdrGetProcedureAddress()
{
	BYTE* modBase = (BYTE*)getNTDLL64();
	IMAGE_NT_HEADERS64* inh = (IMAGE_NT_HEADERS64*)(modBase + ((IMAGE_DOS_HEADER*)modBase)->e_lfanew);
	PIMAGE_DATA_DIRECTORY idd = &inh->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    PIMAGE_EXPORT_DIRECTORY ied;
    DWORD* rvaTable;
    WORD* ordTable;
    DWORD* nameTable;
    DWORD i;

    if (0 == idd->VirtualAddress) {
		return 0;
    }

	ied = (IMAGE_EXPORT_DIRECTORY*)(modBase + idd->VirtualAddress);

	rvaTable = (DWORD*)(modBase + ied->AddressOfFunctions);
	ordTable = (WORD*)(modBase + ied->AddressOfNameOrdinals);
	nameTable = (DWORD*)(modBase + ied->AddressOfNames);
	//lazy search, there is no need to use binsearch for just one function
	for (i = 0; i < ied->NumberOfFunctions; i++) {
        if (strcmp((char*)modBase + nameTable[i], "LdrGetProcedureAddress") == 0) {
			return (DWORD)(modBase + rvaTable[ordTable[i]]);
        }
	}

	return 0;
}

DWORD GetProcAddress64(DWORD hModule, char* funcName)
{
    UNICODE_STRING64 fName = { 0 };
	static DWORD _LdrGetProcedureAddress = 0;
    DWORD64 funcRet = 0;

	if (_LdrGetProcedureAddress == 0) {
		_LdrGetProcedureAddress = getLdrGetProcedureAddress();
        if (_LdrGetProcedureAddress == 0) {
			return 0;
        }
	}

	fName.Buffer = (DWORD64)funcName;
	fName.Length = strlen(funcName);
	fName.MaximumLength = fName.Length + 1;
	
	X64Call(_LdrGetProcedureAddress, 4, (DWORD64)hModule, (DWORD64)&fName, (DWORD64)0, (DWORD64)&funcRet);
	return (DWORD)funcRet;
}

SIZE_T VirtualQueryEx64(HANDLE hProcess, DWORD64 lpAddress, MEMORY_BASIC_INFORMATION64* lpBuffer, SIZE_T dwLength)
{
	static DWORD ntqvm = 0;
    DWORD64 ret = 0;

	if (0 == ntqvm) {
		ntqvm = (DWORD)GetProcAddress64(getNTDLL64(), "NtQueryVirtualMemory");
        if (0 == ntqvm) {
			return 0;
        }
	}
	
	X64Call(ntqvm, 6, (DWORD64)hProcess, lpAddress, (DWORD64)0, (DWORD64)lpBuffer, (DWORD64)dwLength, (DWORD64)&ret);
	return (SIZE_T)ret;
}

DWORD64 VirtualAllocEx64(HANDLE hProcess, DWORD64 lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect)
{
    DWORD64 ret;
    DWORD64 tmpAddr = lpAddress;
    DWORD64 tmpSize = dwSize;
	static DWORD ntavm = 0;

	if (0 == ntavm) {
		ntavm = (DWORD)GetProcAddress64(getNTDLL64(), "NtAllocateVirtualMemory");
		if (0 == ntavm)
			return 0;
	}

	
	ret = X64Call(ntavm, 6, (DWORD64)hProcess, (DWORD64)&tmpAddr, (DWORD64)0, (DWORD64)&tmpSize, (DWORD64)flAllocationType, (DWORD64)flProtect);
    if (STATUS_SUCCESS != ret) {
		return 0;
    }
    else {
		return tmpAddr;
    }
}

BOOL VirtualFreeEx64(HANDLE hProcess, DWORD64 lpAddress, SIZE_T dwSize, DWORD dwFreeType)
{
    DWORD64 ret;
	static DWORD ntfvm = 0;
    DWORD64 tmpAddr = lpAddress;
    DWORD64 tmpSize = dwSize;

	if (0 == ntfvm) {
		ntfvm = (DWORD)GetProcAddress64(getNTDLL64(), "NtFreeVirtualMemory");
        if (0 == ntfvm) {
			return 0;
        }
	}

	
	ret = X64Call(ntfvm, 4, (DWORD64)hProcess, (DWORD64)&tmpAddr, (DWORD64)&tmpSize, (DWORD64)dwFreeType);
    if (STATUS_SUCCESS != ret) {
		return FALSE;
    }
    else {
		return TRUE;
    }
}

BOOL ReadProcessMemory64(HANDLE hProcess, DWORD64 lpBaseAddress, LPVOID lpBuffer, SIZE_T nSize, SIZE_T *lpNumberOfBytesRead)
{
    DWORD64 ret;
	static DWORD nrvm = 0;

	if (0 == nrvm) {
		nrvm = (DWORD)GetProcAddress64(getNTDLL64(), "NtReadVirtualMemory");
		if (0 == nrvm)
			return 0;
	}

	ret = X64Call(nrvm, 5, (DWORD64)hProcess, lpBaseAddress, (DWORD64)lpBuffer, (DWORD64)nSize, (DWORD64)lpNumberOfBytesRead);
    if (STATUS_SUCCESS != ret) {
		return FALSE;
    }
    else {
		return TRUE;
    }
}

BOOL WriteProcessMemory64(HANDLE hProcess, DWORD64 lpBaseAddress, LPVOID lpBuffer, SIZE_T nSize, SIZE_T *lpNumberOfBytesWritten)
{
    DWORD64 ret;
	static DWORD nrvm = 0;

	if (0 == nrvm) {
		nrvm = (DWORD)GetProcAddress64(getNTDLL64(), "NtWriteVirtualMemory");
        if (0 == nrvm) {
			return 0;
        }
	}
	
    ret = X64Call(nrvm, 5, (DWORD64)hProcess, lpBaseAddress, (DWORD64)lpBuffer, (DWORD64)nSize, (DWORD64)lpNumberOfBytesWritten);
    if (STATUS_SUCCESS != ret) {
		return FALSE;
    }
    else {
		return TRUE;
    }
}
