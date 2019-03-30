#include "../../../shared_code/platform.h"
#include "../../../shared_code/types.h"
#include "../../../shared_code/native.h"
#include "../../../shared_code/aes.h"
#include <stdio.h>
#include <Windows.h>

#define USE_AES_ROM_TABLES 1
#define SHARED_HAVE_X86 1
#include "../../../shared_code/aes.c"

#define RING3 1
#include "../../zfs/zfs.c"

#include "../../../shared_code/havege.h"
#include "../../../shared_code/havege.c"

int __cdecl main()
{
 	int err = ERR_OK;
 	pzfs_io_manager_t pIoman;
	zfs_file_t* ffFile;
    char buffer[200];
    uint32_t pointerseek;
    zfs_dir_entry_t dirEntry;
    uint32_t fTime;
    uint32_t i, written, readed;

// 
//     printf("ZFS_UNMOUNTPARTITION: %04x\n", ZFS_UNMOUNTPARTITION);
//     printf("ZFS_FLUSHCACHE: %04x\n", ZFS_FLUSHCACHE);
//     printf("ZFS_BLOCKREAD: %04x\n", ZFS_BLOCKREAD);
//     printf("ZFS_BLOCKWRITE: %04x\n", ZFS_BLOCKWRITE);
//     printf("ZFS_USERDRIVER: %04x\n", ZFS_USERDRIVER);
//     printf("ZFS_FINDNEXTINDIR: %04x\n", ZFS_FINDNEXTINDIR);
//     printf("ZFS_FETCHENTRYWITHCONTEXT: %04x\n", ZFS_FETCHENTRYWITHCONTEXT);
//     printf("ZFS_PUSHENTRYWITHCONTEXT: %04x\n", ZFS_PUSHENTRYWITHCONTEXT);
//     printf("ZFS_GETDIRENTRY: %04x\n", ZFS_GETDIRENTRY);
//     printf("ZFS_FINDFIRST: %04x\n", ZFS_FINDFIRST);
//     printf("ZFS_FINDNEXT: %04x\n", ZFS_FINDNEXT);
//     printf("ZFS_REWINDFIND: %04x\n", ZFS_REWINDFIND);
//     printf("ZFS_FINDFREEDIRENT: %04x\n", ZFS_FINDFREEDIRENT);
//     printf("ZFS_PUTENTRY: %04x\n", ZFS_PUTENTRY);
//     printf("ZFS_CREATESHORTNAME: %04x\n", ZFS_CREATESHORTNAME);
//     printf("ZFS_EXTENDDIRECTORY: %04x\n", ZFS_EXTENDDIRECTORY);
//     printf("ZFS_MKDIR: %04x\n", ZFS_MKDIR);
//     printf("ZFS_OPEN: %04x\n", ZFS_OPEN);
//     printf("ZFS_RMDIR: %04x\n", ZFS_RMDIR);
//     printf("ZFS_MOVE: %04x\n", ZFS_MOVE);
//     printf("ZFS_EXTENDFILE: %04x\n", ZFS_EXTENDFILE);
//     printf("ZFS_READ: %04x\n", ZFS_READ);
//     printf("ZFS_GETC: %04x\n", ZFS_GETC);
//     printf("ZFS_GETLINE: %04x\n", ZFS_GETLINE);
//     printf("ZFS_WRITE: %04x\n", ZFS_WRITE);
//     printf("ZFS_PUTC: %04x\n", ZFS_PUTC);
//     printf("ZFS_SEEK: %04x\n", ZFS_SEEK);
//     printf("ZFS_CHECKVALID: %04x\n", ZFS_CHECKVALID);
//     printf("ZFS_CLOSE: %04x\n", ZFS_CLOSE);
//     printf("ZFS_BYTESLEFT: %04x\n", ZFS_BYTESLEFT);
//     printf("ZFS_SETTIME: %04x\n", ZFS_SETTIME);
//     printf("ZFS_SETFILETIME: %04x\n", ZFS_SETFILETIME);
//     printf("ZFS_GETENTRY: %04x\n", ZFS_GETENTRY);
//     printf("ZFS_CLEARCLUSTER: %04x\n", ZFS_CLEARCLUSTER);
//     printf("ZFS_FINDFREECLUSTER: %04x\n", ZFS_FINDFREECLUSTER);
//     printf("ZFS_PUTZFSENTRY: %04x\n", ZFS_PUTZFSENTRY);
//     printf("ZFS_COUNTFREECLUSTERS: %04x\n", ZFS_COUNTFREECLUSTERS);

    pIoman = zfs_create_io_manager(16 * BDEV_BLOCK_SIZE);

    if (pIoman) {
        if (zfs_open_device(pIoman, "test.img", (uint8_t*)"qweasdzxc", 9) != ERR_OK) {
            bdev_create("test.img", (uint32_t)1024 * 1024 * 1024 * 3);

            if (zfs_open_device(pIoman, "test.img", (uint8_t*)"qweasdzxc", 9) != ERR_OK) {
                zfs_destroy_io_manager(pIoman);
                return -1;
            }

            zfs_format(pIoman);

//             zfs_close_device(pIoman);
//             zfs_destroy_io_manager(pIoman);

        }

        if (zfs_mount(pIoman) != ERR_OK) {
            zfs_close_device(pIoman);
            zfs_destroy_io_manager(pIoman);
            return -1;
        }




        err = zfs_mkdir(pIoman, "\\a3424", 0);
        err = zfs_rmdir(pIoman, "\\te57v634nt8t8t8t8t8t8t8t8t8t8t86", ZFS_SPECIAL_SYSTEM);

        err = zfs_mkdir(pIoman, "\\te57v634nt8t8t8t8t8t8t8t8t8t8t86", ZFS_SPECIAL_SYSTEM);

        err = zfs_mkdir(pIoman, "\\bsdfs", 0);

        err = zfs_open(pIoman, &ffFile, "\\te57v634nt8t8t8t8t8t8t8t8t8t8t86\\propogation_in_mitigation.dat", ZFS_MODE_CREATE | ZFS_MODE_WRITE | ZFS_MODE_TRUNCATE, 0);

        err = zfs_open(pIoman, &ffFile, "\\te57v634nt8t8t8t8t8t8t8t8t8t8t86\\propogation_in_mitigation.dat", ZFS_MODE_CREATE | ZFS_MODE_WRITE | ZFS_MODE_TRUNCATE, ZFS_SPECIAL_SYSTEM);

//         err = zfs_checkvalid(ffFile);
//         
//         memset(buffer,0,sizeof(buffer));
//         strcpy(buffer, "test.");
// 
//         for (i = 0; i < 2; ++i) {
//             err = zfs_write(ffFile, (uint8_t*)buffer, strlen("test."), &written);
//         }
        zfs_close(ffFile);

// //         err = zfs_open(pIoman, &ffFile, "\\te57v634nt8t8t8t8t8t8t8t8t8t8t86\\propogation_in_mitigation.dat", ZFS_MODE_READ);
// //         memset(buffer,0,sizeof(buffer));
// //         zfs_read(ffFile, (uint8_t*)buffer, ffFile->filesize, &readed);
// //         printf("Before tuncating: %s\n", buffer);
// //         zfs_close(ffFile);
// 
//         err = zfs_open(pIoman, &ffFile, "\\te57v634nt8t8t8t8t8t8t8t8t8t8t86\\propogation_in_mitigation.dat", ZFS_MODE_READ | ZFS_MODE_WRITE, 1);
// 
//         memset(buffer,0,sizeof(buffer));
//         strcpy(buffer, "test.");
//         err = zfs_write(ffFile, (uint8_t*)buffer, strlen("test."), &written);
//         //iRet = write_file(handle, (unsigned char*)&buffer, sizeof(buffer));
// //         zfs_seek(ffFile, 0, ZFS_SEEK_END);
// //         pointerseek = zfs_tell(ffFile);
// //         printf("File size: %u\n", ffFile->filesize);
// //         //iRet = get_file_poiner(handle, &pointerseek);
// //         zfs_seek(ffFile, pointerseek / 2, ZFS_SEEK_SET);
//         zfs_set_end_of_file(ffFile);
// //         printf("File size after zfs_set_end_of_file(): %u\n", ffFile->filesize);
// //         zfs_seek(ffFile, 0, ZFS_SEEK_SET);
// //         memset(buffer,0,sizeof(buffer));
// //         zfs_read(ffFile, (uint8_t*)buffer, ffFile->filesize * 2, &readed);
// //         printf("After tuncating: %s\n", buffer);
//         zfs_close(ffFile);
// 
//         err = zfs_open(pIoman, &ffFile, "\\te57v634nt8t8t8t8t8t8t8t8t8t8t86\\propogation_in_mitigation.dat", ZFS_MODE_READ, 1);
//         memset(buffer,0,sizeof(buffer));
//         zfs_read(ffFile, (uint8_t*)buffer, ffFile->filesize, &readed);
//         printf("After truncating (readonly mode): %s\n", buffer);
//         zfs_close(ffFile);
// 
//         err = zfs_get_time(pIoman, "\\te57v634nt8t8t8t8t8t8t8t8t8t8t86\\propogation_in_mitigation.dat", ETimeMod, &fTime, ZFS_SPECIAL_SYSTEM);
//      
// //         handle = 0;
// //         iRet = create_file(L"\\test\\MyFile.txt",&handle, 1);
// //         memset(buffer,0,sizeof(buffer));
// //         iRet = read_file(handle, (unsigned char*)&buffer, sizeof(buffer));
// // 
// //         iRet = getc_file(handle,&ch);
// // 
// //         iRet = is_eof_file(handle, &bEOF);
// // 
// //         iRet = set_file_poiner(handle, 0, ZFS_SEEK_SET);
// // 
// //         memset(buffer,0,sizeof(buffer));
// //         iRet = getline_file(handle,(char*)&buffer, sizeof(buffer));
// // 
// //         if (iRet == 0)
// //             printf("getline = %s\n", &buffer);
// // 
// //         iRet = close_file(handle);
// // 
//         memset(&dirEntry, 0, sizeof(zfs_dir_entry_t));
//         err = zfs_findfirst(pIoman, &dirEntry, "\\*", 0);
//         //err = findfirst_file(L"\\test\\*.txt", &dirEntry);
// 
// //         if (err == 0)
// //         {
// //             printf("find file = %s\n", &Dirent.fileName);
// //         }
// 
//         zfs_findnext(pIoman, &dirEntry, 0);
//         //err = findnext_file(&Dirent);
// 
// //         if (err == 0)
// //         {
// //             printf("find file = %s\n", &Dirent.fileName);
// //         }
// // 
// //         iRet = move_file(L"\\test\\MyFile.txt", L"\\MyFile.txt");
// // 
// //         iRet = delete_file(L"\\MyFile.txt");
// // 
// //         iRet = remove_dir(L"\\test");
// // 
// //         free_file_interface();
// 
//         zfs_rmdir(pIoman, "\\test_dir", 1);
// 	    
//         err = zfs_mkdir(pIoman, "\\test_dir", 1);
// 
//         err = zfs_mkdir(pIoman, "\\test_dir", 0);
// 
//         zfs_rmdir(pIoman, "\\test_dir", 0);
// 
//         if (!ZFS_isERR(err)) {
//             err = zfs_open(pIoman, &ffFile, "\\test_dir\\test.txt", ZFS_MODE_CREATE | ZFS_MODE_WRITE, 1);
// 
//             if (err == ERR_OK) {
//                 zfs_write(ffFile, (uint8_t*)"Hello, World!", 22, &written);
//                 zfs_close(ffFile);
// 
//                 err = zfs_open(pIoman, &ffFile, "\\test_dir\\test.txt", ZFS_MODE_READ, 1);
// 
//                 if (err == ERR_OK) {
//                     char buff[32];
//                     zfs_read(ffFile, (uint8_t*)buff, 22, &readed);
//                     printf("%s\n", buff);
//                     zfs_close(ffFile);
//                 }
// 
//                 zfs_unlink(pIoman, "\\test_dir\\test.txt", 1);
//                 err = zfs_open(pIoman, &ffFile, "\\test_dir\\test.txt", ZFS_MODE_READ, 1);
//                 if (ZFS_isERR(err)) {
//                     printf("\\test_dir\\test.txt successfully deleted!\n");
//                 }
// 
//             }
//         }
// 
        printf("FS free space: %lld Mb\n", zfs_get_free_size(pIoman, &err) / 1024 / 1024);

	    zfs_unmount(pIoman);
        zfs_close_device(pIoman);
	    zfs_destroy_io_manager(pIoman);
	    return 0;
    }
	return -1;
}
