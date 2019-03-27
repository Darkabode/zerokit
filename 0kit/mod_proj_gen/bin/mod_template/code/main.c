#include <ntifs.h>
#include <wdm.h>
#include <ntstrsafe.h>

#include "../../mod_shared/headers.h"

#include "mod_template.c"
#include "mod_templateApi.c"

NTSTATUS mod_templateEntry(uintptr_t modBase, pglobal_block_t pGlobalBlock)
{
	pmod_common_block_t pCommonBlock = pGlobalBlock->pCommonBlock;
	pmod_template_block_t pTemplateBlock;
	pmod_header_t pModHeader = (pmod_header_t)modBase;
	
	pCommonBlock->fncommon_allocate_memory(pCommonBlock, &pTemplateBlock, sizeof(mod_template_block_t), NonPagedPool);
	pGlobalBlock->pTemplateBlock = pTemplateBlock;

#ifndef _SOLID_DRIVER
	
#ifdef _WIN64
    pCommonBlock->fncommon_fix_addr_value((PUINT8)getGlobalDataPtr, 11, GLOBAL_DATA_PATTERN, pGlobalBlock);
#else
    pCommonBlock->fncommon_fix_addr_value((PUINT8)modBase + sizeof(mod_header_t), pModHeader->sizeOfMod, GLOBAL_DATA_PATTERN, pGlobalBlock);
#endif // _WIN64

#endif // _SOLID_DRIVER
	
	{
		PUCHAR ptr;

	}
	
	return STATUS_SUCCESS;
}
