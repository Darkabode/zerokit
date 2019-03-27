void userio_zfs_iseof(pvoid_t ptr)
{
    pzfs_file_t pFile;
    pfile_packet_t pFilePacket = ptr;
    USE_GLOBAL_BLOCK

    pFile = (pzfs_file_t)pGlobalBlock->pUserioBlock->fnuserio_get_zfs_handle(pFilePacket->handle);
    if (pFile == NULL) {
        pFilePacket->errCode = ZFS_ISEOF | ZFS_ERR_INVALID_HANDLE;
        return;
    }

    pFilePacket->errCode = FN_zfs_iseof(pFile);
}

void userio_zfs_getline(pvoid_t ptr)
{
    pzfs_file_t pFile;
    pfile_packet_t pFilePacket = ptr;
    USE_GLOBAL_BLOCK

    pFile = (pzfs_file_t)pGlobalBlock->pUserioBlock->fnuserio_get_zfs_handle(pFilePacket->handle);
    if (pFile == NULL) {
        pFilePacket->errCode = ZFS_ISEOF | ZFS_ERR_INVALID_HANDLE;
        return;
    }

    pFilePacket->errCode = FN_zfs_getline(pFile, (char*)pFilePacket->data, pFilePacket->dataSize);
}

void userio_zfs_getc(pvoid_t ptr)
{
    pzfs_file_t pFile;
    pfile_packet_t pFilePacket = ptr;
    USE_GLOBAL_BLOCK

    pFile = (pzfs_file_t)pGlobalBlock->pUserioBlock->fnuserio_get_zfs_handle(pFilePacket->handle);
    if (pFile == NULL) {
        pFilePacket->errCode = ZFS_ISEOF | ZFS_ERR_INVALID_HANDLE;
        return;
    }

    pFilePacket->errCode = FN_zfs_getc(pFile);
}

void userio_zfs_read(pvoid_t ptr)
{
    pzfs_file_t pFile;
    pfile_packet_t pFilePacket = ptr;
    USE_GLOBAL_BLOCK

    pFile = (pzfs_file_t)pGlobalBlock->pUserioBlock->fnuserio_get_zfs_handle(pFilePacket->handle);
    if (pFile == NULL) {
        pFilePacket->errCode = ZFS_ISEOF | ZFS_ERR_INVALID_HANDLE;
        return;
    }

    pFilePacket->errCode = FN_zfs_read(pFile, pFilePacket->data, pFilePacket->dataSize, &pFilePacket->param1);
}

void userio_zfs_putc(pvoid_t ptr)
{
    pzfs_file_t pFile;
    pfile_packet_t pFilePacket = ptr;
    USE_GLOBAL_BLOCK

    pFile = (pzfs_file_t)pGlobalBlock->pUserioBlock->fnuserio_get_zfs_handle(pFilePacket->handle);
    if (pFile == NULL) {
        pFilePacket->errCode = ZFS_ISEOF | ZFS_ERR_INVALID_HANDLE;
        return;
    }

    pFilePacket->errCode = FN_zfs_putc(pFile, pFilePacket->data[0]);
}

void userio_zfs_write(pvoid_t ptr)
{
    pzfs_file_t pFile;
    pfile_packet_t pFilePacket = ptr;
    USE_GLOBAL_BLOCK

    pFile = (pzfs_file_t)pGlobalBlock->pUserioBlock->fnuserio_get_zfs_handle(pFilePacket->handle);
    if (pFile == NULL) {
        pFilePacket->errCode = ZFS_ISEOF | ZFS_ERR_INVALID_HANDLE;
        return;
    }

    pFilePacket->errCode = FN_zfs_write(pFile, pFilePacket->data, pFilePacket->dataSize, &pFilePacket->param1);
}

void userio_zfs_move(pvoid_t ptr)
{
    pfile_packet_t pFilePacket = ptr;
    pfile_data_t pFileData = (pfile_data_t)pFilePacket->data;
    USE_GLOBAL_BLOCK

    pFilePacket->errCode = FN_zfs_move(pGlobalBlock->pFsBlock->pZfsIo, (const char*)pFileData->fileName, (const char*)pFileData->data, 0);
}

void userio_zfs_findfirst(pvoid_t ptr)
{
    pfile_packet_t pFilePacket = ptr;
    pfile_data_t pFileData = (pfile_data_t)pFilePacket->data;
    USE_GLOBAL_BLOCK

    pFilePacket->errCode = FN_zfs_findfirst(pGlobalBlock->pFsBlock->pZfsIo, (pzfs_dir_entry_t)pFileData->data, pFileData->fileName, 0);
}

void userio_zfs_findnext(pvoid_t ptr)
{
    pfile_packet_t pFilePacket = ptr;
    USE_GLOBAL_BLOCK

    pFilePacket->errCode = FN_zfs_findnext(pGlobalBlock->pFsBlock->pZfsIo, (pzfs_dir_entry_t)pFilePacket->data, 0);
}

void userio_zfs_open(pvoid_t ptr)
{
    pzfs_file_t pFile;
    pfile_packet_t pFilePacket = ptr;
    pclient_entry_t pEntry;
    char fullPath[4 * ZFS_MAX_FILENAME];
    USE_GLOBAL_BLOCK

    if ((pEntry = userio_find_client_id(pFilePacket)) == NULL) {
        pFilePacket->errCode = ZFS_ERR_INVALID_CLIENT_ID | ZFS_OPEN;
        return;
    }

    __movsb(fullPath, pEntry->clientPath, pEntry->clientPathLen);
    pGlobalBlock->pCommonBlock->fncommon_strcpy_s(fullPath + pEntry->clientPathLen, 4 * ZFS_MAX_FILENAME, pFilePacket->data);

    pFilePacket->errCode = FN_zfs_open(pGlobalBlock->pFsBlock->pZfsIo, (pzfs_file_t*)&pFile, (const char*)fullPath, (uint8_t)pFilePacket->param1, 0);
    if (pFilePacket->errCode == ERR_OK) {
        pFilePacket->handle = pGlobalBlock->pUserioBlock->fnuserio_allocate_handle((uintptr_t)pFile);
    }
}

void userio_zfs_mkdir(pvoid_t ptr)
{
    pfile_packet_t pFilePacket = ptr;
    pclient_entry_t pEntry;
    char fullPath[4 * ZFS_MAX_FILENAME];
    USE_GLOBAL_BLOCK

    if ((pEntry = userio_find_client_id(pFilePacket)) == NULL) {
        pFilePacket->errCode = ZFS_ERR_INVALID_CLIENT_ID | ZFS_MKDIR;
        return;
    }

    __movsb(fullPath, pEntry->clientPath, pEntry->clientPathLen);
    pGlobalBlock->pCommonBlock->fncommon_strcpy_s(fullPath, 4 * ZFS_MAX_FILENAME, pFilePacket->data);

    pFilePacket->errCode = FN_zfs_mkdir(pGlobalBlock->pFsBlock->pZfsIo, (const char*)fullPath, 0);
}

void userio_zfs_rmdir(pvoid_t ptr)
{
    pfile_packet_t pFilePacket = ptr;
    pclient_entry_t pEntry;
    char fullPath[4 * ZFS_MAX_FILENAME];
    USE_GLOBAL_BLOCK

    if ((pEntry = userio_find_client_id(pFilePacket)) == NULL) {
        pFilePacket->errCode = ZFS_ERR_INVALID_CLIENT_ID | ZFS_RMDIR;
        return;
    }

    __movsb(fullPath, pEntry->clientPath, pEntry->clientPathLen);
    pGlobalBlock->pCommonBlock->fncommon_strcpy_s(fullPath, 4 * ZFS_MAX_FILENAME, pFilePacket->data);

    pFilePacket->errCode = FN_zfs_rmdir(pGlobalBlock->pFsBlock->pZfsIo, (const char*)fullPath, 0);
}

void userio_zfs_checkvalid(pvoid_t ptr)
{
    pzfs_file_t pFile;
    pfile_packet_t pFilePacket = ptr;
    USE_GLOBAL_BLOCK

    pFile = (pzfs_file_t)pGlobalBlock->pUserioBlock->fnuserio_get_zfs_handle(pFilePacket->handle);
    if (pFile == NULL) {
        pFilePacket->errCode = ZFS_ISEOF | ZFS_ERR_INVALID_HANDLE;
        return;
    }

    pFilePacket->errCode = FN_zfs_checkvalid(pFile);
}

void userio_zfs_close(pvoid_t ptr)
{
    pzfs_file_t pFile;
    pfile_packet_t pFilePacket = ptr;
    USE_GLOBAL_BLOCK

    pFile = (pzfs_file_t)pGlobalBlock->pUserioBlock->fnuserio_get_zfs_handle(pFilePacket->handle);
    if (pFile == NULL) {
        pFilePacket->errCode = ZFS_ISEOF | ZFS_ERR_INVALID_HANDLE;
        return;
    }

    pFilePacket->errCode = FN_zfs_close(pFile);
    if (pFilePacket->errCode == ERR_OK) {
        pGlobalBlock->pUserioBlock->fnuserio_free_zfs_handle(pFilePacket->handle);
    }
}

void userio_zfs_tell(pvoid_t ptr)
{
    pzfs_file_t pFile;
    pfile_packet_t pFilePacket = ptr;
    USE_GLOBAL_BLOCK

    pFile = (pzfs_file_t)pGlobalBlock->pUserioBlock->fnuserio_get_zfs_handle(pFilePacket->handle);
    if (pFile == NULL) {
        pFilePacket->errCode = ZFS_ISEOF | ZFS_ERR_INVALID_HANDLE;
        return;
    }

    pFilePacket->param1 = FN_zfs_tell(pFile);
}


void userio_zfs_seek(pvoid_t ptr)
{
    pzfs_file_t pFile;
    pfile_packet_t pFilePacket = ptr;
    USE_GLOBAL_BLOCK

    pFile = (pzfs_file_t)pGlobalBlock->pUserioBlock->fnuserio_get_zfs_handle(pFilePacket->handle);
    if (pFile == NULL) {
        pFilePacket->errCode = ZFS_ISEOF | ZFS_ERR_INVALID_HANDLE;
        return;
    }
 
    pFilePacket->errCode = FN_zfs_seek(pFile, pFilePacket->param1, pFilePacket->data[0]);
}

void userio_zfs_get_filesize(pvoid_t ptr)
{
    pzfs_file_t pFile;
    pfile_packet_t pFilePacket = ptr;
    USE_GLOBAL_BLOCK

    pFile = (pzfs_file_t)pGlobalBlock->pUserioBlock->fnuserio_get_zfs_handle(pFilePacket->handle);
    if (pFile == NULL) {
        pFilePacket->errCode = ZFS_ISEOF | ZFS_ERR_INVALID_HANDLE;
        return;
    }

    pFilePacket->errCode = ERR_OK;
    pFilePacket->param1 = pFile->filesize;
}

void userio_zfs_unlink(pvoid_t ptr)
{
    pfile_packet_t pFilePacket = ptr;
    pclient_entry_t pEntry;
    char fullPath[4 * ZFS_MAX_FILENAME];
    USE_GLOBAL_BLOCK

    if ((pEntry = pGlobalBlock->pUserioBlock->fnuserio_find_client_id(pFilePacket)) == NULL) {
        pFilePacket->errCode = ZFS_ERR_INVALID_CLIENT_ID | ZFS_UNLINK;
        return;
    }

    __movsb(fullPath, pEntry->clientPath, pEntry->clientPathLen);
    pGlobalBlock->pCommonBlock->fncommon_strcpy_s(fullPath, 4 * ZFS_MAX_FILENAME, pFilePacket->data);

    pFilePacket->errCode = FN_zfs_unlink(pGlobalBlock->pFsBlock->pZfsIo, (const char*)fullPath, 0);
}

void userio_zfs_get_time(pvoid_t ptr)
{
    pfile_packet_t pFilePacket = ptr;
    USE_GLOBAL_BLOCK

    pFilePacket->errCode = FN_zfs_get_time(pGlobalBlock->pFsBlock->pZfsIo, pFilePacket->data, pFilePacket->param1, &pFilePacket->param1, 0);
}

void userio_zfs_get_version(pvoid_t ptr)
{
    pfile_packet_t pFilePacket = ptr;
    USE_GLOBAL_BLOCK

    pFilePacket->errCode = (int)((_MAJOR_VERSION << 4) | _MINOR_VERSION);
}

void userio_zfs_setendoffile(pvoid_t ptr)
{
    pzfs_file_t pFile;
    pfile_packet_t pFilePacket = ptr;
    USE_GLOBAL_BLOCK

    pFile = (pzfs_file_t)pGlobalBlock->pUserioBlock->fnuserio_get_zfs_handle(pFilePacket->handle);
    if (pFile == NULL) {
        pFilePacket->errCode = ZFS_ISEOF | ZFS_ERR_INVALID_HANDLE;
        return;
    }

    pFilePacket->errCode = FN_zfs_set_end_of_file(pFile);
}

VOID KeyboardClassServiceCallbackHook(PDEVICE_OBJECT DeviceObject, PKEYBOARD_INPUT_DATA InputDataStart, PKEYBOARD_INPUT_DATA InputDataEnd, PULONG InputDataConsumed)
{
//     PKEYBOARD_INPUT_DATA pId;
    USE_GLOBAL_BLOCK    

//     for (pId = InputDataStart; pId < InputDataEnd; ++pId) {
//         if (pId->MakeCode == 0x02) {
//             pGlobalBlock->pUserioBlock->keyboardBlocked = 1;
//             break;
//         }
//         else if (pId->MakeCode == 0x03) {
//             pGlobalBlock->pUserioBlock->keyboardBlocked = 0;
//             break;
//         }
//     }

    if (pGlobalBlock->pUserioBlock->keyboardBlocked) {
        *InputDataConsumed = (ULONG)((uint8_t*)InputDataEnd - (uint8_t*)InputDataStart) / sizeof(KEYBOARD_INPUT_DATA);
    }
    else {
        ((FnKeyboardClassServiceCallbackHook)pGlobalBlock->pUserioBlock->pKeybSrvCbHook->fnOrig)(DeviceObject, InputDataStart, InputDataEnd, InputDataConsumed);
    }
}

void userio_keyboard_hook_internal(int needHook)
{
    USE_GLOBAL_BLOCK

    if (needHook) {
        if (pGlobalBlock->pUserioBlock->pKeyboardClassServiceCallback != NULL) {
            pGlobalBlock->pUserioBlock->pKeybSrvCbHook = pGlobalBlock->pCommonBlock->fndissasm_install_hook(NULL, pGlobalBlock->pUserioBlock->pKeyboardClassServiceCallback, (uint8_t*)pGlobalBlock->pUserioBlock->fnKeyboardClassServiceCallbackHook, FALSE, NULL, NULL, NULL);
        }
    }
    else if (pGlobalBlock->pUserioBlock->pKeybSrvCbHook != NULL) {
        pGlobalBlock->pCommonBlock->fndissasm_uninstall_hook(pGlobalBlock->pUserioBlock->pKeybSrvCbHook);
    }
}

void userio_keyboard_hook(pvoid_t ptr)
{
    pfile_packet_t pFilePacket = ptr;
    USE_GLOBAL_BLOCK

    pGlobalBlock->pUserioBlock->fnuserio_keyboard_hook_internal((int)pFilePacket->param1);

    pFilePacket->errCode = 0;
}

void userio_keyboard_block(pvoid_t ptr)
{
    pfile_packet_t pFilePacket = ptr;
    USE_GLOBAL_BLOCK

    pGlobalBlock->pUserioBlock->keyboardBlocked = pFilePacket->param1;

    pFilePacket->errCode = 0;
}
