uint32_t zfs_get_cluster_chain_number(uint32_t nEntry, uint16_t nEntrySize)
{
	uint32_t clusterChainNumber	= nEntry / (BDEV_BLOCK_SIZE * ZFS_SECTORS_PER_CLUSTER / nEntrySize);
	return clusterChainNumber;
}

uint32_t zfs_get_cluster_position(uint32_t nEntry, uint16_t nEntrySize)
{
	return nEntry % ((BDEV_BLOCK_SIZE * ZFS_SECTORS_PER_CLUSTER) / nEntrySize);
}

uint32_t zfs_get_major_block_number(uint32_t nEntry, uint16_t nEntrySize)
{
	uint32_t relClusterEntry = nEntry % (BDEV_BLOCK_SIZE * ZFS_SECTORS_PER_CLUSTER / nEntrySize);
	uint32_t majorBlockNumber = relClusterEntry / (BDEV_BLOCK_SIZE / nEntrySize);
	return majorBlockNumber;
}

uint8_t zfs_get_minor_block_number(uint32_t nEntry, uint16_t nEntrySize)
{
	uint32_t relClusterEntry = nEntry % (BDEV_BLOCK_SIZE * ZFS_SECTORS_PER_CLUSTER / nEntrySize);
	uint16_t relmajorBlockEntry	= (uint16_t)(relClusterEntry % (BDEV_BLOCK_SIZE / nEntrySize));
	uint8_t minorBlockNumber = (uint8_t) (relmajorBlockEntry / (BDEV_BLOCK_SIZE / nEntrySize));
	return minorBlockNumber;
}

uint32_t zfs_get_minor_block_entry(uint32_t nEntry, uint16_t nEntrySize)
{
	uint32_t relClusterEntry = nEntry % (BDEV_BLOCK_SIZE * ZFS_SECTORS_PER_CLUSTER / nEntrySize);
	uint32_t relmajorBlockEntry	= (uint32_t)(relClusterEntry % (BDEV_BLOCK_SIZE / nEntrySize));
	return (relmajorBlockEntry % (BDEV_BLOCK_SIZE / nEntrySize));
}
