#include <Windows.h>
#include <tchar.h>
#include <intrin.h>
#include <stdio.h>

#include "zfs_proxy.h"

#define RC4_KEY "ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnop"

//int Main(HINSTANCE hInstance, HINSTANCE dwReason, LPSTR pReserved)
int __cdecl main(int argc, char** argv)
{
    DWORD iRet = 0;
    int* handle = 0;
    char buffer[200];
    unsigned int pointerseek = 0;
    zfs_dir_entry_t Dirent;
    unsigned int uTime, written;

// #if ZFS_PROXY_PROT_TYPE  == PROT_T_RSA 
//     if (GetPrivateKey(&g_RsaKey, 0) != ERR_OK) {
//         return EXIT_FAILURE;
//     }
// 
//     if (GetPrivateKey(&g_RsaKey_Drv, 1) != ERR_OK) {
//         return EXIT_FAILURE;
//     }
// #endif
// 
    if (zfs_init_proxy((unsigned char*)RC4_KEY) != 0) {
        return 0;
    }

    iRet = zfs_rmdir("\\test");
    printf("zfs_rmdir ret = %x\n", iRet);
    iRet = zfs_mkdir("\\test");
    printf("zfs_mkdir ret = %x\n", iRet);
    handle = 0;
    iRet = zfs_open(&handle, "\\test\\MyFile.txt", ZFS_MODE_CREATE | ZFS_MODE_READ | ZFS_MODE_WRITE);
    printf("zfs_open ret = %x\n", iRet);
    iRet = zfs_checkvalid(handle);
    printf("zfs_checkvalid ret = %x\n", iRet);
    __stosb((unsigned char*)buffer, 0, sizeof(buffer));
    __movsb((unsigned char*)buffer, (unsigned char*)"test", 5);
    iRet = zfs_write(handle, (unsigned char*)&buffer, sizeof(buffer), &written);
    printf("zfs_write ret = %x\n", iRet);
    iRet = zfs_putc(handle,'x');
    printf("zfs_putc ret = %x\n", iRet);
    iRet = zfs_tell(handle, &pointerseek);
    printf("zfs_tell ret = %x\n", iRet);
    iRet = zfs_get_filesize(handle, &written);
    if (iRet != 0) {
        printf("zfs_get_filesize failed = %x\n", iRet);
    }
    else {
        printf("zfs_get_filesize ret = %u\n", written);
    }
    
    iRet = zfs_close(handle);
    printf("zfs_close ret = %x\n", iRet);
    handle = 0;
    iRet = zfs_open(&handle, "\\test\\MyFile.txt", ZFS_MODE_READ | ZFS_MODE_WRITE);
    printf("zfs_open ret = %x\n", iRet);

    __stosb((unsigned char*)buffer,0,sizeof(buffer));
    iRet = zfs_read(handle, (unsigned char*)buffer, sizeof(buffer), NULL);
    printf("zfs_read ret = %x\n", iRet);
    iRet = zfs_getc(handle);
    printf("zfs_getc ret = %x\n", iRet);
    iRet = zfs_iseof(handle);
    printf("zfs_iseof ret = %x\n", iRet);
    iRet = zfs_seek(handle, 0, ZFS_SEEK_SET);
    printf("zfs_seek ret = %x\n", iRet);
    __stosb((unsigned char*)buffer,0,sizeof(buffer));
    iRet = zfs_getline(handle,(char*)buffer, sizeof(buffer));
    printf("zfs_getline ret = %x\n", iRet);
    if (iRet != -1)
        printf("zfs_getline = %s\n", buffer);

    iRet = zfs_close(handle);
    printf("zfs_close ret = %x\n", iRet);
    iRet = zfs_findfirst("\\test\\*.txt", &Dirent);
    printf("zfs_findfirst ret = %x\n", iRet);
    if (iRet != -1) {
        printf("zfs_findfirst = %s\n", &Dirent.fileName);

        iRet = zfs_findnext(&Dirent);
        printf("zfs_findnext ret = %x\n", iRet);
        if (iRet != -1) {
            printf("zfs_findnext = %s\n", &Dirent.fileName);
        }
    }

    iRet = zfs_get_time("\\test\\MyFile.txt", ZFS_CREATED_TIME, &uTime);
    printf("zfs_get_time ret = %x\n", iRet);
    if (iRet != -1) {
        printf("zfs_get_time = %u\n", uTime);
    }

    iRet = zfs_get_version();
    printf("zfs_get_version ret = %x\n", iRet);

    iRet = zfs_move("\\test\\MyFile.txt", "\\MyFile.txt");
    printf("zfs_move_file ret = %x\n", iRet);
    iRet = zfs_unlink("\\MyFile.txt");
    printf("zfs_unlink ret = %x\n", iRet);
    iRet = zfs_rmdir("\\test");
    printf("zfs_rmdir ret = %x\n", iRet);

    zfs_shutdown_proxy();
    
    return 0;
}
