#ifndef __ZFS_BLOCK_DEVICE_H_
#define __ZFS_BLOCK_DEVICE_H_

#define BD_MAGIC 0x77999977

#define BDRV_SECTOR_BITS   9
#define BDRV_SECTOR_SIZE   (1ULL << BDRV_SECTOR_BITS)

#define L2_CACHE_SIZE 16

#define BDRV_SECTOR_BITS   9
#define BDRV_SECTOR_SIZE   (1ULL << BDRV_SECTOR_BITS)
#define BDRV_SECTOR_MASK   ~(BDRV_SECTOR_SIZE - 1)
#define BDEV_BLOCK_SIZE 512

typedef struct _BlockDriverState BlockDriverState;

typedef struct _BlockDriverState
{
    uint32_t total_sectors; /* if we are reading a disk image, give its size in sectors */
    int valid_key; /* if true, a valid encryption key has been set */
    void* opaque;

    char filename[1024];
    void* file;
#if RING3
    void* fileMutex;
#else
    KMUTEX fileMutex;
#endif // RING3
    uint32_t wr_highest_sector;

    /* NOTE: the following infos are only hints for real hardware drivers. They are not used by the block driver */
    int heads, secs;
} BlockDriverState;

#pragma pack(push, 1)

typedef struct _zfs_bdev_header
{
    uint32_t magic;
    uint32_t mtime;
    uint32_t size; /* in bytes */
    uint8_t cluster_bits;
    uint8_t l2_bits;
    uint32_t l1_table_offset;
} zfs_bdev_header_t, *pzfs_bdev_header_t;

typedef struct zfs_bdev_state
{
    int cluster_bits;
    int cluster_size;
    int cluster_sectors;
    int l2_bits;
    int l2_size;
    uint32_t l1_size;
    uint32_t cluster_offset_mask;
    uint32_t l1_table_offset;
    uint32_t *l1_table;
    uint32_t *l2_cache;
    uint32_t l2_cache_offsets[L2_CACHE_SIZE];
    uint32_t l2_cache_counts[L2_CACHE_SIZE];
    uint8_t *cluster_cache;
    uint8_t *cluster_data;
    aes_context_t aes_enc_key;
    aes_context_t aes_dec_key;
#if RING3
    void* mutex;
#else
    KMUTEX mutex;
#endif // RING3
} zfs_bdev_state_t, *pzfs_bdev_state_t;

#pragma pack(pop)

int bdev_create(const char* filename, uint32_t virtSize);
int bdev_open(BlockDriverState** pbs, const char* filename);
int bdev_set_key(BlockDriverState *bs, const uint8_t* key, uint32_t keySize);
void bdev_close(BlockDriverState *bs);
int bdev_read(uint8_t* buf, uint32_t sector_num, uint32_t nb_sectors, BlockDriverState* bs);
int bdev_write(const uint8_t *buf, uint32_t sector_num, uint32_t nb_sectors, BlockDriverState* bs);

#endif // __ZFS_BLOCK_DEVICE_H_
