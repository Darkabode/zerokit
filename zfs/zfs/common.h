#ifndef __ZFS_COMMON_H_
#define __ZFS_COMMON_H_

uint32_t zfs_cluster_to_lba(pzfs_io_manager_t pIoman, uint32_t Cluster);
uint32_t zfs_get_entry(pzfs_io_manager_t pIoman, uint32_t nCluster, int *pError);
int	zfs_put_entry(pzfs_io_manager_t pIoman, uint32_t nCluster, uint32_t Value);
char zfs_is_end_of_chain(uint32_t zfsEntry);
uint32_t zfs_find_free_cluster(pzfs_io_manager_t pIoman, int *pError);
uint32_t zfs_extend_cluster_chain(pzfs_io_manager_t pIoman, uint32_t startCluster, uint32_t Count);
int zfs_unlink_cluster_chain(pzfs_io_manager_t pIoman, uint32_t startCluster);
uint32_t zfs_traverse(pzfs_io_manager_t pIoman, uint32_t Start, uint32_t Count, int *pError);
uint32_t zfs_create_cluster_chain(pzfs_io_manager_t pIoman, int *pError);
uint32_t zfs_get_chain_length(pzfs_io_manager_t pIoman, uint32_t startCluster, uint32_t *piEndOfChain, int *pError);
uint32_t zfs_find_end_of_chain(pzfs_io_manager_t pIoman, uint32_t Start, int *pError);
int zfs_clear_cluster(pzfs_io_manager_t pIoman, uint32_t nCluster);
uint32_t zfs_get_free_size(pzfs_io_manager_t pIoman, int *pError);
uint32_t zfs_count_free_clusters(pzfs_io_manager_t pIoman, int *pError);	// WARNING: If this protoype changes, it must be updated in ff_ioman.c also!
void zfs_lock(pzfs_io_manager_t pIoman);
void zfs_unlock(pzfs_io_manager_t pIoman);

#endif // __ZFS_COMMON_H_
