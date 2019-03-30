#ifndef __ZFS_DIR_H_
#define __ZFS_DIR_H_

typedef struct _zfs_fetch_context
{
	uint32_t ulChainLength;
	uint32_t ulDirCluster;
	uint32_t ulCurrentClusterLCN;
	uint32_t ulCurrentClusterNum;
	uint32_t ulCurrentEntry;
	zfs_buffer_t* pBuffer;
} zfs_fetch_context_t, *pzfs_fetch_context_t;

typedef struct _zfs_dir_entry
{
	uint32_t filesize;
	uint32_t objectCluster;
	// Book Keeping
	uint32_t currentCluster;
	uint32_t addrCurrentCluster;
	uint32_t dirCluster;
	uint16_t currentItem;
	// End Book Keeping
	uint32_t createTime;   // Unix-время создания.
	uint32_t modifiedTime; // Unix-время изменения.
	uint32_t accessedTime; // Unix-время последнего доступа.
	char szWildCard[ZFS_MAX_FILENAME + 1];
	char fileName[ZFS_MAX_FILENAME + 1];
	uint8_t attrib;
    uint8_t special;
	zfs_fetch_context_t fetchContext;
} zfs_dir_entry_t, *pzfs_dir_entry_t;

// PUBLIC API
int zfs_findfirst(pzfs_io_manager_t pIoman, pzfs_dir_entry_t pDirent, const char* path, uint8_t special);
int zfs_mkdir(pzfs_io_manager_t pIoman, const char* path, uint8_t special);
int zfs_findnext(pzfs_io_manager_t pIoman, pzfs_dir_entry_t pDirent, uint8_t special);
int zfs_rewindfind(pzfs_io_manager_t pIoman, pzfs_dir_entry_t pDirent);

// INTERNAL API
int	zfs_get_dir_entry(pzfs_io_manager_t pIoman, uint16_t nEntry, uint32_t DirCluster, pzfs_dir_entry_t pDirent);
int	zfs_put_dir_entry(pzfs_io_manager_t pIoman, uint16_t Entry, uint32_t DirCluster, pzfs_dir_entry_t pDirent);
char zfs_find_dir_entry(pzfs_io_manager_t pIoman, uint32_t DirCluster, char* Name, pzfs_dir_entry_t pDirent, char LFNs);

void zfs_populate_short_dirent(pzfs_dir_entry_t pDirent, uint8_t* entryBuffer);
		
int zfs_init_entry_fetch(pzfs_io_manager_t pIoman, uint32_t ulDirCluster, pzfs_fetch_context_t pContext);
int zfs_fetch_entry_with_context(pzfs_io_manager_t pIoman, uint32_t ulEntry, pzfs_fetch_context_t pContext, uint8_t *pEntryBuffer);
int zfs_push_entry_with_context(pzfs_io_manager_t pIoman, uint32_t ulEntry, pzfs_fetch_context_t pContext, uint8_t *pEntryBuffer);
void zfs_cleanup_entry_fetch(pzfs_io_manager_t pIoman, pzfs_fetch_context_t pContext);
		
char zfs_push_entry(pzfs_io_manager_t pIoman, uint32_t DirCluster, uint16_t nEntry, uint8_t *buffer, void *pParam);
char zfs_is_end_of_dir(uint8_t *EntryBuffer);
int zfs_find_next_in_dir(pzfs_io_manager_t pIoman, pzfs_dir_entry_t pDirent, pzfs_fetch_context_t pFetchContext);

uint32_t zfs_find_entry_in_dir(pzfs_io_manager_t pIoman, uint32_t DirCluster, const char* name, uint8_t pa_Attrib, pzfs_dir_entry_t pDirent, int* pError);
//int zfs_create_short_name(pzfs_io_manager_t pIoman, uint32_t DirCluster, char* ShortName, char* LongName);


void zfs_lock_dir(pzfs_io_manager_t pIoman);
void zfs_unlock_dir(pzfs_io_manager_t pIoman);

uint32_t zfs_create_file(pzfs_io_manager_t pIoman, uint32_t DirCluster, char* FileName, pzfs_dir_entry_t pDirent, uint8_t special, int*pError);

int zfs_create_dirent(pzfs_io_manager_t pIoman, uint32_t DirCluster, pzfs_dir_entry_t pDirent);
int zfs_extend_directory(pzfs_io_manager_t pIoman, uint32_t DirCluster);

uint32_t zfs_find_dir(pzfs_io_manager_t pIoman, const char* path, uint16_t pathLen, uint8_t special, int* pError);

int zfs_rm_lfns(pzfs_io_manager_t pIoman, uint16_t usDirEntry, pzfs_fetch_context_t pContext);

#endif // __ZFS_DIR_H_
