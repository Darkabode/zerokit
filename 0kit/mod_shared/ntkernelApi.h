#ifndef __NTKERNELAPI_H_
#define __NTKERNELAPI_H_

#define PsCreateSystemThread_Hash 0xfd30faf1
typedef NTSTATUS (*FnPsCreateSystemThread)(__out PHANDLE ThreadHandle, __in ulong_t DesiredAccess, __in_opt POBJECT_ATTRIBUTES ObjectAttributes, __in_opt  HANDLE ProcessHandle, __out_opt PCLIENT_ID ClientId, __in PKSTART_ROUTINE StartRoutine, __in_opt __drv_when(return==0, __drv_aliasesMem) void* StartContext);

#define PsTerminateSystemThread_Hash 0x27468573
typedef NTSTATUS (*FnPsTerminateSystemThread)(__in NTSTATUS ExitStatus);

#define ExAllocatePoolWithTag_Hash 0x756ceeda
typedef void* (NTAPI *FnExAllocatePoolWithTag)(__in __drv_strictTypeMatch(__drv_typeExpr) POOL_TYPE PoolType, __in size_t NumberOfBytes, __in ulong_t Tag);

#define MmIsNonPagedSystemAddressValid_Hash 0x26556cf1
typedef bool_t (*FnMmIsNonPagedSystemAddressValid)(__in void* VirtualAddress);

#define ExFreePoolWithTag_Hash 0xc92bd5c8
typedef VOID (*FnExFreePoolWithTag)(__in __drv_freesMem(Mem) void* P, __in ulong_t Tag);

#define KeDelayExecutionThread_Hash 0x5b61fc63
typedef NTSTATUS (*FnKeDelayExecutionThread)(__in KPROCESSOR_MODE WaitMode, __in bool_t Alertable, __in PLARGE_INTEGER Interval);

#define RtlGetVersion_Hash 0x1a530dcb
typedef NTSTATUS (NTAPI *FnRtlGetVersion)(__out __drv_at(lpVersionInformation->dwOSVersionInfoSize,  __inout) PRTL_OSVERSIONINFOW lpVersionInformation);

#ifdef _WIN64
#define PsWrapApcWow64Thread_Hash 0xcacb0c23
typedef NTSTATUS (*FnPsWrapApcWow64Thread)(__inout void* *ApcContext, __inout void* *ApcRoutine);
#endif // _WIN64

#ifndef _WIN64

#define KeInitializeSpinLock_Hash 0x44e4cfe7
typedef VOID (NTAPI *FnKeInitializeSpinLock)(__out PKSPIN_LOCK SpinLock);

#define _allmul_Hash 0xd56276b9
typedef INT64 (*Fn_allmul)(INT64 a, INT64 b);

#define _alldiv_Hash 0xd3e252c3
typedef INT64 (*Fn_alldiv)(INT64 a, INT64 b);

#define _aullrem_Hash 0x565ab2bb
typedef UINT64 (*Fn_aullrem)(UINT64 a, UINT64 b);

#define KeQuerySystemTime_Hash 0xf32434ff
typedef VOID (*FnKeQuerySystemTime)(__out PLARGE_INTEGER CurrentTime);

#endif // _WIN64

#define _vsnprintf_Hash 0xf93b0ec1
typedef int (__cdecl *Fn_vsnprintf)(char *buffer, size_t count, const char *format, va_list argptr);

#define memset_Hash 0x92f2a6aa
typedef void* (__cdecl *Fnmemset)(void* _Dst, __in int _Val, __in size_t _Size);

#define memcpy_Hash 0x945266af
typedef void* (__cdecl *Fnmemcpy)(void* _Dst, const void* _Src, size_t _MaxCount);

#define memmove_Hash 0xd59286b6
typedef void* (__cdecl *Fnmemmove)(void* _Dst, const void* _Src, size_t _MaxCount);

#define strcmp_Hash 0x14e272a9
typedef int (__cdecl *Fnstrcmp)(const char * src, const char * dst);

#define strlen_Hash 0x13e296a7
typedef size_t (__cdecl *Fnstrlen)(const char * str);

#define _stricmp_Hash 0x57ba74c1
typedef int (__cdecl *Fn_stricmp)(const char* _Str1, const char* _Str2);

#define _wcsicmp_Hash 0x57ca52c2
typedef int (__cdecl *Fn_wcsicmp)(__in_z const wchar_t * _Str1, __in_z const wchar_t * _Str2);

#define RtlCompareMemory_Hash 0x9aaf0c5d
typedef size_t (NTAPI *FnRtlCompareMemory)(__in const VOID *Source1, __in const VOID *Source2, __in size_t Length);

#define IoAllocateMdl_Hash 0x683e9a49
typedef PMDL (*FnIoAllocateMdl)(__in_opt __drv_aliasesMem void* VirtualAddress, __in ulong_t Length, __in bool_t SecondaryBuffer, __in bool_t ChargeQuota, __inout_opt PIRP Irp);

#define IoFreeMdl_Hash 0x164a61b0
typedef VOID (*FnIoFreeMdl)(PMDL Mdl);

#define MmProbeAndLockPages_Hash 0xebe708b3
typedef VOID (*FnMmProbeAndLockPages)(__inout PMDLX MemoryDescriptorList, __in KPROCESSOR_MODE AccessMode, __in LOCK_OPERATION Operation);

#define KeStackAttachProcess_Hash 0x67d2b8ed
typedef VOID (*FnKeStackAttachProcess)(__inout PRKPROCESS PROCESS, __out PRKAPC_STATE ApcState);

#define KeUnstackDetachProcess_Hash 0x51d7bbbe
typedef VOID (*FnKeUnstackDetachProcess)(__in PRKAPC_STATE ApcState);

#define MmMapLockedPagesSpecifyCache_Hash 0xe9275cec
typedef void* (*FnMmMapLockedPagesSpecifyCache)(__in PMDLX MemoryDescriptorList, __in __drv_strictType(KPROCESSOR_MODE/enum _MODE,__drv_typeConst) KPROCESSOR_MODE AccessMode, __in __drv_strictTypeMatch(__drv_typeCond) MEMORY_CACHING_TYPE CacheType, __in_opt void* BaseAddress, __in ulong_t BugCheckOnFailure, __in __drv_strictTypeMatch(__drv_typeCond) MM_PAGE_PRIORITY Priority);

#define MmUnmapLockedPages_Hash 0x6f7224f3
typedef VOID (*FnMmUnmapLockedPages)(__in void* BaseAddress, __in PMDL MemoryDescriptorList);

#define MmUnlockPages_Hash 0xe66f154e
typedef VOID (*FnMmUnlockPages)(__inout PMDLX MemoryDescriptorList);

#define ZwCreateFile_Hash 0xb8d72ba8
typedef NTSTATUS (NTAPI *FnZwCreateFile)(__out PHANDLE FileHandle, __in ACCESS_MASK DesiredAccess, __in POBJECT_ATTRIBUTES ObjectAttributes, __out PIO_STATUS_BLOCK IoStatusBlock, __in_opt PLARGE_INTEGER AllocationSize, __in ulong_t FileAttributes, __in ulong_t ShareAccess, __in ulong_t CreateDisposition, __in ulong_t CreateOptions, __in_bcount_opt(EaLength) void* EaBuffer, __in ulong_t EaLength);

#define ZwDeviceIoControlFile_Hash 0x626ad33b
typedef NTSTATUS (NTAPI *FnZwDeviceIoControlFile)(__in HANDLE FileHandle, __in_opt HANDLE Event, __in_opt PIO_APC_ROUTINE ApcRoutine, __in_opt void* ApcContext, __out PIO_STATUS_BLOCK IoStatusBlock, __in ulong_t IoControlCode, __in_bcount_opt(InputBufferLength) void* InputBuffer, __in ulong_t InputBufferLength, __out_bcount_opt(OutputBufferLength) void* OutputBuffer, __in ulong_t OutputBufferLength);

#define ZwClose_Hash 0x9292aab1
typedef NTSTATUS (NTAPI *FnZwClose)(__in HANDLE Handle);

#define ZwWriteFile_Hash 0x38cae3b1
typedef NTSTATUS (NTAPI *FnZwWriteFile)(__in HANDLE FileHandle, __in_opt HANDLE Event, __in_opt PIO_APC_ROUTINE ApcRoutine, __in_opt void* ApcContext, __out PIO_STATUS_BLOCK IoStatusBlock, __in_bcount(Length) void* Buffer, __in ulong_t Length, __in_opt PLARGE_INTEGER ByteOffset, __in_opt ulong_t* Key);

#define ZwReadFile_Hash 0x9652ddac
typedef NTSTATUS (NTAPI *FnZwReadFile)(__in HANDLE FileHandle, __in_opt HANDLE Event, __in_opt PIO_APC_ROUTINE ApcRoutine, __in_opt void* ApcContext, __out PIO_STATUS_BLOCK IoStatusBlock, __out_bcount(Length) void* Buffer, __in ulong_t Length, __in_opt PLARGE_INTEGER ByteOffset, __in_opt ulong_t* Key);

#define ZwQueryInformationFile_Hash 0xab41c18c
typedef NTSTATUS (NTAPI *FnZwQueryInformationFile)(__in HANDLE FileHandle, __out PIO_STATUS_BLOCK IoStatusBlock, __out_bcount(Length) void* FileInformation, __in ulong_t Length, __in FILE_INFORMATION_CLASS FileInformationClass);

#define ZwSetInformationFile_Hash 0xd73937db
typedef NTSTATUS (NTAPI *FnZwSetInformationFile)(__in HANDLE FileHandle, __out PIO_STATUS_BLOCK IoStatusBlock, __in_bcount(Length) void* FileInformation, __in ulong_t Length, __in FILE_INFORMATION_CLASS FileInformationClass);

#define ZwFsControlFile_Hash 0xd9dd40b6
typedef NTSTATUS (NTAPI *FnZwFsControlFile)(__in HANDLE FileHandle, __in_opt HANDLE Event, __in_opt PIO_APC_ROUTINE ApcRoutine, __in_opt void* ApcContext, __out PIO_STATUS_BLOCK IoStatusBlock, __in ulong_t FsControlCode, __in_bcount_opt(InputBufferLength) void* InputBuffer, __in ulong_t InputBufferLength, __out_bcount_opt(OutputBufferLength) void* OutputBuffer, __in ulong_t OutputBufferLength);

#define ZwWaitForSingleObject_Hash 0x3509669c
typedef NTSTATUS (NTAPI *FnZwWaitForSingleObject)(__in HANDLE Handle, __in bool_t Alertable, __in_opt PLARGE_INTEGER Timeout);

#define ZwCreateEvent_Hash 0xf626ff5f
typedef NTSTATUS (NTAPI *FnZwCreateEvent)(__out PHANDLE EventHandle, __in ACCESS_MASK DesiredAccess, __in_opt POBJECT_ATTRIBUTES ObjectAttributes, __in EVENT_TYPE EventType, __in bool_t InitialState);

#define ZwSetEvent_Hash 0x94aaf3d3
typedef NTSTATUS (NTAPI *FnZwSetEvent)(__in HANDLE EventHandle, __out_opt PLONG PreviousState);

#define RtlInitAnsiString_Hash 0xfb4f084c
typedef VOID (NTAPI *FnRtlInitAnsiString)(__out PANSI_STRING DestinationString, __in_z_opt __drv_aliasesMem PCSZ SourceString);

#define RtlAnsiStringToUnicodeString_Hash 0x47775d61
typedef NTSTATUS (NTAPI *FnRtlAnsiStringToUnicodeString)(__drv_when(AllocateDestinationString, __out __drv_at(DestinationString->Buffer, __drv_allocatesMem(Mem))) __drv_when(!AllocateDestinationString, __inout) PUNICODE_STRING DestinationString, __in PCANSI_STRING SourceString, __in bool_t AllocateDestinationString);

#define RtlUnicodeStringToAnsiString_Hash 0x5400d39f
typedef NTSTATUS (NTAPI *FnRtlUnicodeStringToAnsiString)(IN OUT PANSI_STRING DestinationString, IN PUNICODE_STRING SourceString, IN bool_t AllocateDestinationString);

#define RtlInitUnicodeString_Hash 0x4b211890
typedef VOID (NTAPI *FnRtlInitUnicodeString)(__out PUNICODE_STRING DestinationString, __in_z_opt __drv_aliasesMem PCWSTR SourceString);

#define RtlUnicodeStringToInteger_Hash 0x8aaeae70
typedef NTSTATUS (NTAPI *FnRtlUnicodeStringToInteger)(__in PCUNICODE_STRING String, __in_opt ulong_t Base, __out ulong_t* Value);

#define RtlFreeUnicodeString_Hash 0x6b001790
typedef VOID (NTAPI *FnRtlFreeUnicodeString)(__inout __drv_at(UnicodeString->Buffer, __drv_freesMem(Mem)) PUNICODE_STRING UnicodeString);

#define RtlCompareUnicodeString_Hash 0xb73721b0
typedef LONG (NTAPI *FnRtlCompareUnicodeString)(__in PCUNICODE_STRING String1, __in PCUNICODE_STRING String2, __in bool_t CaseInSensitive);

#define ZwOpenKey_Hash 0xd6aa66c3
typedef NTSTATUS (NTAPI *FnZwOpenKey)(__out PHANDLE KeyHandle, __in ACCESS_MASK DesiredAccess, __in POBJECT_ATTRIBUTES ObjectAttributes);

#define ZwQueryValueKey_Hash 0x2924ced7
typedef NTSTATUS (NTAPI *FnZwQueryValueKey)(__in HANDLE KeyHandle, __in PUNICODE_STRING ValueName, __in KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass, __out_bcount_opt(Length) void* KeyValueInformation, __in ulong_t Length, __out ulong_t* ResultLength);

#define PsIsSystemThread_Hash 0x6840f710
typedef bool_t (*FnPsIsSystemThread)(__in PETHREAD Thread);

#define IoGetCurrentProcess_Hash 0xd7884ab0
typedef PEPROCESS (*FnIoGetCurrentProcess)(VOID);

#define PsGetCurrentProcessId_Hash 0x2A4CC432
typedef HANDLE (*FnPsGetCurrentProcessId)(VOID);


#define MmMapIoSpace_Hash 0xf6c6ea38
typedef void* (*FnMmMapIoSpace)(__in PHYSICAL_ADDRESS PhysicalAddress, __in size_t NumberOfBytes, __in MEMORY_CACHING_TYPE CacheType);

#define MmUnmapIoSpace_Hash 0xc764ee43
typedef VOID (*FnMmUnmapIoSpace)(__in_bcount (NumberOfBytes) void* BaseAddress, __in size_t NumberOfBytes);

#define KeInitializeSemaphore_Hash 0x7fb0b2fc
typedef VOID (*FnKeInitializeSemaphore)(__out PRKSEMAPHORE Semaphore, __in LONG Count, __in LONG Limit);

#define KeReleaseSemaphore_Hash 0xdb7424e9
typedef LONG (*FnKeReleaseSemaphore)(__inout PRKSEMAPHORE Semaphore, __in KPRIORITY Increment, __in LONG Adjustment, __in __drv_constant bool_t Wait);

#define KeWaitForSingleObject_Hash 0x350065ac
typedef NTSTATUS (*FnKeWaitForSingleObject)(__in __deref __drv_notPointer void* Object, __in __drv_strictTypeMatch(__drv_typeCond) KWAIT_REASON WaitReason, __in __drv_strictType(KPROCESSOR_MODE/enum _MODE,__drv_typeConst) KPROCESSOR_MODE WaitMode, __in bool_t Alertable, __in_opt PLARGE_INTEGER Timeout);

#define KeInitializeEvent_Hash 0x70261723
typedef VOID (*FnKeInitializeEvent)(__out PRKEVENT Event, __in EVENT_TYPE Type, __in bool_t State);

#define KeClearEvent_Hash 0xb6cf0a4f
typedef VOID (*FnKeClearEvent)(__inout PRKEVENT Event);

#define KeSetEvent_Hash 0xb4aae1d1
typedef LONG (*FnKeSetEvent)(__inout PRKEVENT Event, __in KPRIORITY Increment, __in __drv_constant bool_t Wait);

#define KeInitializeDpc_Hash 0xab189583
typedef VOID (*FnKeInitializeDpc)(__out __drv_aliasesMem PRKDPC Dpc, __in PKDEFERRED_ROUTINE DeferredRoutine, __in_opt __drv_aliasesMem void* DeferredContext);

#define KeSetImportanceDpc_Hash 0xe2dfb7e2
typedef VOID (*FnKeSetImportanceDpc)(__inout PRKDPC Dpc, __in KDPC_IMPORTANCE Importance);

#define KeSetTargetProcessorDpc_Hash 0x2552eb44
typedef VOID (*FnKeSetTargetProcessorDpc)(__inout PRKDPC Dpc, __in CCHAR Number);

#define KeInsertQueueDpc_Hash 0x5200a18a
typedef bool_t (*FnKeInsertQueueDpc)(__inout PRKDPC Dpc, __in_opt void* SystemArgument1, __in_opt void* SystemArgument2);

#define MmIsAddressValid_Hash 0x6fcd4487
typedef bool_t (*FnMmIsAddressValid)(__in void* VirtualAddress);

#define NtQueryInformationProcess_Hash 0xd8328722
typedef NTSTATUS (*FnNtQueryInformationProcess)(HANDLE ProcessHandle, PROCESSINFOCLASS ProcessInformationClass, void* ProcessInformation, ulong_t ProcessInformationLength, ulong_t* ReturnLength);

#define NtQueryInformationThread_Hash 0x13158eb4
typedef NTSTATUS (*FnNtQueryInformationThread)(HANDLE ThreadHandle, THREADINFOCLASS ThreadInformationClass, void* ThreadInformation, ulong_t ThreadInformationLength, ulong_t* ReturnLength);

#define ObOpenObjectByName_Hash 0xb2febb1f
typedef NTSTATUS (*FnObOpenObjectByName)(IN POBJECT_ATTRIBUTES ObjectAttributes, IN POBJECT_TYPE ObjectType OPTIONAL, IN KPROCESSOR_MODE AccessMode, IN OUT PACCESS_STATE AccessState OPTIONAL, IN ACCESS_MASK DesiredAccess OPTIONAL, IN OUT void* ParseContext OPTIONAL, OUT PHANDLE Handle);

#define ObfReferenceObject_Hash 0x888EF1B8
typedef VOID (*FnObfReferenceObject)(IN PVOID  Object);

#define ObOpenObjectByPointer_Hash 0xe5e14a06
typedef NTSTATUS (*FnObOpenObjectByPointer)(void* Object, ulong_t HandleAttributes, PACCESS_STATE PassedAccessState, ACCESS_MASK DesiredAccess, POBJECT_TYPE ObjectType, KPROCESSOR_MODE AccessMode, PHANDLE Handle);

#define ObReferenceObjectByHandle_Hash 0xd189f922
typedef NTSTATUS (*FnObReferenceObjectByHandle)(__in HANDLE Handle, __in ACCESS_MASK DesiredAccess, __in_opt POBJECT_TYPE ObjectType, __in KPROCESSOR_MODE AccessMode, __out void* *Object, __out_opt POBJECT_HANDLE_INFORMATION HandleInformation);

#define ObReferenceObjectByPointer_Hash 0x240a6db8
typedef NTSTATUS (*FnObReferenceObjectByPointer)(__in void* Object, __in ACCESS_MASK DesiredAccess, __in_opt POBJECT_TYPE ObjectType, __in KPROCESSOR_MODE AccessMode);

#define ObfDereferenceObject_Hash 0x08987c9a
typedef LONG_PTR (FASTCALL *FnObfDereferenceObject)(void* Object);

#define IofCallDriver_Hash 0x88baf94e
typedef NTSTATUS (FASTCALL *FnIofCallDriver)(__in PDEVICE_OBJECT DeviceObject, __inout __drv_aliasesMem PIRP Irp);

#define IofCompleteRequest_Hash 0xbb9824b9
typedef VOID (FASTCALL *FnIofCompleteRequest)(__in PIRP Irp, __in CCHAR PriorityBoost);

#define IoCancelIrp_Hash 0x78be2dcf
typedef bool_t (*FnIoCancelIrp)(__in PIRP Irp);

#define NtAllocateVirtualMemory_Hash 0x1ad9af54//0x20dcaf54 -> Zw
typedef NTSTATUS (*FnNtAllocateVirtualMemory)(HANDLE ProcessHandle, void**BaseAddress, ULONG_PTR ZeroBits, PSIZE_T RegionSize, ulong_t AllocationType, ulong_t Protect);

#define ZwFreeVirtualMemory_Hash 0x72732883
typedef NTSTATUS (*FnZwFreeVirtualMemory)(HANDLE ProcessHandle, void** BaseAddress, PSIZE_T RegionSize, ulong_t FreeType);

#define ZwOpenDirectoryObject_Hash 0xa5117901
typedef NTSTATUS (*FnZwOpenDirectoryObject)(__out PHANDLE DirectoryHandle, __in ACCESS_MASK DesiredAccess, __in POBJECT_ATTRIBUTES ObjectAttributes);

#define ZwOpenSymbolicLinkObject_Hash 0xd7263d09
typedef NTSTATUS (*FnZwOpenSymbolicLinkObject)(__out PHANDLE LinkHandle, __in ACCESS_MASK DesiredAccess, __in POBJECT_ATTRIBUTES ObjectAttributes);

#define ZwQuerySymbolicLinkObject_Hash 0xa9233d42
typedef NTSTATUS (*FnZwQuerySymbolicLinkObject)(__in HANDLE LinkHandle, __inout PUNICODE_STRING LinkTarget, __out_opt ulong_t* ReturnedLength);

#define IoBuildDeviceIoControlRequest_Hash 0x607567b5
typedef PIRP (*FnIoBuildDeviceIoControlRequest)(__in ulong_t IoControlCode, __in PDEVICE_OBJECT DeviceObject, __in_opt void* InputBuffer, __in ulong_t InputBufferLength, __out_opt void* OutputBuffer, __in ulong_t OutputBufferLength, __in bool_t InternalDeviceIoControl, __in PKEVENT Event, __out PIO_STATUS_BLOCK IoStatusBlock);

#define ZwQueryDirectoryObject_Hash 0x490b7973
typedef NTSTATUS (*FnZwQueryDirectoryObject)(IN HANDLE DirectoryHandle, OUT void* Buffer, IN ulong_t BufferLength, IN bool_t ReturnSingleEntry, IN bool_t RestartScan, IN OUT ulong_t* Context, OUT ulong_t* ReturnLength OPTIONAL);

#define KeSetBasePriorityThread_Hash 0xeb454106
typedef LONG (*FnKeSetBasePriorityThread)(__inout PKTHREAD Thread, __in LONG Increment);

#define KeQueryTickCount_Hash 0x1fa54a9d
typedef VOID (*FnKeQueryTickCount)(PLARGE_INTEGER CurrentCount);

#define KeQueryTimeIncrement_Hash 0xf6c39cf9
typedef ulong_t (*FnKeQueryTimeIncrement)(VOID);

#define KeInitializeApc_Hash 0xab188983
typedef VOID (*FnKeInitializeApc)(PKAPC Apc, PKTHREAD Thread, uint8_t StateIndex, PKKERNEL_ROUTINE KernelRoutine, PKRUNDOWN_ROUTINE RundownRoutine, PKNORMAL_ROUTINE NormalRoutine, KPROCESSOR_MODE ApcMode, void* NormalContext);

#define KeInsertQueueApc_Hash 0x5200958a
typedef bool_t (*FnKeInsertQueueApc)(PKAPC Apc, void* SystemArgument1, void* SystemArgument2, KPRIORITY Increment);

#define PsSetCreateProcessNotifyRoutine_Hash 0x2cc2271a
typedef NTSTATUS (*FnPsSetCreateProcessNotifyRoutine)(__in PCREATE_PROCESS_NOTIFY_ROUTINE NotifyRoutine, __in bool_t Remove);

#define PsLookupProcessByProcessId_Hash 0x2ea78826
typedef NTSTATUS (*FnPsLookupProcessByProcessId)(__in HANDLE ProcessId, __deref_out PEPROCESS *Process);

#define KeInitializeMutex_Hash 0xef865326
typedef VOID (*FnKeInitializeMutex)(__out PRKMUTEX Mutex, __in ulong_t Level);

#define KeReleaseMutex_Hash 0x26a528e3
typedef LONG (*FnKeReleaseMutex)(__inout PRKMUTEX Mutex, __in bool_t Wait);

#define IoGetDeviceObjectPointer_Hash 0x901130cb
typedef NTSTATUS (*FnIoGetDeviceObjectPointer)(__in PUNICODE_STRING ObjectName, __in ACCESS_MASK DesiredAccess, __out PFILE_OBJECT *FileObject, __out PDEVICE_OBJECT *DeviceObject);

#define IoBuildSynchronousFsdRequest_Hash 0x7582fead
typedef PIRP (*FnIoBuildSynchronousFsdRequest)(__in ulong_t MajorFunction, __in PDEVICE_OBJECT DeviceObject, __inout_opt void* Buffer, __in_opt ulong_t Length,
                             __in_opt PLARGE_INTEGER StartingOffset, __in PKEVENT Event, __out PIO_STATUS_BLOCK IoStatusBlock);

#define IoGetConfigurationInformation_Hash 0x75b4c113
typedef PCONFIGURATION_INFORMATION (*FnIoGetConfigurationInformation)(VOID);

#define RtlQueryRegistryValues_Hash 0x81265c81
typedef NTSTATUS (*FnRtlQueryRegistryValues)(__in ulong_t RelativeTo, __in PCWSTR Path,
                                             __inout __drv_at(*(*QueryTable).EntryContext, __out)PRTL_QUERY_REGISTRY_TABLE QueryTable,
                                             __in_opt void* Context, __in_opt void* Environment);

#define ObReferenceObjectByName_Hash 0x2f510282
typedef NTSTATUS (NTAPI *FnObReferenceObjectByName)(IN PUNICODE_STRING ObjectName, IN ulong_t Attributes, IN PACCESS_STATE PassedAccessState, IN ACCESS_MASK DesiredAccess, IN POBJECT_TYPE ObjectType, IN KPROCESSOR_MODE AccessMode, IN OUT void* ParseContext, OUT void** Object);

#define ExSystemTimeToLocalTime_Hash 0xaeafcdf2
typedef VOID (*FnExSystemTimeToLocalTime)(__in PLARGE_INTEGER SystemTime, __out PLARGE_INTEGER LocalTime);

// #define RtlTimeToTimeFields_Hash 0x917cf893
// typedef VOID (NTAPI *FnRtlTimeToTimeFields)(__in PLARGE_INTEGER Time, __out PTIME_FIELDS TimeFields);

#define RtlTimeToSecondsSince1970_Hash 0x8990e26e
typedef bool_t (NTAPI *FnRtlTimeToSecondsSince1970)(__in PLARGE_INTEGER Time, __out ulong_t* ElapsedSeconds);

#define IoRegisterShutdownNotification_Hash 0xd69b821b
typedef NTSTATUS (*FnIoRegisterShutdownNotification)(__in PDEVICE_OBJECT DeviceObject);

#define IoUnregisterShutdownNotification_Hash 0x025dba34
typedef VOID (*FnIoUnregisterShutdownNotification)(__in PDEVICE_OBJECT DeviceObject);

#define CcCopyWrite_Hash 0x7876d9c6
typedef bool_t (*FnCcCopyWrite)(__in PFILE_OBJECT FileObject, __in PLARGE_INTEGER FileOffset, __in ulong_t Length, __in bool_t Wait, __in_bcount(Length) void* Buffer);

#define IoGetAttachedDeviceReference_Hash 0x372FD3E7
typedef PDEVICE_OBJECT (*FnIoGetAttachedDeviceReference)(__in PDEVICE_OBJECT DeviceObject);

#define IoGetDeviceAttachmentBaseRef_Hash 0x69F8431E
typedef PDEVICE_OBJECT (*FnIoGetDeviceAttachmentBaseRef)(__in PDEVICE_OBJECT DeviceObject);

#define IoAllocateIrp_Hash 0x69FE8A4D
typedef PIRP (*FnIoAllocateIrp)(__in CCHAR StackSize, __in bool_t ChargeQuota);

#define IoFreeIrp_Hash 0x180A51B4
typedef VOID (*FnIoFreeIrp)(__in PIRP Irp);

#define KeInitializeTimer_Hash 0xEFF6371A
typedef VOID (*FnKeInitializeTimer)(IN PKTIMER  Timer);

#define KeSetTimerEx_Hash 0xF4A72549
typedef BOOLEAN (*FnKeSetTimerEx)(IN PKTIMER Timer, IN LARGE_INTEGER DueTime, IN LONG Period OPTIONAL, IN PKDPC Dpc OPTIONAL);

#define KeCancelTimer_Hash 0xA6072D50
typedef BOOLEAN (*FnKeCancelTimer)(IN PKTIMER Timer);

#define IoRegisterLastChanceShutdownNotification_Hash 0x559211C1
typedef NTSTATUS (*FnIoRegisterLastChanceShutdownNotification)(IN PDEVICE_OBJECT  DeviceObject);


#ifdef _WIN64
	#define KeAddSystemServiceTable_Hash 0x5b26ddb9
#endif

#define PsProcessType_Hash 0x998b733b
#define PsThreadType_Hash 0xD99F36B2
#define IoDeviceObjectType_Hash 0xeb82609a
#define IoDriverObjectType_Hash 0xbb9c64db
#define IoFileObjectType_Hash 0x12b35e7b

typedef enum _SYSTEM_INFORMATION_CLASS
{
    SystemBasicInformation,
    SystemProcessorInformation,
    SystemPerformanceInformation,
    SystemTimeOfDayInformation, 
    SystemNotImplemented1,
    SystemProcessesAndThreadsInformation,
    SystemCallCounts, 
    SystemConfigurationInformation, 
    SystemProcessorTimes, 
    SystemGlobalFlag, 
    SystemNotImplemented2, 
    SystemModuleInformation, 
    SystemLockInformation,
    SystemNotImplemented3, 
    SystemNotImplemented4, 
    SystemNotImplemented5, 
    SystemHandleInformation, 
    SystemObjectInformation, 
    SystemPagefileInformation, 
    SystemInstructionEmulationCounts, 
    SystemInvalidInfoClass1, 
    SystemCacheInformation, 
    SystemPoolTagInformation, 
    SystemProcessorStatistics,
    SystemDpcInformation, 
    SystemNotImplemented6,
    SystemLoadImage, 
    SystemUnloadImage, 
    SystemTimeAdjustment, 
    SystemNotImplemented7, 
    SystemNotImplemented8, 
    SystemNotImplemented9,
    SystemCrashDumpInformation, 
    SystemExceptionInformation, 
    SystemCrashDumpStateInformation, 
    SystemKernelDebuggerInformation, 
    SystemContextSwitchInformation, 
    SystemRegistryQuotaInformation, 
    SystemLoadAndCallImage,
    SystemPrioritySeparation, 
    SystemNotImplemented10,
    SystemNotImplemented11, 
    SystemInvalidInfoClass2, 
    SystemInvalidInfoClass3, 
    SystemTimeZoneInformation, 
    SystemLookasideInformation, 
    SystemSetTimeSlipEvent,
    SystemCreateSession,
    SystemDeleteSession, 
    SystemInvalidInfoClass4, 
    SystemRangeStartInformation, 
    SystemVerifierInformation, 
    SystemAddVerifier, 
    SystemSessionProcessesInformation 
} SYSTEM_INFORMATION_CLASS;

#if _AMD64_
typedef ulong_t SYSINF_PAGE_COUNT;
#else
typedef size_t SYSINF_PAGE_COUNT;
#endif

typedef struct _SYSTEM_BASIC_INFORMATION
{
	ulong_t Reserved;
	ulong_t TimerResolution;
	ulong_t PageSize;
	SYSINF_PAGE_COUNT NumberOfPhysicalPages;
	SYSINF_PAGE_COUNT LowestPhysicalPageNumber;
	SYSINF_PAGE_COUNT HighestPhysicalPageNumber;
	ulong_t AllocationGranularity;
	ULONG_PTR MinimumUserModeAddress;
	ULONG_PTR MaximumUserModeAddress;
	ULONG_PTR ActiveProcessorsAffinityMask;
	CCHAR NumberOfProcessors;
} SYSTEM_BASIC_INFORMATION, *PSYSTEM_BASIC_INFORMATION;

typedef struct _SYSTEM_MODULE_INFORMATION
{
	void*  Reserved[2];
	void*  Base;
	ulong_t  Size;
	ulong_t  Flags;
	uint16_t Index;
	uint16_t NameLength;
	uint16_t LoadCount;
	uint16_t PathLength;
	char  ImageName[256];
} SYSTEM_MODULE_INFORMATION, *PSYSTEM_MODULE_INFORMATION;

typedef struct _SYSTEM_MODULE_INFORMATIONS
{
	ulong_t dwNum;
	SYSTEM_MODULE_INFORMATION modinfo[1];
} SYSTEM_MODULE_INFORMATIONS, *PSYSTEM_MODULE_INFORMATIONS;

#define NtQuerySystemInformation_Hash 0xdc19d0fe
typedef NTSTATUS (*FnNtQuerySystemInformation)(SYSTEM_INFORMATION_CLASS SystemInformationClass, void* SystemInformation, ulong_t SystemInformationLength, ulong_t* ReturnLength);

typedef struct ServiceDescriptorEntry
{
	uint8_t* KiServiceTable;
	ulong_t* CounterBaseTable;
	ulong_t nSystemCalls;
	ulong_t* KiArgumentTable;
} SDE, *PSDE;

#define KeServiceDescriptorTable_Hash 0x4faa7513

typedef struct _INITIAL_TEB
{
	struct {
		void* OldStackBase;
		void* OldStackLimit;
	} OldInitialTeb;
	void* StackBase;
	void* StackLimit;
	void* StackAllocationBase;
} INITIAL_TEB, *PINITIAL_TEB;

#define KfAcquireSpinLock_Hash 0x98903e85
#ifdef _WIN64
#define KeAcquireSpinLockRaiseToDpc_Hash 0x90a07e02
#endif
typedef KIRQL (FASTCALL *FnKfAcquireSpinLock)(__inout __deref __drv_acquiresExclusiveResource(KeSpinLockType) PKSPIN_LOCK SpinLock);

#define KfReleaseSpinLock_Hash 0x68743ac5
#ifdef _WIN64
#define KeReleaseSpinLock_Hash 0x60743ac5
#endif
typedef VOID (FASTCALL *FnKfReleaseSpinLock)(__inout __deref __drv_releasesExclusiveResource(KeSpinLockType) PKSPIN_LOCK SpinLock, __in __drv_restoresIRQL KIRQL NewIrql);

#define KfLowerIrql_Hash 0x19e6debb
typedef VOID (FASTCALL *FnKfLowerIrql)(__in __drv_restoresIRQL __drv_nonConstant KIRQL NewIrql);

#define KfRaiseIrql_Hash 0x98a700b7
typedef KIRQL (FASTCALL *FnKfRaiseIrql)(__in KIRQL NewIrql);

#define KeGetCurrentIrql_Hash 0x228d6a01
typedef KIRQL (NTAPI *FnKeGetCurrentIrql)(VOID);

#define MmMapLockedPages_Hash 0x2ec51b13
typedef void* (*FnMmMapLockedPages)(PMDL MemoryDescriptorList, KPROCESSOR_MODE AccessMode);

#define MmBuildMdlForNonPagedPool_Hash 0xe16fa2e0
typedef VOID (*FnMmBuildMdlForNonPagedPool)(__inout PMDLX MemoryDescriptorList);

#define DbgPrint_Hash 0x94e272c5
typedef ulong_t (__cdecl *FnDbgPrint)(PCSTR Format, ...);

#define mmGetSystemAddressForMdl(MDL)                                  \
	(((MDL)->MdlFlags & (MDL_MAPPED_TO_SYSTEM_VA |                    \
	MDL_SOURCE_IS_NONPAGED_POOL)) ?                \
	((MDL)->MappedSystemVa) :                 \
	(pGlobalBlock->pCommonBlock->fnMmMapLockedPages((MDL),KernelMode)))

#define mmGetSystemAddressForMdlSafe(MDL, PRIORITY)                    \
	(((MDL)->MdlFlags & (MDL_MAPPED_TO_SYSTEM_VA |                    \
	MDL_SOURCE_IS_NONPAGED_POOL)) ?                \
	((MDL)->MappedSystemVa) :                 \
	(pGlobalBlock->pCommonBlock->fnMmMapLockedPagesSpecifyCache((MDL),      \
	KernelMode, \
	MmCached,   \
	NULL,       \
	FALSE,      \
	(PRIORITY))))

typedef NTSTATUS (*FnMajorFunction)(PDEVICE_OBJECT pDeviceObject, PIRP pIrp);

#endif // __NTKERNELAPI_H_