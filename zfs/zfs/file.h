#ifndef __ZFS_FILE_H_
#define __ZFS_FILE_H_

#define ZFS_SEEK_SET 1
#define ZFS_SEEK_CUR 2
#define ZFS_SEEK_END 3

#define ZFS_VALID_FLAG_INVALID	0x00000001
#define ZFS_VALID_FLAG_DELETED	0x00000002

enum {
    ETimeCreate = 1,
    ETimeMod = 2,
    ETimeAccess = 4,
};

typedef struct _zfs_file
{
	pzfs_io_manager_t pIoman;   // Ioman Pointer!
	uint32_t filesize;          // File's Size.
	uint32_t objectCluster;     // File's Start Cluster.
	uint32_t iChainLength;      // Total Length of the File's cluster chain.
	uint32_t currentCluster;    // Prevents ZFS Thrashing.
	uint32_t addrCurrentCluster;// Address of the current cluster.
	uint32_t iEndOfChain;       // Address of the last cluster in the chain.
	uint32_t filePointer;       // Current Position Pointer.
	uint32_t dirCluster;        // Cluster Number that the Dirent is in.
	uint32_t validFlags;        // Handle validation flags.
	uint16_t dirEntry;          // Dirent Entry Number describing this file.
	uint8_t mode;               // Mode that File Was opened in.
	struct _zfs_file* pNext;    // Pointer to the next file object in the linked list.
} zfs_file_t, *pzfs_file_t;

int zfs_open(pzfs_io_manager_t pIoman, pzfs_file_t* ppFile, const char* path, uint8_t mode, uint8_t special);
char zfs_is_dir_empty(pzfs_io_manager_t pIoman, const char* path, uint8_t special);
int zfs_unlink(pzfs_io_manager_t pIoman, const char* path, uint8_t special);
int zfs_rmdir(pzfs_io_manager_t pIoman, const char* path, uint8_t special);
int zfs_move(pzfs_io_manager_t pIoman, const char* szSrcFile, const char* szDstFile, uint8_t special);
int zfs_set_time(pzfs_io_manager_t pIoman, const char* path, uint32_t unixTime, uint32_t aWhat, uint8_t special);
int zfs_get_time(pzfs_io_manager_t pIoman, const char* path, uint32_t aWhat, uint32_t* pUnixTime, uint8_t special);
int zfs_close(pzfs_file_t pFile);
int zfs_getc(pzfs_file_t pFile);
int zfs_getline(pzfs_file_t pFile, char* szLine, uint32_t maxLen);
int zfs_read(pzfs_file_t pFile, uint8_t* buffer, uint32_t size, uint32_t* pReaded);
int zfs_write(pzfs_file_t pFile, uint8_t* buffer, uint32_t size, uint32_t* pWritten);
int zfs_iseof(pzfs_file_t pFile);
int zfs_bytesleft(pzfs_file_t pFile);
int zfs_putc(pzfs_file_t pFile, uint8_t Value);
uint32_t zfs_tell(pzfs_file_t pFile);
int zfs_seek(pzfs_file_t pFile, int offset, char origin);
//uint8_t zfs_getmodebits(char* Mode);
int zfs_checkvalid(pzfs_file_t pFile);
int zfs_set_end_of_file(pzfs_file_t pFile);

#endif
