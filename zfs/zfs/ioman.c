void zfs_init_buffer_descriptors(pzfs_io_manager_t pIoman)
{
    uint16_t i;
    pzfs_buffer_t pBuffer = pIoman->pBuffers;
    pIoman->lastReplaced = 0;
    // HT : it is assmued that pBuffer was cleared by MEMSET ()
    for (i = 0; i < pIoman->cacheSize; i++) {
        pBuffer->pBuffer = (uint8_t *)((pIoman->pCacheMem) + (BDEV_BLOCK_SIZE * i));
        pBuffer++;
    }
}

pzfs_io_manager_t zfs_create_io_manager(uint16_t size)
{
    pzfs_io_manager_t pIoman = NULL;
    USE_GLOBAL_BLOCK

	if ((size % BDEV_BLOCK_SIZE) != 0 || size == 0 || size == BDEV_BLOCK_SIZE) {  // size must now be atleast 2 * BlkSize (or a deadlock will occur).
		return NULL;
	}

	pIoman = (pzfs_io_manager_t)SYS_ALLOCATOR(sizeof(zfs_io_manager_t));

	if (pIoman == NULL) {
		return NULL;
	}

	MEMSET(pIoman, 0, sizeof(zfs_io_manager_t));

	pIoman->pCacheMem = (uint8_t*)SYS_ALLOCATOR(size);
	if (pIoman->pCacheMem == NULL) {
		FN_zfs_destroy_io_manager(pIoman);
		return NULL;
	}

	MEMSET (pIoman->pCacheMem, 0, size);

	pIoman->cacheSize = (uint16_t)(size / BDEV_BLOCK_SIZE);

	/*	Malloc() memory for buffer objects. (ZFS never refers to a buffer directly
		but uses buffer objects instead. Allows us to provide thread safety.
    */
	pIoman->pBuffers = (pzfs_buffer_t)SYS_ALLOCATOR(sizeof(zfs_buffer_t) * pIoman->cacheSize);

	if (pIoman->pBuffers == NULL) {
		FN_zfs_destroy_io_manager(pIoman);
		return NULL;	// HT added
	}
    MEMSET(pIoman->pBuffers, 0, sizeof(zfs_buffer_t) * pIoman->cacheSize);

	FN_zfs_init_buffer_descriptors(pIoman);

	// Finally create a Semaphore for Buffer Description modifications.
	FN_ZFS_CREATE_MUTEX(&pIoman->mutex);

	return pIoman;	// Sucess, return the created object.
}

void zfs_destroy_io_manager(pzfs_io_manager_t pIoman)
{
    USE_GLOBAL_BLOCK

	if (pIoman->pBuffers != NULL) {
		SYS_DEALLOCATOR(pIoman->pBuffers);
	}

	if (pIoman->pCacheMem != NULL) {
		SYS_DEALLOCATOR(pIoman->pCacheMem);
	}

	// Destroy any Semaphore that was created.
	FN_ZFS_DESTROY_MUTEX(&pIoman->mutex);

	// Finally free the zfs_io_manager_t object.
	SYS_DEALLOCATOR(pIoman);
}

int zfs_flush_cache(pzfs_io_manager_t pIoman)
{
	uint16_t i, x;
    USE_GLOBAL_BLOCK

	if (pIoman == NULL) {
		return ZFS_ERR_NULL_POINTER | ZFS_FLUSHCACHE;
	}

	FN_ZFS_LOCK_MUTEX(&pIoman->mutex);

	for (i = 0; i < pIoman->cacheSize; i++) {
		if ((pIoman->pBuffers + i)->numHandles == 0 && (pIoman->pBuffers + i)->modified == TRUE) {

			FN_zfs_write_block(pIoman, (pIoman->pBuffers + i)->sector, 1, (pIoman->pBuffers + i)->pBuffer);

			// Buffer has now been flushed, mark it as a read buffer and unmodified.
			(pIoman->pBuffers + i)->mode = ZFS_MODE_READ;
			(pIoman->pBuffers + i)->modified = FALSE;

			// Search for other buffers that used this sector, and mark them as modified
			// So that further requests will result in the new sector being fetched.
			for (x = 0; x < pIoman->cacheSize; x++) {
				if (x != i) {
					if ((pIoman->pBuffers + x)->sector == (pIoman->pBuffers + i)->sector && (pIoman->pBuffers + x)->mode == ZFS_MODE_READ) {
						(pIoman->pBuffers + x)->modified = TRUE;
					}
				}
			}
		}
	}

    FN_ZFS_UNLOCK_MUTEX(&pIoman->mutex);

	return ERR_OK;
}

#define	ZFS_GETBUFFER_SLEEP_TIME	10
#define	ZFS_GETBUFFER_WAIT_TIME	(20000 / ZFS_GETBUFFER_SLEEP_TIME)

pzfs_buffer_t zfs_get_buffer(pzfs_io_manager_t pIoman, uint32_t Sector, uint8_t Mode)
{
	zfs_buffer_t* pBuffer;
	zfs_buffer_t* pBufLRU;
	zfs_buffer_t* pBufMatch = NULL;
	int	RetVal;
	int LoopCount = ZFS_GETBUFFER_WAIT_TIME;
    int cacheSize = pIoman->cacheSize;
    USE_GLOBAL_BLOCK
	
	if (cacheSize <= 0) {
		return NULL;
	}

	while (!pBufMatch) {
		if (!--LoopCount) {
			//
			// *pError = FF_ERR_IOMAN_GETBUFFER_TIMEOUT;
			//
			return NULL;
		}
		FN_ZFS_LOCK_MUTEX(&pIoman->mutex);

		for (pBuffer = pIoman->pBuffers; pBuffer < pIoman->pBuffers + cacheSize; pBuffer++) {
			if (pBuffer->sector == Sector && pBuffer->valid) {
				pBufMatch = pBuffer;
				break;	// Don't look further if you found a perfect match
			}
		}

		if (pBufMatch) {
			// A Match was found process!
			if (Mode == ZFS_MODE_READ && pBufMatch->mode == ZFS_MODE_READ) {
				pBufMatch->numHandles += 1;
				pBufMatch->persistance += 1;
				break;
			}

			if (pBufMatch->numHandles == 0) {
				pBufMatch->mode = (Mode & ZFS_MODE_RD_WR);
				if ((Mode & ZFS_MODE_WRITE) != 0) {	// This buffer has no attached handles.
					pBufMatch->modified = TRUE;
				}
				pBufMatch->numHandles = 1;
				pBufMatch->persistance += 1;
				break;
			}

			pBufMatch = NULL;	// Sector is already in use, keep yielding until its available!

		}
        else {
			pBufLRU   = NULL;	// So put them to NULL here

			for (pBuffer = pIoman->pBuffers; pBuffer < pIoman->pBuffers + cacheSize; pBuffer++) {
				if (pBuffer->numHandles)
					continue;  // Occupied
				pBuffer->lru += 1;

				if (!pBufLRU) {
					pBufLRU = pBuffer;
				}

				if (pBuffer->lru > pBufLRU->lru || (pBuffer->lru == pBufLRU->lru && pBuffer->persistance > pBufLRU->persistance)) {
					pBufLRU = pBuffer;
				}

			}
			// Choose a suitable buffer!
			if (pBufLRU) {
				// Process the suitable candidate.
				if (pBufLRU->modified == TRUE) {
					// Along with the TRUE parameter to indicate semapahore has been claimed
					RetVal = FN_zfs_write_block(pIoman, pBufLRU->sector, 1, pBufLRU->pBuffer);
					if (RetVal < 0) {
						pBufMatch = NULL;
						break;
					}
				}
				if (Mode == ZFS_MODE_WR_ONLY) {
					MEMSET (pBufLRU->pBuffer, '\0', BDEV_BLOCK_SIZE);
				}
                else {
					RetVal = FN_zfs_read_block(pIoman, Sector, 1, pBufLRU->pBuffer);
					if (RetVal < 0) {
						pBufMatch = NULL;
						break;
					}
				}
				pBufLRU->mode = (Mode & ZFS_MODE_RD_WR);
				pBufLRU->persistance = 1;
				pBufLRU->lru = 0;
				pBufLRU->numHandles = 1;
				pBufLRU->sector = Sector;

				pBufLRU->modified = (Mode & ZFS_MODE_WRITE) != 0;

				pBufLRU->valid = TRUE;
				pBufMatch = pBufLRU;
				break;
			}

		}
	}
	FN_ZFS_UNLOCK_MUTEX(&pIoman->mutex);

	return pBufMatch;	// Return the Matched Buffer!
}

void zfs_release_buffer(pzfs_io_manager_t pIoman, pzfs_buffer_t pBuffer)
{
    USE_GLOBAL_BLOCK
	// Protect description changes with a semaphore.
	FN_ZFS_LOCK_MUTEX(&pIoman->mutex);
	if (pBuffer->numHandles) {
		pBuffer->numHandles--;
	}
    else {
		//printf ("FF_ReleaseBuffer: buffer not claimed\n");
	}
	FN_ZFS_UNLOCK_MUTEX(&pIoman->mutex);
}

int zfs_read_block(pzfs_io_manager_t pIoman, uint32_t ulSectorLBA, uint32_t ulNumSectors, void *pBuffer)
{
	int slRetVal = 0;

	if (pIoman->totalSectors) {
		if ((ulSectorLBA + ulNumSectors) > pIoman->totalSectors) {
			return (ZFS_ERR_OUT_OF_BOUNDS_READ | ZFS_BLOCKREAD);		
		}
	}
	
    slRetVal = bdev_read(pBuffer, ulSectorLBA, ulNumSectors, pIoman->pbs);
	return slRetVal;
}

int zfs_write_block(pzfs_io_manager_t pIoman, uint32_t ulSectorLBA, uint32_t ulNumSectors, void *pBuffer)
{
	int slRetVal = 0;

	if (pIoman->totalSectors) {
		if ((ulSectorLBA + ulNumSectors) > pIoman->totalSectors) {
			return (ZFS_ERR_OUT_OF_BOUNDS_WRITE | ZFS_BLOCKWRITE);
		}
	}
	
    slRetVal = bdev_write(pBuffer, ulSectorLBA, ulNumSectors, pIoman->pbs);

	return slRetVal;
}

int zfs_mount(pzfs_io_manager_t pIoman)
{
	zfs_buffer_t* pBuffer = 0;
    USE_GLOBAL_BLOCK

	MEMSET(pIoman->pBuffers, 0, sizeof(zfs_buffer_t) * pIoman->cacheSize);
	MEMSET(pIoman->pCacheMem, 0, BDEV_BLOCK_SIZE * pIoman->cacheSize);

	FN_zfs_init_buffer_descriptors(pIoman);
	pIoman->firstFile = 0;

	pBuffer = FN_zfs_get_buffer(pIoman, 0, ZFS_MODE_READ);
	if (pBuffer == NULL) {
		return ERR_BAD;
	}

	// Assume ZFS16, then we'll adjust if its ZFS32
	pIoman->reservedSectors = *(uint16_t*)(pBuffer->pBuffer + ZFS_RESERVED_SECTORS);
	pIoman->beginLBA = pIoman->reservedSectors;

	pIoman->sectorsPerZFS	= *(uint32_t*)(pBuffer->pBuffer + ZFS_SECTORS_PER_ZFS);
	pIoman->rootDirCluster	= *(uint32_t*)(pBuffer->pBuffer + ZFS_ROOT_DIR_CLUSTER);
	pIoman->clusterBeginLBA	= pIoman->reservedSectors + pIoman->sectorsPerZFS;
	pIoman->totalSectors = *(uint32_t*)(pBuffer->pBuffer + ZFS_TOTAL_SECTORS);

	FN_zfs_release_buffer(pIoman, pBuffer);	// Release the buffer finally!

	pIoman->rootDirSectors	= ((*(uint16_t*)(pBuffer->pBuffer + ZFS_ROOT_ENTRY_COUNT) * ZFS_ENTRY_SIZE) + BDEV_BLOCK_SIZE - 1) / BDEV_BLOCK_SIZE;
	pIoman->firstDataSector	= pIoman->clusterBeginLBA + pIoman->rootDirSectors;
	pIoman->dataSectors		= pIoman->totalSectors - (pIoman->reservedSectors + pIoman->sectorsPerZFS + pIoman->rootDirSectors);
	
	pIoman->numClusters = pIoman->dataSectors / ZFS_SECTORS_PER_CLUSTER;

	pIoman->partitionMounted = TRUE;
	pIoman->lastFreeCluster	= 0;
	pIoman->freeClusterCount = 0;

	return ERR_OK;
}

int zfs_open_device(pzfs_io_manager_t pIoman, const char* fsPath, uint8_t* fsKey, uint32_t keySize)
{
    BlockDriverState* bs;
    int err;
    USE_GLOBAL_BLOCK

    err = FN_bdev_open(&bs, fsPath);

    if (err != ERR_OK) {
        return err;
    }

    err = FN_bdev_set_key(bs, fsKey, keySize);

    if (err != ERR_OK) {
        return err;
    }

    pIoman->pbs = bs;
    
    return err;
}

void zfs_close_device(pzfs_io_manager_t pIoman)
{
    USE_GLOBAL_BLOCK

    FN_bdev_close(pIoman->pbs);
}

char zfs_active_handles(pzfs_io_manager_t pIoman)
{
	uint32_t	i;
	zfs_buffer_t* pBuffer;

	for (i = 0; i < pIoman->cacheSize; ++i) {
		pBuffer = (pIoman->pBuffers + i);
		if (pBuffer->numHandles) {
			return TRUE;
		}
	}

	return FALSE;
}

int zfs_unmount(pzfs_io_manager_t pIoman)
{
	int RetVal = ERR_OK;
    USE_GLOBAL_BLOCK

	if (!pIoman) {
		return ZFS_ERR_NULL_POINTER | ZFS_UNMOUNTPARTITION;
	}
	if (!pIoman->partitionMounted)
		return ERR_OK;

	FN_ZFS_LOCK_MUTEX(&pIoman->mutex);	// Ensure that there are no File Handles
	if (!FN_zfs_active_handles(pIoman)) {
		if (pIoman->firstFile == NULL) {
			// Release Semaphore to call this function!
			FN_ZFS_UNLOCK_MUTEX(&pIoman->mutex);
			FN_zfs_flush_cache(pIoman);			// Flush any unwritten sectors to disk.
			// Reclaim Semaphore
			FN_ZFS_LOCK_MUTEX(&pIoman->mutex);
			pIoman->partitionMounted = FALSE;
		}
        else {
			RetVal = ZFS_ERR_ACTIVE_HANDLES | ZFS_UNMOUNTPARTITION;
		}
	}
    else {
		RetVal = ZFS_ERR_ACTIVE_HANDLES | ZFS_UNMOUNTPARTITION;	// Active handles found on the cache.
	}
	FN_ZFS_UNLOCK_MUTEX(&pIoman->mutex);

	return RetVal;
}


int zfs_increase_free_clusters(pzfs_io_manager_t pIoman, uint32_t Count)
{
	int Error;
    USE_GLOBAL_BLOCK

    if (!pIoman->freeClusterCount) {
		pIoman->freeClusterCount = FN_zfs_count_free_clusters(pIoman, &Error);
		if (ZFS_isERR(Error)) {
			return Error;
		}
	}
    else {
		pIoman->freeClusterCount += Count;
	}

	return ERR_OK;
}

int zfs_decrease_free_clusters(pzfs_io_manager_t pIoman, uint32_t Count)
{
	int Error;
    USE_GLOBAL_BLOCK

	if (!pIoman->freeClusterCount) {
		pIoman->freeClusterCount = FN_zfs_count_free_clusters(pIoman, &Error);
		if (ZFS_isERR(Error)) {
			return Error;
		}
	}
    else {
		pIoman->freeClusterCount -= Count;
	}

	return ERR_OK;
}

uint32_t zfs_get_size(pzfs_io_manager_t pIoman)
{
    if (pIoman != NULL) {
        return (uint32_t)(pIoman->dataSectors * BDEV_BLOCK_SIZE);
    }
    return 0;
}
