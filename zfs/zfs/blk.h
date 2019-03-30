#ifndef _FF_BLK_H_
#define _FF_BLK_H_

#include "ioman.h"

uint32_t zfs_get_cluster_position(uint32_t nEntry, uint16_t nEntrySize);
uint32_t zfs_get_cluster_chain_number(uint32_t nEntry, uint16_t nEntrySize);
uint32_t zfs_get_major_block_number(uint32_t nEntry, uint16_t nEntrySize);
uint8_t	zfs_get_minor_block_number(uint32_t nEntry, uint16_t nEntrySize);
uint32_t zfs_get_minor_block_entry(uint32_t nEntry, uint16_t nEntrySize);

#endif
