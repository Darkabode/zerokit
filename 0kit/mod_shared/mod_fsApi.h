#ifndef __MOD_FSAPI_H_
#define __MOD_FSAPI_H_

typedef void (*Fnzfs_tolower)(char* string, uint32_t strLen);
typedef void (*Fnzfs_toupper)(char* string, uint32_t strLen);
typedef char (*Fnzfs_strmatch)(const char* str1, const char* str2, uint16_t len);
typedef char* (*Fnzfs_strtok)(const char* string, char* token, uint16_t *tokenNumber, char* last, uint16_t Length);
typedef char (*Fnzfs_wildcompare)(const char* pszWildCard, const char* pszString);

#include "../../zfs/zfs/zfsdef.h"
#include "../../zfs/zfs/config.h"
#include "../../zfs/zfs/error.h"
#include "../../zfs/zfs/blockdev.h"
#include "../../zfs/zfs/ioman.h"
#include "../../zfs/zfs/format.h"
#include "../../zfs/zfs/dir.h"
#include "../../zfs/zfs/file.h"

typedef int (*Fnbdev_native_open)(BlockDriverState* bs, const char* fileName);
typedef int (*Fnbdev_narive_read)(BlockDriverState* bs, uint8_t* buffer, uint32_t sector, uint16_t sectors);
typedef int (*Fnbdev_native_write)(BlockDriverState* bs, const uint8_t* buffer, ulong_t sector, uint16_t sectors);
typedef int (*Fnbdev_pread)(BlockDriverState* bs, uint32_t offset, uint8_t *buf, int count1);
typedef int (*Fnbdev_pwrite)(BlockDriverState* bs, uint32_t offset, const uint8_t* buf, int count1);
typedef uint32_t (*Fnbdev_write_full)(HANDLE hFile, const uint8_t* buf, uint32_t count);
typedef int (*Fnbdev_create)(const char* fileName, uint32_t virtSize);
typedef int (*Fnbdev_ptruncate)(HANDLE hFile, uint32_t offset);
typedef int (*Fnbdev_make_empty)(BlockDriverState* bs);
typedef int (*Fnbdev_open)(BlockDriverState** pbs, const char* filename);
typedef int (*Fnbdev_set_key)(BlockDriverState *bs, const uint8_t* key, uint32_t keySize);
typedef void (*Fnbdev_close)(BlockDriverState* bs);
typedef uint32_t (*Fnbdev_getlength)(HANDLE hFile);
typedef void (*Fnbdev_encrypt_sectors)(uint32_t sector_num, uint8_t *out_buf, const uint8_t *in_buf, uint32_t nb_sectors, int enc, const paes_context_t pCtx);
typedef uint32_t (*Fnbdev_get_cluster_offset)(BlockDriverState *bs, uint32_t offset, int allocate, int n_start, int n_end);
typedef int (*Fnbdev_read)(uint8_t* buf, uint32_t sector_num, uint32_t nb_sectors, BlockDriverState* bs);
typedef int (*Fnbdev_write)(const uint8_t *buf, uint32_t sector_num, uint32_t nb_sectors, BlockDriverState* bs);

typedef void (*Fnzfs_put_boot)(zfsparams * ft, unsigned char* boot);
typedef void (*Fnzfs_put_fs_info)(unsigned char* sector, zfsparams *ft);
typedef int (*Fnzfs_format)(pzfs_io_manager_t pIoman);

typedef uint32_t (*Fnzfs_get_cluster_chain_number)(uint32_t nEntry, uint16_t nEntrySize);
typedef uint32_t (*Fnzfs_get_cluster_position)(uint32_t nEntry, uint16_t nEntrySize);
typedef uint32_t (*Fnzfs_get_major_block_number)(uint32_t nEntry, uint16_t nEntrySize);
typedef uint8_t (*Fnzfs_get_minor_block_number)(uint32_t nEntry, uint16_t nEntrySize);
typedef uint32_t (*Fnzfs_get_minor_block_entry)(uint32_t nEntry, uint16_t nEntrySize);

typedef void (*Fnzfs_init_buffer_descriptors)(pzfs_io_manager_t pIoman);
typedef pzfs_io_manager_t (*Fnzfs_create_io_manager)(uint16_t size);
typedef void (*Fnzfs_destroy_io_manager)(pzfs_io_manager_t pIoman);
typedef int (*Fnzfs_flush_cache)(pzfs_io_manager_t pIoman);
typedef pzfs_buffer_t (*Fnzfs_get_buffer)(pzfs_io_manager_t pIoman, uint32_t Sector, uint8_t Mode);
typedef void (*Fnzfs_release_buffer)(pzfs_io_manager_t pIoman, pzfs_buffer_t pBuffer);
typedef int (*Fnzfs_read_block)(pzfs_io_manager_t pIoman, uint32_t ulSectorLBA, uint32_t ulNumSectors, void *pBuffer);
typedef int (*Fnzfs_write_block)(pzfs_io_manager_t pIoman, uint32_t ulSectorLBA, uint32_t ulNumSectors, void *pBuffer);
typedef int (*Fnzfs_mount)(pzfs_io_manager_t pIoman);
typedef int (*Fnzfs_open_device)(pzfs_io_manager_t pIoman, const char* fsPath, uint8_t* fsKey, uint32_t keySize);
typedef void (*Fnzfs_close_device)(pzfs_io_manager_t pIoman);
typedef char (*Fnzfs_active_handles)(pzfs_io_manager_t pIoman);
typedef int (*Fnzfs_unmount)(pzfs_io_manager_t pIoman);
typedef int (*Fnzfs_increase_free_clusters)(pzfs_io_manager_t pIoman, uint32_t Count);
typedef int (*Fnzfs_decrease_free_clusters)(pzfs_io_manager_t pIoman, uint32_t Count);
typedef uint32_t (*Fnzfs_get_size)(pzfs_io_manager_t pIoman);

typedef void (*Fnzfs_process_short_name)(char* name);
typedef void (*Fnzfs_lock_dir)(pzfs_io_manager_t pIoman);
typedef void (*Fnzfs_unlock_dir)(pzfs_io_manager_t pIoman);
typedef int (*Fnzfs_find_next_in_dir)(pzfs_io_manager_t pIoman, pzfs_dir_entry_t pDirent, pzfs_fetch_context_t pFetchContext);
typedef char (*Fnzfs_short_name_exists)(pzfs_io_manager_t pIoman, uint32_t ulDirCluster, char* szShortName, int*pError);
typedef uint32_t (*Fnzfs_find_entry_in_dir)(pzfs_io_manager_t pIoman, uint32_t DirCluster, const char* name, uint8_t pa_Attrib, pzfs_dir_entry_t pDirent, int*pError);
typedef uint32_t (*Fnzfs_find_dir)(pzfs_io_manager_t pIoman, const char* path, uint16_t pathLen, uint8_t special, int* pError);
typedef void (*Fnzfs_case_name)(char* name, uint8_t attrib);
typedef void (*Fnzfs_populate_short_dirent)(pzfs_dir_entry_t pDirent, uint8_t *EntryBuffer);
typedef int (*Fnzfs_init_entry_fetch)(pzfs_io_manager_t pIoman, uint32_t ulDirCluster, pzfs_fetch_context_t pContext);
typedef void (*Fnzfs_cleanup_entry_fetch)(pzfs_io_manager_t pIoman, pzfs_fetch_context_t pContext);
typedef int (*Fnzfs_fetch_entry_with_context)(pzfs_io_manager_t pIoman, uint32_t ulEntry, pzfs_fetch_context_t pContext, uint8_t *pEntryBuffer);
typedef int (*Fnzfs_push_entry_with_context)(pzfs_io_manager_t pIoman, uint32_t ulEntry, pzfs_fetch_context_t pContext, uint8_t *pEntryBuffer);
typedef int (*Fnzfs_get_dir_entry)(pzfs_io_manager_t pIoman, uint16_t nEntry, uint32_t DirCluster, pzfs_dir_entry_t pDirent);
typedef char (*Fnzfs_is_end_of_dir)(uint8_t *EntryBuffer);
typedef int (*Fnzfs_findfirst)(pzfs_io_manager_t pIoman, pzfs_dir_entry_t pDirent, const char* path, uint8_t special);
typedef int (*Fnzfs_findnext)(pzfs_io_manager_t pIoman, pzfs_dir_entry_t pDirent, uint8_t special);
typedef int (*Fnzfs_rewindfind)(pzfs_io_manager_t pIoman, pzfs_dir_entry_t pDirent);
typedef int (*Fnzfs_find_free_dirent)(pzfs_io_manager_t pIoman, uint32_t dirCluster, uint16_t sequential);
typedef int (*Fnzfs_put_dir_entry)(pzfs_io_manager_t pIoman, uint16_t Entry, uint32_t DirCluster, pzfs_dir_entry_t pDirent);
typedef int (*Fnzfs_create_name)(pzfs_io_manager_t pIoman, uint32_t DirCluster, char* ShortName, char* LongName);
typedef int (*Fnzfs_extend_directory)(pzfs_io_manager_t pIoman, uint32_t DirCluster);
typedef void (*Fnzfs_make_name_compliant)(char* name);
typedef int (*Fnzfs_create_dirent)(pzfs_io_manager_t pIoman, uint32_t DirCluster, pzfs_dir_entry_t pDirent);
typedef uint32_t (*Fnzfs_create_file)(pzfs_io_manager_t pIoman, uint32_t DirCluster, char* FileName, pzfs_dir_entry_t pDirent, uint8_t special, int*pError);
typedef int (*Fnzfs_mkdir)(pzfs_io_manager_t pIoman, const char* path, uint8_t special);
typedef int (*Fnzfs_rm_lfns)(pzfs_io_manager_t pIoman, uint16_t usDirEntry, pzfs_fetch_context_t pContext);

typedef void (*Fnzfs_lock)(pzfs_io_manager_t pIoman);
typedef void (*Fnzfs_unlock)(pzfs_io_manager_t pIoman);
typedef uint32_t (*Fnzfs_cluster_to_lba)(pzfs_io_manager_t pIoman, uint32_t Cluster);
typedef uint32_t (*Fnzfs_lba_to_cluster)(pzfs_io_manager_t pIoman, uint32_t Address);
typedef uint32_t (*Fnzfs_get_entry)(pzfs_io_manager_t pIoman, uint32_t nCluster, int*pError);
typedef int (*Fnzfs_clear_cluster)(pzfs_io_manager_t pIoman, uint32_t nCluster);
typedef uint32_t (*Fnzfs_traverse)(pzfs_io_manager_t pIoman, uint32_t Start, uint32_t Count, int*pError);
typedef uint32_t (*Fnzfs_find_end_of_chain)(pzfs_io_manager_t pIoman, uint32_t Start, int*pError);
typedef char (*Fnzfs_is_end_of_chain)(uint32_t zfsEntry);
typedef int (*Fnzfs_put_entry)(pzfs_io_manager_t pIoman, uint32_t nCluster, uint32_t Value);
typedef uint32_t (*Fnzfs_find_free_cluster)(pzfs_io_manager_t pIoman, int*pError);
typedef uint32_t (*Fnzfs_create_cluster_chain)(pzfs_io_manager_t pIoman, int*pError);
typedef uint32_t (*Fnzfs_get_chain_length)(pzfs_io_manager_t pIoman, uint32_t startCluster, uint32_t *piEndOfChain, int*pError);
typedef int (*Fnzfs_unlink_cluster_chain)(pzfs_io_manager_t pIoman, uint32_t startCluster);
typedef uint32_t (*Fnzfs_count_free_clusters)(pzfs_io_manager_t pIoman, int*pError);
typedef uint32_t (*Fnzfs_get_free_size)(pzfs_io_manager_t pIoman, int*pError);

typedef uint8_t (*Fnzfs_getmodebits)(char* mode);
typedef int (*Fnzfs_open)(pzfs_io_manager_t pIoman, pzfs_file_t* ppFile, const char* path, uint8_t mode, uint8_t special);
typedef char (*Fnzfs_is_dir_empty)(pzfs_io_manager_t pIoman, const char* path, uint8_t special);
typedef int (*Fnzfs_rmdir)(pzfs_io_manager_t pIoman, const char* path, uint8_t special);
typedef int (*Fnzfs_unlink)(pzfs_io_manager_t pIoman, const char* path, uint8_t special);
typedef int (*Fnzfs_move)(pzfs_io_manager_t pIoman, const char* szSourceFile, const char* szDestinationFile, uint8_t special);
typedef char (*Fnzfs_iseof)(pzfs_file_t pFile);
typedef int (*Fnzfs_bytesleft)(pzfs_file_t pFile);
typedef uint32_t (*Fnzfs_get_sequential_clusters)(pzfs_io_manager_t pIoman, uint32_t StartCluster, uint32_t Limit, int* pError);
typedef int (*Fnzfs_read_clusters)(pzfs_file_t pFile, uint32_t Count, uint8_t *buffer);
typedef int (*Fnzfs_extend_file)(pzfs_file_t pFile, uint32_t Size);
typedef int (*Fnzfs_write_clusters)(pzfs_file_t pFile, uint32_t Count, uint8_t *buffer);
typedef int (*Fnzfs_read)(pzfs_file_t pFile, uint8_t* buffer, uint32_t size, uint32_t* pReaded);
typedef int (*Fnzfs_getc)(pzfs_file_t pFile);
typedef int (*Fnzfs_getline)(pzfs_file_t pFile, char* szLine, uint32_t ulLimit);
typedef int (*Fnzfs_write)(pzfs_file_t pFile, uint8_t* buffer, uint32_t size, uint32_t* pWritten);
typedef int (*Fnzfs_putc)(pzfs_file_t pFile, uint8_t pa_cValue);
typedef uint32_t (*Fnzfs_tell)(pzfs_file_t pFile);
typedef int (*Fnzfs_seek)(pzfs_file_t pFile, int Offset, char Origin);
typedef int (*Fnzfs_checkvalid)(pzfs_file_t pFile);
typedef int (*Fnzfs_set_end_of_file)(pzfs_file_t pFile);
typedef int (*Fnzfs_set_time)(pzfs_io_manager_t pIoman, const char* path, uint32_t unixTime, uint32_t aWhat, uint8_t special);
typedef int (*Fnzfs_get_time)(pzfs_io_manager_t pIoman, const char* path, uint32_t aWhat, uint32_t* pUnixTime, uint8_t special);
typedef int (*Fnzfs_close)(pzfs_file_t pFile);

typedef void (*Fnfs_shutdown_routine)();

typedef struct _mod_fs_private
{
    Fnfs_shutdown_routine fnfs_shutdown_routine;

    Fnzfs_tolower fnzfs_tolower;
    Fnzfs_toupper fnzfs_toupper;
    Fnzfs_strmatch fnzfs_strmatch;
    Fnzfs_strtok fnzfs_strtok;
    Fnzfs_wildcompare fnzfs_wildcompare;

    Fnbdev_native_open fnbdev_native_open;
    Fnbdev_narive_read fnbdev_narive_read;
    Fnbdev_native_write fnbdev_native_write;
    Fnbdev_pread fnbdev_pread;
    Fnbdev_pwrite fnbdev_pwrite;
    Fnbdev_write_full fnbdev_write_full;
    Fnbdev_create fnbdev_create;
    Fnbdev_ptruncate fnbdev_ptruncate;
    //Fnbdev_make_empty fnbdev_make_empty;
    Fnbdev_open fnbdev_open;
    Fnbdev_set_key fnbdev_set_key;
    Fnbdev_close fnbdev_close;
    Fnbdev_getlength fnbdev_getlength;
    Fnbdev_encrypt_sectors fnbdev_encrypt_sectors;
    Fnbdev_get_cluster_offset fnbdev_get_cluster_offset;
    Fnbdev_read fnbdev_read;
    Fnbdev_write fnbdev_write;

    Fnzfs_put_boot fnzfs_put_boot;
    Fnzfs_put_fs_info fnzfs_put_fs_info;
    Fnzfs_format fnzfs_format;

    Fnzfs_get_cluster_chain_number fnzfs_get_cluster_chain_number;
    Fnzfs_get_cluster_position fnzfs_get_cluster_position;
    Fnzfs_get_major_block_number fnzfs_get_major_block_number;
    Fnzfs_get_minor_block_number fnzfs_get_minor_block_number;
    Fnzfs_get_minor_block_entry fnzfs_get_minor_block_entry;

    Fnzfs_init_buffer_descriptors fnzfs_init_buffer_descriptors;
    Fnzfs_create_io_manager fnzfs_create_io_manager;
    Fnzfs_destroy_io_manager fnzfs_destroy_io_manager;
    Fnzfs_flush_cache fnzfs_flush_cache;
    Fnzfs_get_buffer fnzfs_get_buffer;
    Fnzfs_release_buffer fnzfs_release_buffer;
    Fnzfs_read_block fnzfs_read_block;
    Fnzfs_write_block fnzfs_write_block;
    Fnzfs_mount fnzfs_mount;
    Fnzfs_open_device fnzfs_open_device;
    Fnzfs_close_device fnzfs_close_device;
    Fnzfs_active_handles fnzfs_active_handles;
    Fnzfs_unmount fnzfs_unmount;
    Fnzfs_increase_free_clusters fnzfs_increase_free_clusters;
    Fnzfs_decrease_free_clusters fnzfs_decrease_free_clusters;
    Fnzfs_get_size fnzfs_get_size;

    Fnzfs_process_short_name fnzfs_process_short_name;
    Fnzfs_lock_dir fnzfs_lock_dir;
    Fnzfs_unlock_dir fnzfs_unlock_dir;
    Fnzfs_find_next_in_dir fnzfs_find_next_in_dir;
    Fnzfs_short_name_exists fnzfs_short_name_exists;
    Fnzfs_find_entry_in_dir fnzfs_find_entry_in_dir;
    Fnzfs_find_dir fnzfs_find_dir;
    Fnzfs_populate_short_dirent fnzfs_populate_short_dirent;
    Fnzfs_init_entry_fetch fnzfs_init_entry_fetch;
    Fnzfs_cleanup_entry_fetch fnzfs_cleanup_entry_fetch;
    Fnzfs_fetch_entry_with_context fnzfs_fetch_entry_with_context;
    Fnzfs_push_entry_with_context fnzfs_push_entry_with_context;
    Fnzfs_get_dir_entry fnzfs_get_dir_entry;
    Fnzfs_is_end_of_dir fnzfs_is_end_of_dir;
    Fnzfs_findfirst fnzfs_findfirst;
    Fnzfs_findnext fnzfs_findnext;
    Fnzfs_rewindfind fnzfs_rewindfind;
    Fnzfs_find_free_dirent fnzfs_find_free_dirent;
    Fnzfs_put_dir_entry fnzfs_put_dir_entry;
    Fnzfs_create_name fnzfs_create_name;
    Fnzfs_extend_directory fnzfs_extend_directory;
    Fnzfs_make_name_compliant fnzfs_make_name_compliant;
    Fnzfs_create_dirent fnzfs_create_dirent;
    Fnzfs_create_file fnzfs_create_file;
    Fnzfs_mkdir fnzfs_mkdir;
    Fnzfs_rm_lfns fnzfs_rm_lfns;

    Fnzfs_lock fnzfs_lock;
    Fnzfs_unlock fnzfs_unlock;
    Fnzfs_cluster_to_lba fnzfs_cluster_to_lba;
//    Fnzfs_lba_to_cluster fnzfs_lba_to_cluster;
    Fnzfs_get_entry fnzfs_get_entry;
    Fnzfs_clear_cluster fnzfs_clear_cluster;
    Fnzfs_traverse fnzfs_traverse;
    Fnzfs_find_end_of_chain fnzfs_find_end_of_chain;
    Fnzfs_is_end_of_chain fnzfs_is_end_of_chain;
    Fnzfs_put_entry fnzfs_put_entry;
    Fnzfs_find_free_cluster fnzfs_find_free_cluster;
    Fnzfs_create_cluster_chain fnzfs_create_cluster_chain;
    Fnzfs_get_chain_length fnzfs_get_chain_length;
    Fnzfs_unlink_cluster_chain fnzfs_unlink_cluster_chain;
    Fnzfs_count_free_clusters fnzfs_count_free_clusters;
    Fnzfs_get_free_size fnzfs_get_free_size;

    Fnzfs_getmodebits fnzfs_getmodebits;
    Fnzfs_open fnzfs_open;
    Fnzfs_is_dir_empty fnzfs_is_dir_empty;
    Fnzfs_rmdir fnzfs_rmdir;
    Fnzfs_unlink fnzfs_unlink;
    Fnzfs_move fnzfs_move;
    Fnzfs_iseof fnzfs_iseof;
    Fnzfs_bytesleft fnzfs_bytesleft;
    Fnzfs_get_sequential_clusters fnzfs_get_sequential_clusters;
    Fnzfs_read_clusters fnzfs_read_clusters;
    Fnzfs_extend_file fnzfs_extend_file;
    Fnzfs_write_clusters fnzfs_write_clusters;
    Fnzfs_read fnzfs_read;
    Fnzfs_getc fnzfs_getc;
    Fnzfs_getline fnzfs_getline;
    Fnzfs_write fnzfs_write;
    Fnzfs_putc fnzfs_putc;
    Fnzfs_tell fnzfs_tell;
    Fnzfs_seek fnzfs_seek;
    Fnzfs_checkvalid fnzfs_checkvalid;
    Fnzfs_set_end_of_file fnzfs_set_end_of_file;
    Fnzfs_set_time fnzfs_set_time;
    Fnzfs_get_time fnzfs_get_time;
    Fnzfs_close fnzfs_close;

    uint8_t* pModBase;
} mod_fs_private_t, *pmod_fs_private_t;

typedef struct _mod_fs_block
{


    pzfs_io_manager_t pZfsIo;

	mod_fs_private_t;
} mod_fs_block_t, *pmod_fs_block_t;

#define FN_zfs_tolower pGlobalBlock->pFsBlock->fnzfs_tolower
#define FN_zfs_strmatch pGlobalBlock->pFsBlock->fnzfs_strmatch
#define FN_zfs_strtok pGlobalBlock->pFsBlock->fnzfs_strtok
#define FN_zfs_wildcompare pGlobalBlock->pFsBlock->fnzfs_wildcompare
#define FN_ZFS_GET_SYSTEM_TIME pGlobalBlock->pCommonBlock->fncommon_get_system_time

/////////////////////////////////////////////////////////////////////////

#define FN_bdev_native_open pGlobalBlock->pFsBlock->fnbdev_native_open
#define FN_bdev_native_read pGlobalBlock->pFsBlock->fnbdev_narive_read
#define FN_bdev_native_write pGlobalBlock->pFsBlock->fnbdev_native_write
#define FN_bdev_pread pGlobalBlock->pFsBlock->fnbdev_pread
#define FN_bdev_pwrite pGlobalBlock->pFsBlock->fnbdev_pwrite
#define FN_bdev_write_full pGlobalBlock->pFsBlock->fnbdev_write_full
#define FN_bdev_create pGlobalBlock->pFsBlock->fnbdev_create
#define FN_bdev_ptruncate pGlobalBlock->pFsBlock->fnbdev_ptruncate
//#define FN_bdev_make_empty pGlobalBlock->pFsBlock->fnbdev_make_empty
#define FN_bdev_open pGlobalBlock->pFsBlock->fnbdev_open
#define FN_bdev_set_key pGlobalBlock->pFsBlock->fnbdev_set_key
#define FN_bdev_close pGlobalBlock->pFsBlock->fnbdev_close
#define FN_bdev_getlength pGlobalBlock->pFsBlock->fnbdev_getlength
#define FN_bdev_encrypt_sectors pGlobalBlock->pFsBlock->fnbdev_encrypt_sectors
#define FN_bdev_get_cluster_offset pGlobalBlock->pFsBlock->fnbdev_get_cluster_offset
#define FN_bdev_read pGlobalBlock->pFsBlock->fnbdev_read
#define FN_bdev_write pGlobalBlock->pFsBlock->fnbdev_write

/////////////////////////////////////////////////////////////////////////

#define FN_zfs_get_cluster_chain_number pGlobalBlock->pFsBlock->fnzfs_get_cluster_chain_number
#define FN_zfs_get_cluster_position pGlobalBlock->pFsBlock->fnzfs_get_cluster_position
#define FN_zfs_get_major_block_number pGlobalBlock->pFsBlock->fnzfs_get_major_block_number
#define FN_zfs_get_minor_block_number pGlobalBlock->pFsBlock->fnzfs_get_minor_block_number
#define FN_zfs_get_minor_block_entry pGlobalBlock->pFsBlock->fnzfs_get_minor_block_entry

/////////////////////////////////////////////////////////////////////////

#define FN_zfs_init_buffer_descriptors pGlobalBlock->pFsBlock->fnzfs_init_buffer_descriptors
#define FN_zfs_create_io_manager pGlobalBlock->pFsBlock->fnzfs_create_io_manager
#define FN_zfs_destroy_io_manager pGlobalBlock->pFsBlock->fnzfs_destroy_io_manager
#define FN_zfs_flush_cache pGlobalBlock->pFsBlock->fnzfs_flush_cache
#define FN_zfs_get_buffer pGlobalBlock->pFsBlock->fnzfs_get_buffer
#define FN_zfs_release_buffer pGlobalBlock->pFsBlock->fnzfs_release_buffer
#define FN_zfs_read_block pGlobalBlock->pFsBlock->fnzfs_read_block
#define FN_zfs_write_block pGlobalBlock->pFsBlock->fnzfs_write_block
#define FN_zfs_mount pGlobalBlock->pFsBlock->fnzfs_mount
#define FN_zfs_open_device pGlobalBlock->pFsBlock->fnzfs_open_device
#define FN_zfs_close_device pGlobalBlock->pFsBlock->fnzfs_close_device
#define FN_zfs_active_handles pGlobalBlock->pFsBlock->fnzfs_active_handles
#define FN_zfs_unmount pGlobalBlock->pFsBlock->fnzfs_unmount
#define FN_zfs_increase_free_clusters pGlobalBlock->pFsBlock->fnzfs_increase_free_clusters
#define FN_zfs_decrease_free_clusters pGlobalBlock->pFsBlock->fnzfs_decrease_free_clusters
#define FN_zfs_get_size pGlobalBlock->pFsBlock->fnzfs_get_size

/////////////////////////////////////////////////////////////////////////

#define FN_zfs_put_boot pGlobalBlock->pFsBlock->fnzfs_put_boot
#define FN_zfs_put_fs_info pGlobalBlock->pFsBlock->fnzfs_put_fs_info
#define FN_zfs_format pGlobalBlock->pFsBlock->fnzfs_format

/////////////////////////////////////////////////////////////////////////

#define FN_zfs_process_short_name pGlobalBlock->pFsBlock->fnzfs_process_short_name
#define FN_zfs_lock_dir pGlobalBlock->pFsBlock->fnzfs_lock_dir
#define FN_zfs_unlock_dir pGlobalBlock->pFsBlock->fnzfs_unlock_dir
#define FN_zfs_find_next_in_dir pGlobalBlock->pFsBlock->fnzfs_find_next_in_dir
#define FN_zfs_short_name_exists pGlobalBlock->pFsBlock->fnzfs_short_name_exists
#define FN_zfs_find_entry_in_dir pGlobalBlock->pFsBlock->fnzfs_find_entry_in_dir
#define FN_zfs_find_dir pGlobalBlock->pFsBlock->fnzfs_find_dir
#define FN_zfs_populate_short_dirent pGlobalBlock->pFsBlock->fnzfs_populate_short_dirent
#define FN_zfs_init_entry_fetch pGlobalBlock->pFsBlock->fnzfs_init_entry_fetch
#define FN_zfs_cleanup_entry_fetch pGlobalBlock->pFsBlock->fnzfs_cleanup_entry_fetch
#define FN_zfs_fetch_entry_with_context pGlobalBlock->pFsBlock->fnzfs_fetch_entry_with_context
#define FN_zfs_push_entry_with_context pGlobalBlock->pFsBlock->fnzfs_push_entry_with_context
#define FN_zfs_get_dir_entry pGlobalBlock->pFsBlock->fnzfs_get_dir_entry
#define FN_zfs_is_end_of_dir pGlobalBlock->pFsBlock->fnzfs_is_end_of_dir
#define FN_zfs_findfirst pGlobalBlock->pFsBlock->fnzfs_findfirst
#define FN_zfs_findnext pGlobalBlock->pFsBlock->fnzfs_findnext
#define FN_zfs_rewindfind pGlobalBlock->pFsBlock->fnzfs_rewindfind
#define FN_zfs_find_free_dirent pGlobalBlock->pFsBlock->fnzfs_find_free_dirent
#define FN_zfs_put_dir_entry pGlobalBlock->pFsBlock->fnzfs_put_dir_entry
#define FN_zfs_create_name pGlobalBlock->pFsBlock->fnzfs_create_name
#define FN_zfs_extend_directory pGlobalBlock->pFsBlock->fnzfs_extend_directory
#define FN_zfs_make_name_compliant pGlobalBlock->pFsBlock->fnzfs_make_name_compliant
#define FN_zfs_create_dirent pGlobalBlock->pFsBlock->fnzfs_create_dirent
#define FN_zfs_create_file pGlobalBlock->pFsBlock->fnzfs_create_file
#define FN_zfs_mkdir pGlobalBlock->pFsBlock->fnzfs_mkdir
#define FN_zfs_rm_lfns pGlobalBlock->pFsBlock->fnzfs_rm_lfns

/////////////////////////////////////////////////////////////////////////

#define FN_zfs_lock pGlobalBlock->pFsBlock->fnzfs_lock
#define FN_zfs_unlock pGlobalBlock->pFsBlock->fnzfs_unlock
#define FN_zfs_cluster_to_lba pGlobalBlock->pFsBlock->fnzfs_cluster_to_lba
#define FN_zfs_get_entry pGlobalBlock->pFsBlock->fnzfs_get_entry
#define FN_zfs_clear_cluster pGlobalBlock->pFsBlock->fnzfs_clear_cluster
#define FN_zfs_traverse pGlobalBlock->pFsBlock->fnzfs_traverse
#define FN_zfs_find_end_of_chain pGlobalBlock->pFsBlock->fnzfs_find_end_of_chain
#define FN_zfs_is_end_of_chain pGlobalBlock->pFsBlock->fnzfs_is_end_of_chain
#define FN_zfs_put_entry pGlobalBlock->pFsBlock->fnzfs_put_entry
#define FN_zfs_find_free_cluster pGlobalBlock->pFsBlock->fnzfs_find_free_cluster
#define FN_zfs_create_cluster_chain pGlobalBlock->pFsBlock->fnzfs_create_cluster_chain
#define FN_zfs_get_chain_length pGlobalBlock->pFsBlock->fnzfs_get_chain_length
#define FN_zfs_unlink_cluster_chain pGlobalBlock->pFsBlock->fnzfs_unlink_cluster_chain
#define FN_zfs_count_free_clusters pGlobalBlock->pFsBlock->fnzfs_count_free_clusters
#define FN_zfs_get_free_size pGlobalBlock->pFsBlock->fnzfs_get_free_size

/////////////////////////////////////////////////////////////////////////

#define FN_zfs_getmodebits pGlobalBlock->pFsBlock->fnzfs_getmodebits
#define FN_zfs_open pGlobalBlock->pFsBlock->fnzfs_open
#define FN_zfs_is_dir_empty pGlobalBlock->pFsBlock->fnzfs_is_dir_empty
#define FN_zfs_rmdir pGlobalBlock->pFsBlock->fnzfs_rmdir
#define FN_zfs_unlink pGlobalBlock->pFsBlock->fnzfs_unlink
#define FN_zfs_move pGlobalBlock->pFsBlock->fnzfs_move
#define FN_zfs_iseof pGlobalBlock->pFsBlock->fnzfs_iseof
#define FN_zfs_bytesleft pGlobalBlock->pFsBlock->fnzfs_bytesleft
#define FN_zfs_get_sequential_clusters pGlobalBlock->pFsBlock->fnzfs_get_sequential_clusters
#define FN_zfs_read_clusters pGlobalBlock->pFsBlock->fnzfs_read_clusters
#define FN_zfs_extend_file pGlobalBlock->pFsBlock->fnzfs_extend_file
#define FN_zfs_write_clusters pGlobalBlock->pFsBlock->fnzfs_write_clusters
#define FN_zfs_read pGlobalBlock->pFsBlock->fnzfs_read
#define FN_zfs_getc pGlobalBlock->pFsBlock->fnzfs_getc
#define FN_zfs_getline pGlobalBlock->pFsBlock->fnzfs_getline
#define FN_zfs_write pGlobalBlock->pFsBlock->fnzfs_write
#define FN_zfs_putc pGlobalBlock->pFsBlock->fnzfs_putc
#define FN_zfs_tell pGlobalBlock->pFsBlock->fnzfs_tell
#define FN_zfs_seek pGlobalBlock->pFsBlock->fnzfs_seek
#define FN_zfs_checkvalid pGlobalBlock->pFsBlock->fnzfs_checkvalid
#define FN_zfs_set_end_of_file pGlobalBlock->pFsBlock->fnzfs_set_end_of_file
#define FN_zfs_set_time pGlobalBlock->pFsBlock->fnzfs_set_time
#define FN_zfs_get_time pGlobalBlock->pFsBlock->fnzfs_get_time
#define FN_zfs_close pGlobalBlock->pFsBlock->fnzfs_close

#endif // __MOD_FSAPI_H_
