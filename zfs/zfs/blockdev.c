int bdev_native_open(BlockDriverState* bs, const char* fileName)
{
#ifndef RING3
    NTSTATUS ntStatus;
    OBJECT_ATTRIBUTES objAttrs;
    ANSI_STRING aFileName;
    UNICODE_STRING uFileName;
    IO_STATUS_BLOCK ioStatus;
#endif
    USE_GLOBAL_BLOCK

#if RING3
    bs->file = CreateFileA(fileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING , 0, NULL);

    if (bs->file == INVALID_HANDLE_VALUE) {
        return ERR_BAD;
    }
#else

    RTL_INIT_ANSI_STRING(&aFileName, fileName);

    RTL_ANSI_STRING_TO_UNICODE_STRING(&uFileName, &aFileName, TRUE);

    InitializeObjectAttributes(&objAttrs, &uFileName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, 0, NULL);

    ntStatus = ZW_CREATE_FILE(&(HANDLE)bs->file, FILE_WRITE_DATA | FILE_READ_DATA | SYNCHRONIZE, &objAttrs, &ioStatus, NULL, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM, 0,
        FILE_OPEN, FILE_SYNCHRONOUS_IO_NONALERT | FILE_RANDOM_ACCESS | FILE_WRITE_THROUGH | FILE_NON_DIRECTORY_FILE, NULL, 0);

    RTL_FREE_UNICODE_STRING(&uFileName);

    if (!NT_SUCCESS(ntStatus)) {
        return ERR_BAD;
    }

#endif // RING3
    return ERR_OK;
}

int bdev_narive_read(BlockDriverState* bs, uint8_t* buffer, uint32_t sector, uint16_t sectors)
{
#ifndef RING3
    NTSTATUS ntStatus;
    IO_STATUS_BLOCK ioStatus;
#endif // RING3    
	LARGE_INTEGER address;
	ulong_t Read = 0;
    USE_GLOBAL_BLOCK
	
    address.LowPart = sector * 512;
    address.HighPart = 0;

	FN_ZFS_LOCK_MUTEX(&bs->fileMutex);
#if RING3
	SetFilePointerEx(bs->file, address, NULL, FILE_BEGIN);
	ReadFile(bs->file, buffer, 512 * sectors, &Read, NULL);
#else
    ntStatus = ZW_READ_FILE(bs->file, NULL, NULL, NULL, &ioStatus, buffer, 512 * sectors, &address, NULL);
    if (ntStatus != STATUS_SUCCESS) {
        FN_ZFS_UNLOCK_MUTEX(&bs->fileMutex);
        return -1;
    }    
    Read = 512 * sectors;
#endif // RING3
	FN_ZFS_UNLOCK_MUTEX(&bs->fileMutex);

	return Read / 512;
}

int bdev_native_write(BlockDriverState* bs, const uint8_t* buffer, ulong_t sector, uint16_t sectors)
{
#ifndef RING3
    NTSTATUS ntStatus;
    IO_STATUS_BLOCK ioStatus;
#endif // RING3
	LARGE_INTEGER address;
	ulong_t written;
    USE_GLOBAL_BLOCK

	address.LowPart = sector * 512;
    address.HighPart = 0;
	
	FN_ZFS_LOCK_MUTEX(&bs->fileMutex);
#if RING3
	SetFilePointerEx(bs->file, address, NULL, FILE_BEGIN);
	WriteFile(bs->file, buffer, 512 * sectors, &written, NULL);
#else
    ntStatus = ZW_WRITE_FILE((HANDLE)bs->file, NULL, NULL, NULL, &ioStatus, (pvoid_t)buffer, (ULONG)(512 * sectors), &address, NULL);
    if (ntStatus != STATUS_SUCCESS) {
        FN_ZFS_UNLOCK_MUTEX(&bs->fileMutex);
        return -1;
    }
    written = 512 * sectors;
#endif // RING3
	FN_ZFS_UNLOCK_MUTEX(&bs->fileMutex);

	return written / 512;
}

uint32_t bdev_pread(BlockDriverState* bs, uint32_t offset, uint8_t* buf, uint32_t count1)
{
    uint8_t tmp_buf[BDRV_SECTOR_SIZE];
    uint32_t len, nb_sectors, count;
    ulong_t sector_num;
    int ret;
    USE_GLOBAL_BLOCK

    count = count1;
    /* first read to align to sector start */
    len = (BDRV_SECTOR_SIZE - offset) & (BDRV_SECTOR_SIZE - 1);
    if (len > count)
        len = count;
    sector_num = (ulong_t)(offset >> BDRV_SECTOR_BITS);
    if (len > 0) {
        if ((ret = FN_bdev_native_read(bs, tmp_buf, sector_num, 1)) < 0)
            return ret;
        MEMCPY(buf, tmp_buf + (offset & (BDRV_SECTOR_SIZE - 1)), len);
        count -= len;
        if (count == 0)
            return count1;
        sector_num++;
        buf += len;
    }

    /* read the sectors "in place" */
    nb_sectors = count >> BDRV_SECTOR_BITS;
    if (nb_sectors > 0) {
        if ((ret = FN_bdev_native_read(bs, buf, sector_num, (uint16_t)nb_sectors)) < 0)
            return ret;
        sector_num += nb_sectors;
        len = nb_sectors << BDRV_SECTOR_BITS;
        buf += len;
        count -= len;
    }

    /* add data from the last sector */
    if (count > 0) {
        if ((ret = FN_bdev_native_read(bs, tmp_buf, sector_num, 1)) < 0)
            return ret;
        MEMCPY(buf, tmp_buf, count);
    }
    return count1;
}

int bdev_pwrite(BlockDriverState* bs, uint32_t offset, const uint8_t* buf, int count1)
{
    uint8_t tmp_buf[BDRV_SECTOR_SIZE];
    int len, nb_sectors, count;
    uint32_t sector_num;
    int ret;
    USE_GLOBAL_BLOCK

    count = count1;
    /* first write to align to sector start */
    len = (BDRV_SECTOR_SIZE - offset) & (BDRV_SECTOR_SIZE - 1);
    if (len > count)
        len = count;
    sector_num = (offset >> BDRV_SECTOR_BITS);
    if (len > 0) {
        if ((ret = FN_bdev_native_read(bs, tmp_buf, sector_num, 1)) < 0)
            return ret;
        MEMCPY(tmp_buf + (offset & (BDRV_SECTOR_SIZE - 1)), buf, len);
        if ((ret = FN_bdev_native_write(bs, tmp_buf, sector_num, 1)) < 0)
            return ret;
        count -= len;
        if (count == 0)
            return count1;
        sector_num++;
        buf += len;
    }

    /* write the sectors "in place" */
    nb_sectors = count >> BDRV_SECTOR_BITS;
    if (nb_sectors > 0) {
        if ((ret = FN_bdev_native_write(bs, buf, sector_num, (uint16_t)nb_sectors)) < 0)
            return ret;
        sector_num += nb_sectors;
        len = nb_sectors << BDRV_SECTOR_BITS;
        buf += len;
        count -= len;
    }

    /* add data from the last sector */
    if (count > 0) {
        if ((ret = FN_bdev_native_read(bs, tmp_buf, sector_num, 1)) < 0)
            return ret;
        MEMCPY(tmp_buf, buf, count);
        if ((ret = FN_bdev_native_write(bs, tmp_buf, sector_num, 1)) < 0)
            return ret;
    }
    return count1;
}

uint32_t bdev_write_full(HANDLE hFile, const uint8_t* buf, uint32_t count)
{
#ifndef RING3
    NTSTATUS ntStatus;
    IO_STATUS_BLOCK ioStatus;
    LARGE_INTEGER address;
#else
    ulong_t written;
#endif // RING3
    uint32_t total = 0;
    USE_GLOBAL_BLOCK

#if RING3
    while (count) {
        if (!WriteFile(hFile, buf, count, &written, NULL)) {
            break;
        }
        count -= written;
        buf += written;
        total += written;
    }
#else
    address.HighPart = -1;
    address.LowPart = FILE_USE_FILE_POINTER_POSITION;
    ntStatus = ZW_WRITE_FILE(hFile, NULL, NULL, NULL, &ioStatus, (pvoid_t)buf, (ULONG)count, &address, NULL);

    if (ntStatus != STATUS_SUCCESS) {
        return 0;        
    }
    total = count;
#endif // RING3

    return total;
}

int bdev_create(const char* fileName, uint32_t virtSize)
{
    uint32_t header_size, l1_size, i, shift;
    zfs_bdev_header_t header;
    uint64_t tmp;
    uint32_t total_size = 0;
    int ret = ERR_OK;
    HANDLE hFile;
#ifndef RING3
    NTSTATUS ntStatus;
    OBJECT_ATTRIBUTES objAttrs;
    ANSI_STRING aFileName;
    UNICODE_STRING uFileName;
    IO_STATUS_BLOCK ioStatus;
    FILE_POSITION_INFORMATION fp;
#endif
    USE_GLOBAL_BLOCK

#if RING3
    hFile = CreateFileA(fileName, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, 0, NULL);

    if (hFile == INVALID_HANDLE_VALUE) {
        return ERR_BAD;
    }
#else

    RTL_INIT_ANSI_STRING(&aFileName, fileName);

    RTL_ANSI_STRING_TO_UNICODE_STRING(&uFileName, &aFileName, TRUE);

    InitializeObjectAttributes(&objAttrs, &uFileName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, 0, NULL);

    ntStatus = ZW_CREATE_FILE(&hFile, FILE_READ_DATA | FILE_WRITE_DATA | SYNCHRONIZE, &objAttrs, &ioStatus, NULL, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM, 0,
        FILE_OVERWRITE_IF, FILE_SYNCHRONOUS_IO_NONALERT | FILE_WRITE_THROUGH |FILE_RANDOM_ACCESS | FILE_NON_DIRECTORY_FILE, NULL, 0);

//     if (ioStatus.Information == FILE_CREATED) {
//         ntStatus = pGlobalBlock->pCommonBlock->fnZwFsControlFile(hFile, NULL, NULL, NULL,
//             &ioStatus, FSCTL_SET_SPARSE, NULL, 0, NULL, 0);
//     }

    RTL_FREE_UNICODE_STRING(&uFileName);

    if (!NT_SUCCESS(ntStatus)) {
        return ERR_BAD;
    }
#endif // RING3

    total_size = virtSize / 512;
    MEMSET(&header, 0, sizeof(header));
    header.magic = BD_MAGIC;
    header.size = total_size * 512;
    header_size = sizeof(header);
    header.cluster_bits = 12; /* 4 KB clusters */
    header.l2_bits = 9; /* 4 KB L2 tables */
    header_size = (header_size + 7) & ~7;
    shift = header.cluster_bits + header.l2_bits;
    l1_size = ((total_size * 512) + (1 << shift) - 1) >> shift;

    header.l1_table_offset = header_size;

    /* write all the data */
    ret = FN_bdev_write_full(hFile, (const uint8_t*)&header, sizeof(header));
    if (ret != sizeof(header)) {
        ret = ERR_BAD;
        goto exit;
    }
#if RING3
    SetFilePointer(hFile, header_size, NULL, FILE_BEGIN);
#else
    fp.CurrentByteOffset.LowPart = header_size;
    fp.CurrentByteOffset.HighPart = 0;
    if (FN_ZW_SET_INFORMATION_FILE(hFile, &ioStatus, &fp, sizeof(fp), FilePositionInformation) != STATUS_SUCCESS) {
        ret = ERR_BAD;
        goto exit;
    }
#endif // RING3
    tmp = 0;
    for (i = 0;i < l1_size; ++i) {
        ret = FN_bdev_write_full(hFile, (const uint8_t*)&tmp, sizeof(tmp));
        if (ret != sizeof(tmp)) {
            ret = ERR_BAD;
            goto exit;
        }
    }

    ret = 0;
exit:
#if RING3
    CloseHandle(hFile);
#else
    ZW_CLOSE(hFile);
#endif
    return ret;
}

int bdev_ptruncate(HANDLE hFile, uint32_t offset)
{
#ifndef RING3
    IO_STATUS_BLOCK ioStatus;
    FILE_POSITION_INFORMATION fp;
    USE_GLOBAL_BLOCK
#endif // RING3

#ifdef RING3
    if (!SetFilePointer(hFile, (LONG)offset, NULL, FILE_BEGIN))
        return ERR_BAD;
    if (!SetEndOfFile(hFile))
        return ERR_BAD;
#else
    fp.CurrentByteOffset.LowPart = offset;
    fp.CurrentByteOffset.HighPart = 0;
    if (FN_ZW_SET_INFORMATION_FILE(hFile, &ioStatus, &fp, sizeof(fp), FilePositionInformation) != STATUS_SUCCESS)
        return ERR_BAD;
    
    if (FN_ZW_SET_INFORMATION_FILE(hFile, &ioStatus, &fp, sizeof(fp), FileEndOfFileInformation) != STATUS_SUCCESS)
        return ERR_BAD;

    if (FN_ZW_SET_INFORMATION_FILE(hFile, &ioStatus, &fp, sizeof(fp), FileAllocationInformation) != STATUS_SUCCESS)
        return ERR_BAD;
#endif // RING3
    return ERR_OK;
}

int bdev_make_empty(BlockDriverState* bs)
{
    zfs_bdev_state_t *s = bs->opaque;
    uint32_t l1_length = s->l1_size * sizeof(uint64_t);
    int ret;
    USE_GLOBAL_BLOCK

    MEMSET(s->l1_table, 0, l1_length);
    if (FN_bdev_pwrite(bs, s->l1_table_offset, (const uint8_t*)s->l1_table, l1_length) < 0)
        return -1;
    ret = FN_bdev_ptruncate(bs->file, s->l1_table_offset + l1_length);
    if (ret != ERR_OK)
        return ret;

    MEMSET(s->l2_cache, 0, s->l2_size * L2_CACHE_SIZE * sizeof(uint64_t));
    MEMSET(s->l2_cache_offsets, 0, L2_CACHE_SIZE * sizeof(uint64_t));
    MEMSET(s->l2_cache_counts, 0, L2_CACHE_SIZE * sizeof(uint32_t));

    return 0;
}

int bdev_open(BlockDriverState** pbs, const char* filename)
{
    BlockDriverState* bs;
    int ret = ERR_BAD;
    zfs_bdev_state_t* s;
    int shift;
    zfs_bdev_header_t header;
    USE_GLOBAL_BLOCK

    bs = SYS_ALLOCATOR(sizeof(BlockDriverState));
    MEMSET(bs, 0, sizeof(BlockDriverState));
    bs->file = NULL;
    bs->total_sectors = 0;
    bs->valid_key = 0;

    MEMCPY(bs->filename, filename, STRLEN(filename) + 1);

    bs->opaque = SYS_ALLOCATOR(sizeof(zfs_bdev_state_t));
    MEMSET(bs->opaque, 0, sizeof(zfs_bdev_state_t));

    if (FN_bdev_native_open(bs, filename) != ERR_OK) {
        bs->file = NULL;
        goto free_and_fail;
    }

    s = bs->opaque;
    FN_ZFS_CREATE_MUTEX(&s->mutex);
    FN_ZFS_CREATE_MUTEX(&bs->fileMutex);

    if (FN_bdev_pread(bs, 0, (uint8_t*)&header, sizeof(header)) != sizeof(header)) {
        goto free_and_fail;
    }

    if (header.magic != BD_MAGIC) {
        goto free_and_fail;
    }
    if (header.size <= 1 || header.cluster_bits < 9) {
        goto free_and_fail;
    }
    s->cluster_bits = header.cluster_bits;
    s->cluster_size = 1 << s->cluster_bits;
    s->cluster_sectors = 1 << (s->cluster_bits - 9);
    s->l2_bits = header.l2_bits;
    s->l2_size = 1 << s->l2_bits;
    bs->total_sectors = header.size / 512;
    s->cluster_offset_mask = (1 << (63 - s->cluster_bits)) - 1;

    /* read the level 1 table */
    shift = s->cluster_bits + s->l2_bits;
    s->l1_size = (header.size + (1 << shift) - 1) >> shift;

    s->l1_table_offset = header.l1_table_offset;
    s->l1_table = SYS_ALLOCATOR(s->l1_size * sizeof(uint64_t));
    if (!s->l1_table) {
        goto free_and_fail;
    }
    if (FN_bdev_pread(bs, s->l1_table_offset, (uint8_t*)s->l1_table, s->l1_size * sizeof(uint64_t)) != s->l1_size * sizeof(uint64_t)) {
        goto free_and_fail;
    }
    /* alloc L2 cache */
    s->l2_cache = SYS_ALLOCATOR(s->l2_size * L2_CACHE_SIZE * sizeof(uint64_t));
    if (!s->l2_cache) {
        goto free_and_fail;
    }
    s->cluster_cache = SYS_ALLOCATOR(s->cluster_size);
    if (!s->cluster_cache) {
        goto free_and_fail;
    }
    s->cluster_data = SYS_ALLOCATOR(s->cluster_size);
    if (!s->cluster_data) {
        goto free_and_fail;
    }

    *pbs = bs;
    return ERR_OK;

free_and_fail:
    FN_bdev_close(bs);

    return ret;
}

int bdev_set_key(BlockDriverState *bs, const uint8_t* key, uint32_t keySize)
{
    zfs_bdev_state_t *s = bs->opaque;
    uint8_t keybuf[32];
    USE_GLOBAL_BLOCK

    MEMSET(keybuf, 0, 32);
    if (keySize > 32)
        keySize = 32;

    MEMCPY(keybuf, key, keySize);

    if (AES_SETKEY_ENC(&s->aes_enc_key, keybuf, 256) != 0)
        return -1;
    if (AES_SETKEY_DEC(&s->aes_dec_key, keybuf, 256) != 0)
        return -1;

    bs->valid_key = 1;

    return 0;
}

void bdev_close(BlockDriverState* bs)
{
    zfs_bdev_state_t *s = bs->opaque;
    USE_GLOBAL_BLOCK

    if (s != NULL) {
        if (s->l1_table != NULL) {
            SYS_DEALLOCATOR(s->l1_table);
        }
        if (s->l2_cache != NULL) {
            SYS_DEALLOCATOR(s->l2_cache);
        }
        if (s->cluster_cache != NULL) {
            SYS_DEALLOCATOR(s->cluster_cache);
        }
        if (s->cluster_data != NULL) {
            SYS_DEALLOCATOR(s->cluster_data);
        }
        FN_ZFS_DESTROY_MUTEX(&s->mutex);
        SYS_DEALLOCATOR(s);
        bs->opaque = NULL;
    }

    if (bs->file != NULL) {
#if RING3
        CloseHandle(bs->file);
#else
        ZW_CLOSE(bs->file);
#endif // RING3
        FN_ZFS_DESTROY_MUTEX(&bs->fileMutex);
    }

    SYS_DEALLOCATOR(bs);
}

uint32_t bdev_getlength(HANDLE hFile)
{
#ifndef RING3
    NTSTATUS ntStatus;
    IO_STATUS_BLOCK ioStatus;
    FILE_STANDARD_INFORMATION fStdInfo;
#else
    uint32_t loSize, hiSize;
#endif // RING3
    
    USE_GLOBAL_BLOCK

#if RING3
    loSize = GetFileSize(hFile, (LPDWORD)&hiSize);
    return loSize /*+ ((int64_t)hiSize << 32)*/;
#else
    ntStatus = ZW_QUERY_INFORMATION_FILE(hFile, &ioStatus, &fStdInfo, sizeof(fStdInfo), FileStandardInformation);
    return fStdInfo.EndOfFile.LowPart;
#endif // RING3
}

void bdev_encrypt_sectors(uint32_t sector_num, uint8_t *out_buf, const uint8_t *in_buf, uint32_t nb_sectors, int enc, const paes_context_t pCtx)
{
    union {
        uint32_t ll[4];
        uint8_t b[16];
    } ivec;
    uint32_t i;
    USE_GLOBAL_BLOCK

    for (i = 0; i < nb_sectors; ++i) {
        ivec.ll[0] = sector_num;
        ivec.ll[1] = ivec.ll[2] = ivec.ll[3] = 0;
        AES_CRYPT_CBC(pCtx, enc, 512, ivec.b, in_buf, out_buf);
        ++sector_num;
        in_buf += 512;
        out_buf += 512;
    }
}

uint32_t bdev_get_cluster_offset(BlockDriverState *bs, uint32_t offset, int allocate, int n_start, int n_end)
{
    zfs_bdev_state_t *s = bs->opaque;
    int min_index, i, j, l1_index, l2_index;
    uint32_t l2_offset, *l2_table, cluster_offset, tmp;
    uint32_t min_count;
    int new_l2_table;
    USE_GLOBAL_BLOCK

    l1_index = offset >> (s->l2_bits + s->cluster_bits);
    l2_offset = s->l1_table[l1_index];
    new_l2_table = 0;
    if (!l2_offset) {
        if (!allocate)
            return 0;
        /* allocate a new l2 entry */
        l2_offset = FN_bdev_getlength(bs->file);
        /* round to cluster size */
        l2_offset = (l2_offset + s->cluster_size - 1) & ~(s->cluster_size - 1);
        /* update the L1 entry */
        s->l1_table[l1_index] = l2_offset;
        tmp = l2_offset;
        if (FN_bdev_pwrite(bs, s->l1_table_offset + l1_index * sizeof(tmp), (const uint8_t*)&tmp, sizeof(tmp)) < 0)
            return 0;
        new_l2_table = 1;
    }
    for (i = 0; i < L2_CACHE_SIZE; i++) {
        if (l2_offset == s->l2_cache_offsets[i]) {
            /* increment the hit count */
            if (++s->l2_cache_counts[i] == 0xffffffff) {
                for (j = 0; j < L2_CACHE_SIZE; j++) {
                    s->l2_cache_counts[j] >>= 1;
                }
            }
            l2_table = s->l2_cache + (i << s->l2_bits);
            goto found;
        }
    }
    /* not found: load a new entry in the least used one */
    min_index = 0;
    min_count = 0xffffffff;
    for (i = 0; i < L2_CACHE_SIZE; i++) {
        if (s->l2_cache_counts[i] < min_count) {
            min_count = s->l2_cache_counts[i];
            min_index = i;
        }
    }
    l2_table = s->l2_cache + (min_index << s->l2_bits);
    if (new_l2_table) {
        MEMSET(l2_table, 0, s->l2_size * sizeof(uint64_t));
        if (FN_bdev_pwrite(bs, l2_offset, (const uint8_t*)l2_table, s->l2_size * sizeof(uint64_t)) < 0)
            return 0;
    } else {
        if (FN_bdev_pread(bs, l2_offset, (uint8_t*)l2_table, s->l2_size * sizeof(uint64_t)) !=
            s->l2_size * sizeof(uint64_t))
            return 0;
    }
    s->l2_cache_offsets[min_index] = l2_offset;
    s->l2_cache_counts[min_index] = 1;
 found:
    l2_index = (offset >> s->cluster_bits) & (s->l2_size - 1);
    cluster_offset = l2_table[l2_index];
    if (!cluster_offset) {
        if (!allocate)
            return 0;
        cluster_offset = FN_bdev_getlength(bs->file);
        if (allocate == 1) {
            /* round to cluster size */
            cluster_offset = (cluster_offset + s->cluster_size - 1) & ~(s->cluster_size - 1);
            FN_bdev_ptruncate(bs->file, cluster_offset + s->cluster_size);
            /* if encrypted, we must initialize the cluster content which won't be written */
            if ((n_end - n_start) < s->cluster_sectors) {
                uint32_t start_sect;
                start_sect = (offset & ~(s->cluster_size - 1)) >> 9;
                MEMSET(s->cluster_data + 512, 0x00, 512);
                for (i = 0; i < s->cluster_sectors; i++) {
                    if (i < n_start || i >= n_end) {
                        FN_bdev_encrypt_sectors(start_sect + i, s->cluster_data, s->cluster_data + 512, 1, AES_ENCRYPT, &s->aes_enc_key);
                        if (FN_bdev_pwrite(bs, cluster_offset + i * 512, s->cluster_data, 512) != 512)
                            return 0;
                    }
                }
            }
        }
        /* update L2 table */
        tmp = cluster_offset;
        l2_table[l2_index] = tmp;
        if (FN_bdev_pwrite(bs, l2_offset + l2_index * sizeof(tmp), (const uint8_t*)&tmp, sizeof(tmp)) != sizeof(tmp))
            return 0;
    }
    return cluster_offset;
}

int bdev_read(uint8_t* buf, uint32_t sector_num, uint32_t nb_sectors, BlockDriverState* bs)
{
    zfs_bdev_state_t* s = bs->opaque;
    int index_in_cluster;
    int ret = ERR_OK;
    uint32_t n;
    uint32_t cluster_offset;
    void* orig_buf;
    USE_GLOBAL_BLOCK

    orig_buf = NULL;

    FN_ZFS_LOCK_MUTEX(&s->mutex);

    while (nb_sectors != 0) {
        cluster_offset = FN_bdev_get_cluster_offset(bs, sector_num << 9, 0, 0, 0);
        index_in_cluster = sector_num & (s->cluster_sectors - 1);
        n = s->cluster_sectors - index_in_cluster;
        if (n > nb_sectors) {
            n = nb_sectors;
        }

        if (!cluster_offset) {
            MEMSET(buf, 0, 512 * n);
        }
        else {
            if ((cluster_offset & 511) != 0) {
                goto fail;
            }
            FN_ZFS_UNLOCK_MUTEX(&s->mutex);
            ret = FN_bdev_pread(bs, ((cluster_offset >> 9) + index_in_cluster) * 512, buf, n * 512);
            FN_ZFS_LOCK_MUTEX(&s->mutex);
            if (ret < ERR_OK) {
                break;
            }
            FN_bdev_encrypt_sectors(sector_num, buf, buf, n, AES_DECRYPT, &s->aes_dec_key);
        }

        nb_sectors -= n;
        sector_num += n;
        buf += n * 512;
    }

done:
    FN_ZFS_UNLOCK_MUTEX(&s->mutex);

    return ret;

fail:
    ret = ERR_BAD;
    goto done;

}

int bdev_write(const uint8_t *buf, uint32_t sector_num, uint32_t nb_sectors, BlockDriverState* bs)
{
    zfs_bdev_state_t *s = bs->opaque;
    int index_in_cluster;
    uint32_t cluster_offset;
    const uint8_t *src_buf;
    int ret = ERR_OK, written;
    uint32_t n;
    uint8_t *cluster_data = NULL;
    void *orig_buf;
    USE_GLOBAL_BLOCK

    if (bs->wr_highest_sector < sector_num + nb_sectors - 1) {
        bs->wr_highest_sector = sector_num + nb_sectors - 1;
    }

    orig_buf = NULL;

    FN_ZFS_LOCK_MUTEX(&s->mutex);

    while (nb_sectors != 0) {
        index_in_cluster = sector_num & (s->cluster_sectors - 1);
        n = s->cluster_sectors - index_in_cluster;
        if (n > nb_sectors) {
            n = nb_sectors;
        }
        cluster_offset = FN_bdev_get_cluster_offset(bs, sector_num << 9, 1,
            index_in_cluster,
            index_in_cluster + n);
        if (!cluster_offset || (cluster_offset & 511) != 0) {
            ret = ERR_BAD;
            break;
        }

        if (!cluster_data) {
            cluster_data = SYS_ALLOCATOR(s->cluster_size);
            MEMSET(cluster_data, 0, s->cluster_size);
        }
        FN_bdev_encrypt_sectors(sector_num, cluster_data, buf, n, AES_ENCRYPT, /*&s->aes_encrypt_key*/&s->aes_enc_key);
        src_buf = cluster_data;

        FN_ZFS_UNLOCK_MUTEX(&s->mutex);
        written = FN_bdev_pwrite(bs, ((cluster_offset >> 9) + index_in_cluster) * 512, src_buf, n * 512);
        FN_ZFS_LOCK_MUTEX(&s->mutex);
        if (written < ERR_OK) {
            ret = ERR_BAD;
            break;
        }

        nb_sectors -= n;
        sector_num += n;
        buf += n * 512;
    }
    FN_ZFS_UNLOCK_MUTEX(&s->mutex);

    if (cluster_data != NULL)
        SYS_DEALLOCATOR(cluster_data);

    return ret;
}
