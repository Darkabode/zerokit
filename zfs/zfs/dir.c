void zfs_lock_dir(pzfs_io_manager_t pIoman)
{
    USE_GLOBAL_BLOCK

	FN_ZFS_LOCK_MUTEX(&pIoman->mutex);
	while ((pIoman->locks & ZFS_DIR_LOCK)) {
		FN_ZFS_UNLOCK_MUTEX(&pIoman->mutex);
		FN_ZFS_THREAD_YIELD(); // Keep Releasing and Yielding until we have the DIR protector.
		FN_ZFS_LOCK_MUTEX(&pIoman->mutex);
	}
	pIoman->locks |= ZFS_DIR_LOCK;
	FN_ZFS_UNLOCK_MUTEX(&pIoman->mutex);
}

void zfs_unlock_dir(pzfs_io_manager_t pIoman)
{
    USE_GLOBAL_BLOCK

    FN_ZFS_LOCK_MUTEX(&pIoman->mutex);
    pIoman->locks &= ~ZFS_DIR_LOCK;
    FN_ZFS_UNLOCK_MUTEX(&pIoman->mutex);
}

int zfs_find_next_in_dir(pzfs_io_manager_t pIoman, pzfs_dir_entry_t pDirent, pzfs_fetch_context_t pFetchContext)
{
	uint8_t numLFNs;
	uint8_t EntryBuffer[ZFS_ENTRY_SIZE];
	int Error;
    USE_GLOBAL_BLOCK

	if (!pIoman) {
		return ZFS_ERR_NULL_POINTER | ZFS_FINDNEXTINDIR;
	}

	for (; pDirent->currentItem < 0xFFFF; pDirent->currentItem += 1) {
		Error = FN_zfs_fetch_entry_with_context(pIoman, pDirent->currentItem, pFetchContext, EntryBuffer);
		
		if (ZFS_isERR(Error)) {
			return Error;
		}
		
		if (EntryBuffer[0] != 0xE5) {
			if (FN_zfs_is_end_of_dir(EntryBuffer)){
				return ZFS_ERR_DIR_END_OF_DIR | ZFS_FINDNEXTINDIR;
			}
			pDirent->attrib = EntryBuffer[ZFS_DIRENT_ATTRIB];
			if ((pDirent->attrib & ZFS_ATTR_LFN) == ZFS_ATTR_LFN) {
				// LFN Processing
				numLFNs = (uint8_t)(EntryBuffer[0] & ~0x40);
				pDirent->currentItem += (numLFNs - 1);
			} else if ((pDirent->attrib & ZFS_ATTR_VOLID) == ZFS_ATTR_VOLID) {
				// Do Nothing
			
			} else {
				FN_zfs_populate_short_dirent(pDirent, EntryBuffer);
				pDirent->currentItem += 1;
				return ERR_OK;
			}
		}
	}
	
	return ZFS_ERR_DIR_END_OF_DIR | ZFS_FINDNEXTINDIR;
}

char zfs_short_name_exists(pzfs_io_manager_t pIoman, uint32_t ulDirCluster, char* szShortName, int*pError)
{
    uint16_t i;
    uint8_t EntryBuffer[ZFS_ENTRY_SIZE];
    uint8_t attrib;
	zfs_fetch_context_t	FetchContext;
    USE_GLOBAL_BLOCK

    *pError = ERR_OK;

    *pError = FN_zfs_init_entry_fetch(pIoman, ulDirCluster, &FetchContext);
	if (*pError) {
		return FALSE;
	}

	for (i = 0; i < 0xFFFF; i++) {
        *pError = FN_zfs_fetch_entry_with_context(pIoman, i, &FetchContext, EntryBuffer);
		if (*pError) {
			break;
		}
		attrib = EntryBuffer[ZFS_DIRENT_ATTRIB];
		if (EntryBuffer[0x00] != 0xE5) {
			if (attrib != ZFS_ATTR_LFN) {
				//FN_zfs_process_short_name((char* )EntryBuffer);
				if (FN_zfs_is_end_of_dir(EntryBuffer)) {
					FN_zfs_cleanup_entry_fetch(pIoman, &FetchContext);
					return FALSE;
				}
				if (STRCMP(szShortName, (char* )EntryBuffer) == 0) {
					FN_zfs_cleanup_entry_fetch(pIoman, &FetchContext);
					return TRUE;
				}
			}
		}
	}

	FN_zfs_cleanup_entry_fetch(pIoman, &FetchContext);
    return FALSE;
}


uint32_t zfs_find_entry_in_dir(pzfs_io_manager_t pIoman, uint32_t DirCluster, const char* name, uint8_t pa_Attrib, pzfs_dir_entry_t pDirent, int* pError)
{
	zfs_fetch_context_t FetchContext;
	uint8_t* src;       // Pointer to read from pBuffer
	uint8_t* lastSrc;
	uint8_t	lastAttrib;
	char totalLFNs = 0;
    USE_GLOBAL_BLOCK

	if (pError) {
        *pError = ERR_OK;
	}

	pDirent->currentItem = 0;
	pDirent->attrib = 0;
    pDirent->special = 0;

	FN_zfs_init_entry_fetch(pIoman, DirCluster, &FetchContext);

	while (pDirent->currentItem < 0xFFFF) {
		if (FN_zfs_fetch_entry_with_context(pIoman, pDirent->currentItem, &FetchContext, NULL)) {
			break;
		}
		lastSrc = FetchContext.pBuffer->pBuffer + BDEV_BLOCK_SIZE;
		for (src = FetchContext.pBuffer->pBuffer; src < lastSrc; src += ZFS_ENTRY_SIZE, pDirent->currentItem++) {
			if (FN_zfs_is_end_of_dir(src)) {	// 0x00: end-of-dir
				FN_zfs_cleanup_entry_fetch(pIoman, &FetchContext);
				return 0;
			}
			if (src[0] == 0xE5) {	// Entry not used
				pDirent->attrib = 0;
				continue;
			}
			lastAttrib = pDirent->attrib;
			pDirent->attrib = src[ZFS_DIRENT_ATTRIB];
            pDirent->special = src[ZFS_DIRENT_SPECIAL];
			if ((pDirent->attrib & ZFS_ATTR_LFN) == ZFS_ATTR_LFN) {
				continue;
			}
			if ((pDirent->attrib & ZFS_ATTR_VOLID) == ZFS_ATTR_VOLID) {
				totalLFNs = 0;
				continue;
			}
		    MEMCPY(pDirent->fileName, src, ZFS_MAX_FILENAME);
            pDirent->fileName[ZFS_MAX_FILENAME] = '\0';
		    //FN_zfs_process_short_name(pDirent->fileName);
		    totalLFNs = 0;

			if ((pDirent->attrib & pa_Attrib) == pa_Attrib){
				if (!STRCMP(name, pDirent->fileName)) {
                    // Finally get the complete information		    
				    FN_zfs_populate_short_dirent(pDirent, src);
				    // HT: CurrentItem wasn't increased here
				    pDirent->currentItem += 1;
					// Object found!
					FN_zfs_cleanup_entry_fetch(pIoman, &FetchContext);
					return pDirent->objectCluster;	// Return the cluster number
				}
			}
			totalLFNs = 0;
		}
	}	// for (src = FetchContext.pBuffer->pBuffer; src < lastSrc; src += 32, pDirent->CurrentItem++)

	FN_zfs_cleanup_entry_fetch(pIoman, &FetchContext);

	return 0;
}

uint32_t zfs_find_dir(pzfs_io_manager_t pIoman, const char* path, uint16_t pathLen, uint8_t special, int* pError)
{
    uint32_t dirCluster = pIoman->rootDirCluster;
	char mytoken[ZFS_MAX_FILENAME + 1];
	char* token;
    uint16_t it = 0;
    char last = FALSE;
    zfs_dir_entry_t myDir;
    USE_GLOBAL_BLOCK

    *pError = ERR_OK;

    if (pathLen <= 1) {      // Must be the root dir! (/ or \)
		return pIoman->rootDirCluster;
    }
    
    if (path[pathLen-1] == '\\' || path[pathLen-1] == '/') {
		pathLen--;      
    }
	
    token = FN_zfs_strtok(path, mytoken, &it, &last, pathLen);

     do {
        myDir.currentItem = 0;
        dirCluster = FN_zfs_find_entry_in_dir(pIoman, dirCluster, token, ZFS_ATTR_DIR, &myDir, pError);

		if (*pError) {
			return 0;
		}

        if (!(special & ZFS_SPECIAL_SYSTEM) && (myDir.special & ZFS_SPECIAL_SYSTEM)) {
            return 0;
        }

		/*if (dirCluster == 0 && MyDir.CurrentItem == 2 && MyDir.FileName[0] == '.') { // .. Dir Entry pointing to root dir.
			dirCluster = pIoman->pPartition->RootDirCluster;
        }*/
        token = FN_zfs_strtok(path, mytoken, &it, &last, pathLen);
    } while (token != NULL);

    return dirCluster;
}

void zfs_populate_short_dirent(pzfs_dir_entry_t pDirent, uint8_t* entryBuffer)
{
    USE_GLOBAL_BLOCK
	
	MEMCPY(pDirent->fileName, entryBuffer, ZFS_MAX_FILENAME);	// Copy the filename into the Dirent object.
    pDirent->fileName[ZFS_MAX_FILENAME] = '\0';

	FN_zfs_tolower(pDirent->fileName, (uint32_t)STRLEN(pDirent->fileName));

	pDirent->objectCluster = *(uint32_t*)(entryBuffer + ZFS_DIRENT_CLUSTER);
	pDirent->createTime = *(uint32_t*)(entryBuffer + ZFS_DIRENT_CREATE_TIME);
	pDirent->modifiedTime = *(uint32_t*)(entryBuffer + ZFS_DIRENT_LASTMOD_TIME);
	pDirent->accessedTime = *(uint32_t*)(entryBuffer + ZFS_DIRENT_LASTACC_TIME);
	pDirent->filesize = *(uint32_t*)(entryBuffer + ZFS_DIRENT_FILESIZE);
	pDirent->attrib = entryBuffer[ZFS_DIRENT_ATTRIB];
    pDirent->special = entryBuffer[ZFS_DIRENT_SPECIAL];
}

/*
	Initialises a context object for FF_FetchEntryWithContext()
*/
int zfs_init_entry_fetch(pzfs_io_manager_t pIoman, uint32_t ulDirCluster, pzfs_fetch_context_t pContext)
{
	int Error;
    USE_GLOBAL_BLOCK

	MEMSET(pContext, 0, sizeof(zfs_fetch_context_t));

	pContext->ulChainLength = FN_zfs_get_chain_length(pIoman, ulDirCluster, NULL, &Error);	// Get the total length of the chain.
	if (ZFS_isERR(Error)) {
		return Error;
	}
	pContext->ulDirCluster = ulDirCluster;
	pContext->ulCurrentClusterLCN = ulDirCluster;
	pContext->ulCurrentClusterNum = 0;
	pContext->ulCurrentEntry = 0;

	return ERR_OK;
}

void zfs_cleanup_entry_fetch(pzfs_io_manager_t pIoman, pzfs_fetch_context_t pContext)
{
    USE_GLOBAL_BLOCK

	if (pContext->pBuffer) {
		FN_zfs_release_buffer(pIoman, pContext->pBuffer);
		pContext->pBuffer = NULL;
	}
}

int zfs_fetch_entry_with_context(pzfs_io_manager_t pIoman, uint32_t ulEntry, pzfs_fetch_context_t pContext, uint8_t *pEntryBuffer)
{
	
	uint32_t ulItemLBA;
	uint32_t ulRelItem;
	uint32_t ulClusterNum;
	int err;
    USE_GLOBAL_BLOCK

	ulClusterNum = FN_zfs_get_cluster_chain_number(ulEntry, ZFS_ENTRY_SIZE);
	ulRelItem = FN_zfs_get_minor_block_entry(ulEntry, ZFS_ENTRY_SIZE);

	if (ulClusterNum != pContext->ulCurrentClusterNum) {
		// Traverse the zfs gently!
		if (ulClusterNum > pContext->ulCurrentClusterNum) {
			pContext->ulCurrentClusterLCN = FN_zfs_traverse(pIoman, pContext->ulCurrentClusterLCN, (ulClusterNum - pContext->ulCurrentClusterNum), &err);
			if (ZFS_isERR(err)) {
				return err;
			}
		}
        else {
			pContext->ulCurrentClusterLCN = FN_zfs_traverse(pIoman, pContext->ulDirCluster, ulClusterNum, &err);
			if (ZFS_isERR(err)) {
				return err;
			}
		}
		pContext->ulCurrentClusterNum = ulClusterNum;
	}

	if ((ulClusterNum + 1) > pContext->ulChainLength) {
		return ZFS_ERR_DIR_END_OF_DIR | ZFS_FETCHENTRYWITHCONTEXT;	// End of Dir was reached!
	}

	ulItemLBA = FN_zfs_cluster_to_lba(pIoman, pContext->ulCurrentClusterLCN) + FN_zfs_get_major_block_number(ulEntry, ZFS_ENTRY_SIZE);
	ulItemLBA = ulItemLBA + FN_zfs_get_minor_block_number(ulRelItem, ZFS_ENTRY_SIZE);

	if (!pContext->pBuffer || (pContext->pBuffer->sector != ulItemLBA)) {
		if (pContext->pBuffer) {
			FN_zfs_release_buffer(pIoman, pContext->pBuffer);
		}
		pContext->pBuffer = FN_zfs_get_buffer(pIoman, ulItemLBA, ZFS_MODE_READ);
		if (!pContext->pBuffer) {
			return ZFS_ERR_DEVICE_DRIVER_FAILED | ZFS_FETCHENTRYWITHCONTEXT;
		}
	}
	
	if (pEntryBuffer) {	// HT Because it might be called with NULL
		MEMCPY(pEntryBuffer, (pContext->pBuffer->pBuffer + (ulRelItem * ZFS_ENTRY_SIZE)), ZFS_ENTRY_SIZE);
	}
	 
    return ERR_OK;
}


int zfs_push_entry_with_context(pzfs_io_manager_t pIoman, uint32_t ulEntry, pzfs_fetch_context_t pContext, uint8_t *pEntryBuffer)
{
	uint32_t ulItemLBA;
	uint32_t ulRelItem;
	uint32_t ulClusterNum;
	int	err;
    USE_GLOBAL_BLOCK

	ulClusterNum = FN_zfs_get_cluster_chain_number(ulEntry, ZFS_ENTRY_SIZE);
	ulRelItem = FN_zfs_get_minor_block_entry(ulEntry, ZFS_ENTRY_SIZE);

	if (ulClusterNum != pContext->ulCurrentClusterNum) {
		// Traverse the zfs gently!
		if (ulClusterNum > pContext->ulCurrentClusterNum) {
			pContext->ulCurrentClusterLCN = FN_zfs_traverse(pIoman, pContext->ulCurrentClusterLCN, (ulClusterNum - pContext->ulCurrentClusterNum), &err);
			if (ZFS_isERR(err)) {
				return err;
			}
		}
        else {
			pContext->ulCurrentClusterLCN = FN_zfs_traverse(pIoman, pContext->ulDirCluster, ulClusterNum, &err);
			if (ZFS_isERR(err)) {
				return err;
			}
		}
		pContext->ulCurrentClusterNum = ulClusterNum;
	}

	if ((ulClusterNum + 1) > pContext->ulChainLength) {
		return ZFS_ERR_DIR_END_OF_DIR | ZFS_PUSHENTRYWITHCONTEXT;	// End of Dir was reached!
	}

	ulItemLBA = FN_zfs_cluster_to_lba(pIoman, pContext->ulCurrentClusterLCN) + FN_zfs_get_major_block_number(ulEntry, ZFS_ENTRY_SIZE);
	ulItemLBA = ulItemLBA + FN_zfs_get_minor_block_number(ulRelItem, ZFS_ENTRY_SIZE);

	if (!pContext->pBuffer || (pContext->pBuffer->sector != ulItemLBA)) {
		if (pContext->pBuffer) {
			FN_zfs_release_buffer(pIoman, pContext->pBuffer);
		}
		pContext->pBuffer = FN_zfs_get_buffer(pIoman, ulItemLBA, ZFS_MODE_READ);
		if (!pContext->pBuffer) {
			return ZFS_ERR_DEVICE_DRIVER_FAILED | ZFS_PUSHENTRYWITHCONTEXT;
		}
	}

	MEMCPY((pContext->pBuffer->pBuffer + (ulRelItem * ZFS_ENTRY_SIZE)), pEntryBuffer, ZFS_ENTRY_SIZE);
	pContext->pBuffer->mode = ZFS_MODE_WRITE;
	pContext->pBuffer->modified = TRUE;
	 
    return ERR_OK;
}

int zfs_get_dir_entry(pzfs_io_manager_t pIoman, uint16_t nEntry, uint32_t dirCluster, pzfs_dir_entry_t pDirent)
{
	uint8_t entryBuffer[ZFS_ENTRY_SIZE];
	uint8_t numLFNs;
	zfs_fetch_context_t	FetchContext;
	int Error;
    USE_GLOBAL_BLOCK

	Error = FN_zfs_init_entry_fetch(pIoman, dirCluster, &FetchContext);
	if (ZFS_isERR(Error)) {
		return Error;
	}
	
	Error = FN_zfs_fetch_entry_with_context(pIoman, nEntry, &FetchContext, entryBuffer);
	if (ZFS_isERR(Error)) {
		FN_zfs_cleanup_entry_fetch(pIoman, &FetchContext);
		return Error;
	}
	if (entryBuffer[0] != 0xE5) {
		if (FN_zfs_is_end_of_dir(entryBuffer)){
			FN_zfs_cleanup_entry_fetch(pIoman, &FetchContext);
			return ZFS_ERR_DIR_END_OF_DIR | ZFS_GETDIRENTRY;
		}
		
        pDirent->special = entryBuffer[ZFS_DIRENT_SPECIAL];
		pDirent->attrib = entryBuffer[ZFS_DIRENT_ATTRIB];
		
		if ((pDirent->attrib & ZFS_ATTR_LFN) == ZFS_ATTR_LFN) {
			// LFN Processing
			numLFNs = (uint8_t)(entryBuffer[0] & ~0x40);
            pDirent->currentItem += (numLFNs - 1);
		}
        else if ((pDirent->attrib & ZFS_ATTR_VOLID) == ZFS_ATTR_VOLID) {
			// Do Nothing
		
		}
        else {
			FN_zfs_populate_short_dirent(pDirent, entryBuffer);
			pDirent->currentItem += 1;
			FN_zfs_cleanup_entry_fetch(pIoman, &FetchContext);
			return 0;
		}
	}

	FN_zfs_cleanup_entry_fetch(pIoman, &FetchContext);
	return ERR_OK;
}

char zfs_is_end_of_dir(uint8_t *EntryBuffer)
{
	return !(EntryBuffer[0]);
}

int zfs_findfirst(pzfs_io_manager_t pIoman, pzfs_dir_entry_t pDirent, const char* path, uint8_t special)
{
	uint16_t pathLen;
	int	err;
	uint16_t i = 0;
	const char* szWildCard;	// Check for a Wild-card.
    USE_GLOBAL_BLOCK

    pathLen = (uint16_t)STRLEN(path);

	if (pIoman == NULL) {
		return ZFS_ERR_NULL_POINTER | ZFS_FINDFIRST;
	}

    MEMSET(pDirent, 0, sizeof(zfs_dir_entry_t));

	pDirent->szWildCard[0] = '\0';	// WildCard blank if its not a wildCard.

	szWildCard = &path[pathLen - 1];

	if (pathLen) {
		while (*szWildCard != '\\' && *szWildCard != '/') {	// Open the dir of the last token.
			i++;
			szWildCard--;
			if (!(pathLen - i)) {
				break;
			}
		}
	}
			
	pDirent->dirCluster = FN_zfs_find_dir(pIoman, path, pathLen - i, special, &err);
	if (ZFS_isERR(err)) {
		return err;
	}
	if (pDirent->dirCluster) {
		// Valid Dir found, copy the wildCard to filename!
		FN_STRCPY_S(pDirent->szWildCard, ZFS_MAX_FILENAME + 1, ++szWildCard);
	}

	if (pDirent->dirCluster == 0) {
		return ZFS_ERR_DIR_INVALID_PATH | ZFS_FINDFIRST;
	}

	// Initialise the Fetch Context
	err = FN_zfs_init_entry_fetch(pIoman, pDirent->dirCluster, &pDirent->fetchContext);
	if (ZFS_isERR(err)) {
		return err;
	}
	
	pDirent->currentItem = 0;

	return FN_zfs_findnext(pIoman, pDirent, special);
}

int zfs_findnext(pzfs_io_manager_t pIoman, pzfs_dir_entry_t pDirent, uint8_t special)
{
	int	err;
	uint8_t	numLFNs;
	uint8_t	EntryBuffer[ZFS_ENTRY_SIZE];
    USE_GLOBAL_BLOCK

	if (pIoman == NULL) {
		return ZFS_ERR_NULL_POINTER | ZFS_FINDNEXT;
	}

	for ( ; pDirent->currentItem < 0xFFFF; ++pDirent->currentItem) {
		err = FN_zfs_fetch_entry_with_context(pIoman, pDirent->currentItem, &pDirent->fetchContext, EntryBuffer);
		if (ZFS_isERR(err)) {
			FN_zfs_cleanup_entry_fetch(pIoman, &pDirent->fetchContext);
			return err;
		}
		if (EntryBuffer[0] != ZFS_DELETED) {
			if (FN_zfs_is_end_of_dir(EntryBuffer)){
				FN_zfs_cleanup_entry_fetch(pIoman, &pDirent->fetchContext);
				return ZFS_ERR_DIR_END_OF_DIR | ZFS_FINDNEXT;
			}
			pDirent->attrib = EntryBuffer[ZFS_DIRENT_ATTRIB];
			if ((pDirent->attrib & ZFS_ATTR_LFN) == ZFS_ATTR_LFN) {
				// LFN Processing
				numLFNs = (uint8_t)(EntryBuffer[0] & ~0x40);
				// Get the shortname and check if it is marked deleted.
                pDirent->currentItem += (numLFNs - 1);
			}
            else if ((pDirent->attrib & ZFS_ATTR_VOLID) == ZFS_ATTR_VOLID) {
				// Do Nothing
			
			}
            else {
                // ѕропускаем, если не достаточно привелегий.
                if (!(special & ZFS_SPECIAL_SYSTEM) && (EntryBuffer[ZFS_DIRENT_SPECIAL] & ZFS_SPECIAL_SYSTEM)) {
                    continue;
                }
				FN_zfs_populate_short_dirent(pDirent, EntryBuffer);
				if (pDirent->szWildCard[0]) {
					if (FN_zfs_wildcompare(pDirent->szWildCard, pDirent->fileName)) {
						FN_zfs_cleanup_entry_fetch(pIoman, &pDirent->fetchContext);
						pDirent->currentItem += 1;
						return ERR_OK;
					}
				}
                else {
					FN_zfs_cleanup_entry_fetch(pIoman, &pDirent->fetchContext);
					pDirent->currentItem += 1;
					return ERR_OK;
				}
			}
		}
	}

	FN_zfs_cleanup_entry_fetch(pIoman, &pDirent->fetchContext);
	
	return ZFS_ERR_DIR_END_OF_DIR | ZFS_FINDNEXT;
}

int zfs_rewindfind(pzfs_io_manager_t pIoman, pzfs_dir_entry_t pDirent)
{
	if (!pIoman) {
		return ZFS_ERR_NULL_POINTER | ZFS_REWINDFIND;
	}
	pDirent->currentItem = 0;
	return ERR_OK;
}

int zfs_find_free_dirent(pzfs_io_manager_t pIoman, uint32_t dirCluster, uint16_t sequential)
{

	uint8_t entryBuffer[ZFS_ENTRY_SIZE];
	uint16_t i = 0;
	uint16_t nEntry;
	int err;
	uint32_t dirLength;
	zfs_fetch_context_t	fetchContext;
    USE_GLOBAL_BLOCK

	err = FN_zfs_init_entry_fetch(pIoman, dirCluster, &fetchContext);
	if (ZFS_isERR(err)) {
		return err;
	}
	
	for (nEntry = 0; nEntry < 0xFFFF; nEntry++) {
		err = FN_zfs_fetch_entry_with_context(pIoman, nEntry, &fetchContext, entryBuffer);
		if (ZFS_GETERROR(err) == ZFS_ERR_DIR_END_OF_DIR) {
			
			err = FN_zfs_extend_directory(pIoman, dirCluster);
			FN_zfs_cleanup_entry_fetch(pIoman, &fetchContext);

			if (ZFS_isERR(err)) {
				return err;
			}

			return nEntry;
		}
        else {
			if (ZFS_isERR(err)) {
				FN_zfs_cleanup_entry_fetch(pIoman, &fetchContext);
				return err;
			}
		}
		if (FN_zfs_is_end_of_dir(entryBuffer)) {	// If its the end of the Dir, then FreeDirents from here.
			// Check Dir is long enough!
			dirLength = fetchContext.ulChainLength;//FF_GetChainLength(pIoman, DirCluster, &iEndOfChain);
			if ((nEntry + sequential) > (uint16_t)(((ZFS_SECTORS_PER_CLUSTER * BDEV_BLOCK_SIZE) * dirLength) / ZFS_ENTRY_SIZE)) {
				err = FN_zfs_extend_directory(pIoman, dirCluster);
			}

			FN_zfs_cleanup_entry_fetch(pIoman, &fetchContext);

			if (ZFS_isERR(err)) {
				return err;
			}

			return nEntry;
		}
		if (entryBuffer[0] == 0xE5) {
			i++;
		}
        else {
			i = 0;
		}

		if (i == sequential) {
			FN_zfs_cleanup_entry_fetch(pIoman, &fetchContext);
			return (nEntry - (sequential - 1));// Return the beginning entry in the sequential sequence.
		}
	}
	
	FN_zfs_cleanup_entry_fetch(pIoman, &fetchContext);

	return ZFS_ERR_DIR_DIRECTORY_FULL | ZFS_FINDFREEDIRENT;
}

int zfs_put_dir_entry(pzfs_io_manager_t pIoman, uint16_t Entry, uint32_t DirCluster, pzfs_dir_entry_t pDirent)
{
	pzfs_buffer_t pBuffer;
	int	error;
	uint32_t itemLBA;
	uint32_t clusterNum;
	uint32_t relItem;
	uint32_t clusterAddress;
	uint8_t* entryPtr;
    USE_GLOBAL_BLOCK

    clusterNum = FN_zfs_get_cluster_chain_number(Entry, ZFS_ENTRY_SIZE);
    relItem = FN_zfs_get_minor_block_entry(Entry, ZFS_ENTRY_SIZE);
    clusterAddress = FN_zfs_traverse(pIoman, DirCluster, clusterNum, &error);
	
    if (ZFS_isERR(error)) {
		return error;
	}

	itemLBA = FN_zfs_cluster_to_lba(pIoman, clusterAddress) + FN_zfs_get_major_block_number(Entry, ZFS_ENTRY_SIZE);
	itemLBA = itemLBA + FN_zfs_get_minor_block_number(relItem, ZFS_ENTRY_SIZE);
	
	pBuffer = FN_zfs_get_buffer(pIoman, itemLBA, ZFS_MODE_WRITE);

	if (pBuffer == NULL) {
		return ZFS_ERR_DEVICE_DRIVER_FAILED | ZFS_PUTENTRY;
	}

    entryPtr = pBuffer->pBuffer + relItem * ZFS_ENTRY_SIZE;
	entryPtr[ZFS_DIRENT_ATTRIB] = pDirent->attrib;
    entryPtr[ZFS_DIRENT_SPECIAL] = pDirent->special;
    *(uint32_t*)(entryPtr + ZFS_DIRENT_CLUSTER) = pDirent->objectCluster;
    *(uint32_t*)(entryPtr + ZFS_DIRENT_FILESIZE) = pDirent->filesize;
	pDirent->accessedTime = FN_ZFS_GET_SYSTEM_TIME();
	*(uint32_t*)&entryPtr[ZFS_DIRENT_LASTACC_TIME] = pDirent->accessedTime;
    *(uint32_t*)&entryPtr[ZFS_DIRENT_CREATE_TIME] = pDirent->createTime;
    *(uint32_t*)&entryPtr[ZFS_DIRENT_LASTMOD_TIME] = pDirent->modifiedTime;

	FN_zfs_release_buffer(pIoman, pBuffer);
 
    return 0;
}

int zfs_create_name(pzfs_io_manager_t pIoman, uint32_t dirCluster, char* name, char* reqName)
{
	uint16_t i, x;
	char tmpShortName[ZFS_MAX_FILENAME + 1];
	zfs_dir_entry_t	tmpDir;
	int	err;
    USE_GLOBAL_BLOCK

    if (STRLEN(reqName) > ZFS_MAX_FILENAME) {
        return ZFS_ERR_DIR_NAME_TOO_LONG | ZFS_CREATESHORTNAME;
    }

    MEMSET(name, 0, ZFS_MAX_FILENAME);

	for (i = 0, x = 0; i < ZFS_MAX_FILENAME; x++) {
		char ch = (char) reqName[x];
        if (ch == '\0') {
			break;
        }
        if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9')
            || (ch == '$') || (ch == '%') || (ch == '-') || (ch == '_') || (ch == '@') || (ch == '~') || (ch == '`') || (ch == '!')
            || (ch == '(') || (ch == ')') || (ch == '{') || (ch == '}') || (ch == '^') || (ch == '#') || (ch == '&') || (ch == '.')) {
            name[i++] = ch;
        }
        else {
            MEMSET(name, 0, i);
            return ZFS_ERR_DIR_NAME_BAD | ZFS_CREATESHORTNAME;
		}
	}
    
	MEMCPY(tmpShortName, name, ZFS_MAX_FILENAME);
    tmpShortName[ZFS_MAX_FILENAME] = '\0';
	if (!FN_zfs_find_entry_in_dir(pIoman, dirCluster, tmpShortName, 0x00, &tmpDir, &err)) {
		return ERR_OK;
	}
	return ZFS_ERR_DIR_OBJECT_EXISTS | ZFS_CREATESHORTNAME;
}

int zfs_extend_directory(pzfs_io_manager_t pIoman, uint32_t DirCluster)
{
	uint32_t CurrentCluster;
	uint32_t NextCluster;
	int Error;
    USE_GLOBAL_BLOCK

	if (!pIoman->freeClusterCount) {
		pIoman->freeClusterCount = FN_zfs_count_free_clusters(pIoman, &Error);
		if (ZFS_isERR(Error)) {
			return Error;
		}
		if (pIoman->freeClusterCount == 0) {
			return ZFS_ERR_ZFS_NO_FREE_CLUSTERS | ZFS_EXTENDDIRECTORY;
		}
	}
	
	FN_zfs_lock(pIoman);
	CurrentCluster = FN_zfs_find_end_of_chain(pIoman, DirCluster, &Error);
	if (ZFS_isERR(Error)) {
		FN_zfs_unlock(pIoman);
		return Error;
	}

	NextCluster = FN_zfs_find_free_cluster(pIoman, &Error);
	if (ZFS_isERR(Error)) {
		FN_zfs_unlock(pIoman);
		return Error;
	}

	Error = FN_zfs_put_entry(pIoman, CurrentCluster, NextCluster);
	if (ZFS_isERR(Error)) {
		FN_zfs_unlock(pIoman);
		return Error;
	}

	Error = FN_zfs_put_entry(pIoman, NextCluster, 0xFFFFFFFF);
	if (ZFS_isERR(Error)) {
		FN_zfs_unlock(pIoman);
		return Error;
	}
	FN_zfs_unlock(pIoman);

	Error = FN_zfs_clear_cluster(pIoman, NextCluster);
	if (ZFS_isERR(Error)) {
		FN_zfs_unlock(pIoman);
		return Error;
	}
	
	Error = FN_zfs_decrease_free_clusters(pIoman, 1);
	if (ZFS_isERR(Error)) {
		FN_zfs_unlock(pIoman);
		return Error;
	}

	return ERR_OK;
}

void zfs_make_name_compliant(char* name)
{
	while (*name) {
		if (*name < 0x20 || *name == 0x7F || *name == 0x22 || *name == 0x7C) {	// Leave all extended chars as they are.
            *name = '_';
		}
		if (*name >= 0x2A && *name <= 0x2F && *name != 0x2B && *name != 0x2E && *name != 0x2D) {
            *name = '_';
		}
		if (*name >= 0x3A && *name <= 0x3F) {
            *name = '_';
		}
		if (*name >= 0x5B && *name <= 0x5C) {
            *name = '_';
		}
		name++;
	}
}

int zfs_create_dirent(pzfs_io_manager_t pIoman, uint32_t dirCluster, pzfs_dir_entry_t pDirent)
{
	uint8_t	entryBuffer[ZFS_ENTRY_SIZE];
	int	freeEntry;
	int	err = ERR_OK;
	uint8_t	entries;
	zfs_fetch_context_t fetchContext;
    USE_GLOBAL_BLOCK

	FN_zfs_make_name_compliant(pDirent->fileName);	// Ensure we don't break the Dir tables.
	MEMSET(entryBuffer, 0, sizeof entryBuffer);

    entries = 1;

	FN_zfs_lock_dir(pIoman);
	err = FN_zfs_create_name(pIoman, dirCluster, (char*)entryBuffer, pDirent->fileName);
	if (err < 0) {
		FN_zfs_unlock_dir(pIoman);
		return err;
	}

    if ((freeEntry = FN_zfs_find_free_dirent(pIoman, dirCluster, entries)) >= 0) {
		if (err == 0) {
			pDirent->createTime = FN_ZFS_GET_SYSTEM_TIME();   // Date and Time Created.
			pDirent->modifiedTime = pDirent->createTime;    // Date and Time Modified.
			pDirent->accessedTime = pDirent->createTime;    // Date of Last Access.
			*(uint32_t*)(entryBuffer + ZFS_DIRENT_CREATE_TIME) = pDirent->createTime;
			*(uint32_t*)(entryBuffer + ZFS_DIRENT_LASTMOD_TIME) = pDirent->modifiedTime;

			entryBuffer[ZFS_DIRENT_ATTRIB] = pDirent->attrib;
            entryBuffer[ZFS_DIRENT_SPECIAL] = pDirent->special;
            *(uint32_t*)(entryBuffer + ZFS_DIRENT_CLUSTER) = pDirent->objectCluster;
            *(uint32_t*)(entryBuffer + ZFS_DIRENT_FILESIZE) = pDirent->filesize;

			err = FN_zfs_init_entry_fetch(pIoman, dirCluster, &fetchContext);
			if (err) {
				FN_zfs_unlock_dir(pIoman);
				return err;
			}
			err = FN_zfs_push_entry_with_context(pIoman, (uint16_t)freeEntry, &fetchContext, entryBuffer);
			FN_zfs_cleanup_entry_fetch(pIoman, &fetchContext);
			if (err) {
				FN_zfs_unlock_dir(pIoman);
				return err;
			}
		}
	}
	FN_zfs_unlock_dir(pIoman);

	if (err) {
		return err;
	}

	if (pDirent) {
		pDirent->currentItem = (uint16_t) (freeEntry);
	}
	
	return ERR_OK;
}

uint32_t zfs_create_file(pzfs_io_manager_t pIoman, uint32_t dirCluster, char* fileName, pzfs_dir_entry_t pDirent, uint8_t special, int* pError)
{
    int err = ERR_OK;
	zfs_dir_entry_t	fileEntry;
    USE_GLOBAL_BLOCK

    do {
	    MEMSET(&fileEntry, 0, sizeof fileEntry);    
	    FN_STRCPY_S(fileEntry.fileName, ZFS_MAX_FILENAME + 1, fileName);
        fileEntry.special = special;
	    fileEntry.objectCluster = FN_zfs_create_cluster_chain(pIoman, &err);
        
	    if (err) {
            break;
	    }

        err = FN_zfs_create_dirent(pIoman, dirCluster, &fileEntry);

	    if (err) {
            break;
	    }

	    FN_zfs_flush_cache(pIoman);

	    if (pDirent) {
		    MEMCPY(pDirent, &fileEntry, sizeof(zfs_dir_entry_t));
	    }
    } while (0);

    *pError = err;

    if (err) {
        FN_zfs_unlink_cluster_chain(pIoman, fileEntry.objectCluster);
        FN_zfs_flush_cache(pIoman);
        return 0;
    }

	return fileEntry.objectCluster;
}

int zfs_mkdir(pzfs_io_manager_t pIoman, const char* path, uint8_t special)
{
	zfs_dir_entry_t	newDir;
	uint32_t dirCluster;
	const char* dirName;
	uint8_t	entryBuffer[ZFS_ENTRY_SIZE];
	uint32_t DotDotCluster;
	uint16_t	i;
	int	err = ERR_OK;
	zfs_fetch_context_t fetchContext;
    USE_GLOBAL_BLOCK

	i = (uint16_t)STRLEN(path);

	while (i != 0) {
		if (path[i] == '\\' || path[i] == '/') {
			break;
		}
		i--;
	}

	dirName = (path + i + 1);

	if (i == 0) {
		i = 1;
	}

	dirCluster = FN_zfs_find_dir(pIoman, path, i, special, &err);

	if (ZFS_isERR(err)) {
		return err;
	}

	if (!dirCluster) {
		return ZFS_ERR_DIR_INVALID_PATH | ZFS_MKDIR;
	}
	MEMSET(&newDir, 0, sizeof(newDir));

	if (FN_zfs_find_entry_in_dir(pIoman, dirCluster, dirName, 0x00, &newDir, &err)) {
		return ZFS_ERR_DIR_OBJECT_EXISTS | ZFS_MKDIR;
	}

	if (err && ZFS_GETERROR(err) != ZFS_ERR_DIR_END_OF_DIR) {
		return err;	
	}

    FN_STRCPY_S(newDir.fileName, ZFS_MAX_FILENAME + 1, dirName);
	newDir.filesize = 0;
    newDir.attrib = ZFS_ATTR_DIR;
    newDir.special = special;
	newDir.objectCluster = FN_zfs_create_cluster_chain(pIoman, &err);
	if (ZFS_isERR(err)) {
		return err;
	}
	if (!newDir.objectCluster) {
		// Couldn't allocate any space for the dir!
		return ZFS_ERR_DIR_EXTEND_FAILED | ZFS_MKDIR;
	}
	err = FN_zfs_clear_cluster(pIoman, newDir.objectCluster);
	if (ZFS_isERR(err)) {
		FN_zfs_unlink_cluster_chain(pIoman, newDir.objectCluster);
		FN_zfs_flush_cache(pIoman);
		return err;
	}

	err = FN_zfs_create_dirent(pIoman, dirCluster, &newDir);

	if (ZFS_isERR(err)) {
		FN_zfs_unlink_cluster_chain(pIoman, newDir.objectCluster);
		FN_zfs_flush_cache(pIoman);
		return err;
	}
	
    MEMSET(entryBuffer, 0, ZFS_ENTRY_SIZE);
	entryBuffer[0] = '.';
	entryBuffer[ZFS_DIRENT_ATTRIB] = ZFS_ATTR_DIR;
    *(uint32_t*)(entryBuffer + ZFS_DIRENT_CLUSTER) = newDir.objectCluster;

	err = FN_zfs_init_entry_fetch(pIoman, newDir.objectCluster, &fetchContext);
	if (ZFS_isERR(err)) {
		FN_zfs_unlink_cluster_chain(pIoman, newDir.objectCluster);
		FN_zfs_flush_cache(pIoman);
		return err;
	}
	
	err = FN_zfs_push_entry_with_context(pIoman, 0, &fetchContext, entryBuffer);
	if (ZFS_isERR(err)) {
		FN_zfs_unlink_cluster_chain(pIoman, newDir.objectCluster);
		FN_zfs_flush_cache(pIoman);
		FN_zfs_cleanup_entry_fetch(pIoman, &fetchContext);
		return err;
	}

    MEMSET(entryBuffer, 0, 64);
	entryBuffer[0] = '.';
	entryBuffer[1] = '.';
		
	if (dirCluster == pIoman->rootDirCluster) {
		DotDotCluster = 0;
	}
    else {
		DotDotCluster = dirCluster;
	}

	entryBuffer[ZFS_DIRENT_ATTRIB] = ZFS_ATTR_DIR;
    *(uint32_t*)(entryBuffer + ZFS_DIRENT_CLUSTER) = DotDotCluster;
	
	//FF_PushEntry(pIoman, MyDir.ObjectCluster, 1, EntryBuffer);
	err = FN_zfs_push_entry_with_context(pIoman, 1, &fetchContext, entryBuffer);
	if (ZFS_isERR(err)) {
		FN_zfs_unlink_cluster_chain(pIoman, newDir.objectCluster);
		FN_zfs_flush_cache(pIoman);
		FN_zfs_cleanup_entry_fetch(pIoman, &fetchContext);
		return err;
	}
	FN_zfs_cleanup_entry_fetch(pIoman, &fetchContext);

	FN_zfs_flush_cache(pIoman);	// Ensure dir was flushed to the disk!

	return ERR_OK;
}

int zfs_rm_lfns(pzfs_io_manager_t pIoman, uint16_t usDirEntry, pzfs_fetch_context_t pContext)
{
	int	Error;
	uint8_t	EntryBuffer[ZFS_ENTRY_SIZE];
    USE_GLOBAL_BLOCK

    if (usDirEntry > 0 )
	    usDirEntry--;

	do {
		Error = FN_zfs_fetch_entry_with_context(pIoman, usDirEntry, pContext, EntryBuffer);
		if (ZFS_isERR(Error)) {
			return Error;
		}
		
		if (EntryBuffer[ZFS_DIRENT_ATTRIB] == ZFS_ATTR_LFN) {
			EntryBuffer[0] = 0xE5;
			Error = FN_zfs_push_entry_with_context(pIoman, usDirEntry, pContext, EntryBuffer);
			if (ZFS_isERR(Error)) {
				return Error;
			}
		}

		if (usDirEntry == 0) {
			break;
		}
		usDirEntry--;
	} while (EntryBuffer[ZFS_DIRENT_ATTRIB] == ZFS_ATTR_LFN);

	return ERR_OK;
}
