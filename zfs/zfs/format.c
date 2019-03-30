void zfs_put_boot(zfsparams * ft, unsigned char* boot)
{
    int cnt = 0;
    USE_GLOBAL_BLOCK

    boot[cnt++] = 0xeb;	/* boot jump */
    boot[cnt++] = 0x3c;
    boot[cnt++] = 0x90;
    MEMSET (boot + cnt, 0x20, 8); /* system id */
    cnt += 8;
    *(__int16 *)(boot + cnt) = BDEV_BLOCK_SIZE;	/* bytes per sector */
    cnt += 2;
    boot[cnt++] = ZFS_SECTORS_PER_CLUSTER;			/* sectors per cluster */
    *(__int16 *)(boot + cnt) = ft->reserved;		/* reserved sectors */
    cnt += 2;
    boot[cnt++] = 1;					/* 1 zfs */

    boot[cnt++] = 0x00;
    boot[cnt++] = 0x00;

    *(__int16 *)(boot + cnt) = 0;		/* # sectors */
    cnt += 2;
    boot[cnt++] = 0xf8;					/* media byte */

    boot[cnt++] = 0x00;                 /* sectors per zfs */
    boot[cnt++] = 0x00;

    *(__int16 *)(boot + cnt) = 1;	/* # sectors per track */
    cnt += 2;
    *(__int16 *)(boot + cnt) = 1;			/* # heads */
    cnt += 2;
    *(__int32 *)(boot + cnt) = 0;		/* # hidden sectors */
    cnt += 4;
    *(__int32 *)(boot + cnt) = ft->total_sect;	/* # huge sectors */
    cnt += 4;

    *(__int32 *)(boot + cnt) = ft->zfs_length; cnt += 4;	/* zfs size 32 */
    boot[cnt++] = 0x00;	/* ExtFlags */
    boot[cnt++] = 0x00;
    boot[cnt++] = 0x00;	/* FSVer */
    boot[cnt++] = 0x00;
    boot[cnt++] = 0x02;	/* RootClus */
    boot[cnt++] = 0x00;
    boot[cnt++] = 0x00;
    boot[cnt++] = 0x00;
    boot[cnt++] = 0x01;	/* FSInfo */
    boot[cnt++] = 0x00;
    boot[cnt++] = 0x06;	/* BkBootSec */
    boot[cnt++] = 0x00;
    MEMSET(boot+cnt, 0, 12); cnt+=12;	/* Reserved */

    boot[cnt++] = 0x00;	/* drive number */   // FIXED 80 > 00
    boot[cnt++] = 0x00;	/* reserved */
    boot[cnt++] = 0x29;	/* boot sig */

    *(uint32_t*)(boot + cnt) = 0xAABBCCDD;		/* vol id */
    cnt += 4;

    MEMSET(boot + cnt, 0x20, 11);	/* vol title */
    cnt += 11;

//     *(uint32_t*)(boot + cnt) = 0x39313437;
//     *(uint32_t*)(boot + cnt + sizeof(uint32_t)) = 0x31313538;
    cnt += 8;

    MEMSET (boot + cnt, 0, 420);	/* boot code */
    cnt += 420;
    boot[cnt++] = 0x55;
    boot[cnt++] = 0xaa;	/* boot sig */
}

void zfs_put_fs_info(unsigned char* sector, zfsparams *ft)
{
    USE_GLOBAL_BLOCK

    MEMSET(sector, 0, BDEV_BLOCK_SIZE);
    sector[3]=0x41; /* LeadSig */
    sector[2]=0x61; 
    sector[1]=0x52; 
    sector[0]=0x52; 
    sector[484+3]=0x61; /* StrucSig */
    sector[484+2]=0x41; 
    sector[484+1]=0x72; 
    sector[484+0]=0x72; 

    // Free cluster count
    *(uint32_t*)(sector + 488) = ft->cluster_count - ft->size_root_dir / BDEV_BLOCK_SIZE / 8;

    // Next free cluster
    *(uint32_t*)(sector + 492) = 2;

    sector[508+3]=0xaa; /* TrailSig */
    sector[508+2]=0x55;
    sector[508+1]=0x00;
    sector[508+0]=0x00;
}

int zfs_format(pzfs_io_manager_t pIoman)
{
    zfsparams ft;
	uint8_t sector[BDEV_BLOCK_SIZE];
	uint32_t nSecNo = 0;
	int x, n;
    uint32_t zfssecs;
    USE_GLOBAL_BLOCK

    ft.size_root_dir = 16 * BDEV_BLOCK_SIZE;
    ft.reserved = 32 - 1;

    do {
        ft.reserved++;
        zfssecs = pIoman->pbs->total_sectors - ft.reserved;
        ft.cluster_count = (int) ((zfssecs * BDEV_BLOCK_SIZE) / (8 * BDEV_BLOCK_SIZE));
        ft.zfs_length = (ft.cluster_count * 4 + BDEV_BLOCK_SIZE - 1) / BDEV_BLOCK_SIZE;
        // Align data area on TC_MAX_VOLUME_SECTOR_SIZE
    } while ((ft.reserved * BDEV_BLOCK_SIZE + ft.zfs_length * BDEV_BLOCK_SIZE) % BDEV_BLOCK_SIZE != 0);

    ft.cluster_count -= ft.zfs_length / 8;
    ft.total_sect = pIoman->pbs->total_sectors;

	MEMSET(sector, 0, 512);

	FN_zfs_put_boot(&ft, (unsigned char*)sector);

    if (FN_bdev_write((const uint8_t*)sector, nSecNo++, 1, pIoman->pbs) != ERR_OK)
        goto fail;

	/* zfs32 boot area */
	/* fsinfo */
	FN_zfs_put_fs_info((unsigned char* ) sector, &ft);
    if (FN_bdev_write(sector, nSecNo++, 1, pIoman->pbs) != ERR_OK)
		goto fail;

	/* reserved */
	while (nSecNo < 6) {
		MEMSET (sector, 0, BDEV_BLOCK_SIZE);
		sector[508+3]=0xaa; /* TrailSig */
		sector[508+2]=0x55;
        if (FN_bdev_write(sector, nSecNo++, 1, pIoman->pbs) != ERR_OK)
			goto fail;
	}
	
	/* bootsector backup */
	MEMSET (sector, 0, BDEV_BLOCK_SIZE);
	FN_zfs_put_boot(&ft, (unsigned char* ) sector);
    if (FN_bdev_write(sector, nSecNo++, 1, pIoman->pbs) != ERR_OK)
		goto fail;

	FN_zfs_put_fs_info((unsigned char* ) sector, &ft);
	if (FN_bdev_write(sector, nSecNo++, 1, pIoman->pbs) != ERR_OK)
		goto fail;

	/* reserved */
	while (nSecNo < (unsigned int)ft.reserved) {
		MEMSET(sector, 0, BDEV_BLOCK_SIZE);
		if (FN_bdev_write(sector, nSecNo++, 1, pIoman->pbs) != ERR_OK)
			goto fail;
	}

	/* write zfs */
	for (n = 0; n < ft.zfs_length; n++) {
		MEMSET(sector, 0, BDEV_BLOCK_SIZE);

		if (n == 0) {
			unsigned char zfs_sig[12];
			zfs_sig[0] = 0xf8;
			zfs_sig[1] = zfs_sig[2] = 0xff;
			zfs_sig[3] = 0x0f;
			zfs_sig[4] = zfs_sig[5] = zfs_sig[6] = 0xff;
			zfs_sig[7] = 0x0f;
			zfs_sig[8] = zfs_sig[9] = zfs_sig[10] = 0xff;
			zfs_sig[11] = 0x0f;
			MEMCPY(sector, zfs_sig, 12);
		}

		if (FN_bdev_write(sector, nSecNo++, 1, pIoman->pbs) != ERR_OK)
			goto fail;
	}


	/* write rootdir */
	for (x = 0; x < ft.size_root_dir / BDEV_BLOCK_SIZE; x++) {
		MEMSET(sector, 0, BDEV_BLOCK_SIZE);
		if (FN_bdev_write(sector, nSecNo++, 1, pIoman->pbs) != ERR_OK)
			goto fail;

	}

	return ERR_OK;

fail:
	return -ERR_BAD;
}