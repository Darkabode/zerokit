#ifndef __WOW64EXT_H_
#define __WOW64EXT_H_

#ifndef STATUS_SUCCESS
#	define STATUS_SUCCESS 0
#endif

#pragma pack(push)
#pragma pack(1)

typedef struct ___LIST_ENTRY32
{
    DWORD Flink;
    DWORD Blink;
} __LIST_ENTRY32;

typedef struct ___LIST_ENTRY64
{
    DWORD64 Flink;
    DWORD64 Blink;
} __LIST_ENTRY64;

typedef struct _UNICODE_STRING32
{
    union
    {
        struct
        {
            WORD Length;
            WORD MaximumLength;
        };
        DWORD dummy;
    };
    DWORD Buffer;
} UNICODE_STRING32;

typedef struct _UNICODE_STRING64
{
    union
    {
        struct
        {
            WORD Length;
            WORD MaximumLength;
        };
        DWORD64 dummy;
    };
    DWORD64 Buffer;
} UNICODE_STRING64;


typedef struct ___NT_TIB32
{
    DWORD ExceptionList;
    DWORD StackBase;
    DWORD StackLimit;
    DWORD SubSystemTib;
    DWORD FiberData;
    DWORD ArbitraryUserPointer;
    DWORD Self;
} __NT_TIB32;

typedef struct ___NT_TIB64
{
    DWORD64 ExceptionList;
    DWORD64 StackBase;
    DWORD64 StackLimit;
    DWORD64 SubSystemTib;
    DWORD64 FiberData;
    DWORD64 ArbitraryUserPointer;
    DWORD64 Self;
} __NT_TIB64;


typedef struct _CLIENT_ID32
{
	DWORD UniqueProcess;
	DWORD UniqueThread;
} CLIENT_ID32;

typedef struct _CLIENT_ID64
{
    DWORD64 UniqueProcess;
    DWORD64 UniqueThread;
} CLIENT_ID64;

typedef struct _TEB32
{
    __NT_TIB32 NtTib;
    DWORD EnvironmentPointer;
    CLIENT_ID32 ClientId;
    DWORD ActiveRpcHandle;
    DWORD ThreadLocalStoragePointer;
    DWORD ProcessEnvironmentBlock;
    DWORD LastErrorValue;
    DWORD CountOfOwnedCriticalSections;
    DWORD CsrClientThread;
    DWORD Win32ThreadInfo;
    DWORD User32Reserved[26];
    //rest of the structure is not defined for now, as it is not needed
} TEB32;

typedef struct _TEB64
{
    __NT_TIB64 NtTib;
    DWORD64 EnvironmentPointer;
    CLIENT_ID64 ClientId;
    DWORD64 ActiveRpcHandle;
    DWORD64 ThreadLocalStoragePointer;
    DWORD64 ProcessEnvironmentBlock;
    DWORD LastErrorValue;
    DWORD CountOfOwnedCriticalSections;
    DWORD64 CsrClientThread;
    DWORD64 Win32ThreadInfo;
    DWORD User32Reserved[26];
    //rest of the structure is not defined for now, as it is not needed
} TEB64;

typedef struct _LDR_DATA_TABLE_ENTRY32
{
    __LIST_ENTRY32 InLoadOrderLinks;
    __LIST_ENTRY32 InMemoryOrderLinks;
    __LIST_ENTRY32 InInitializationOrderLinks;
    DWORD DllBase;
    DWORD EntryPoint;
    union
    {
        DWORD SizeOfImage;
        DWORD dummy01;
    };
    UNICODE_STRING32 FullDllName;
    UNICODE_STRING32 BaseDllName;
    DWORD Flags;
    WORD LoadCount;
    WORD TlsIndex;
    union
    {
        __LIST_ENTRY32 HashLinks;
        struct 
        {
            DWORD SectionPointer;
            DWORD CheckSum;
        };
    };
    union
    {
        DWORD LoadedImports;
        DWORD TimeDateStamp;
    };
    DWORD EntryPointActivationContext;
    DWORD PatchInformation;
    __LIST_ENTRY32 ForwarderLinks;
    __LIST_ENTRY32 ServiceTagLinks;
    __LIST_ENTRY32 StaticLinks;
    DWORD ContextInformation;
    DWORD OriginalBase;
    LARGE_INTEGER LoadTime;
} LDR_DATA_TABLE_ENTRY32;

typedef struct _LDR_DATA_TABLE_ENTRY64
{
    __LIST_ENTRY64 InLoadOrderLinks;
    __LIST_ENTRY64 InMemoryOrderLinks;
    __LIST_ENTRY64 InInitializationOrderLinks;
    DWORD64 DllBase;
    DWORD64 EntryPoint;
    union
    {
        DWORD SizeOfImage;
        DWORD64 dummy01;
    };
    UNICODE_STRING64 FullDllName;
    UNICODE_STRING64 BaseDllName;
    DWORD Flags;
    WORD LoadCount;
    WORD TlsIndex;
    union
    {
        __LIST_ENTRY64 HashLinks;
        struct 
        {
            DWORD64 SectionPointer;
            DWORD64 CheckSum;
        };
    };
    union
    {
        DWORD64 LoadedImports;
        DWORD TimeDateStamp;
    };
    DWORD64 EntryPointActivationContext;
    DWORD64 PatchInformation;
    __LIST_ENTRY64 ForwarderLinks;
    __LIST_ENTRY64 ServiceTagLinks;
    __LIST_ENTRY64 StaticLinks;
    DWORD64 ContextInformation;
    DWORD64 OriginalBase;
    LARGE_INTEGER LoadTime;
} LDR_DATA_TABLE_ENTRY64;

typedef struct _PEB_LDR_DATA32
{
	DWORD Length;
	DWORD Initialized;
	DWORD SsHandle;
	__LIST_ENTRY32 InLoadOrderModuleList;
	__LIST_ENTRY32 InMemoryOrderModuleList;
	__LIST_ENTRY32 InInitializationOrderModuleList;
	DWORD EntryInProgress;
	DWORD ShutdownInProgress;
	DWORD ShutdownThreadId;
} PEB_LDR_DATA32;

typedef struct _PEB_LDR_DATA64
{
    DWORD Length;
    DWORD Initialized;
    DWORD64 SsHandle;
    __LIST_ENTRY64 InLoadOrderModuleList;
    __LIST_ENTRY64 InMemoryOrderModuleList;
    __LIST_ENTRY64 InInitializationOrderModuleList;
    DWORD64 EntryInProgress;
    DWORD ShutdownInProgress;
    DWORD64 ShutdownThreadId;
} PEB_LDR_DATA64;

typedef struct _PEB32
{
	union
	{
		struct
		{
			BYTE InheritedAddressSpace;
			BYTE ReadImageFileExecOptions;
			BYTE BeingDebugged;
			BYTE BitField;
		};
		DWORD dummy01;
	};
	DWORD Mutant;
	DWORD ImageBaseAddress;
	DWORD Ldr;
	DWORD ProcessParameters;
	DWORD SubSystemData;
	DWORD ProcessHeap;
	DWORD FastPebLock;
	DWORD AtlThunkSListPtr;
	DWORD IFEOKey;
	DWORD CrossProcessFlags;
	DWORD UserSharedInfoPtr;
	DWORD SystemReserved;
	DWORD AtlThunkSListPtr32;
	DWORD ApiSetMap;
	DWORD TlsExpansionCounter;
	DWORD TlsBitmap;
	DWORD TlsBitmapBits[2];
	DWORD ReadOnlySharedMemoryBase;
	DWORD HotpatchInformation;
	DWORD ReadOnlyStaticServerData;
	DWORD AnsiCodePageData;
	DWORD OemCodePageData;
	DWORD UnicodeCaseTableData;
	DWORD NumberOfProcessors;
	union
	{
		DWORD NtGlobalFlag;
		DWORD64 dummy02;
	};
	LARGE_INTEGER CriticalSectionTimeout;
	DWORD HeapSegmentReserve;
	DWORD HeapSegmentCommit;
	DWORD HeapDeCommitTotalFreeThreshold;
	DWORD HeapDeCommitFreeBlockThreshold;
	DWORD NumberOfHeaps;
	DWORD MaximumNumberOfHeaps;
	DWORD ProcessHeaps;
	DWORD GdiSharedHandleTable;
	DWORD ProcessStarterHelper;
	DWORD GdiDCAttributeList;
	DWORD LoaderLock;
	DWORD OSMajorVersion;
	DWORD OSMinorVersion;
	WORD OSBuildNumber;
	WORD OSCSDVersion;
	DWORD OSPlatformId;
	DWORD ImageSubsystem;
	DWORD ImageSubsystemMajorVersion;
	DWORD ImageSubsystemMinorVersion;
	DWORD ActiveProcessAffinityMask;
	DWORD GdiHandleBuffer[34];
	DWORD PostProcessInitRoutine; 
	DWORD TlsExpansionBitmap; 
	DWORD TlsExpansionBitmapBits[32];
	DWORD SessionId;
	ULARGE_INTEGER AppCompatFlags;
	ULARGE_INTEGER AppCompatFlagsUser;
	DWORD pShimData;
	DWORD AppCompatInfo;
	UNICODE_STRING32 CSDVersion;
	DWORD ActivationContextData;
	DWORD ProcessAssemblyStorageMap;
	DWORD SystemDefaultActivationContextData;
	DWORD SystemAssemblyStorageMap;
	DWORD MinimumStackCommit;
	DWORD FlsCallback;
	__LIST_ENTRY32 FlsListHead;
	DWORD FlsBitmap;
	DWORD FlsBitmapBits[4];
	DWORD FlsHighIndex;
	DWORD WerRegistrationData;
	DWORD WerShipAssertPtr;
	DWORD pContextData;
	DWORD pImageHeaderHash;
	DWORD TracingFlags;
} PEB32;

typedef struct _PEB64
{
    union
    {
        struct
        {
            BYTE InheritedAddressSpace;
            BYTE ReadImageFileExecOptions;
            BYTE BeingDebugged;
            BYTE BitField;
        };
        DWORD64 dummy01;
    };
    DWORD64 Mutant;
    DWORD64 ImageBaseAddress;
    DWORD64 Ldr;
    DWORD64 ProcessParameters;
    DWORD64 SubSystemData;
    DWORD64 ProcessHeap;
    DWORD64 FastPebLock;
    DWORD64 AtlThunkSListPtr;
    DWORD64 IFEOKey;
    DWORD64 CrossProcessFlags;
    DWORD64 UserSharedInfoPtr;
    DWORD SystemReserved;
    DWORD AtlThunkSListPtr32;
    DWORD64 ApiSetMap;
    DWORD64 TlsExpansionCounter;
    DWORD64 TlsBitmap;
    DWORD TlsBitmapBits[2];
    DWORD64 ReadOnlySharedMemoryBase;
    DWORD64 HotpatchInformation;
    DWORD64 ReadOnlyStaticServerData;
    DWORD64 AnsiCodePageData;
    DWORD64 OemCodePageData;
    DWORD64 UnicodeCaseTableData;
    DWORD NumberOfProcessors;
    union
    {
        DWORD NtGlobalFlag;
        DWORD dummy02;
    };
    LARGE_INTEGER CriticalSectionTimeout;
    DWORD64 HeapSegmentReserve;
    DWORD64 HeapSegmentCommit;
    DWORD64 HeapDeCommitTotalFreeThreshold;
    DWORD64 HeapDeCommitFreeBlockThreshold;
    DWORD NumberOfHeaps;
    DWORD MaximumNumberOfHeaps;
    DWORD64 ProcessHeaps;
    DWORD64 GdiSharedHandleTable;
    DWORD64 ProcessStarterHelper;
    DWORD64 GdiDCAttributeList;
    DWORD64 LoaderLock;
    DWORD OSMajorVersion;
    DWORD OSMinorVersion;
    WORD OSBuildNumber;
    WORD OSCSDVersion;
    DWORD OSPlatformId;
    DWORD ImageSubsystem;
    DWORD ImageSubsystemMajorVersion;
    DWORD64 ImageSubsystemMinorVersion;
    DWORD64 ActiveProcessAffinityMask;
    DWORD64 GdiHandleBuffer[30];
    DWORD64 PostProcessInitRoutine; 
    DWORD64 TlsExpansionBitmap; 
    DWORD TlsExpansionBitmapBits[32];
    DWORD64 SessionId;
    ULARGE_INTEGER AppCompatFlags;
    ULARGE_INTEGER AppCompatFlagsUser;
    DWORD64 pShimData;
    DWORD64 AppCompatInfo;
    UNICODE_STRING64 CSDVersion;
    DWORD64 ActivationContextData;
    DWORD64 ProcessAssemblyStorageMap;
    DWORD64 SystemDefaultActivationContextData;
    DWORD64 SystemAssemblyStorageMap;
    DWORD64 MinimumStackCommit;
    DWORD64 FlsCallback;
    __LIST_ENTRY64 FlsListHead;
    DWORD64 FlsBitmap;
    DWORD FlsBitmapBits[4];
    DWORD64 FlsHighIndex;
    DWORD64 WerRegistrationData;
    DWORD64 WerShipAssertPtr;
    DWORD64 pContextData;
    DWORD64 pImageHeaderHash;
    DWORD64 TracingFlags;
} PEB64;

typedef struct _OBJECT_ATTRIBUTES64
{
    ULONG Length;
    DWORD64 RootDirectory;
    UNICODE_STRING64* ObjectName;
    ULONG Attributes;
    DWORD64 SecurityDescriptor;        // Points to type SECURITY_DESCRIPTOR
    DWORD64 SecurityQualityOfService;  // Points to type SECURITY_QUALITY_OF_SERVICE
} OBJECT_ATTRIBUTES64;

#pragma pack(pop)


DWORD64 __cdecl X64Call(DWORD func, int argC, ...);
DWORD GetModuleHandle64(wchar_t* lpModuleName);
DWORD GetProcAddress64(DWORD hModule, char* funcName);
SIZE_T VirtualQueryEx64(HANDLE hProcess, DWORD64 lpAddress, MEMORY_BASIC_INFORMATION64* lpBuffer, SIZE_T dwLength);
DWORD64 VirtualAllocEx64(HANDLE hProcess, DWORD64 lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect);
BOOL VirtualFreeEx64(HANDLE hProcess, DWORD64 lpAddress, SIZE_T dwSize, DWORD dwFreeType);
BOOL ReadProcessMemory64(HANDLE hProcess, DWORD64 lpBaseAddress, LPVOID lpBuffer, SIZE_T nSize, SIZE_T *lpNumberOfBytesRead);
BOOL WriteProcessMemory64(HANDLE hProcess, DWORD64 lpBaseAddress, LPVOID lpBuffer, SIZE_T nSize, SIZE_T *lpNumberOfBytesWritten);

#endif // __WOW64EXT_H_