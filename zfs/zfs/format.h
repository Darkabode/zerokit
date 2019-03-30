#ifndef __ZFS_FORMAT_H_
#define __ZFS_FORMAT_H_

typedef struct zfsparams_t
{
    uint32_t cluster_count;
    int size_root_dir;
    int zfs_length;
    int16_t reserved;
    unsigned int total_sect;
} zfsparams;

int zfs_format(pzfs_io_manager_t pIoman);

#endif
