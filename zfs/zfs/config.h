#ifndef __ZFS_CONFIG_H_
#define __ZFS_CONFIG_H_

#define ZFS_DRIVER_BUSY_SLEEP   20		// How long ZFS should sleep the thread for in ms, if ZFS_ERR_DRIVER_BUSY is recieved.
#define	ZFS_MAX_FILENAME        32
#define ZFS_MAX_PATH            2600
#define ZFS_SECTORS_PER_CLUSTER 8

#endif // __ZFS_CONFIG_H_
