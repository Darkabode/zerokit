#ifndef __ZFS_IOMAN_H_
#define __ZFS_IOMAN_H_

#define	ZFS_GETBUFFER_SLEEP_TIME	10
#define	ZFS_GETBUFFER_WAIT_TIME	(20000 / ZFS_GETBUFFER_SLEEP_TIME)

#define ZFS_MODE_READ       0x01    // Buffer / FILE Mode for Read Access.
#define	ZFS_MODE_WRITE      0x02    // Buffer / FILE Mode for Write Access.
#define ZFS_MODE_APPEND     0x04    // FILE Mode Append Access.
#define	ZFS_MODE_CREATE     0x08    // FILE Mode Create file if not existing.
#define ZFS_MODE_TRUNCATE   0x10    // FILE Mode Truncate an Existing file.


#define ZFS_MODE_DIR        0x80    // Special Mode to open a Dir. (Internal use ONLY!)
#define ZFS_MODE_RD_WR      (ZFS_MODE_READ|ZFS_MODE_WRITE)  // Just for bit filtering
#define	ZFS_MODE_WR_ONLY    (0x40|ZFS_MODE_WRITE)           // Buffer for Write-only Access (Internal use ONLY!)
#define ZFS_BUF_MAX_HANDLES 0xFFFF  // Maximum number handles sharing a buffer. (16 bit integer, we don't want to overflow it!)

/**
 *	@private
 *	@brief	ZFS handles memory with buffers, described as below.
 *	@note	This may change throughout development.
 **/
typedef struct _zfs_buffer
{
	uint32_t sector;        // The LBA of the Cached sector.
	uint32_t lru;           // For the Least Recently Used algorithm.
	uint16_t numHandles;    // Number of objects using this buffer.
	uint16_t persistance;   // For the persistance algorithm.
	uint8_t mode;           // Read or Write mode.
	char modified;          // If the sector was modified since read.
	char valid;             // Initially FALSE.
	uint8_t* pBuffer;       // Pointer to the cache block.
} zfs_buffer_t, *pzfs_buffer_t;

#define ZFS_LOCK        0x01
#define ZFS_DIR_LOCK    0x02

typedef struct _zfs_io_manager
{
    BlockDriverState* pbs;
    uint32_t partSize;          // Size of Partition in number of sectors.
    uint32_t beginLBA;          // LBA of the ZFS tables.
    uint32_t sectorsPerZFS;     // Number of sectors per zfs.
    uint32_t totalSectors;
    uint32_t dataSectors;
    uint32_t rootDirSectors;
    uint32_t firstDataSector;
    uint16_t reservedSectors;
    uint32_t clusterBeginLBA;   // LBA of first cluster.
    uint32_t numClusters;       // Number of clusters.
    uint32_t rootDirCluster;    // Cluster number of the root directory entry.
    uint32_t lastFreeCluster;
    uint32_t freeClusterCount;  // Records free space on mount.
    char partitionMounted;      // TRUE if the partition is mounted, otherwise FALSE.
	zfs_buffer_t* pBuffers;     // Pointer to the first buffer description.
#if RING3
	void* mutex;               // Pointer to a Semaphore object. (For buffer description modifications only!).
#else
    KMUTEX mutex;
#endif // RING3
	void* firstFile;            // Pointer to the first File object.
	uint8_t* pCacheMem;         // Pointer to a block of memory for the cache.
	uint32_t lastReplaced;      // Marks which sector was last replaced in the cache.
	uint16_t cacheSize;         // Size of the cache in number of Sectors.
	uint8_t preventFlush;       // Flushing to disk only allowed when 0
	uint8_t locks;              // Lock Flag for ZFS & DIR Locking etc (This must be accessed via a semaphore).
} zfs_io_manager_t, *pzfs_io_manager_t;

// Bit-Masks for Memory Allocation testing.
#define	ZFS_IOMAN_ALLOC_BUFDESCR    0x04    // Flags the pBuffers pointer is allocated.
#define	ZFS_IOMAN_ALLOC_BUFFERS     0x08    // Flags the pCacheMem pointer is allocated.
#define ZFS_IOMAN_ALLOC_RESERVED    0xF0    // Reserved Section.

pzfs_io_manager_t zfs_create_io_manager(uint16_t size);
void zfs_destroy_io_manager(pzfs_io_manager_t pIoman);
int zfs_open_device(pzfs_io_manager_t pIoman, const char* fsPath, uint8_t* fsKey, uint32_t keySize);
void zfs_close_device(pzfs_io_manager_t pIoman);
int zfs_mount(pzfs_io_manager_t pIoman);
int zfs_unmount(pzfs_io_manager_t pIoman);
int zfs_flush_cache(pzfs_io_manager_t pIoman);
uint32_t zfs_get_size(pzfs_io_manager_t pIoman);
int zfs_read_block(pzfs_io_manager_t pIoman, uint32_t ulSectorLBA, uint32_t ulNumSectors, void *pBuffer);
int zfs_write_block(pzfs_io_manager_t pIoman, uint32_t ulSectorLBA, uint32_t ulNumSectors, void *pBuffer);
int zfs_increase_free_clusters(pzfs_io_manager_t pIoman, uint32_t Count);
int zfs_decrease_free_clusters(pzfs_io_manager_t pIoman, uint32_t Count);
zfs_buffer_t* zfs_get_buffer(pzfs_io_manager_t pIoman, uint32_t Sector, uint8_t Mode);
void zfs_release_buffer(pzfs_io_manager_t pIoman, pzfs_buffer_t pBuffer);

#endif // __ZFS_IOMAN_H_
