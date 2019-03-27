#ifndef __MODSHARED_NETWORKAPI_H_
#define __MODSHARED_NETWORKAPI_H_

#include <ndis.h>

// NDIS defines and functions

#define NET_BUFFER_LIST_POOL_PARAMETERS_REVISION_1 1

#define NDIS_RECEIVE_FLAGS_DISPATCH_LEVEL               0x00000001
#define NDIS_RECEIVE_FLAGS_RESOURCES                    0x00000002

#define NDIS_RETURN_FLAGS_DISPATCH_LEVEL               0x00000001

#define NDIS_TEST_RECEIVE_FLAG(_Flags, _Fl)         (((_Flags) & (_Fl)) == (_Fl))
//#define NDIS_SET_RECEIVE_FLAG(_Flags, _Fl)          ((_Flags) |= (_Fl))

#define NDIS_SET_RETURN_FLAG(_Flags, _Fl)           ((_Flags) |= (_Fl))

#define NDIS_TEST_RECEIVE_AT_DISPATCH_LEVEL(_Flags)      \
	NDIS_TEST_RECEIVE_FLAG((_Flags), NDIS_RECEIVE_FLAGS_DISPATCH_LEVEL)


#define NDIS_TEST_RECEIVE_CAN_PEND(_Flags)         \
	(((_Flags) & NDIS_RECEIVE_FLAGS_RESOURCES) == 0)

#define NET_BUFFER_LIST_FIRST_NB(_NBL)              ((_NBL)->FirstNetBuffer)
#define NET_BUFFER_LIST_NEXT_NBL(_NBL)              ((_NBL)->Next)
#define NET_BUFFER_FIRST_MDL(_NB)                   ((_NB)->MdlChain)
#define NET_BUFFER_DATA_OFFSET(_NB)                 ((_NB)->DataOffset)
#define NET_BUFFER_DATA_LENGTH(_NB)                 ((_NB)->DataLength)
#define NET_BUFFER_NEXT_NB(_NB)                     ((_NB)->Next)

typedef struct _NET_BUFFER NET_BUFFER, *PNET_BUFFER;
typedef struct _NET_BUFFER_LIST_CONTEXT NET_BUFFER_LIST_CONTEXT, *PNET_BUFFER_LIST_CONTEXT;
typedef struct _NET_BUFFER_LIST NET_BUFFER_LIST, *PNET_BUFFER_LIST;
typedef PHYSICAL_ADDRESS NDIS_PHYSICAL_ADDRESS, *PNDIS_PHYSICAL_ADDRESS;


typedef union _NET_BUFFER_DATA_LENGTH
{
	ulong_t   DataLength;
	SIZE_T  stDataLength;
} NET_BUFFER_DATA_LENGTH, *PNET_BUFFER_DATA_LENGTH;


typedef struct _NET_BUFFER_DATA
{
	PNET_BUFFER Next;
	PMDL        CurrentMdl;
	ulong_t       CurrentMdlOffset;
	NET_BUFFER_DATA_LENGTH;
	PMDL        MdlChain;
	ulong_t       DataOffset;
} NET_BUFFER_DATA, *PNET_BUFFER_DATA;

typedef union _NET_BUFFER_HEADER
{
	NET_BUFFER_DATA;
	SLIST_HEADER    Link;

} NET_BUFFER_HEADER, *PNET_BUFFER_HEADER;

typedef struct _NET_BUFFER
{
	NET_BUFFER_HEADER;

	USHORT          ChecksumBias;
	USHORT          Reserved;
	NDIS_HANDLE     NdisPoolHandle;
	DECLSPEC_ALIGN(MEMORY_ALLOCATION_ALIGNMENT)pvoid_t NdisReserved[2];
	DECLSPEC_ALIGN(MEMORY_ALLOCATION_ALIGNMENT)pvoid_t ProtocolReserved[6];
	DECLSPEC_ALIGN(MEMORY_ALLOCATION_ALIGNMENT)pvoid_t MiniportReserved[4];
	NDIS_PHYSICAL_ADDRESS   DataPhysicalAddress;
#if (NDIS_SUPPORT_NDIS620)
	union
	{
		PNET_BUFFER_SHARED_MEMORY   SharedMemoryInfo;
		PSCATTER_GATHER_LIST        ScatterGatherList;
	};
#endif
}NET_BUFFER, *PNET_BUFFER;


typedef struct _NET_BUFFER_LIST_CONTEXT
{
	PNET_BUFFER_LIST_CONTEXT    Next;
	USHORT                      Size;
	USHORT                      Offset;
	DECLSPEC_ALIGN(MEMORY_ALLOCATION_ALIGNMENT)     UCHAR      ContextData[];
} NET_BUFFER_LIST_CONTEXT, *PNET_BUFFER_LIST_CONTEXT;


typedef struct _NET_BUFFER_LIST_DATA
{
	PNET_BUFFER_LIST    Next;           // Next NetBufferList in the chain
	PNET_BUFFER         FirstNetBuffer; // First NetBuffer on this NetBufferList
} NET_BUFFER_LIST_DATA, *PNET_BUFFER_LIST_DATA;

typedef union _NET_BUFFER_LIST_HEADER
{
	NET_BUFFER_LIST_DATA;
	SLIST_HEADER            Link;           // used in SLIST of free NetBuffers in the block
} NET_BUFFER_LIST_HEADER, *PNET_BUFFER_LIST_HEADER;


typedef enum _NDIS_NET_BUFFER_LIST_INFO
{
	TcpIpChecksumNetBufferListInfo,
	TcpOffloadBytesTransferred = TcpIpChecksumNetBufferListInfo,
	IPsecOffloadV1NetBufferListInfo,
#if (NDIS_SUPPORT_NDIS61)
	IPsecOffloadV2NetBufferListInfo = IPsecOffloadV1NetBufferListInfo,
#endif // (NDIS_SUPPORT_NDIS61)
	TcpLargeSendNetBufferListInfo,
	TcpReceiveNoPush = TcpLargeSendNetBufferListInfo,
	ClassificationHandleNetBufferListInfo,
	Ieee8021QNetBufferListInfo,
	NetBufferListCancelId,
	MediaSpecificInformation,
	NetBufferListFrameType,
	NetBufferListProtocolId = NetBufferListFrameType,
	NetBufferListHashValue,
	NetBufferListHashInfo,
	WfpNetBufferListInfo,
#if (NDIS_SUPPORT_NDIS61)
	IPsecOffloadV2TunnelNetBufferListInfo,
	IPsecOffloadV2HeaderNetBufferListInfo,
#endif  // (NDIS_SUPPORT_NDIS61)

#if (NDIS_SUPPORT_NDIS620)
	NetBufferListCorrelationId,
	NetBufferListFilteringInfo,


	MediaSpecificInformationEx,
	NblOriginalInterfaceIfIndex,
	NblReAuthWfpFlowContext = NblOriginalInterfaceIfIndex,
	TcpReceiveBytesTransferred,
#endif // (NDIS_SUPPORT_NDIS620)

	MaxNetBufferListInfo
} NDIS_NET_BUFFER_LIST_INFO, *PNDIS_NET_BUFFER_LIST_INFO;


typedef struct _NET_BUFFER_LIST
{
	NET_BUFFER_LIST_HEADER;
	PNET_BUFFER_LIST_CONTEXT    Context;
	PNET_BUFFER_LIST            ParentNetBufferList;
	NDIS_HANDLE                 NdisPoolHandle;
	DECLSPEC_ALIGN(MEMORY_ALLOCATION_ALIGNMENT)pvoid_t NdisReserved[2];
	DECLSPEC_ALIGN(MEMORY_ALLOCATION_ALIGNMENT)pvoid_t ProtocolReserved[4];
	DECLSPEC_ALIGN(MEMORY_ALLOCATION_ALIGNMENT)pvoid_t MiniportReserved[2];
	pvoid_t                       Scratch;
	NDIS_HANDLE                 SourceHandle;
	ulong_t                       NblFlags;   // public flags
	LONG                        ChildRefCount;
	ulong_t                       Flags;      // private flags used by NDIs, protocols, miniport, etc.
	NDIS_STATUS                 Status;
	pvoid_t                       NetBufferListInfo[MaxNetBufferListInfo];
} NET_BUFFER_LIST, *PNET_BUFFER_LIST;

typedef struct _NET_BUFFER_LIST NET_BUFFER_LIST, *PNET_BUFFER_LIST;
typedef ulong_t NDIS_PORT_NUMBER, *PNDIS_PORT_NUMBER;

typedef struct _NET_BUFFER_LIST_POOL_PARAMETERS
{
	//
	// Set ObjectHeader.Type to NDIS_OBJECT_TYPE_DEFAULT
	//
	NDIS_OBJECT_HEADER      Header;
	UCHAR                   ProtocolId;
	BOOLEAN                 fAllocateNetBuffer;
	USHORT                  ContextSize;
	ulong_t                   PoolTag;
	ulong_t                   DataSize;
}NET_BUFFER_LIST_POOL_PARAMETERS, *PNET_BUFFER_LIST_POOL_PARAMETERS;

typedef VOID (*FnNdisMSendNetBufferListsComplete)(pvoid_t SendNBLsCompleteContext, PNET_BUFFER_LIST NetBufferList, ulong_t SendCompleteFlags);


typedef pvoid_t (*FnNdisGetRoutineAddress)(__in PNDIS_STRING NdisRoutineName);

#define NdisGetPoolFromPacket_Hash 0x54f42695
__drv_maxIRQL(DISPATCH_LEVEL)
typedef NDIS_HANDLE (*FnNdisGetPoolFromPacket)(__in PNDIS_PACKET Packet);

#define NdisUnchainBufferAtFront_Hash 0xcad2f493
typedef VOID (*FnNdisUnchainBufferAtFront)(IN OUT PNDIS_PACKET Packet, OUT PNDIS_BUFFER* Buffer);

#define ndisFreeBuffer(Buffer) pGlobalBlock->pCommonBlock->fnIoFreeMdl(Buffer)

#define NdisFreePacket_Hash 0xd842f34d
__drv_maxIRQL(DISPATCH_LEVEL)
typedef VOID (*FnNdisFreePacket)(__in PNDIS_PACKET Packet);

#define NdisDprFreePacket_Hash 0xf8d7110e
__drv_maxIRQL(DISPATCH_LEVEL) 
__drv_minIRQL(DISPATCH_LEVEL)
typedef VOID (*FnNdisDprFreePacket)(__in PNDIS_PACKET Packet);

#define NdisAllocatePacket_Hash 0x40f50a55
__drv_maxIRQL(DISPATCH_LEVEL)
typedef VOID (*FnNdisAllocatePacket)(__out PNDIS_STATUS Status, __out PNDIS_PACKET* Packet, __in NDIS_HANDLE PoolHandle);

#define NdisFreeMemory_Hash 0xda22fd57
typedef VOID (*FnNdisFreeMemory)(__in pvoid_t VirtualAddress, __in UINT Length, __in UINT MemoryFlags);

#define NdisAllocateBufferPool_Hash 0xb32ddfe1
__drv_maxIRQL(DISPATCH_LEVEL)
typedef VOID (*FnNdisAllocateBufferPool)(__out PNDIS_STATUS Status, __out PNDIS_HANDLE PoolHandle, __in UINT NumberOfDescriptors);

#define NdisFreeBufferPool_Hash 0x42b754c0
__drv_maxIRQL(DISPATCH_LEVEL)
typedef VOID (*FnNdisFreeBufferPool)(__in NDIS_HANDLE PoolHandle);

#define NdisAllocatePacketPool_Hash 0xb335cbe4
__drv_maxIRQL(DISPATCH_LEVEL)
typedef VOID (*FnNdisAllocatePacketPool)(__out PNDIS_STATUS Status, __out PNDIS_HANDLE PoolHandle, __in UINT NumberOfDescriptors, __in UINT ProtocolReservedLength);

#define NdisFreePacketPool_Hash 0x42bf40c3
__drv_maxIRQL(DISPATCH_LEVEL)
typedef VOID (*FnNdisFreePacketPool)(__in NDIS_HANDLE PoolHandle);

#define NdisAllocateBuffer_Hash 0xc234da54
__drv_maxIRQL(DISPATCH_LEVEL)
typedef VOID (*FnNdisAllocateBuffer)(__out PNDIS_STATUS Status, __out PNDIS_BUFFER* Buffer, __in_opt NDIS_HANDLE PoolHandle, __in_bcount(Length) pvoid_t VirtualAddress, __in UINT Length);


#define NdisAllocateMemoryWithTag_Hash 0x765c3959
__drv_maxIRQL(DISPATCH_LEVEL)
typedef NDIS_STATUS (*FnNdisAllocateMemoryWithTag)(__deref_out_bcount_opt(Length) __drv_allocatesMem(Mem) pvoid_t* VirtualAddress, __in UINT Length, __in ulong_t Tag);

#define NdisRequest_Hash 0x9a42c7cf
__drv_maxIRQL(DISPATCH_LEVEL)
typedef VOID (*FnNdisRequest)(__out PNDIS_STATUS Status, __in NDIS_HANDLE NdisBindingHandle, __in PNDIS_REQUEST NdisRequest);


#define FnNdis5ChainBufferAtFront(Packet, Buffer)                              \
{                                                                           \
	PNDIS_BUFFER TmpBuffer = (Buffer);                                      \
	\
	for (;;)                                                                \
{                                                                       \
	if (TmpBuffer->Next == (PNDIS_BUFFER)NULL)                          \
	break;                                                          \
	TmpBuffer = TmpBuffer->Next;                                        \
}                                                                       \
	if ((Packet)->Private.Head == NULL)                                     \
{                                                                       \
	(Packet)->Private.Tail = TmpBuffer;                                 \
}                                                                       \
	TmpBuffer->Next = (Packet)->Private.Head;                               \
	(Packet)->Private.Head = (Buffer);                                      \
	(Packet)->Private.ValidCounts = FALSE;                                  \
}

#define ndisQueryBuffer(_Buffer, _VirtualAddress, _Length)                  \
{                                                                           \
	if (ARGUMENT_PRESENT(_VirtualAddress))                                  \
{                                                                       \
	*(pvoid_t *)(_VirtualAddress) = mmGetSystemAddressForMdl(_Buffer);    \
}                                                                       \
	*(_Length) = MmGetMdlByteCount(_Buffer);                                \
}

#define ndisQueryBufferSafe(_Buffer, _VirtualAddress, _Length, _Priority)   \
{                                                                           \
	if (ARGUMENT_PRESENT(_VirtualAddress))                                  \
{                                                                       \
	*(pvoid_t *)(_VirtualAddress) = mmGetSystemAddressForMdlSafe(_Buffer, _Priority); \
}                                                                       \
	*(_Length) = MmGetMdlByteCount(_Buffer);                                \
}

#define ndisGetNextBuffer(CurrentBuffer, NextBuffer)                        \
{                                                                           \
	*(NextBuffer) = (CurrentBuffer)->Next;                                  \
}

__drv_ret(__drv_allocatesMem(mem))
__drv_maxIRQL(DISPATCH_LEVEL)
typedef pvoid_t (*FnNdisMiniportSendPackets)(NDIS_HANDLE BindingMiniport, PNDIS_PACKET* PacketArray, UINT NumberOfPackets);

typedef VOID (*FnMiniportReturnPacket)(IN NDIS_HANDLE MiniportAdapterContext, IN PNDIS_PACKET Packet); 

// NDIS 6.X
#define NdisAllocateMdl_Hash 0x6906afcb
__drv_ret(__drv_allocatesMem(mem))
__drv_maxIRQL(DISPATCH_LEVEL)
typedef PMDL (*FnNdisAllocateMdl)(__in NDIS_HANDLE NdisHandle, __in_bcount(Length) pvoid_t VirtualAddress, __in UINT Length);

#define NdisAllocateNetBufferListPool_Hash 0x934c3c9c
__drv_ret(__drv_allocatesMem(mem))
__drv_maxIRQL(DISPATCH_LEVEL)
typedef NDIS_HANDLE (*FnNdisAllocateNetBufferListPool)(__in_opt NDIS_HANDLE NdisHandle, __in PNET_BUFFER_LIST_POOL_PARAMETERS Parameters);

#define NdisFreeNetBufferListPool_Hash 0x3a40b8e8
__drv_maxIRQL(DISPATCH_LEVEL)
typedef VOID (*FnNdisFreeNetBufferListPool)(__in  __drv_freesMem(mem) NDIS_HANDLE PoolHandle);

#define NdisAllocateNetBufferAndNetBufferList_Hash 0x8ce51ffe
__drv_ret(__drv_allocatesMem(mem))
__drv_maxIRQL(DISPATCH_LEVEL)
typedef PNET_BUFFER_LIST (*FnNdisAllocateNetBufferAndNetBufferList)(__in NDIS_HANDLE PoolHandle, __in USHORT ContextSize, __in USHORT ContextBackFill, __in_opt __drv_aliasesMem PMDL MdlChain, __in ulong_t DataOffset, __in SIZE_T DataLength);


#define NdisReturnNetBufferLists_Hash 0xbece847b
__drv_maxIRQL(DISPATCH_LEVEL)
typedef VOID (*FnNdisReturnNetBufferLists)(__in NDIS_HANDLE NdisFilterHandle, __in PNET_BUFFER_LIST NetBufferLists, __in ulong_t ReturnFlags);

__drv_maxIRQL(DISPATCH_LEVEL)
typedef VOID (*FnNdisSendNetBufferLists)(__in NDIS_HANDLE ndisHandle, __in PNET_BUFFER_LIST NetBufferLists, __in NDIS_PORT_NUMBER PortNumber, __in ulong_t SendFlags);

#define NdisGetPoolFromNetBufferList_Hash 0x8a01284f
__drv_maxIRQL(DISPATCH_LEVEL)
typedef NDIS_HANDLE (*FnNdisGetPoolFromNetBufferList)(__in PNET_BUFFER_LIST NetBufferList);

#define NdisFreeMdl_Hash 0x97a281bc
__drv_maxIRQL(DISPATCH_LEVEL)
typedef VOID (*FnNdisFreeMdl)(__in __drv_freesMem(mem) PMDL Mdl);

#define NdisFreeNetBufferList_Hash 0xefc542c5
__drv_maxIRQL(DISPATCH_LEVEL)
typedef VOID (*FnNdisFreeNetBufferList)(__in __drv_freesMem(mem) PNET_BUFFER_LIST NetBufferList);


#define ndisQueryMdl(_Mdl, _VirtualAddress, _Length, _Priority)             \
{                                                                           \
	if (ARGUMENT_PRESENT(_VirtualAddress))                                  \
{                                                                       \
	*(pvoid_t *)(_VirtualAddress) = mmGetSystemAddressForMdlSafe(_Mdl, _Priority); \
}                                                                       \
	*(_Length) = MmGetMdlByteCount(_Mdl);                                   \
}

#define NET_BUFFER_LIST_CONTEXT_DATA_START(_NBL) ((uint8_t*)(((_NBL)->Context)+1)+(_NBL)->Context->Offset)
#define NET_BUFFER_LIST_CONTEXT_DATA_SIZE(_NBL) (((_NBL)->Context)->Size)


typedef enum _NDIS_HALT_ACTION
{
	NdisHaltDeviceDisabled,
	NdisHaltDeviceInstanceDeInitialized,
	NdisHaltDevicePoweredDown,
	NdisHaltDeviceSurpriseRemoved,
	NdisHaltDeviceFailed,
	NdisHaltDeviceInitializationFailed,
	NdisHaltDeviceStopped
} NDIS_HALT_ACTION, *PNDIS_HALT_ACTION;

typedef struct _NDIS_MINIPORT_PAUSE_PARAMETERS
{
	NDIS_OBJECT_HEADER          Header;
	ulong_t                       Flags;
	ulong_t                       PauseReason;
} NDIS_MINIPORT_PAUSE_PARAMETERS, *PNDIS_MINIPORT_PAUSE_PARAMETERS;

typedef  struct _NDIS_RESTART_ATTRIBUTES NDIS_RESTART_ATTRIBUTES, *PNDIS_RESTART_ATTRIBUTES;

typedef  struct _NDIS_RESTART_ATTRIBUTES
{  
	PNDIS_RESTART_ATTRIBUTES   Next;
	NDIS_OID                   Oid;
	ulong_t                      DataLength;
	DECLSPEC_ALIGN(MEMORY_ALLOCATION_ALIGNMENT) UCHAR  Data[1];
}NDIS_RESTART_ATTRIBUTES, *PNDIS_RESTART_ATTRIBUTES;

typedef struct _NDIS_MINIPORT_RESTART_PARAMETERS
{
	NDIS_OBJECT_HEADER          Header;
	PNDIS_RESTART_ATTRIBUTES    RestartAttributes;
	ulong_t                       Flags;
} NDIS_MINIPORT_RESTART_PARAMETERS, *PNDIS_MINIPORT_RESTART_PARAMETERS;

typedef VOID (*FnNdis5HaltHandler)(__in NDIS_HANDLE MiniportAdapterContext);
typedef VOID (*FnNdis6HaltHandler)(__in NDIS_HANDLE MiniportAdapterContext, __in NDIS_HALT_ACTION HaltAction);
typedef NDIS_STATUS (*FnNdis6PauseHandler)(__in NDIS_HANDLE MiniportAdapterContext, __in PNDIS_MINIPORT_PAUSE_PARAMETERS MiniportPauseParameters);
typedef NDIS_STATUS (*FnNdis6RestartHandler)(__in NDIS_HANDLE MiniportAdapterContext, __in PNDIS_MINIPORT_RESTART_PARAMETERS MiniportRestartParameters);


typedef struct _ndis_adapter ndis_adapter_t, *pndis_adapter_t;
typedef struct _HOOK_CONTEXT HOOK_CONTEXT, *PHOOK_CONTEXT;

typedef struct _ZOMBI_QUEUED_CLOSE
{
	INT32 Status;
	WORK_QUEUE_ITEM WorkItem;
} ZOMBI_QUEUED_CLOSE;

// typedef struct _ZOMBI_NDIS_OBJECT_HEADER
// {
// 	UCHAR   Type;
// 	UCHAR   Revision;
// 	USHORT  Size;
// } ZOMBI_NDIS_OBJECT_HEADER, *PZOMBI_NDIS_OBJECT_HEADER;

//#pragma pack(push, 1)
typedef struct _zombi_ndis_open_block_xp
{
	/*32-bit*/ /*64-bit*/ 
	/*+0x000*/ /*+0x000*/ pvoid_t MacHandle;
	/*+0x004*/ /*+0x008*/ pvoid_t BindingHandle;
	/*+0x008*/ /*+0x010*/ pvoid_t MiniportHandle;
	/*+0x00c*/ /*+0x018*/ pvoid_t ProtocolHandle;
	/*+0x010*/ /*+0x020*/ pvoid_t ProtocolBindingContext;
	/*+0x014*/ /*+0x028*/ pvoid_t MiniportNextOpen;
	/*+0x018*/ /*+0x030*/ pvoid_t ProtocolNextOpen;
	/*+0x01c*/ /*+0x038*/ pvoid_t MiniportAdapterContext;
	/*+0x020*/ /*+0x040*/ uint8_t Reserved1;
	/*+0x021*/ /*+0x041*/ uint8_t Reserved2;
	/*+0x022*/ /*+0x042*/ uint8_t Reserved3;
	/*+0x023*/ /*+0x043*/ uint8_t Reserved4;
	/*+0x024*/ /*+0x048*/ PUNICODE_STRING BindDeviceName;
	/*+0x028*/ /*+0x050*/ UINT_PTR Reserved5;
	/*+0x02c*/ /*+0x058*/ PUNICODE_STRING RootDeviceName;
	union {
		/*+0x030*/ /*+0x060*/ FARPROC SendHandler;
		/*+0x030*/ /*+0x060*/ FARPROC WanSendHandler;
	};
	/*+0x034*/ /*+0x068*/ FARPROC TransferDataHandler;
	/*+0x038*/ /*+0x070*/ FARPROC SendCompleteHandler;
	/*+0x03c*/ /*+0x078*/ FARPROC TransferDataCompleteHandler;
	/*+0x040*/ /*+0x080*/ FARPROC ReceiveHandler;
	/*+0x044*/ /*+0x088*/ FARPROC ReceiveCompleteHandler;
	/*+0x048*/ /*+0x090*/ FARPROC WanReceiveHandler;
	/*+0x04c*/ /*+0x098*/ FARPROC RequestCompleteHandler;
	/*+0x050*/ /*+0x0a0*/ FARPROC ReceivePacketHandler;
	/*+0x054*/ /*+0x0a8*/ FARPROC SendPacketsHandler;
	/*+0x058*/ /*+0x0b0*/ FARPROC ResetHandler;
	/*+0x05c*/ /*+0x0b8*/ FARPROC RequestHandler;
	/*+0x060*/ /*+0x0c0*/ FARPROC ResetCompleteHandler;
	/*+0x064*/ /*+0x0c8*/ FARPROC StatusHandler;
	/*+0x068*/ /*+0x0d0*/ FARPROC StatusCompleteHandler;
	/*+0x06c*/ /*+0x0d8*/ uint32_t Flags;
	/*+0x070*/ /*+0x0dc*/ INT32 References;
	/*+0x074*/ /*+0x0e0*/ UINT_PTR SpinLock;
	/*+0x078*/ /*+0x0e8*/ pvoid_t FilterHandle;
	/*+0x07c*/ /*+0x0f0*/ uint32_t ProtocolOptions;
	/*+0x080*/ /*+0x0f4*/ uint16_t CurrentLookahead;
	/*+0x082*/ /*+0x0f6*/ uint16_t ConnectDampTicks;
	/*+0x084*/ /*+0x0f8*/ uint16_t DisconnectDampTicks;
	/*+0x088*/ /*+0x100*/ FARPROC WSendHandler;
	/*+0x08c*/ /*+0x108*/ FARPROC WTransferDataHandler;
	/*+0x090*/ /*+0x110*/ FARPROC WSendPacketsHandler;
	/*+0x094*/ /*+0x118*/ FARPROC CancelSendPacketsHandler;
	/*+0x098*/ /*+0x120*/ uint32_t WakeUpEnable;
	/*+0x09c*/ /*+0x128*/ PKEVENT CloseCompleteEvent;
	/*+0x0a0*/ /*+0x130*/ ZOMBI_QUEUED_CLOSE QC;
	/*+0x0b4*/ /*+0x158*/ INT32  AfReferences;
	/*+0x0b8*/ /*+0x160*/ pvoid_t NextGlobalOpen;
#ifdef _WIN64
	/*      */ /*+0x168*/ FARPROC InitiateOffloadCompleteHandler;
	/*      */ /*+0x170*/ FARPROC TerminateOffloadCompleteHandler;
	/*      */ /*+0x178*/ FARPROC UpdateOffloadCompleteHandler;
	/*      */ /*+0x180*/ FARPROC InvalidateOffloadCompleteHandler;
	/*      */ /*+0x188*/ FARPROC QueryOffloadCompleteHandler;
	/*      */ /*+0x190*/ FARPROC IndicateOffloadEventHandler;
	/*      */ /*+0x198*/ FARPROC TcpOffloadSendCompleteHandler;
	/*      */ /*+0x1a0*/ FARPROC TcpOffloadReceiveCompleteHandler;
	/*      */ /*+0x1a8*/ FARPROC TcpOffloadDisconnectCompleteHandler;
	/*      */ /*+0x1b0*/ FARPROC TcpOffloadForwardCompleteHandler;
	/*      */ /*+0x1b8*/ FARPROC TcpOffloadEventHandler;
	/*      */ /*+0x1c0*/ FARPROC TcpOffloadReceiveIndicateHandler;
#endif
	/*+0x0bc*/ /*+0x1c8*/ pvoid_t NextAf;
	/*+0x0c0*/ /*+0x1d0*/ FARPROC MiniportCoCreateVcHandler;
	/*+0x0c4*/ /*+0x1d8*/ FARPROC MiniportCoRequestHandler;
	/*+0x0c8*/ /*+0x1e0*/ FARPROC CoCreateVcHandler;
	/*+0x0cc*/ /*+0x1e8*/ FARPROC CoDeleteVcHandler;
	/*+0x0d0*/ /*+0x1f0*/ FARPROC CmActivateVcCompleteHandler;
	/*+0x0d4*/ /*+0x1f8*/ FARPROC CmDeactivateVcCompleteHandler;
	/*+0x0d8*/ /*+0x200*/ FARPROC CoRequestCompleteHandler;
	/*+0x0dc*/ /*+0x208*/ LIST_ENTRY ActiveVcHead;
	/*+0x0e4*/ /*+0x218*/ LIST_ENTRY InactiveVcHead;
	/*+0x0ec*/ /*+0x228*/ INT32 PendingAfNotifications;
	/*+0x0f0*/ /*+0x230*/ PKEVENT AfNotifyCompleteEvent;
} zombi_ndis_open_block_xp_t, *pzombi_ndis_open_block_xp_t;


// 
// typedef struct _ZOMBI_NDIS_OPEN_BLOCK_VISTA
// {
// 	/*32-bit*/ /*64-bit*/
// 	union {
// 	/*+0x000*/ /*      */ pvoid_t MacHandle;
// 	/*+0x000*/ /*      */ ZOMBI_NDIS_OBJECT_HEADER Header;
// 	};
// 	/*+0x004*/ /*      */ pvoid_t BindingHandle;
// 	/*+0x008*/ /*      */ pvoid_t MiniportHandle;
// 	/*+0x00c*/ /*      */ pvoid_t ProtocolHandle;
// 	/*+0x010*/ /*      */ pvoid_t ProtocolBindingContext;
// 	/*+0x014*/ /*      */ pvoid_t NextSendHandler;
// 	/*+0x018*/ /*      */ pvoid_t NextSendContext;
// 	/*+0x01c*/ /*      */ pvoid_t MiniportAdapterContext;
// 	/*+0x020*/ /*      */ uint8_t Reserved1;
// 	/*+0x021*/ /*      */ uint8_t CallingFromNdis6Protocol;
// 	/*+0x022*/ /*      */ uint8_t Reserved3;
// 	/*+0x023*/ /*      */ uint8_t Reserved4;
// 	/*+0x024*/ /*      */ pvoid_t NextReturnNetBufferListsHandler;
// 	/*+0x028*/ /*      */ uint32_t Reserved5;
// 	/*+0x02c*/ /*      */ pvoid_t NextReturnNetBufferListsContext;
// 	union {
// 	/*+0x030*/ /*      */ FARPROC SendHandler;
// 	/*+0x030*/ /*      */ FARPROC WanSendHandler;
// 	};
// 	/*+0x034*/ /*      */ FARPROC TransferDataHandler;
// 	/*+0x038*/ /*      */ FARPROC SendCompleteHandler;
// 	/*+0x03c*/ /*      */ FARPROC TransferDataCompleteHandler;
// 	/*+0x040*/ /*      */ FARPROC ReceiveHandler;
// 	/*+0x044*/ /*      */ FARPROC ReceiveCompleteHandler;
// 	/*+0x048*/ /*      */ FARPROC WanReceiveHandler;
// 	/*+0x04c*/ /*      */ FARPROC RequestCompleteHandler;
// 	/*+0x050*/ /*      */ FARPROC ReceivePacketHandler;
// 	/*+0x054*/ /*      */ FARPROC SendPacketsHandler;
// 	/*+0x058*/ /*      */ FARPROC ResetHandler;
// 	/*+0x05c*/ /*      */ FARPROC RequestHandler;
// 	/*+0x060*/ /*      */ FARPROC OidRequestHandler;
// 	/*+0x064*/ /*      */ FARPROC ResetCompleteHandler;
// 	union {
// 	/*+0x068*/ /*      */ FARPROC StatusHandler;
// 	/*+0x068*/ /*      */ FARPROC StatusHandlerEx;
// 	};
// 	/*+0x06c*/ /*      */ FARPROC StatusCompleteHandler;
// 	/*+0x070*/ /*      */ uint32_t Flags;
// 	/*+0x074*/ /*      */ INT32 References;
// 	/*+0x078*/ /*      */ uint32_t SpinLock;
// 	/*+0x07c*/ /*      */ pvoid_t FilterHandle;
// 	/*+0x080*/ /*      */ uint32_t FrameTypeArraySize;
// 	/*+0x084*/ /*      */ uint16_t FrameTypeArray[4];
// 	/*+0x08c*/ /*      */ uint32_t ProtocolOptions;
// 	/*+0x090*/ /*      */ uint32_t CurrentLookahead;
// 	/*+0x094*/ /*      */ FARPROC WSendHandler;
// 	/*+0x098*/ /*      */ FARPROC WTransferDataHandler;
// 	/*+0x09c*/ /*      */ FARPROC WSendPacketsHandler;
// 	/*+0x0a0*/ /*      */ FARPROC CancelSendPacketsHandler;
// 	/*+0x0a4*/ /*      */ uint32_t WakeUpEnable;
// 	/*+0x0a8*/ /*      */ PKEVENT CloseCompleteEvent;
// 	/*+0x0ac*/ /*      */ ZOMBI_QUEUED_CLOSE QC;
// 	/*+0x0c0*/ /*      */ INT32 AfReferences;
// 	/*+0x0c4*/ /*      */ pvoid_t NextGlobalOpen;
// 	/*+0x0c8*/ /*      */ pvoid_t MiniportNextOpen;
// 	/*+0x0cc*/ /*      */ pvoid_t ProtocolNextOpen;
// 	/*+0x0d0*/ /*      */ PUNICODE_STRING BindDeviceName;
// 	/*+0x0d4*/ /*      */ PUNICODE_STRING RootDeviceName;
// 	/*+0x0d8*/ /*      */ pvoid_t FilterNextOpen;
// 	/*+0x0dc*/ /*      */ uint32_t PacketFilters;
// 	/*+0x0e0*/ /*      */ uint32_t OldPacketFilters;
// 	union {
// 		struct _struct1 {
// 	/*+0x0e4*/ /*      */ uint32_t MaxMulticastAddresses;
// 	/*+0x0e8*/ /*      */ pvoid_t MCastAddressBuf;
// 	/*+0x0ec*/ /*      */ uint32_t NumAddresses;
// 	/*+0x0f0*/ /*      */ pvoid_t OldMCastAddressBuf;
// 	/*+0x0f4*/ /*      */ uint32_t OldNumAddresses;
// 		};
// 		struct _struct2 {
// 	/*+0x0e4*/ /*      */ uint32_t FunctionalAddress;
// 	/*+0x0e8*/ /*      */ uint32_t OldFunctionalAddress;
// 	/*+0x0ec*/ /*      */ uint8_t UsingGroupAddress;
// 	/*+0x0ed*/ /*      */ uint8_t OldUsingGroupAddress;
// 	/*+0x0f0*/ /*      */ uint32_t FARefCount[32];
// 		};
// 	};
// 	/*+0x170*/ /*      */ uint32_t OldFARefCount[32];
// 	/*+0x1f0*/ /*      */ uint8_t RSSParametersBuf[196];
// 	/*+0x2b4*/ /*      */ pvoid_t NdisRSSParameters;
// 	/*+0x2b8*/ /*      */ SINGLE_LIST_ENTRY PatternList;
// 	/*+0x2bc*/ /*      */ pvoid_t ProtSendNetBufferListsComplete;
// 	/*+0x2c0*/ /*      */ pvoid_t NextSendNetBufferListsComplete;
// 	/*+0x2c4*/ /*      */ pvoid_t NextSendNetBufferListsCompleteContext;
// 	/*+0x2c8*/ /*      */ pvoid_t SendCompleteNdisPacketContext;
// 	/*+0x2cc*/ /*      */ pvoid_t SendCompleteNetBufferListsContext;
// 	/*+0x2d0*/ /*      */ pvoid_t ReceiveNetBufferLists;
// 	/*+0x2d4*/ /*      */ pvoid_t ReceiveNetBufferListsContext;
// 	/*+0x2d8*/ /*      */ FARPROC SavedSendNBLHandler;
// 	/*+0x2dc*/ /*      */ FARPROC SavedSendPacketsHandler;
// 	/*+0x2e0*/ /*      */ FARPROC SavedCancelSendPacketsHandler;
// 	union {
// 	/*+0x2e4*/ /*      */ FARPROC SavedSendHandler;
// 	/*+0x2e4*/ /*      */ FARPROC SavedWanSendHandler;
// 	};
// 	/*+0x2e8*/ /*      */ FARPROC InitiateOffloadCompleteHandler;
// 	/*+0x2ec*/ /*      */ FARPROC TerminateOffloadCompleteHandler;
// 	/*+0x2f0*/ /*      */ FARPROC UpdateOffloadCompleteHandler;
// 	/*+0x2f4*/ /*      */ FARPROC InvalidateOffloadCompleteHandler;
// 	/*+0x2f8*/ /*      */ FARPROC QueryOffloadCompleteHandler;
// 	/*+0x2fc*/ /*      */ FARPROC IndicateOffloadEventHandler;
// 	/*+0x300*/ /*      */ FARPROC TcpOffloadSendCompleteHandler;
// 	/*+0x304*/ /*      */ FARPROC TcpOffloadReceiveCompleteHandler;
// 	/*+0x308*/ /*      */ FARPROC TcpOffloadDisconnectCompleteHandler;
// 	/*+0x30c*/ /*      */ FARPROC TcpOffloadForwardCompleteHandler;
// 	/*+0x310*/ /*      */ FARPROC TcpOffloadEventHandler;
// 	/*+0x314*/ /*      */ FARPROC TcpOffloadReceiveIndicateHandler;
// 	/*+0x318*/ /*      */ uint32_t ProtocolMajorVersion;
// 	/*+0x31c*/ /*      */ pvoid_t* IfBlock;
// 	/*+0x320*/ /*      */ NDIS_SPIN_LOCK PnPStateLock;
// 	/*+0x328*/ /*      */ uint32_t PnPState; // _NDIS_NDIS5_DRIVER_STATE
// 	/*+0x32c*/ /*      */ uint32_t TranslationState; // _NDIS_OPEN_TRANSLATION_STATE
// 	/*+0x330*/ /*      */ INT32 OutstandingSends;
// 	/*+0x334*/ /*      */ NDIS_EVENT PauseEvent;
// 	/*+0x344*/ /*      */ pvoid_t Ndis5WanSendHandler;
// 	/*+0x348*/ /*      */ pvoid_t ProtSendCompleteHandler;
// 	/*+0x34c*/ /*      */ pvoid_t OidRequestCompleteHandler;
// 	/*+0x350*/ /*      */ pvoid_t OidRequestCompleteContext;
// 	/*+0x354*/ /*      */ pvoid_t SetInfoBuf;
// 	/*+0x358*/ /*      */ uint16_t SetInfoBufLen;
// 	/*+0x35c*/ /*      */ uint32_t RequestBuffer;
// 	/*+0x360*/ /*      */ uint32_t SetInfoOid;
// 	/*+0x364*/ /*      */ pvoid_t OidContext;
// 	/*+0x368*/ /*      */ INT32 NumOfPauseRestartRequests;
// 	/*+0x36c*/ /*      */ uint32_t State; // _NDIS_OPEN_STATE
// 	/*+0x370*/ /*      */ pvoid_t Offload;
// 	/*+0x374*/ /*      */ pvoid_t StatusUnbindWorkItem;
// 	/*+0x378*/ /*      */ UINT64 DpcStartCycle;
// 	/*+0x380*/ /*      */ uint32_t NumberOfNetBufferLists;
// 	/*+0x384*/ /*      */ uint8_t ReceivedAPacket[32];
// 	/*+0x3a4*/ /*      */ FARPROC DirectOidRequestCompleteHandler;
// 	/*+0x3a8*/ /*      */ FARPROC DirectOidRequestHandler;
// 	/*+0x3ac*/ /*      */ pvoid_t DirectOidRequestCompleteContext;
// 	/*+0x3b0*/ /*      */ pvoid_t NextAf;
// 	/*+0x3b4*/ /*      */ FARPROC MiniportCoCreateVcHandler;
// 	/*+0x3b8*/ /*      */ FARPROC MiniportCoRequestHandler;
// 	/*+0x3bc*/ /*      */ FARPROC CoCreateVcHandler;
// 	/*+0x3c0*/ /*      */ FARPROC CoDeleteVcHandler;
// 	/*+0x3c4*/ /*      */ FARPROC CmActivateVcCompleteHandler;
// 	/*+0x3c8*/ /*      */ FARPROC CmDeactivateVcCompleteHandler;
// 	/*+0x3cc*/ /*      */ FARPROC CoRequestCompleteHandler;
// 	/*+0x3d0*/ /*      */ FARPROC CoRequestHandler;
// 	/*+0x3d4*/ /*      */ LIST_ENTRY ActiveVcHead;
// 	/*+0x3dc*/ /*      */ LIST_ENTRY InactiveVcHead;
// 	/*+0x3e4*/ /*      */ INT32 PendingAfNotifications;
// 	/*+0x3e8*/ /*      */ PKEVENT AfNotifyCompleteEvent;
// 	/*+0x3ec*/ /*      */ FARPROC MiniportCoOidRequestHandler;
// 	/*+0x3f0*/ /*      */ FARPROC CoOidRequestCompleteHandler;
// 	/*+0x3f4*/ /*      */ FARPROC CoOidRequestHandler;
// } ZOMBI_NDIS_OPEN_BLOCK_VISTA, *PZOMBI_NDIS_OPEN_BLOCK_VISTA;

#define LOADER_SELF_PKT 'zwsp'

typedef struct _SEND_RSVD
{
	uint8_t* originalPacket;
	ulong_t tag;
	struct _SEND_RSVD* pNext;
	NDIS_HANDLE pSourceHandle;
} SEND_RSVD, *PSEND_RSVD;

typedef struct _ndis_adapter
{
	struct _ndis_adapter* pNext;
	uint8_t* pOpenBlock;					// ndis!_NDIS_OPEN_BLOCK
	uint8_t* pMiniBlock;					// ndis!_NDIS_MINIPORT_BLOCK
    uint8_t* pFilterBlock;                  // ndis!_NDIS_FILTER_BLOCK (NT6.X)
	uint8_t* pMiniAdapterCtx;				// Указатель на MiniportAdapterContext
    uint8_t* SourceHandle;

	NDIS_HANDLE pSendHandle;			// Указатель, который используется для отправки пакетов

	NDIS_HANDLE packetPoolHandle;	// NDIS 5.X
	NDIS_HANDLE bufferPoolHandle;	// NDIS 5.X
	PSEND_RSVD pSendPackets;
	NDIS_HANDLE nbPoolHandle;		// NDIS 6.X

    pvoid_t *ppNdisHaltHandler, *ppNdisPacketIndicateHandler, *ppNdisMPSendCompleteHandler, *ppNdisEthRxIndicateHandler, *ppNdisSendCompleteHandler;
    pvoid_t *ppNdisPauseHandler, *ppNdisRestartHandler, *ppNdisIndicateNBLHandler, *ppNdisSndNetBufLstsCmptHandler;

	NDIS_PHYSICAL_MEDIUM physMediumType;
	uint32_t flags;
	uint8_t ndisMajorVersion;
	uint16_t Reserved1;
	uint8_t Reserved2;
    uint8_t* pNwifiBase;
// 	uint8_t* pModuleBase;		    // База драйвера минипорта
// 	uint8_t* pMiniportDpc;			// Адрес функции обработки DPC-сообщений (драйвер сетевой карты)

	PHOOK_CONTEXT pHookContext;		// Список хуков связанных с данным адаптером
//	psplice_hooker_data_t pHook;		// Указатель на сплайс-хук обработчика прерываний сетевой карты
	NDIS_STRING miniportName;		// Имя минипорта

	BOOLEAN bSupportedHardware;		// Указывает на поддержку транспортного протокла (пока что только Ethernet).
	BOOLEAN bObtainedIPs;			// Указывает на то, что были считаны сетевые параметры.
	uint8_t obtainedMacs[4];			// Указывает, на то, что этот адаптер может использоваться для доступа к интернету.
    uint32_t isComplete;
	uint16_t Reserved3;
	// Этот параметр может быть выставлен в TRUE, только у одного адаптера, даже если их несколько.
	KEVENT dynDetectEvent;			// Объект синхронизации для сигнала завершения динамического поиска MAC-адреса.

	BOOLEAN miniportPaused;			// Данный флаг указывает на приостановку сетевого интерфейса.

	uint32_t ipAddr;
	uint32_t ipMask;
	uint32_t ipGateway;
	uint32_t ipNameServers[2];
	uint8_t macAddr[6];
	uint16_t Reserved4;

	FARPROC OriginalSendCompleteHandler;
	zombi_ndis_open_block_xp_t zombiBlockXP;	// Зомби-структура (для NDIS_OPEN_BLOCK), которая посылается функциям для отправки пакетов
    //uint8_t reserved[256];
} ndis_adapter_t, *pndis_adapter_t;

#define NDIS_FLAGS_BUS_MASTER		0x00000008
#define NDIS_FLAGS_DESERIALIZED		0x00040000
#define NDIS_FLAGS_MEDIA_CONNECTED	0x20000000

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma pack(push, 1)

typedef struct _LESS4_HOOK_CODE
{
	uint16_t code1_movRXXh;					// mov r9, 0AABBCCDDEEFF0011h
	pvoid_t pHookContext;
	UCHAR code2_0x68;						// push loAddr
	uint32_t lJumpAddr;
	uint32_t code3_0xC7442404;				// mov [rsp + 4], dword hiAddr
	uint32_t hJumpAddr;
	UCHAR code4_C3;							// ret
} LESS4_HOOK_CODE;

typedef struct _ABOVE4_HOOK_CODE
{
	uint16_t code1_48B8h;						// mov rax, 0AABBCCDDEEFF0011h
	pvoid_t pHookContext;
	uint32_t code21_48894424h;				// mov [rsp+20h], rax
	uint8_t code22_20h;
	uint8_t code3_58h;						// pop rax
	uint32_t code4_4883EC10h;					// sub rsp, 10h
	uint32_t code51_48894424h;				// mov [rsp+20h], rax
	UCHAR code52_0x20;
	UCHAR code6_0x68;						// push loAddr
	uint32_t lOurRetAddr;
	uint32_t code7_0xC7442404;				// mov [rsp + 4], dword hiAddr
	uint32_t hOurRetAddr;
	UCHAR code8_0x68;						// push loAddr
	uint32_t lJumpAddr;
	uint32_t code9_0xC7442404;				// mov [rsp + 4], dword hiAddr
	uint32_t hJumpAddr;
	UCHAR codeA_C3;							// ret
	uint32_t codeB_4883C410h;					// add rsp, 10h
	uint32_t codeC_FF642410h;					// jmp qword ptr [rsp+10h]
} ABOVE4_HOOK_CODE;

struct _HOOK_CONTEXT
{
#ifdef _AMD64_
	union {
		LESS4_HOOK_CODE less4;
		ABOVE4_HOOK_CODE above4;
	} hookCode;
#else
	UCHAR code1_0x58; //0x58 | pop  eax      | pop caller IP from stack to eax
	UCHAR code2_0x68; //0x68 | push IMM32      | push our hook context address
	struct _HOOK_CONTEXT* pHookContext; //point this 
	UCHAR code3_0x50; //0x50 | push eax		| push caller IP from eax to stack 
	UCHAR code4_0xE9; //0xE9 | jmp HookProc  | jump our hook proc
	ulong_t pHookProcOffset;
#endif
	//our context data
	pvoid_t pOrigFunc;
	pvoid_t pHookFunc;
	pndis_adapter_t pAdapter;
	pvoid_t* ppOriginFunc;

	struct _HOOK_CONTEXT* pNext;
};

#pragma pack(pop)

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef void (*FnndisQueryPacket)(IN PNDIS_PACKET _Packet, OUT PUINT _PhysicalBufferCount OPTIONAL, OUT PUINT _BufferCount OPTIONAL, OUT PNDIS_BUFFER* _FirstBuffer OPTIONAL, OUT PUINT _TotalPacketLength OPTIONAL);

// NDIS hooks
typedef VOID (*FnHookNdis5_EthRxIndicateHandler)(PHOOK_CONTEXT pHookContext, IN NDIS_HANDLE BindingContext, IN NDIS_HANDLE MacReceiveContext, IN PCHAR Address, IN pvoid_t HeaderBuffer, IN UINT HeaderBufferSize, IN pvoid_t LookaheadBuffer, IN UINT LookaheadBufferSize, IN UINT PacketSize);
typedef VOID (*FnHookNdis5_PacketIndicateHandler)(PHOOK_CONTEXT pHookContext, IN NDIS_HANDLE MiniportAdapterHandle, IN PPNDIS_PACKET ReceivePackets, IN UINT NumberOfPackets);
typedef VOID (*FnHookNdis5_HaltHandler)(PHOOK_CONTEXT pHookContext, NDIS_HANDLE MiniportAdapterContext);
typedef VOID (*FnHookNdis5_SendCompleteHandler)(IN PHOOK_CONTEXT pHookContext, IN NDIS_HANDLE ProtocolBindingContext, IN PNDIS_PACKET Packet, IN NDIS_STATUS Status);
typedef VOID (*FnHookNdis5_OpenSendCompleteHandler)(IN PHOOK_CONTEXT pHookContext, IN NDIS_HANDLE ProtocolBindingContext, IN PNDIS_PACKET Packet, IN NDIS_STATUS Status);
typedef VOID (*FnHookNdis6_ReceiveHandler)(IN PHOOK_CONTEXT pHookContext, IN OUT NDIS_HANDLE ProtocolBindingContext, IN PNET_BUFFER_LIST NetBufferLists, IN NDIS_PORT_NUMBER PortNumber, IN ulong_t NumberOfNetBufferLists, IN ulong_t ReceiveFlags);
typedef VOID (*FnHookNdis6_HaltHandler)(PHOOK_CONTEXT pHookContext, NDIS_HANDLE MiniportAdapterContext, NDIS_HALT_ACTION HaltAction);
typedef NDIS_STATUS (*FnHookNdis6_PauseHandler)(PHOOK_CONTEXT pHookContext, __in NDIS_HANDLE MiniportAdapterContext, __in PNDIS_MINIPORT_PAUSE_PARAMETERS MiniportPauseParameters);
typedef NDIS_STATUS (*FnHookNdis6_RestartHandler)(PHOOK_CONTEXT pHookContext, __in NDIS_HANDLE MiniportAdapterContext, __in PNDIS_MINIPORT_RESTART_PARAMETERS MiniportRestartParameters);
typedef VOID (*FnHookNdis6_MSendNetBufferListsComplete)(PHOOK_CONTEXT pHookContext, IN NDIS_HANDLE ProtocolBindingContext, IN PNET_BUFFER_LIST pNetBufferLists, IN ulong_t SendCompleteFlags);

typedef PHOOK_CONTEXT (*FnHookNdisFunc)(pvoid_t pHookProc, pvoid_t* ppOrigProc, pndis_adapter_t pAdapter, int numberOfArgs);
typedef void (*Fnnetwork_unhook_ndis)(pndis_adapter_t pAdapter);

// В зависимости от возвращаемого функцией результата, обработчик входящих пакетов будет или пропускать пакет дальше, или извлекать его или изменять его содержимое.
// 0 - извлечь пакет.
// 1 - пропустить пакет.
// 3 - обновить пакет и пропустить его.
#define HOOK_REJECT_PACKET 0x01
#define HOOK_PASS_PACKET 0x02
#define HOOK_UPDATE_PACKET 0x04
typedef int (*FnInputPacketHandler)(uint8_t* pBuffer, uint32_t size);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef NTSTATUS (*FnconvertStringIpToUINT32)(wchar_t* stringIp, uint32_t* uVal);
typedef void (*FnRegistryFindAdapterInfo)(PUNICODE_STRING pMiniportName, pndis_adapter_t* ppAdapter);

typedef pndis_adapter_t (*Fnnetwork_destroy_adapter)(pndis_adapter_t pAdapter);
typedef void (*Fnnetwork_destroy_all_adapters)();

typedef void (*Fnnetwork_on_halt)(pndis_adapter_t pAdapter);


typedef struct _mod_network_private
{
	// 	NETWORK_PUBLIC_FUNCTIONS;
	// 	NETWORK_INTERNAL_FUNCTIONS;
	// 	NETWORK_INTERNAL_DATA;
	// 
	// NDIS!
	FnNdisAllocateMemoryWithTag fnNdisAllocateMemoryWithTag;
	FnNdisFreeMemory fnNdisFreeMemory;
	FnNdisGetPoolFromPacket fnNdisGetPoolFromPacket;
	FnNdisUnchainBufferAtFront fnNdisUnchainBufferAtFront;
	FnNdisFreePacket fnNdisFreePacket;
	FnNdisDprFreePacket fnNdisDprFreePacket;
	FnNdisAllocatePacket fnNdisAllocatePacket;
	FnNdisFreeBufferPool fnNdisFreeBufferPool;
	FnNdisFreePacketPool fnNdisFreePacketPool;
	FnNdisAllocatePacketPool fnNdisAllocatePacketPool;
	FnNdisAllocateBufferPool fnNdisAllocateBufferPool;
	FnNdisAllocateBuffer fnNdisAllocateBuffer;
	FnNdisRequest fnNdisRequest;

	FnNdisAllocateMdl fnNdisAllocateMdl;
	FnNdisAllocateNetBufferListPool fnNdisAllocateNetBufferListPool;
	FnNdisFreeNetBufferListPool fnNdisFreeNetBufferListPool;
	FnNdisAllocateNetBufferAndNetBufferList fnNdisAllocateNetBufferAndNetBufferList;
	FnNdisFreeNetBufferList fnNdisFreeNetBufferList;
	FnNdisGetPoolFromNetBufferList fnNdisGetPoolFromNetBufferList;
	FnNdisFreeMdl fnNdisFreeMdl;

	FnNdisMiniportSendPackets fnNdisMiniportSendPackets;
	FnNdisSendNetBufferLists fnNdisSendNetBufferLists;

	// NDIS Function Address Hooker
	FnHookNdisFunc fnHookNdisFunc;
// 	FnMPHandleInterrupt_Hook5X fnMPHandleInterrupt_Hook5X;
// 	FnMPHandleInterrupt_Hook6X fnMPHandleInterrupt_Hook6X;
	Fnnetwork_unhook_ndis fnnetwork_unhook_ndis;

	FnInputPacketHandler fnHookInternalReceiveHandler;

	FnndisQueryPacket fnndisQueryPacket;
	FnMiniportReturnPacket fnMiniportReturnPacket;
	FnNdisReturnNetBufferLists fnNdisReturnNetBufferLists;

	// NDIS hooks
	FnHookNdis5_EthRxIndicateHandler fnHookNdis5_EthRxIndicateHandler;
	FnHookNdis5_PacketIndicateHandler fnHookNdis5_PacketIndicateHandler;
	FnHookNdis5_HaltHandler fnHookNdis5_HaltHandler;
	FnHookNdis5_SendCompleteHandler fnHookNdis5_SendCompleteHandler;
	FnHookNdis5_OpenSendCompleteHandler fnHookNdis5_OpenSendCompleteHandler;
	FnHookNdis6_ReceiveHandler fnHookNdis6_ReceiveHandler;
	FnHookNdis6_HaltHandler fnHookNdis6_HaltHandler;
	FnHookNdis6_PauseHandler fnHookNdis6_PauseHandler;
	FnHookNdis6_RestartHandler fnHookNdis6_RestartHandler;
	FnHookNdis6_MSendNetBufferListsComplete fnHookNdis6_MSendNetBufferListsComplete;

	FnconvertStringIpToUINT32 fnconvertStringIpToUINT32;
	
	FnRegistryFindAdapterInfo fnRegistryFindAdapterInfo;

	Fnnetwork_destroy_adapter fnnetwork_destroy_adapter;
    Fnnetwork_destroy_all_adapters fnnetwork_destroy_all_adapters;
    Fnnetwork_on_halt fnnetwork_on_halt;

    uint8_t* pModBase;
    wchar_t nwifi[12];
	PUNICODE_STRING netParams[4/*NUMBER_OF_PARAMS*/];
	PUNICODE_STRING altNetParams[4/*NUMBER_OF_PARAMS*/];

	uint8_t* pInPacketBuffer;

	// Системно-зависимые смещения
	uint32_t dwNdisNextGlobalMiniport;
	uint32_t dwNdisEthDB;
	uint32_t dwNdisOpenList;
	uint32_t dwNdisBindingHandle;
	uint32_t dwNdisFlags;
	uint32_t dwNdisMajorVersion;
	uint32_t dwNdisInterrupt;
	uint32_t dwNdisInterruptEx;
	uint32_t dwNdisMiniportDpc;
	uint32_t dwNdisMDriverBlock;	
	uint32_t dwNdisMpAdapterContext;
	uint32_t dwNdisPhysicalMediumType;
	uint32_t dwNdisEthRxIndicateHandler;		// NDIS 5
	uint32_t dwNdisPacketIndicateHandler;		// NDIS 5
	uint32_t dwNdisIndicateNBLHandler;		// NDIS 6
	uint32_t dwNdisSendPacketsHandler;		// NDIS 5
	uint32_t dwNdisMSendHandler;				// NDIS 5
	uint32_t dwNdisMSendPacketsHandles;		// NDIS 5
	uint32_t dwNdisSendNetBufferLists;		// NDIS 6
	uint32_t dwNdisMReturnPacket;				// NDIS 5
	uint32_t dwNdisReturnNetBufferLists;		// NDIS 6
	uint32_t dwNdisMPSendCompleteHandler;		// NDIS 5
	uint32_t dwNdisSendCompleteHandler;		// NDIS 5
	uint32_t dwNdisSndNetBufLstsCmptHandler;	// NDIS 6
	uint32_t dwNdisHaltHandler;
	uint32_t dwNdisPauseHandler;
	uint32_t dwNdisRestartHandler;
	uint32_t dwNdisMiniportName;

    // WiFi
    uint32_t dwNdisLowestFilter;
    uint32_t dwNdisFilterDriver;
    uint32_t dwNdisDriverObject;
    uint32_t dwNdisWiFiSendNetBufferListsHandler;
    uint32_t dwNdisWiFiReturnNetBufferListsHandler;
    uint32_t dwNdisFilterModuleContext;
    uint32_t dwNdisWiFiDriverObject;
    uint32_t dwNdisHigherFilter;
} mod_network_private_t, *pmod_network_private_t;

typedef struct _mod_network_data
{
	uint8_t* ndisBase;
	uint32_t ndisSize;

	pndis_adapter_t pHeadAdapter;   // Указатель на первый найденный адаптер в списке адаптеров.
	pndis_adapter_t pActiveAdapter; // Текущий используемый адаптер для доступа к сети.

	bool_t needReconfigure;         // Данный флаг указывает на то, что сеть должны быть реинициализированна.
} mod_network_data_t, *pmod_network_data_t;


// Интерфейсные функции.
typedef void (*Fnnetwork_shutdown_routine)();
typedef bool_t (*Fnnetwork_search_for_adapters)();
typedef bool_t (*Fnnetwork_plug_next_adapter)();
typedef void (*Fnnetwork_confirm_active_adapter)();
typedef void (*Fnnetwork_validate_hooks)();
typedef void (*Fnnetwork_set_input_packet_handler)(FnInputPacketHandler fnInputPacketHandler);

// Функции для mod_tcpip.
typedef uint8_t* (*Fnnetwork_allocate_packet_buffer)(uint16_t len);
typedef int (*Fnnetwork_send_packet)(uint8_t* pData, uint16_t len);

typedef struct _mod_network_block
{
    Fnnetwork_shutdown_routine fnnetwork_shutdown_routine;
    Fnnetwork_search_for_adapters fnnetwork_search_for_adapters;
    Fnnetwork_plug_next_adapter fnnetwork_plug_next_adapter;
    Fnnetwork_confirm_active_adapter fnnetwork_confirm_active_adapter;	
    Fnnetwork_validate_hooks fnnetwork_validate_hooks;
    Fnnetwork_set_input_packet_handler fnnetwork_set_input_packet_handler;

    Fnnetwork_allocate_packet_buffer fnnetwork_allocate_packet_buffer;
    Fnnetwork_send_packet fnnetwork_send_packet;




    mod_network_data_t;
    mod_network_private_t;
} mod_network_block_t, *pmod_network_block_t;

#endif // __MODSHARED_NETWORKAPI_H_
