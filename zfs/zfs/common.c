void zfs_lock(pzfs_io_manager_t pIoman)
{
    USE_GLOBAL_BLOCK

	FN_ZFS_LOCK_MUTEX(&pIoman->mutex);
	while ((pIoman->locks & ZFS_LOCK)) {
		FN_ZFS_UNLOCK_MUTEX(&pIoman->mutex);
		FN_ZFS_THREAD_YIELD();
		FN_ZFS_LOCK_MUTEX(&pIoman->mutex);
	}
	pIoman->locks |= ZFS_LOCK;
	FN_ZFS_UNLOCK_MUTEX(&pIoman->mutex);
}

void zfs_unlock(pzfs_io_manager_t pIoman)
{
    USE_GLOBAL_BLOCK

    FN_ZFS_LOCK_MUTEX(&pIoman->mutex);
    pIoman->locks &= ~ZFS_LOCK;
    FN_ZFS_UNLOCK_MUTEX(&pIoman->mutex);
}

uint32_t zfs_cluster_to_lba(pzfs_io_manager_t pIoman, uint32_t Cluster)
{
	uint32_t lba = 0;

	if (pIoman) {
		if (Cluster > 1) {
			lba = ((Cluster - 2) * ZFS_SECTORS_PER_CLUSTER) + pIoman->firstDataSector;
		}
        else {
			lba = pIoman->clusterBeginLBA;
		}
	}
	return lba;
}

// uint32_t zfs_lba_to_cluster(pzfs_io_manager_t pIoman, uint32_t Address)
// {
// 	uint32_t cluster = 0;
// 	if (pIoman != NULL) {
// 		cluster = ((Address - pIoman->clusterBeginLBA) / ZFS_SECTORS_PER_CLUSTER) + 2;
// 	}
// 	return cluster;
// }

uint32_t zfs_get_entry(pzfs_io_manager_t pIoman, uint32_t nCluster, int* pError)
{
	zfs_buffer_t* pBuffer;
	uint32_t zfsOffset;
	uint32_t zfsSector;
	uint32_t zfsSectorEntry;
	uint32_t zfsEntry;
	uint32_t LBAadjust;
	uint32_t relClusterEntry;
    USE_GLOBAL_BLOCK

    *pError = ERR_OK;

	if (nCluster >= pIoman->numClusters) {
		// HT: find a more specific error code
        *pError = ZFS_ERR_NOT_ENOUGH_FREE_SPACE | ZFS_GETENTRY;
		return 0;
	}
	zfsOffset = 4 * nCluster;
	
	zfsSector = pIoman->beginLBA + (zfsOffset / BDEV_BLOCK_SIZE);
	zfsSectorEntry = zfsOffset % BDEV_BLOCK_SIZE;
	
	LBAadjust = (uint32_t)(zfsSectorEntry / BDEV_BLOCK_SIZE);
	relClusterEntry = zfsSectorEntry % BDEV_BLOCK_SIZE;
	
	pBuffer = FN_zfs_get_buffer(pIoman, zfsSector + LBAadjust, ZFS_MODE_READ);
	if (pBuffer == NULL) {
        *pError = ZFS_ERR_DEVICE_DRIVER_FAILED | ZFS_GETENTRY;
		return 0;
	}
	
	zfsEntry = *(uint32_t*)(pBuffer->pBuffer + relClusterEntry);
	zfsEntry &= 0x0fffffff;	// Clear the top 4 bits.
	FN_zfs_release_buffer(pIoman, pBuffer);

	return zfsEntry;
}

int zfs_clear_cluster(pzfs_io_manager_t pIoman, uint32_t nCluster)
{
	pzfs_buffer_t pBuffer = NULL;
	int i;
	uint32_t BaseLBA;
	int ret = 0;
    USE_GLOBAL_BLOCK

	BaseLBA = FN_zfs_cluster_to_lba(pIoman, nCluster);

	for (i = 0; i < ZFS_SECTORS_PER_CLUSTER; i++) {
		if (i == 0) {
			pBuffer = FN_zfs_get_buffer(pIoman, BaseLBA, ZFS_MODE_WR_ONLY);
			if (!pBuffer) {
				return ZFS_ERR_DEVICE_DRIVER_FAILED | ZFS_CLEARCLUSTER;
			}
			MEMSET(pBuffer->pBuffer, 0x00, 512);
		}
		ret = FN_zfs_write_block(pIoman, BaseLBA+i, 1, pBuffer->pBuffer);
		if (ret < 0) {
			break;
		}
	}
	pBuffer->modified = FALSE;
	FN_zfs_release_buffer(pIoman, pBuffer);

	if (ZFS_isERR(ret)) {
		return ret;
	}

	return ERR_OK;
}

uint32_t zfs_traverse(pzfs_io_manager_t pIoman, uint32_t Start, uint32_t Count, int*pError)
{
	uint32_t i;
	uint32_t zfsEntry = Start, currentCluster = Start;
    USE_GLOBAL_BLOCK

    *pError = ERR_OK;

	for (i = 0; i < Count; i++) {
		zfsEntry = FN_zfs_get_entry(pIoman, currentCluster, pError);
		if (*pError) {
			return 0;
		}

		if (FN_zfs_is_end_of_chain(zfsEntry)) {
			return currentCluster;
		}
        else {
			currentCluster = zfsEntry;
		}	
	}
	
	return zfsEntry;
}

uint32_t zfs_find_end_of_chain(pzfs_io_manager_t pIoman, uint32_t Start, int*pError)
{
	uint32_t zfsEntry = Start, currentCluster = Start;
    USE_GLOBAL_BLOCK

    *pError = ERR_OK;

	while (!FN_zfs_is_end_of_chain(zfsEntry)) {
		zfsEntry = FN_zfs_get_entry(pIoman, currentCluster, pError);
		if (*pError) {
			return 0;
		}

		if (FN_zfs_is_end_of_chain(zfsEntry)) {
			return currentCluster;
		}
        else {
			currentCluster = zfsEntry;
		}	
	}	
	return zfsEntry;
}

char zfs_is_end_of_chain(uint32_t zfsEntry)
{
	char result = FALSE;
	if ((zfsEntry & 0x0fffffff) >= 0x0ffffff8) {
		result = TRUE;
	}
	if (zfsEntry == 0x00000000) {
		result = TRUE;	//Perhaps trying to read a deleted file!
	}
	return result;
}

int zfs_put_entry(pzfs_io_manager_t pIoman, uint32_t nCluster, uint32_t val)
{
	zfs_buffer_t* pBuffer;
	uint32_t zfsOffset;
	uint32_t zfsSector;
	uint32_t zfsSectorEntry;
	uint32_t LBAadjust;
	uint32_t relClusterEntry;
    USE_GLOBAL_BLOCK
	
	// HT: avoid corrupting the disk
	if (!nCluster || nCluster >= pIoman->numClusters) {
		// find a more specific error code
		return ZFS_ERR_NOT_ENOUGH_FREE_SPACE | ZFS_PUTZFSENTRY;
	}
	zfsOffset = nCluster * 4;
	
	zfsSector = pIoman->beginLBA + (zfsOffset / BDEV_BLOCK_SIZE);
	zfsSectorEntry = zfsOffset % BDEV_BLOCK_SIZE;
	
	LBAadjust = (uint32_t) (zfsSectorEntry / BDEV_BLOCK_SIZE);
	relClusterEntry = zfsSectorEntry % BDEV_BLOCK_SIZE;
	
	pBuffer = FN_zfs_get_buffer(pIoman, zfsSector + LBAadjust, ZFS_MODE_WRITE);
	if (!pBuffer) {
		return ZFS_ERR_DEVICE_DRIVER_FAILED | ZFS_PUTZFSENTRY;
	}
	val &= 0x0fffffff;	// Clear the top 4 bits.
    *(uint32_t*)(pBuffer->pBuffer + relClusterEntry) = val;
	FN_zfs_release_buffer(pIoman, pBuffer);

	return ERR_OK;
}

uint32_t zfs_find_free_cluster(pzfs_io_manager_t pIoman, int*pError)
{
	zfs_buffer_t* pBuffer;
	uint32_t x, nCluster = pIoman->lastFreeCluster;
	uint32_t zfsOffset;
	uint32_t zfsSector;
	uint32_t zfsSectorEntry;
	uint32_t EntriesPerSector;
	uint32_t zfsEntry = 1;
	const int EntrySize = 4;
    USE_GLOBAL_BLOCK

    *pError = ERR_OK;

	EntriesPerSector = BDEV_BLOCK_SIZE / EntrySize;
	zfsOffset = nCluster * EntrySize;
	
	for (zfsSector = (zfsOffset / BDEV_BLOCK_SIZE);
		zfsSector < pIoman->sectorsPerZFS;
		zfsSector++) {
		pBuffer = FN_zfs_get_buffer(pIoman, pIoman->beginLBA + zfsSector, ZFS_MODE_READ);
		if (!pBuffer) {
            *pError = ZFS_ERR_DEVICE_DRIVER_FAILED | ZFS_FINDFREECLUSTER;
			return 0;
		}
		// HT double-check: don't use non-existing clusters
		if (nCluster >= pIoman->numClusters) {
			FN_zfs_release_buffer(pIoman, pBuffer);
            *pError = ZFS_ERR_NOT_ENOUGH_FREE_SPACE | ZFS_FINDFREECLUSTER;
			return 0;
		}
		for (x = nCluster % EntriesPerSector; x < EntriesPerSector; x++) {
			zfsSectorEntry	= zfsOffset % BDEV_BLOCK_SIZE;
			zfsEntry = *(uint32_t*)(pBuffer->pBuffer + zfsSectorEntry);
			zfsEntry &= 0x0fffffff;	// Clear the top 4 bits.
			if (zfsEntry == 0x00000000) {
				FN_zfs_release_buffer(pIoman, pBuffer);
				pIoman->lastFreeCluster = nCluster;
				return nCluster;
			}
			zfsOffset += EntrySize;
			nCluster++;
		}
		FN_zfs_release_buffer(pIoman, pBuffer);
	}
    *pError = ZFS_ERR_NOT_ENOUGH_FREE_SPACE | ZFS_FINDFREECLUSTER;
	return 0;
}

uint32_t zfs_create_cluster_chain(pzfs_io_manager_t pIoman, int*pError)
{
	uint32_t iStartCluster;
	int	Error;
    USE_GLOBAL_BLOCK

    *pError = ERR_OK;

	FN_zfs_lock(pIoman);
	iStartCluster = FN_zfs_find_free_cluster(pIoman, &Error);
	if (ZFS_isERR(Error)) {
        *pError = Error;
		FN_zfs_unlock(pIoman);
		return 0;
	}

	if (iStartCluster) {
		Error = FN_zfs_put_entry(pIoman, iStartCluster, 0xFFFFFFFF); // Mark the cluster as End-Of-Chain
		if (ZFS_isERR(Error)) {
            *pError = Error;
			FN_zfs_unlock(pIoman);
			return 0;
		}
	}
	FN_zfs_unlock(pIoman);

	if (iStartCluster) {
		Error = FN_zfs_decrease_free_clusters(pIoman, 1);
		if (ZFS_isERR(Error)) {
            *pError = Error;
			return 0;
		}
	}

	return iStartCluster;
}

uint32_t zfs_get_chain_length(pzfs_io_manager_t pIoman, uint32_t startCluster, uint32_t *pEndOfChain, int* pError)
{
	uint32_t len = 0, prevCluster = startCluster;
    USE_GLOBAL_BLOCK

    *pError = ERR_OK;
	
	FN_zfs_lock(pIoman);
	while (!FN_zfs_is_end_of_chain(startCluster)) {
        prevCluster = startCluster;
		startCluster = FN_zfs_get_entry(pIoman, startCluster, pError);
		if (*pError) {
			// break to call FF_unlockZFS
			len = 0;
			break;
		}
		++len;
	}
	if (pEndOfChain) {
        *pEndOfChain = prevCluster;
	}
	FN_zfs_unlock(pIoman);

	return len;
}

int zfs_unlink_cluster_chain(pzfs_io_manager_t pIoman, uint32_t startCluster)
{
	uint32_t zfsEntry;
	uint32_t currentCluster;
	uint32_t iLen = 0;
	uint32_t lastFree = startCluster;	/* HT addition : reset LastFreeCluster */
	int	Error;
    USE_GLOBAL_BLOCK

	zfsEntry = startCluster;

	// Free all clusters in the chain!
	currentCluster = startCluster;
	zfsEntry = currentCluster;
    do {
		zfsEntry = FN_zfs_get_entry(pIoman, zfsEntry, &Error);
		if (ZFS_isERR(Error)) {
			return Error;
		}
		Error = FN_zfs_put_entry(pIoman, currentCluster, 0x00000000);
		if (ZFS_isERR(Error)) {
			return Error;
		}

		if (lastFree > currentCluster) {
			lastFree = currentCluster;
		}
		currentCluster = zfsEntry;
		iLen ++;
	} while (!FN_zfs_is_end_of_chain(zfsEntry));
	if (pIoman->lastFreeCluster > lastFree) {
		pIoman->lastFreeCluster = lastFree;
	}
	Error = FN_zfs_increase_free_clusters(pIoman, iLen);
	if (ZFS_isERR(Error)) {
		return Error;
	}

	return ERR_OK;
}

uint32_t zfs_count_free_clusters(pzfs_io_manager_t pIoman, int*pError)
{
	zfs_buffer_t* pBuffer;
	uint32_t i, x;
	uint32_t zfsEntry;
	uint32_t EntriesPerSector;
	uint32_t FreeClusters = 0;
    USE_GLOBAL_BLOCK

    *pError = ERR_OK;

	EntriesPerSector = BDEV_BLOCK_SIZE / 4;

	for (i = 0; i < pIoman->sectorsPerZFS; i++) {
		pBuffer = FN_zfs_get_buffer(pIoman, pIoman->beginLBA + i, ZFS_MODE_READ);
		if (!pBuffer) {
            *pError = ZFS_ERR_DEVICE_DRIVER_FAILED | ZFS_COUNTFREECLUSTERS;
			return 0;
		}
		for (x = 0; x < EntriesPerSector; x++) {
			zfsEntry = *(uint32_t*)(pBuffer->pBuffer + x * 4) & 0x0fffffff; // Clearing the top 4 bits.
			if (!zfsEntry) {
				FreeClusters++;
			}
		}
		FN_zfs_release_buffer(pIoman, pBuffer);
	}

	return FreeClusters <= pIoman->numClusters ? FreeClusters : pIoman->numClusters;
}

uint32_t zfs_get_free_size(pzfs_io_manager_t pIoman, int*pError)
{
    uint32_t freeClusters;
    USE_GLOBAL_BLOCK

    if (pIoman) {
        FN_zfs_lock(pIoman);	
        if (!pIoman->freeClusterCount) {
            pIoman->freeClusterCount = FN_zfs_count_free_clusters(pIoman, pError);
        }
        freeClusters = pIoman->freeClusterCount;
        FN_zfs_unlock(pIoman);
        return (freeClusters * (ZFS_SECTORS_PER_CLUSTER * BDEV_BLOCK_SIZE));
    }
    return 0;
}

