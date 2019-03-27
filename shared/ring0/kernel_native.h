#ifndef __SHARED__RING0__NATIVE_H_
#define __SHARED__RING0__NATIVE_H_

/** В данном файле определены стандартные константы для всех kernel-mode функций. Если же, определённые
	константы необходимо переопределить, то это необходимо сделать до подключения данного файла.
*/

#define ALLOCATOR_TAG 0xAABBCCDD

#ifndef SYS_ALLOCATOR
#define SYS_ALLOCATOR(sz) ExAllocatePoolWithTag(NonPagedPool, sz, ALLOCATOR_TAG)
#endif // SYS_ALLOCATOR

#ifndef SYS_DEALLOCATOR
#define SYS_DEALLOCATOR(ptr) ExFreePoolWithTag(ptr, ALLOCATOR_TAG)
#endif // SYS_DEALLOCATOR

#ifndef MEMCPY
#define MEMCPY RtlCopyMemory
#endif // MEMCPY

#ifndef MEMCMP
#define MEMCMP(dest, src, size) RtlCompareMemory(dest, src, size) == size
#endif // MEMCMP

#ifndef STRLEN
#define STRLEN strlen
#endif // STRLEN

#ifndef STRCMP
#define STRCMP strcmp
#endif // STRCMP

#ifndef STRCPY
#define STRCPY strcpy
#endif // STRCPY

#ifndef RTL_INIT_ANSI_STRING
#define RTL_INIT_ANSI_STRING RtlInitAnsiString
#endif // RTL_INIT_ANSI_STRING

#ifndef RTL_ANSI_STRING_TO_UNICODE_STRING
#define RTL_ANSI_STRING_TO_UNICODE_STRING RtlAnsiStringToUnicodeString
#endif // RTL_ANSI_STRING_TO_UNICODE_STRING

#ifndef RTL_FREE_UNICODE_STRING
#define RTL_FREE_UNICODE_STRING RtlFreeUnicodeString
#endif // RTL_FREE_UNICODE_STRING

#ifndef NT_QUERY_INFORMATION_PROCESS
#define NT_QUERY_INFORMATION_PROCESS NtQueryInformationProcess
#endif // NT_QUERY_INFORMATION_PROCESS

#ifndef MM_IS_ADDRESS_VALID
#define MM_IS_ADDRESS_VALID MmIsAddressValid
#endif // MM_IS_ADDRESS_VALID

#ifndef EX_ALLOCATE_POOL_WITH_TAG
#define EX_ALLOCATE_POOL_WITH_TAG ExAllocatePoolWithTag
#endif // EX_ALLOCATE_POOL_WITH_TAG

#ifndef EX_FREE_POOL_WITH_TAG
#define EX_FREE_POOL_WITH_TAG ExFreePoolWithTag
#endif // EX_FREE_POOL_WITH_TAG

#ifndef KE_STACK_ATTACH_PROCESS
#define KE_STACK_ATTACH_PROCESS KeStackAttachProcess
#endif // KE_STACK_ATTACH_PROCESS

#ifndef KE_UNSTACK_DETACH_PROCESS
#define KE_UNSTACK_DETACH_PROCESS KeUnstackDetachProcess
#endif // KE_UNSTACK_DETACH_PROCESS

#ifndef KE_INITIALIZE_APC
#define KE_INITIALIZE_APC KeInitializeApc
#endif // KE_INITIALIZE_APC

#ifndef KE_INSERT_QUEUE_APC
#define KE_INSERT_QUEUE_APC KeInsertQueueApc
#endif // KE_INSERT_QUEUE_APC

#ifndef PS_WRAP_APC_WOW64_THREAD
#define PS_WRAP_APC_WOW64_THREAD PsWrapApcWow64Thread
#endif // PS_WRAP_APC_WOW64_THREAD

#ifndef NT_ALLOCATE_VIRTUAL_MEMORY
#define NT_ALLOCATE_VIRTUAL_MEMORY NtAllocateVirtualMemory
#endif // NT_ALLOCATE_VIRTUAL_MEMORY

#ifndef ZW_CREATE_FILE
#define ZW_CREATE_FILE ZwCreateFile
#endif // ZW_CREATE_FILE

#ifndef ZW_READ_FILE
#define ZW_READ_FILE ZwReadFile
#endif // ZW_READ_FILE

#ifndef ZW_WRITE_FILE
#define ZW_WRITE_FILE ZwWriteFile
#endif // ZW_WRITE_FILE

#ifndef ZW_CLOSE
#define ZW_CLOSE ZwClose
#endif // ZW_CLOSE

#ifndef ZW_QUERY_INFORMATION_FILE
#define ZW_QUERY_INFORMATION_FILE ZwQueryInformationFile
#endif // ZW_QUERY_INFORMATION_FILE

#ifndef FN_STRICMP
#define FN_STRICMP _stricmp
#endif // FN_STRICMP

#ifndef FN_RTL_STRING_CCH_PRINTFA
#define FN_RTL_STRING_CCH_PRINTFA RtlStringCchPrintfA
#endif // FN_RTL_STRING_CCH_PRINTFA

#include "../native.h"

#endif // __SHARED_CODE__RING0__NATIVE_H_
