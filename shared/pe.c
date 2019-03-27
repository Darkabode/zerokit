#ifndef _WIN32_WINNT            // Specifies that the minimum required platform is Windows Vista.
#define _WIN32_WINNT _WIN32_WINNT_WINXP     // Change this to the appropriate value to target other versions of Windows.
#endif

// Modify the following defines if you have to target a platform prior to the ones specified below.
// Refer to MSDN for the latest info on corresponding values for different platforms.
#ifndef WINVER                          // Specifies that the minimum required platform is Windows Vista.
#define WINVER _WIN32_WINNT           // Change this to the appropriate value to target other versions of Windows.
#endif

#ifndef _WIN32_WINDOWS          // Specifies that the minimum required platform is Windows 98.
#define _WIN32_WINDOWS 0x0410 // Change this to the appropriate value to target Windows Me or later.
#endif

#ifndef _WIN32_IE                       // Specifies that the minimum required platform is Internet Explorer 7.0.
#define _WIN32_IE 0x0700        // Change this to the appropriate value to target other versions of IE.
#endif


#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Psapi.h>
#include <ImageHlp.h>

#include "types.h"
#ifndef WIN32
#include "pe.h"
#endif // WIN32

#define UE_PE_OFFSET 0
#define UE_IMAGEBASE 1
#define UE_OEP 2
#define UE_SIZEOFIMAGE 3
#define UE_SIZEOFHEADERS 4
#define UE_SIZEOFOPTIONALHEADER 5
#define UE_SECTIONALIGNMENT 6
#define UE_IMPORTTABLEADDRESS 7
#define UE_IMPORTTABLESIZE 8
#define UE_RESOURCETABLEADDRESS 9
#define UE_RESOURCETABLESIZE 10
#define UE_EXPORTTABLEADDRESS 11
#define UE_EXPORTTABLESIZE 12
#define UE_TLSTABLEADDRESS 13
#define UE_TLSTABLESIZE 14
#define UE_RELOCATIONTABLEADDRESS 15
#define UE_RELOCATIONTABLESIZE 16
#define UE_TIMEDATESTAMP 17
#define UE_SECTIONNUMBER 18
#define UE_CHECKSUM 19
#define UE_SUBSYSTEM 20
#define UE_CHARACTERISTICS 21
#define UE_NUMBEROFRVAANDSIZES 22
#define UE_SECTIONNAME 23
#define UE_SECTIONVIRTUALOFFSET 24
#define UE_SECTIONVIRTUALSIZE 25
#define UE_SECTIONRAWOFFSET 26
#define UE_SECTIONRAWSIZE 27
#define UE_SECTIONFLAGS 28

#define UE_ACCESS_READ 0
#define UE_ACCESS_WRITE 1
#define UE_ACCESS_ALL 2

int EngineCloseHandle(HANDLE myHandle){

	DWORD HandleFlags;

	if (GetHandleInformation(myHandle, &HandleFlags)) {
		if (CloseHandle(myHandle)) {
			return 1;
		}
		else {
			return 0;
		}
	}
	else {
		return 0;
	}
}

int MapFileEx(char* szFileName, DWORD ReadOrWrite, LPHANDLE FileHandle, LPDWORD FileSize, LPHANDLE FileMap, LPVOID FileMapVA, DWORD SizeModifier){

	HANDLE hFile = 0;
	DWORD FileAccess = 0;
	DWORD FileMapType = 0;
	DWORD FileMapViewType = 0;
	DWORD mfFileSize = 0;
	HANDLE mfFileMap = 0;
	LPVOID mfFileMapVA = 0;

	if(ReadOrWrite == UE_ACCESS_READ){
		FileAccess = GENERIC_READ;
		FileMapType = PAGE_READONLY;
		FileMapViewType = FILE_MAP_READ;
	}else if(ReadOrWrite == UE_ACCESS_WRITE){
		FileAccess = GENERIC_WRITE;
		FileMapType = PAGE_READWRITE;
		FileMapViewType = FILE_MAP_WRITE;
	}else if(ReadOrWrite == UE_ACCESS_ALL){
		FileAccess = GENERIC_READ+GENERIC_WRITE+GENERIC_EXECUTE;
		FileMapType = PAGE_EXECUTE_READWRITE;
		FileMapViewType = FILE_MAP_WRITE;
	}else{
		FileAccess = GENERIC_READ+GENERIC_WRITE+GENERIC_EXECUTE;
		FileMapType = PAGE_EXECUTE_READWRITE;
		FileMapViewType = FILE_MAP_ALL_ACCESS;
	}

	hFile = CreateFileA(szFileName, FileAccess, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hFile != INVALID_HANDLE_VALUE){
		*FileHandle = hFile;
		mfFileSize = GetFileSize(hFile,NULL);
		mfFileSize = mfFileSize + SizeModifier;
		*FileSize = mfFileSize;
		mfFileMap = CreateFileMappingA(hFile, NULL, FileMapType, 0, mfFileSize, NULL);
		if(mfFileMap != NULL){
			*FileMap = mfFileMap;
			mfFileMapVA = MapViewOfFile(mfFileMap, FileMapViewType, 0, 0, 0);
			if(mfFileMapVA != NULL){
				RtlMoveMemory(FileMapVA, &mfFileMapVA, sizeof(ULONG_PTR));
				return 1;
			}
		}
		RtlZeroMemory(FileMapVA, sizeof(ULONG_PTR));
		*FileHandle = NULL;
		*FileSize = 0;
		EngineCloseHandle(hFile);
	}
	else {
		RtlZeroMemory(FileMapVA, sizeof(ULONG_PTR));
	}
	return 0;
}

void UnMapFileEx(HANDLE FileHandle, DWORD FileSize, HANDLE FileMap, ULONG_PTR FileMapVA){

	LPVOID ufFileMapVA = (void*)FileMapVA;

	if(UnmapViewOfFile(ufFileMapVA)) {
		EngineCloseHandle(FileMap);
		SetFilePointer(FileHandle,FileSize,NULL,FILE_BEGIN);
		SetEndOfFile(FileHandle);
		EngineCloseHandle(FileHandle);
	}
}

int EngineValidateHeader(ULONG_PTR FileMapVA, HANDLE hFileProc, LPVOID ImageBase, PIMAGE_DOS_HEADER DOSHeader, int IsFile) {

	MODULEINFO ModuleInfo;
	DWORD MemorySize = 0;
	PIMAGE_NT_HEADERS32 PEHeader32;
	IMAGE_NT_HEADERS32 RemotePEHeader32;
	MEMORY_BASIC_INFORMATION MemoryInfo;
	puint_t NumberOfBytesRW = 0;

	if(IsFile) {
		if(hFileProc == NULL){
			VirtualQueryEx(GetCurrentProcess(), (LPVOID)FileMapVA, &MemoryInfo, sizeof(MEMORY_BASIC_INFORMATION));
			VirtualQueryEx(GetCurrentProcess(), MemoryInfo.AllocationBase, &MemoryInfo, sizeof(MEMORY_BASIC_INFORMATION));
			MemorySize = (DWORD)((ULONG_PTR)MemoryInfo.AllocationBase + (ULONG_PTR)MemoryInfo.RegionSize - (ULONG_PTR)FileMapVA);
		}
		else {
			MemorySize = GetFileSize(hFileProc, NULL);
		}
		
		if(DOSHeader->e_magic == 0x5A4D){
			if(DOSHeader->e_lfanew + sizeof(IMAGE_DOS_HEADER) + sizeof(IMAGE_NT_HEADERS64) < MemorySize) {
				PEHeader32 = (PIMAGE_NT_HEADERS32)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
				if(PEHeader32->Signature != 0x4550){
					return 0;
				}
				else {
					return 1;
				}
			}
			else {
				return 0;
			}
		}
		else {
			return 0;
		}
	
	}
	else {
		RtlZeroMemory(&ModuleInfo, sizeof(MODULEINFO));
		GetModuleInformation(hFileProc, (HMODULE)ImageBase, &ModuleInfo, sizeof(MODULEINFO));
		
		if(DOSHeader->e_magic == 0x5A4D){
			if(DOSHeader->e_lfanew + sizeof(IMAGE_DOS_HEADER) + sizeof(IMAGE_NT_HEADERS64) < ModuleInfo.SizeOfImage){
				if(ReadProcessMemory(hFileProc, (LPVOID)((ULONG_PTR)ImageBase + DOSHeader->e_lfanew), &RemotePEHeader32, sizeof(IMAGE_NT_HEADERS32), &NumberOfBytesRW)){
					PEHeader32 = (PIMAGE_NT_HEADERS32)(&RemotePEHeader32);
					if(PEHeader32->Signature != 0x4550){
						return 0;
					}
					else {
						return 1;
					}
				}
				else {
					return 0;
				}
			}
			else {
				return 0;
			}
		}
		else {
			return 0;
		}
	}
}

int SetPE32DataForMappedFile(puint_t FileMapVA, DWORD WhichSection, DWORD WhichData, puint_t NewDataValue)
{

	PIMAGE_DOS_HEADER DOSHeader;
	PIMAGE_NT_HEADERS32 PEHeader32;
	PIMAGE_NT_HEADERS64 PEHeader64;
	PIMAGE_SECTION_HEADER PESections;
	DWORD SectionNumber = 0;
	int FileIs64;

	if(FileMapVA != 0) {
		DOSHeader = (PIMAGE_DOS_HEADER)FileMapVA;
		if(EngineValidateHeader(FileMapVA, NULL, NULL, DOSHeader, 1)){
			PEHeader32 = (PIMAGE_NT_HEADERS32)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
			PEHeader64 = (PIMAGE_NT_HEADERS64)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
			if(PEHeader32->OptionalHeader.Magic == 0x10B){
				FileIs64 = 0;
			}else if(PEHeader32->OptionalHeader.Magic == 0x20B){
				FileIs64 = 1;
			}else{
				return 0;
			}
			if(!FileIs64) {
				PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PEHeader32 + PEHeader32->FileHeader.SizeOfOptionalHeader + sizeof(IMAGE_FILE_HEADER) + 4);
				SectionNumber = PEHeader32->FileHeader.NumberOfSections;
				
				if(WhichData < UE_SECTIONNAME){
					if(WhichData == UE_PE_OFFSET){
						DOSHeader->e_lfanew = (DWORD)NewDataValue;
						return 1;
					}else if(WhichData == UE_IMAGEBASE){
						PEHeader32->OptionalHeader.ImageBase = (DWORD)NewDataValue;
						return 1;
					}else if(WhichData == UE_OEP){
						PEHeader32->OptionalHeader.AddressOfEntryPoint = (DWORD)NewDataValue;
						return 1;
					}else if(WhichData == UE_SIZEOFIMAGE){
						PEHeader32->OptionalHeader.SizeOfImage = (DWORD)NewDataValue;
						return 1;
					}else if(WhichData == UE_SIZEOFHEADERS){
						PEHeader32->OptionalHeader.SizeOfHeaders = (DWORD)NewDataValue;
						return 1;
					}else if(WhichData == UE_SIZEOFOPTIONALHEADER){
						PEHeader32->FileHeader.SizeOfOptionalHeader = (WORD)NewDataValue;
						return 1;
					}else if(WhichData == UE_SECTIONALIGNMENT){
						PEHeader32->OptionalHeader.SectionAlignment = (WORD)NewDataValue;
						return 1;
					}else if(WhichData == UE_IMPORTTABLEADDRESS){
						PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress = (DWORD)NewDataValue;
						return 1;
					}else if(WhichData == UE_IMPORTTABLESIZE){
						PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size = (DWORD)NewDataValue;
						return 1;
					}else if(WhichData == UE_RESOURCETABLEADDRESS){
						PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress = (DWORD)NewDataValue;
						return 1;
					}else if(WhichData == UE_RESOURCETABLESIZE){
						PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size = (DWORD)NewDataValue;
						return 1;
					}else if(WhichData == UE_EXPORTTABLEADDRESS){
						PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress = (DWORD)NewDataValue;
						return 1;
					}else if(WhichData == UE_EXPORTTABLESIZE){
						PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size = (DWORD)NewDataValue;
						return 1;
					}else if(WhichData == UE_TLSTABLEADDRESS){
						PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress = (DWORD)NewDataValue;
						return 1;
					}else if(WhichData == UE_TLSTABLESIZE){
						PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size = (DWORD)NewDataValue;
						return 1;
					}else if(WhichData == UE_RELOCATIONTABLEADDRESS){
						PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress = (DWORD)NewDataValue;
						return 1;
					}else if(WhichData == UE_RELOCATIONTABLESIZE){
						PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size = (DWORD)NewDataValue;
						return 1;
					}else if(WhichData == UE_TIMEDATESTAMP){
						PEHeader32->FileHeader.TimeDateStamp = (DWORD)NewDataValue;
						return 1;
					}else if(WhichData == UE_SECTIONNUMBER){
						PEHeader32->FileHeader.NumberOfSections = (WORD)NewDataValue;
						return 1;
					}else if(WhichData == UE_CHECKSUM){
						PEHeader32->OptionalHeader.CheckSum = (DWORD)NewDataValue;
						return 1;
					}else if(WhichData == UE_SUBSYSTEM){
						PEHeader32->OptionalHeader.Subsystem = (WORD)NewDataValue;
						return 1;
					}else if(WhichData == UE_CHARACTERISTICS){
						PEHeader32->FileHeader.Characteristics = (WORD)NewDataValue;
						return 1;
					}else if(WhichData == UE_NUMBEROFRVAANDSIZES){
						PEHeader32->OptionalHeader.NumberOfRvaAndSizes = (DWORD)NewDataValue;
						return 1;
					}else{
						return 0;
					}
				}
				else {
					if(WhichSection <= SectionNumber){
						PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PESections + WhichSection * IMAGE_SIZEOF_SECTION_HEADER);
						if(WhichData == UE_SECTIONNAME){
							return 0;
						}else if(WhichData == UE_SECTIONVIRTUALOFFSET){
							PESections->VirtualAddress = (DWORD)NewDataValue;
							return 1;
						}else if(WhichData == UE_SECTIONVIRTUALSIZE){
							PESections->Misc.VirtualSize = (DWORD)NewDataValue;
							return 1;
						}else if(WhichData == UE_SECTIONRAWOFFSET){
							PESections->PointerToRawData = (DWORD)NewDataValue;
							return 1;
						}else if(WhichData == UE_SECTIONRAWSIZE){
							PESections->SizeOfRawData = (DWORD)NewDataValue;
							return 1;
						}else if(WhichData == UE_SECTIONFLAGS){
							PESections->Characteristics = (DWORD)NewDataValue;
							return 1;
						}else{
							return 0;
						}
					}
				}
			
				return 0;
			}else{
				PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PEHeader64 + PEHeader64->FileHeader.SizeOfOptionalHeader + sizeof(IMAGE_FILE_HEADER) + 4);			
				SectionNumber = PEHeader64->FileHeader.NumberOfSections;

				if(WhichData < UE_SECTIONNAME){
					if(WhichData == UE_PE_OFFSET){
						DOSHeader->e_lfanew = (DWORD)NewDataValue;
						return 1;
					}else if(WhichData == UE_IMAGEBASE){
						PEHeader64->OptionalHeader.ImageBase = NewDataValue;
						return 1;
					}else if(WhichData == UE_OEP){
						PEHeader64->OptionalHeader.AddressOfEntryPoint = (DWORD)NewDataValue;
						return 1;
					}else if(WhichData == UE_SIZEOFIMAGE){
						PEHeader64->OptionalHeader.SizeOfImage = (DWORD)NewDataValue;
						return 1;
					}else if(WhichData == UE_SIZEOFHEADERS){
						PEHeader64->OptionalHeader.SizeOfHeaders = (DWORD)NewDataValue;
						return 1;
					}else if(WhichData == UE_SIZEOFOPTIONALHEADER){
						PEHeader64->FileHeader.SizeOfOptionalHeader = (WORD)NewDataValue;
						return 1;
					}else if(WhichData == UE_SECTIONALIGNMENT){
						PEHeader64->OptionalHeader.SectionAlignment = (DWORD)NewDataValue;
						return 1;
					}else if(WhichData == UE_IMPORTTABLEADDRESS){
						PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress = (DWORD)NewDataValue;
						return 1;
					}else if(WhichData == UE_IMPORTTABLESIZE){
						PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size = (DWORD)NewDataValue;
						return 1;
					}else if(WhichData == UE_RESOURCETABLEADDRESS){
						PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress = (DWORD)NewDataValue;
						return 1;
					}else if(WhichData == UE_RESOURCETABLESIZE){
						PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size = (DWORD)NewDataValue;
						return 1;
					}else if(WhichData == UE_EXPORTTABLEADDRESS){
						PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress = (DWORD)NewDataValue;
						return 1;
					}else if(WhichData == UE_EXPORTTABLESIZE){
						PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size = (DWORD)NewDataValue;
						return 1;
					}else if(WhichData == UE_TLSTABLEADDRESS){
						PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress = (DWORD)NewDataValue;
						return 1;
					}else if(WhichData == UE_TLSTABLESIZE){
						PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size = (DWORD)NewDataValue;
						return 1;
					}else if(WhichData == UE_RELOCATIONTABLEADDRESS){
						PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress = (DWORD)NewDataValue;
						return 1;
					}else if(WhichData == UE_RELOCATIONTABLESIZE){
						PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size = (DWORD)NewDataValue;
						return 1;
					}else if(WhichData == UE_TIMEDATESTAMP){
						PEHeader64->FileHeader.TimeDateStamp = (DWORD)NewDataValue;
						return 1;
					}else if(WhichData == UE_SECTIONNUMBER){
						PEHeader64->FileHeader.NumberOfSections = (WORD)NewDataValue;
						return 1;
					}else if(WhichData == UE_CHECKSUM){
						PEHeader64->OptionalHeader.CheckSum = (DWORD)NewDataValue;
						return 1;
					}else if(WhichData == UE_SUBSYSTEM){
						PEHeader64->OptionalHeader.Subsystem = (WORD)NewDataValue;
						return 1;
					}else if(WhichData == UE_CHARACTERISTICS){
						PEHeader64->FileHeader.Characteristics = (WORD)NewDataValue;
						return 1;
					}else if(WhichData == UE_NUMBEROFRVAANDSIZES){
						PEHeader64->OptionalHeader.NumberOfRvaAndSizes = (DWORD)NewDataValue;
						return 1;
					}else{
						return(0);
					}
				}else{
					if(WhichSection <= SectionNumber){
						PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PESections + WhichSection * IMAGE_SIZEOF_SECTION_HEADER);
						if(WhichData == UE_SECTIONNAME){
							return 0;
						}else if(WhichData == UE_SECTIONVIRTUALOFFSET){
							PESections->VirtualAddress = (DWORD)NewDataValue;
							return 1;
						}else if(WhichData == UE_SECTIONVIRTUALSIZE){
							PESections->Misc.VirtualSize = (DWORD)NewDataValue;
							return 1;
						}else if(WhichData == UE_SECTIONRAWOFFSET){
							PESections->PointerToRawData = (DWORD)NewDataValue;
							return 1;
						}else if(WhichData == UE_SECTIONRAWSIZE){
							PESections->SizeOfRawData = (DWORD)NewDataValue;
							return 1;
						}else if(WhichData == UE_SECTIONFLAGS){
							PESections->Characteristics = (DWORD)NewDataValue;
							return 1;
						}else{
							return 0;
						}
					}
				}
			
				return 0;
			}
		}else{
			return 0;
		}
	}
	return 0;
}

int SetPE32Data(char* szFileName, DWORD WhichSection, DWORD WhichData, puint_t NewDataValue)
{
	HANDLE FileHandle;
	DWORD FileSize;
	HANDLE FileMap;
	puint_t FileMapVA;
	long long ReturnValue = 0;

	if(MapFileEx(szFileName, UE_ACCESS_ALL, &FileHandle, &FileSize, &FileMap, &FileMapVA, 0)) {
		ReturnValue = SetPE32DataForMappedFile(FileMapVA, WhichSection, WhichData, NewDataValue);
		UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
		if(ReturnValue){
			return 1;
		}else{
			return 0;
		}
	}else{
		return 0;
	}
}

// TitanEngine.Realigner.functions:
int FixHeaderCheckSum(char* szFileName)
{
	DWORD HeaderSum = 0;
	DWORD CheckSum  = 0;

	if(MapFileAndCheckSumA(szFileName, &HeaderSum, &CheckSum) == 0) {
		SetPE32Data(szFileName, 0, UE_CHECKSUM, (puint_t)CheckSum);
		return 1;
	}

	return 0;
}
