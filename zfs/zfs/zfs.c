#include "config.h"
#include "error.h"
#include "string.h"
#include "zfsdef.h"
#include "blockdev.h"
#include "ioman.h"
#include "format.h"
#include "common.h"
#include "dir.h"
#include "file.h"

#ifndef FN_zfs_tolower
#define FN_zfs_tolower zfs_tolower
#endif // FN_zfs_tolower

#ifndef FN_zfs_strmatch
#define FN_zfs_strmatch zfs_strmatch
#endif // FN_zfs_strmatch

#ifndef FN_zfs_strtok
#define FN_zfs_strtok zfs_strtok
#endif // FN_zfs_strtok

#ifndef FN_zfs_wildcompare
#define FN_zfs_wildcompare zfs_wildcompare
#endif // FN_zfs_wildcompare

/////////////////////////////////////////////////////////////////////////

#ifndef FN_bdev_native_open
#define FN_bdev_native_open bdev_native_open
#endif // FN_bdev_native_open

#ifndef FN_bdev_native_read
#define FN_bdev_native_read bdev_narive_read
#endif // FN_bdev_native_read

#ifndef FN_bdev_native_write
#define FN_bdev_native_write bdev_native_write
#endif // FN_bdev_native_write

#ifndef FN_bdev_pread
#define FN_bdev_pread bdev_pread
#endif // FN_bdev_pread

#ifndef FN_bdev_pwrite
#define FN_bdev_pwrite bdev_pwrite
#endif // FN_bdev_pwrite

#ifndef FN_bdev_write_full
#define FN_bdev_write_full bdev_write_full
#endif // FN_bdev_write_full

#ifndef FN_bdev_create
#define FN_bdev_create bdev_create
#endif // FN_bdev_create

#ifndef FN_bdev_ptruncate
#define FN_bdev_ptruncate bdev_ptruncate
#endif // FN_bdev_ptruncate

#ifndef FN_bdev_make_empty
#define FN_bdev_make_empty bdev_make_empty
#endif // FN_bdev_make_empty

#ifndef FN_bdev_open
#define FN_bdev_open bdev_open
#endif // FN_bdev_open

#ifndef FN_bdev_set_key
#define FN_bdev_set_key bdev_set_key
#endif // FN_bdev_set_key

#ifndef FN_bdev_close
#define FN_bdev_close bdev_close
#endif // FN_bdev_close

#ifndef FN_bdev_getlength
#define FN_bdev_getlength bdev_getlength
#endif // FN_bdev_getlength

#ifndef FN_bdev_encrypt_sectors
#define FN_bdev_encrypt_sectors bdev_encrypt_sectors
#endif // FN_bdev_encrypt_sectors

#ifndef FN_bdev_get_cluster_offset
#define FN_bdev_get_cluster_offset bdev_get_cluster_offset
#endif // FN_bdev_get_cluster_offset

#ifndef FN_bdev_read
#define FN_bdev_read bdev_read
#endif // FN_bdev_read

#ifndef FN_bdev_write
#define FN_bdev_write bdev_write
#endif // FN_bdev_write

/////////////////////////////////////////////////////////////////////////

#ifndef FN_zfs_get_cluster_chain_number
#define FN_zfs_get_cluster_chain_number zfs_get_cluster_chain_number
#endif // FN_zfs_get_cluster_chain_number

#ifndef FN_zfs_get_cluster_position
#define FN_zfs_get_cluster_position zfs_get_cluster_position
#endif // FN_zfs_get_cluster_position

#ifndef FN_zfs_get_major_block_number
#define FN_zfs_get_major_block_number zfs_get_major_block_number
#endif // FN_zfs_get_major_block_number

#ifndef FN_zfs_get_minor_block_number
#define FN_zfs_get_minor_block_number zfs_get_minor_block_number
#endif // FN_zfs_get_minor_block_number

#ifndef FN_zfs_get_minor_block_entry
#define FN_zfs_get_minor_block_entry zfs_get_minor_block_entry
#endif // FN_zfs_get_minor_block_entry

/////////////////////////////////////////////////////////////////////////

#ifndef FN_zfs_init_buffer_descriptors
#define FN_zfs_init_buffer_descriptors zfs_init_buffer_descriptors
#endif // FN_zfs_init_buffer_descriptors

#ifndef FN_zfs_create_io_manager
#define FN_zfs_create_io_manager zfs_create_io_manager
#endif // FN_zfs_create_io_manager

#ifndef FN_zfs_destroy_io_manager
#define FN_zfs_destroy_io_manager zfs_destroy_io_manager
#endif // FN_zfs_destroy_io_manager

#ifndef FN_zfs_flush_cache
#define FN_zfs_flush_cache zfs_flush_cache
#endif // FN_zfs_flush_cache

#ifndef FN_zfs_get_buffer
#define FN_zfs_get_buffer zfs_get_buffer
#endif // FN_zfs_get_buffer

#ifndef FN_zfs_release_buffer
#define FN_zfs_release_buffer zfs_release_buffer
#endif // FN_zfs_release_buffer

#ifndef FN_zfs_read_block
#define FN_zfs_read_block zfs_read_block
#endif // FN_zfs_read_block

#ifndef FN_zfs_write_block
#define FN_zfs_write_block zfs_write_block
#endif // FN_zfs_write_block

#ifndef FN_zfs_mount
#define FN_zfs_mount zfs_mount
#endif // FN_zfs_mount

#ifndef FN_zfs_open_device
#define FN_zfs_open_device zfs_open_device
#endif // FN_zfs_open_device

#ifndef FN_zfs_close_device
#define FN_zfs_close_device zfs_close_device
#endif // FN_zfs_close_device

#ifndef FN_zfs_active_handles
#define FN_zfs_active_handles zfs_active_handles
#endif // FN_zfs_active_handles

#ifndef FN_zfs_unmount
#define FN_zfs_unmount zfs_unmount
#endif // FN_zfs_unmount

#ifndef FN_zfs_increase_free_clusters
#define FN_zfs_increase_free_clusters zfs_increase_free_clusters
#endif // FN_zfs_increase_free_clusters

#ifndef FN_zfs_decrease_free_clusters
#define FN_zfs_decrease_free_clusters zfs_decrease_free_clusters
#endif // FN_zfs_decrease_free_clusters

#ifndef FN_zfs_get_size
#define FN_zfs_get_size zfs_get_size
#endif // FN_zfs_get_size

/////////////////////////////////////////////////////////////////////////

#ifndef FN_zfs_put_boot
#define FN_zfs_put_boot zfs_put_boot
#endif // FN_zfs_put_boot

#ifndef FN_zfs_put_fs_info
#define FN_zfs_put_fs_info zfs_put_fs_info
#endif // FN_zfs_put_fs_info

#ifndef FN_zfs_format
#define FN_zfs_format zfs_format
#endif // FN_zfs_format

/////////////////////////////////////////////////////////////////////////

#ifndef FN_zfs_lock_dir
#define FN_zfs_lock_dir zfs_lock_dir
#endif // FN_zfs_lock_dir

#ifndef FN_zfs_unlock_dir
#define FN_zfs_unlock_dir zfs_unlock_dir
#endif // FN_zfs_unlock_dir

#ifndef FN_zfs_find_next_in_dir
#define FN_zfs_find_next_in_dir zfs_find_next_in_dir
#endif // FN_zfs_find_next_in_dir

#ifndef FN_zfs_short_name_exists
#define FN_zfs_short_name_exists zfs_short_name_exists
#endif // FN_zfs_short_name_exists

#ifndef FN_zfs_find_entry_in_dir
#define FN_zfs_find_entry_in_dir zfs_find_entry_in_dir
#endif // FN_zfs_find_entry_in_dir

#ifndef FN_zfs_find_dir
#define FN_zfs_find_dir zfs_find_dir
#endif // FN_zfs_find_dir

#ifndef FN_zfs_populate_short_dirent
#define FN_zfs_populate_short_dirent zfs_populate_short_dirent
#endif // FN_zfs_populate_short_dirent

#ifndef FN_zfs_init_entry_fetch
#define FN_zfs_init_entry_fetch zfs_init_entry_fetch
#endif // FN_zfs_init_entry_fetch

#ifndef FN_zfs_cleanup_entry_fetch
#define FN_zfs_cleanup_entry_fetch zfs_cleanup_entry_fetch
#endif // FN_zfs_cleanup_entry_fetch

#ifndef FN_zfs_fetch_entry_with_context
#define FN_zfs_fetch_entry_with_context zfs_fetch_entry_with_context
#endif // FN_zfs_fetch_entry_with_context

#ifndef FN_zfs_push_entry_with_context
#define FN_zfs_push_entry_with_context zfs_push_entry_with_context
#endif // FN_zfs_push_entry_with_context

#ifndef FN_zfs_get_dir_entry
#define FN_zfs_get_dir_entry zfs_get_dir_entry
#endif // FN_zfs_get_dir_entry

#ifndef FN_zfs_is_end_of_dir
#define FN_zfs_is_end_of_dir zfs_is_end_of_dir
#endif // FN_zfs_is_end_of_dir

#ifndef FN_zfs_findfirst
#define FN_zfs_findfirst zfs_findfirst
#endif // FN_zfs_findfirst

#ifndef FN_zfs_findnext
#define FN_zfs_findnext zfs_findnext
#endif // FN_zfs_findnext

#ifndef FN_zfs_rewindfind
#define FN_zfs_rewindfind zfs_rewindfind
#endif // FN_zfs_rewindfind

#ifndef FN_zfs_find_free_dirent
#define FN_zfs_find_free_dirent zfs_find_free_dirent
#endif // FN_zfs_find_free_dirent

#ifndef FN_zfs_put_dir_entry
#define FN_zfs_put_dir_entry zfs_put_dir_entry
#endif // FN_zfs_put_dir_entry

#ifndef FN_zfs_create_name
#define FN_zfs_create_name zfs_create_name
#endif // FN_zfs_create_name

#ifndef FN_zfs_extend_directory
#define FN_zfs_extend_directory zfs_extend_directory
#endif // FN_zfs_extend_directory

#ifndef FN_zfs_make_name_compliant
#define FN_zfs_make_name_compliant zfs_make_name_compliant
#endif // FN_zfs_make_name_compliant

#ifndef FN_zfs_create_dirent
#define FN_zfs_create_dirent zfs_create_dirent
#endif // FN_zfs_create_dirent

#ifndef FN_zfs_create_file
#define FN_zfs_create_file zfs_create_file
#endif // FN_zfs_create_file

#ifndef FN_zfs_mkdir
#define FN_zfs_mkdir zfs_mkdir
#endif // FN_zfs_mkdir

#ifndef FN_zfs_rm_lfns
#define FN_zfs_rm_lfns zfs_rm_lfns
#endif // FN_zfs_rm_lfns

/////////////////////////////////////////////////////////////////////////

#ifndef FN_zfs_lock
#define FN_zfs_lock zfs_lock
#endif // FN_zfs_lock

#ifndef FN_zfs_unlock
#define FN_zfs_unlock zfs_unlock
#endif // FN_zfs_unlock

#ifndef FN_zfs_cluster_to_lba
#define FN_zfs_cluster_to_lba zfs_cluster_to_lba
#endif // FN_zfs_cluster_to_lba

#ifndef FN_zfs_get_entry
#define FN_zfs_get_entry zfs_get_entry
#endif // FN_zfs_get_entry

#ifndef FN_zfs_clear_cluster
#define FN_zfs_clear_cluster zfs_clear_cluster
#endif // FN_zfs_clear_cluster

#ifndef FN_zfs_traverse
#define FN_zfs_traverse zfs_traverse
#endif // FN_zfs_traverse

#ifndef FN_zfs_find_end_of_chain
#define FN_zfs_find_end_of_chain zfs_find_end_of_chain
#endif // FN_zfs_find_end_of_chain

#ifndef FN_zfs_is_end_of_chain
#define FN_zfs_is_end_of_chain zfs_is_end_of_chain
#endif // FN_zfs_is_end_of_chain

#ifndef FN_zfs_put_entry
#define FN_zfs_put_entry zfs_put_entry
#endif // FN_zfs_put_entry

#ifndef FN_zfs_find_free_cluster
#define FN_zfs_find_free_cluster zfs_find_free_cluster
#endif // FN_zfs_find_free_cluster

#ifndef FN_zfs_create_cluster_chain
#define FN_zfs_create_cluster_chain zfs_create_cluster_chain
#endif // FN_zfs_create_cluster_chain

#ifndef FN_zfs_get_chain_length
#define FN_zfs_get_chain_length zfs_get_chain_length
#endif // FN_zfs_get_chain_length

#ifndef FN_zfs_unlink_cluster_chain
#define FN_zfs_unlink_cluster_chain zfs_unlink_cluster_chain
#endif // FN_zfs_unlink_cluster_chain

#ifndef FN_zfs_count_free_clusters
#define FN_zfs_count_free_clusters zfs_count_free_clusters
#endif // FN_zfs_count_free_clusters

#ifndef FN_zfs_get_free_size
#define FN_zfs_get_free_size zfs_get_free_size
#endif // FN_zfs_get_free_size

/////////////////////////////////////////////////////////////////////////

// #ifndef FN_zfs_getmodebits
// #define FN_zfs_getmodebits zfs_getmodebits
// #endif // FN_zfs_getmodebits

#ifndef FN_zfs_open
#define FN_zfs_open zfs_open
#endif // FN_zfs_open

#ifndef FN_zfs_is_dir_empty
#define FN_zfs_is_dir_empty zfs_is_dir_empty
#endif // FN_zfs_is_dir_empty

#ifndef FN_zfs_rmdir
#define FN_zfs_rmdir zfs_rmdir
#endif // FN_zfs_rmdir

#ifndef FN_zfs_unlink
#define FN_zfs_unlink zfs_unlink
#endif // FN_zfs_unlink

#ifndef FN_zfs_move
#define FN_zfs_move zfs_move
#endif // FN_zfs_move

#ifndef FN_zfs_iseof
#define FN_zfs_iseof zfs_iseof
#endif // FN_zfs_iseof

#ifndef FN_zfs_bytesleft
#define FN_zfs_bytesleft zfs_bytesleft
#endif // FN_zfs_bytesleft

#ifndef FN_zfs_get_sequential_clusters
#define FN_zfs_get_sequential_clusters zfs_get_sequential_clusters
#endif // FN_zfs_get_sequential_clusters

#ifndef FN_zfs_read_clusters
#define FN_zfs_read_clusters zfs_read_clusters
#endif // FN_zfs_read_clusters

#ifndef FN_zfs_extend_file
#define FN_zfs_extend_file zfs_extend_file
#endif // FN_zfs_extend_file

#ifndef FN_zfs_write_clusters
#define FN_zfs_write_clusters zfs_write_clusters
#endif // FN_zfs_write_clusters

#ifndef FN_zfs_read
#define FN_zfs_read zfs_read
#endif // FN_zfs_read

#ifndef FN_zfs_getc
#define FN_zfs_getc zfs_getc
#endif // FN_zfs_getc

#ifndef FN_zfs_getline
#define FN_zfs_getline zfs_getline
#endif // FN_zfs_getline

#ifndef FN_zfs_write
#define FN_zfs_write zfs_write
#endif // FN_zfs_write

#ifndef FN_zfs_putc
#define FN_zfs_putc zfs_putc
#endif // FN_zfs_putc

#ifndef FN_zfs_tell
#define FN_zfs_tell zfs_tell
#endif // FN_zfs_tell

#ifndef FN_zfs_seek
#define FN_zfs_seek zfs_seek
#endif // FN_zfs_seek

#ifndef FN_zfs_checkvalid
#define FN_zfs_checkvalid zfs_checkvalid
#endif // FN_zfs_checkvalid

#ifndef FN_zfs_set_end_of_file
#define FN_zfs_set_end_of_file zfs_set_end_of_file
#endif // FN_zfs_set_end_of_file

#ifndef FN_zfs_set_time
#define FN_zfs_set_time zfs_set_time
#endif // FN_zfs_set_time

#ifndef FN_zfs_get_time
#define FN_zfs_get_time zfs_get_time
#endif // FN_zfs_get_time

#ifndef FN_zfs_close
#define FN_zfs_close zfs_close
#endif // FN_zfs_close

#include "sysspec.c"
#include "string.c"
#include "blockdev.c"
#include "blk.c"
#include "ioman.c"
#include "format.c"
#include "dir.c"
#include "common.c"
#include "file.c"
