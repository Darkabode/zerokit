#include <ntifs.h>
#include <wdm.h>
#include <ntstrsafe.h>

#include "../../mod_shared/headers.h"

#include "mod_tasks.c"
#include "mod_tasksApi.c"

NTSTATUS mod_tasksEntry(uintptr_t modBase, pglobal_block_t pGlobalBlock)
{
    pmod_common_block_t pCommonBlock = pGlobalBlock->pCommonBlock;
    pmod_tasks_block_t pTasksBlock;
    pmod_header_t pModHeader = (pmod_header_t)modBase;

    pCommonBlock->fncommon_allocate_memory(pCommonBlock, &pTasksBlock, sizeof(mod_tasks_block_t), NonPagedPool);
    pGlobalBlock->pTasksBlock = pTasksBlock;
    pTasksBlock->pModBase = (uint8_t*)modBase;

#ifndef _SOLID_DRIVER
    
#ifdef _WIN64
    pCommonBlock->fncommon_fix_addr_value((PUINT8)getGlobalDataPtr, 11, GLOBAL_DATA_PATTERN, pGlobalBlock);
#else
    pCommonBlock->fncommon_fix_addr_value((PUINT8)modBase + sizeof(mod_header_t), pModHeader->sizeOfModReal, GLOBAL_DATA_PATTERN, pGlobalBlock);
#endif // _WIN64

#endif // _SOLID_DRIVER

    // Интерфейсные функции
    DECLARE_GLOBAL_FUNC(pTasksBlock, tasks_filter_uint32_pair);
    DECLARE_GLOBAL_FUNC(pTasksBlock, tasks_filter_numeric);
    DECLARE_GLOBAL_FUNC(pTasksBlock, tasks_filter);
    DECLARE_GLOBAL_FUNC(pTasksBlock, tasks_shutdown_routine);
    DECLARE_GLOBAL_FUNC(pTasksBlock, tasks_add_task);
    DECLARE_GLOBAL_FUNC(pTasksBlock, tasks_remove_all);
    DECLARE_GLOBAL_FUNC(pTasksBlock, tasks_get_completed_task_count);
    DECLARE_GLOBAL_FUNC(pTasksBlock, tasks_get_next_obtained);
    DECLARE_GLOBAL_FUNC(pTasksBlock, tasks_fill_with_completed);
    DECLARE_GLOBAL_FUNC(pTasksBlock, tasks_destroy);
    DECLARE_GLOBAL_FUNC(pTasksBlock, tasks_save_bundle_entry);
    DECLARE_GLOBAL_FUNC(pTasksBlock, tasks_load_bundle_entries);
    DECLARE_GLOBAL_FUNC(pTasksBlock, tasks_save_bundle_entries);

    {
//         uint8_t* ptr;

        *((uint32_t*)pTasksBlock->biPath) = 0x7369625C;

    }
    
    return STATUS_SUCCESS;
}
