#include <ntifs.h>
#include <wdm.h>
#include <ntstrsafe.h>

#include "../../mod_shared/headers.h"

#include "mod_fs.c"
#include "mod_fsApi.c"

NTSTATUS mod_fsEntry(uintptr_t modBase, pglobal_block_t pGlobalBlock)
{
    pmod_common_block_t pCommonBlock = pGlobalBlock->pCommonBlock;
    pmod_fs_block_t pFsBlock;
    pmod_header_t pModHeader = (pmod_header_t)modBase;

    pCommonBlock->fncommon_allocate_memory(pCommonBlock, &pFsBlock, sizeof(mod_fs_block_t), NonPagedPool);
    pGlobalBlock->pFsBlock = pFsBlock;
    pFsBlock->pModBase = (uint8_t*)modBase;

#ifndef _SOLID_DRIVER
    
#ifdef _WIN64
    pCommonBlock->fncommon_fix_addr_value((PUINT8)getGlobalDataPtr, 11, GLOBAL_DATA_PATTERN, pGlobalBlock);
#else
    pCommonBlock->fncommon_fix_addr_value((PUINT8)modBase + sizeof(mod_header_t), pModHeader->sizeOfModReal, GLOBAL_DATA_PATTERN, pGlobalBlock);
#endif // _WIN64

#endif // _SOLID_DRIVER

    DECLARE_GLOBAL_FUNC(pFsBlock, fs_shutdown_routine);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_tolower);
//    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_toupper);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_strmatch);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_strtok);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_wildcompare);

    DECLARE_GLOBAL_FUNC(pFsBlock, bdev_native_open);
    DECLARE_GLOBAL_FUNC(pFsBlock, bdev_narive_read);
    DECLARE_GLOBAL_FUNC(pFsBlock, bdev_native_write);
    DECLARE_GLOBAL_FUNC(pFsBlock, bdev_pread);
    DECLARE_GLOBAL_FUNC(pFsBlock, bdev_pwrite);
    DECLARE_GLOBAL_FUNC(pFsBlock, bdev_write_full);
    DECLARE_GLOBAL_FUNC(pFsBlock, bdev_create);
    DECLARE_GLOBAL_FUNC(pFsBlock, bdev_ptruncate);
    //DECLARE_GLOBAL_FUNC(pFsBlock, bdev_make_empty);
    DECLARE_GLOBAL_FUNC(pFsBlock, bdev_open);
    DECLARE_GLOBAL_FUNC(pFsBlock, bdev_set_key);
    DECLARE_GLOBAL_FUNC(pFsBlock, bdev_close);
    DECLARE_GLOBAL_FUNC(pFsBlock, bdev_getlength);
    DECLARE_GLOBAL_FUNC(pFsBlock, bdev_encrypt_sectors);
    DECLARE_GLOBAL_FUNC(pFsBlock, bdev_get_cluster_offset);
    DECLARE_GLOBAL_FUNC(pFsBlock, bdev_read);
    DECLARE_GLOBAL_FUNC(pFsBlock, bdev_write);

    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_put_boot);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_put_fs_info);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_format);

    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_get_cluster_chain_number);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_get_cluster_position);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_get_major_block_number);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_get_minor_block_number);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_get_minor_block_entry);

    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_init_buffer_descriptors);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_create_io_manager);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_destroy_io_manager);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_flush_cache);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_get_buffer);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_release_buffer);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_read_block);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_write_block);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_mount);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_open_device);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_close_device);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_active_handles);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_unmount);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_increase_free_clusters);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_decrease_free_clusters);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_get_size);

    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_lock_dir);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_unlock_dir);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_find_next_in_dir);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_short_name_exists);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_find_entry_in_dir);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_find_dir);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_populate_short_dirent);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_init_entry_fetch);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_cleanup_entry_fetch);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_fetch_entry_with_context);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_push_entry_with_context);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_get_dir_entry);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_is_end_of_dir);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_findfirst);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_findnext);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_rewindfind);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_find_free_dirent);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_put_dir_entry);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_create_name);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_extend_directory);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_make_name_compliant);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_create_dirent);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_create_file);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_mkdir);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_rm_lfns);

    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_lock);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_unlock);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_cluster_to_lba);
//    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_lba_to_cluster);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_get_entry);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_clear_cluster);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_traverse);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_find_end_of_chain);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_is_end_of_chain);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_put_entry);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_find_free_cluster);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_create_cluster_chain);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_get_chain_length);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_unlink_cluster_chain);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_count_free_clusters);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_get_free_size);

    //DECLARE_GLOBAL_FUNC(pFsBlock, zfs_getmodebits);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_open);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_is_dir_empty);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_rmdir);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_unlink);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_move);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_iseof);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_bytesleft);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_get_sequential_clusters);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_read_clusters);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_extend_file);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_write_clusters);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_read);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_getc);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_getline);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_write);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_putc);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_tell);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_seek);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_checkvalid);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_set_end_of_file);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_set_time);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_get_time);
    DECLARE_GLOBAL_FUNC(pFsBlock, zfs_close);
    
//     {
//         PUCHAR ptr;
// 
//     }

    // Инициализируем менеджер файловой системы.
    pFsBlock->pZfsIo = pFsBlock->fnzfs_create_io_manager(pGlobalBlock->pCommonBlock->pConfig->fsCacheSize * BDEV_BLOCK_SIZE);

    return STATUS_SUCCESS;
}
