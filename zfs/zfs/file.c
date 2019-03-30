// uint8_t zfs_getmodebits(char* mode)
// {
//     uint8_t modeBits = 0x00;
// 
//     while (*mode) {
//         switch(*mode) {
//             case 'r':    // Allow Read
//             case 'R':
//                 modeBits |= ZFS_MODE_READ;
//                 break;
//             case 'w':    // Allow Write
//             case 'W':
//                 modeBits |= ZFS_MODE_WRITE;
//                 modeBits |= ZFS_MODE_CREATE;    // Create if not exist.
//                 modeBits |= ZFS_MODE_TRUNCATE;
//                 break;
//             case 'a':    // Append new writes to the end of the file.
//             case 'A':
//                 modeBits |= ZFS_MODE_WRITE;
//                 modeBits |= ZFS_MODE_APPEND;
//                 modeBits |= ZFS_MODE_CREATE;    // Create if not exist.
//                 break;
//             case '+':    // Update the file, don't Append!
//                 modeBits |= ZFS_MODE_READ;    // RW Mode
//                 modeBits |= ZFS_MODE_WRITE;    // RW Mode
//                 break;
//             /*case 'D':    // Internal use only!
//                 ModeBits |= FF_MODE_DIR;
//                 break;*/
//             default:    // b|B flags not supported (Binary mode is native anyway).
//                 break;
//         }
//         ++mode;
//     }
// 
//     return modeBits;
// }

int zfs_open(pzfs_io_manager_t pIoman, pzfs_file_t* ppFile, const char* path, uint8_t mode, uint8_t special)
{
    zfs_file_t* pFile;
    zfs_file_t* pFileChain;
    zfs_dir_entry_t    dirEntry;
    uint32_t dirCluster, fileCluster;
    char filename[ZFS_MAX_FILENAME + 1];
    int    err;
    uint16_t i;
    USE_GLOBAL_BLOCK

    if (pIoman == NULL || ppFile == NULL) {
        return ZFS_ERR_NULL_POINTER | ZFS_OPEN;
    }

    *ppFile = NULL;

    pFile = SYS_ALLOCATOR(sizeof(zfs_file_t));
    if (pFile == NULL) {
        return ZFS_ERR_NOT_ENOUGH_MEMORY | ZFS_OPEN;
    }

    do {
        MEMSET(pFile, 0, sizeof(zfs_file_t));
        // Get the Mode Bits.
        pFile->mode = mode;

        i = (uint16_t)STRLEN(path);

        while (i != 0) {
            if (path[i] == '\\' || path[i] == '/') {
                break;
            }
            i--;
        }
        FN_STRCPY_S(filename, ZFS_MAX_FILENAME + 1, (path + i + 1));

        if (i == 0) {
            i = 1;
        }

        dirCluster = FN_zfs_find_dir(pIoman, path, i, special, &err);
        if (ZFS_isERR(err)) {
            break;
        }

        err = ZFS_ERR_FILE_INVALID_PATH | ZFS_OPEN;
        if (dirCluster > 0) {
            fileCluster = FN_zfs_find_entry_in_dir(pIoman, dirCluster, filename, 0x00, &dirEntry, &err);
            if (ZFS_isERR(err)) {
                break;
            }

            if (fileCluster == 0) {    // If 0 was returned, it might be because the file has no allocated cluster
                if (STRLEN(filename) == STRLEN(dirEntry.fileName)) {
                    if (dirEntry.filesize == 0 && FN_zfs_strmatch(filename, dirEntry.fileName, (uint16_t)STRLEN(filename)) == TRUE) {
                        // The file really was found!
                        fileCluster = 1;
                    } 
                }
            }

            if (fileCluster == 0) {
                if ((mode & ZFS_MODE_CREATE)) {
                    fileCluster = FN_zfs_create_file(pIoman, dirCluster, filename, &dirEntry, special, &err);
                    if (ZFS_isERR(err)) {
                        break;
                    }
                    dirEntry.currentItem += 1;
                }
            }
            
            if (fileCluster > 0) {
                // Проверяем на доступ.
                if (!(special & ZFS_SPECIAL_SYSTEM) && (dirEntry.special & ZFS_SPECIAL_SYSTEM)) {
                    err = ZFS_ERR_FILE_NOT_FOUND | ZFS_OPEN;
                    break;
                }

                if (dirEntry.attrib == ZFS_ATTR_DIR) {
                    if (!(mode & ZFS_MODE_DIR)) {
                        // Not the object, File Not Found!
                        err = ZFS_ERR_FILE_OBJECT_IS_A_DIR | ZFS_OPEN;
                        break;
                    }
                }
                
                //---------- Ensure Read-Only files don't get opened for Writing.
                if ((mode & ZFS_MODE_WRITE) || (mode & ZFS_MODE_APPEND)) {
                    if ((dirEntry.attrib & ZFS_ATTR_READONLY)) {
                        err = ZFS_ERR_FILE_IS_READ_ONLY | ZFS_OPEN;
                        break;
                    }
                }
                pFile->pIoman = pIoman;
                pFile->filePointer = 0;
                pFile->objectCluster = dirEntry.objectCluster;
                pFile->filesize = dirEntry.filesize;
                pFile->currentCluster = 0;
                pFile->addrCurrentCluster = pFile->objectCluster;
                //pFile->Mode                    = Mode;
                pFile->pNext = NULL;
                pFile->dirCluster = dirCluster;
                pFile->dirEntry = dirEntry.currentItem - 1;
                pFile->iChainLength = 0;
                pFile->iEndOfChain = 0;
                pFile->validFlags &= ~(ZFS_VALID_FLAG_DELETED); //FALSE;

                // File Permission Processing
                // Only "w" and "w+" mode strings can erase a file's contents.
                // Any other combinations will not cause an erase.
                if ((pFile->mode & ZFS_MODE_TRUNCATE)) {
                    pFile->filesize = 0;
                    pFile->filePointer = 0;
                }

                /*
                    Add pFile onto the end of our linked list of FF_FILE objects.
                */
                FN_ZFS_LOCK_MUTEX(&pIoman->mutex);
                if (pIoman->firstFile == NULL) {
                    pIoman->firstFile = pFile;
                }
                else {
                    pFileChain = (pzfs_file_t)pIoman->firstFile;
                    do {
                        if (pFileChain->objectCluster == pFile->objectCluster) {
                            // HT: Only fail if any of them has write access...
                            // Why not have 2 open read handles to a single file?
                            if ((pFileChain->mode | pFile->mode) & (ZFS_MODE_WRITE | ZFS_MODE_APPEND)) {
                                // File is already open! DON'T ALLOW IT!
                                FN_ZFS_UNLOCK_MUTEX(&pIoman->mutex);
                                SYS_DEALLOCATOR(pFile);
                                return ZFS_ERR_FILE_ALREADY_OPEN | ZFS_OPEN;
                            }
                        }
                        if (!pFileChain->pNext) {
                            pFileChain->pNext = pFile;
                            break;
                        }
                        pFileChain = (pzfs_file_t)pFileChain->pNext;
                    } while (pFileChain != NULL);
                }
                FN_ZFS_UNLOCK_MUTEX(&pIoman->mutex);
                *ppFile = pFile;
                return ERR_OK;
            }
            else {
                err = ZFS_ERR_FILE_NOT_FOUND | ZFS_OPEN;
                break;
            } 
        }
    } while (0);

    SYS_DEALLOCATOR(pFile);

    return err;
}

char zfs_is_dir_empty(pzfs_io_manager_t pIoman, const char* path, uint8_t special)
{
    zfs_dir_entry_t    MyDir;
    int    err;
    uint8_t    i = 0;
    USE_GLOBAL_BLOCK

    if (!pIoman) {
        return FALSE;
    }
    
    err = FN_zfs_findfirst(pIoman, &MyDir, path, special);
    while (err == 0) {
        i++;
        err = FN_zfs_findnext(pIoman, &MyDir, special);
        if (i > 2) {
            return FALSE;
        }
    }

    return TRUE;
}

int zfs_rmdir(pzfs_io_manager_t pIoman, const char* path, uint8_t special)
{
    zfs_file_t* pFile;
    int err = ERR_OK;
    uint8_t EntryBuffer[ZFS_ENTRY_SIZE];
    zfs_fetch_context_t FetchContext;
    USE_GLOBAL_BLOCK

    err = FN_zfs_open(pIoman, &pFile, path, ZFS_MODE_DIR, special);

    if (ZFS_isERR(err)) {
        return err;    // File in use or File not found!
    }

    pFile->validFlags |= ZFS_VALID_FLAG_DELETED;
    
    FN_zfs_lock_dir(pIoman);
    if (FN_zfs_is_dir_empty(pIoman, path, special)) {
        FN_zfs_lock(pIoman);
        err = FN_zfs_unlink_cluster_chain(pIoman, pFile->objectCluster);

        FN_zfs_unlock(pIoman);

        if (ZFS_isERR(err)) {
            FN_zfs_unlock_dir(pIoman);
            FN_zfs_close(pFile);
            return err;                
        }

        // Initialise the dirent Fetch Context object for faster removal of dirents.

        err = FN_zfs_init_entry_fetch(pIoman, pFile->dirCluster, &FetchContext);
        if (ZFS_isERR(err)) {
            FN_zfs_unlock_dir(pIoman);
            FN_zfs_close(pFile);
            return err;
        }
        
        // Edit the Directory Entry! (So it appears as deleted);
        err = FN_zfs_rm_lfns(pIoman, pFile->dirEntry, &FetchContext);
        if (ZFS_isERR(err)) {
            FN_zfs_cleanup_entry_fetch(pIoman, &FetchContext);
            FN_zfs_unlock_dir(pIoman);
            FN_zfs_close(pFile);
            return err;
        }
        err = FN_zfs_fetch_entry_with_context(pIoman, pFile->dirEntry, &FetchContext, EntryBuffer);
        if (ZFS_isERR(err)) {
            FN_zfs_cleanup_entry_fetch(pIoman, &FetchContext);
            FN_zfs_unlock_dir(pIoman);
            FN_zfs_close(pFile);
            return err;
        }
        EntryBuffer[0] = 0xE5;
        err = FN_zfs_push_entry_with_context(pIoman, pFile->dirEntry, &FetchContext, EntryBuffer);
        if (ZFS_isERR(err)) {
            FN_zfs_cleanup_entry_fetch(pIoman, &FetchContext);
            FN_zfs_unlock_dir(pIoman);
            FN_zfs_close(pFile);
            return err;
        }
    
        err = FN_zfs_increase_free_clusters(pIoman, pFile->iChainLength);
        if (ZFS_isERR(err)) {
            FN_zfs_cleanup_entry_fetch(pIoman, &FetchContext);
            FN_zfs_unlock_dir(pIoman);
            FN_zfs_close(pFile);
            return err;
        }

        FN_zfs_cleanup_entry_fetch(pIoman, &FetchContext);

        err = FN_zfs_flush_cache(pIoman);
        if (ZFS_isERR(err)) {
            FN_zfs_unlock_dir(pIoman);
            FN_zfs_close(pFile);
            return err;
        }
    } else {
        err = (ZFS_ERR_DIR_NOT_EMPTY | ZFS_RMDIR);
    }
    FN_zfs_unlock_dir(pIoman);
    err = FN_zfs_close(pFile); // Free the file pointer resources
    if (ZFS_isERR(err)) {
        return err;
    }

    // File is now lost!
    return err;
}

int zfs_unlink(pzfs_io_manager_t pIoman, const char* path, uint8_t special)
{
    pzfs_file_t pFile;
    int err = ERR_OK;
    uint8_t EntryBuffer[ZFS_ENTRY_SIZE];
    zfs_fetch_context_t FetchContext;
    USE_GLOBAL_BLOCK

    err = FN_zfs_open(pIoman, &pFile, path, ZFS_MODE_READ, special);

    if (ZFS_isERR(err)) {
        return err;    // File in use or File not found!
    }

    pFile->validFlags |= ZFS_VALID_FLAG_DELETED;//TRUE;

    if (pFile->objectCluster) {    // Ensure there is actually a cluster chain to delete!
        FN_zfs_lock(pIoman);    // Lock the ZFS so its thread-safe.
        err = FN_zfs_unlink_cluster_chain(pIoman, pFile->objectCluster);

        FN_zfs_unlock(pIoman);

        if (ZFS_isERR(err)) {
            FN_zfs_close(pFile);
            return err;
        }
    }

    // Edit the Directory Entry! (So it appears as deleted);
    FN_zfs_lock_dir(pIoman);
    err = FN_zfs_init_entry_fetch(pIoman, pFile->dirCluster, &FetchContext);
    if (ZFS_isERR(err)) {
        FN_zfs_unlock_dir(pIoman);
        FN_zfs_close(pFile);
        return err;
    }
    err = FN_zfs_rm_lfns(pIoman, pFile->dirEntry, &FetchContext);
    if (ZFS_isERR(err)) {
        FN_zfs_cleanup_entry_fetch(pIoman, &FetchContext);
        FN_zfs_unlock_dir(pIoman);
        FN_zfs_close(pFile);
        return err;
    }
    err = FN_zfs_fetch_entry_with_context(pIoman, pFile->dirEntry, &FetchContext, EntryBuffer);
    if (ZFS_isERR(err)) {
        FN_zfs_cleanup_entry_fetch(pIoman, &FetchContext);
        FN_zfs_unlock_dir(pIoman);
        FN_zfs_close(pFile);
        return err;
    }
    EntryBuffer[0] = 0xE5;
    
    err = FN_zfs_push_entry_with_context(pIoman, pFile->dirEntry, &FetchContext, EntryBuffer);
    if (ZFS_isERR(err)) {
        FN_zfs_cleanup_entry_fetch(pIoman, &FetchContext);
        FN_zfs_unlock_dir(pIoman);
        FN_zfs_close(pFile);
        return err;
    }

    FN_zfs_cleanup_entry_fetch(pIoman, &FetchContext);
    FN_zfs_unlock_dir(pIoman);

    err = FN_zfs_flush_cache(pIoman);
    if (ZFS_isERR(err)) {
        FN_zfs_close(pFile);
        return err;
    }
    
    err = FN_zfs_close(pFile); // Free the file pointer resources
    
    return err;
}

int zfs_move(pzfs_io_manager_t pIoman, const char* szSrcFile, const char* szDstFile, uint8_t special)
{
    int    err;
    zfs_file_t *pSrcFile, *pDestFile;
    zfs_dir_entry_t    fileEntry;
    uint8_t    entryBuffer[ZFS_ENTRY_SIZE];
    uint16_t i;
    uint32_t dirCluster;
    zfs_fetch_context_t    fetchContext;
    USE_GLOBAL_BLOCK

    if (pIoman == NULL) {
        return (ZFS_ERR_NULL_POINTER | ZFS_MOVE);
    }

    // Check destination file doesn't exist!
    err = FN_zfs_open(pIoman, &pDestFile, szDstFile, ZFS_MODE_READ, special);

    if (pDestFile != NULL || (ZFS_GETERROR(err) == ZFS_ERR_FILE_OBJECT_IS_A_DIR)) {
        if (pDestFile != NULL) {
            FN_zfs_close(pDestFile);
        }
        
        return (ZFS_ERR_FILE_DESTINATION_EXISTS | ZFS_MOVE);    // YES -- FAIL
    }

    err = FN_zfs_open(pIoman, &pSrcFile, szSrcFile, ZFS_MODE_READ, special);

    if (ZFS_GETERROR(err) == ZFS_ERR_FILE_OBJECT_IS_A_DIR) {
        // Open a directory for moving!
        err = FN_zfs_open(pIoman, &pSrcFile, szSrcFile, ZFS_MODE_DIR, special);
    }

    if (ZFS_isERR(err)) {
        return err;
    }

    // Create the new dirent.
    err = FN_zfs_init_entry_fetch(pIoman, pSrcFile->dirCluster, &fetchContext);
    if (ZFS_isERR(err)) {
        FN_zfs_close(pSrcFile);
        return err;
    }
    err = FN_zfs_fetch_entry_with_context(pIoman, pSrcFile->dirEntry, &fetchContext, entryBuffer);
    if (ZFS_isERR(err)) {
        FN_zfs_close(pSrcFile);
        FN_zfs_cleanup_entry_fetch(pIoman, &fetchContext);
        return err;
    }

    //FF_FetchEntry(pIoman, pSrcFile->DirCluster, pSrcFile->DirEntry, EntryBuffer);
    fileEntry.attrib = entryBuffer[ZFS_DIRENT_ATTRIB];
    fileEntry.special = entryBuffer[ZFS_DIRENT_SPECIAL];
    fileEntry.filesize = pSrcFile->filesize;
    fileEntry.objectCluster = pSrcFile->objectCluster;
    fileEntry.currentItem = 0;

    i = (uint16_t)STRLEN(szDstFile);

    while (i != 0) {
        if (szDstFile[i] == '\\' || szDstFile[i] == '/') {
            break;
        }
        --i;
    }

    FN_STRCPY_S(fileEntry.fileName, ZFS_MAX_FILENAME + 1, (szDstFile + i + 1));

    if (i == 0) {
        i = 1;
    }

    dirCluster = FN_zfs_find_dir(pIoman, szDstFile, i, special, &err);
    if (ZFS_isERR(err)) {
        FN_zfs_close(pSrcFile);
        FN_zfs_cleanup_entry_fetch(pIoman, &fetchContext);
        return err;
    }
    
    if (dirCluster) {
        // HT: Cleaup because FF_CreateDirent might want to write the same sector
        FN_zfs_cleanup_entry_fetch(pIoman, &fetchContext);
        // Destination Dir was found, we can now create the new entry.
        err = FN_zfs_create_dirent(pIoman, dirCluster, &fileEntry);
        if (ZFS_isERR(err)) {
            FN_zfs_close(pSrcFile);
            FN_zfs_cleanup_entry_fetch(pIoman, &fetchContext);
            return err;    // FAILED
        }

        // Edit the Directory Entry! (So it appears as deleted);
        FN_zfs_lock_dir(pIoman);
        err = FN_zfs_rm_lfns(pIoman, pSrcFile->dirEntry, &fetchContext);
        if (ZFS_isERR(err)) {
            FN_zfs_unlock_dir(pIoman);
            FN_zfs_close(pSrcFile);
            FN_zfs_cleanup_entry_fetch(pIoman, &fetchContext);
            return err;
        }
        err = FN_zfs_fetch_entry_with_context(pIoman, pSrcFile->dirEntry, &fetchContext, entryBuffer);
        if (ZFS_isERR(err)) {
            FN_zfs_unlock_dir(pIoman);
            FN_zfs_close(pSrcFile);
            FN_zfs_cleanup_entry_fetch(pIoman, &fetchContext);
            return err;
        }
        entryBuffer[0] = 0xE5;
        //FF_PushEntry(pIoman, pSrcFile->DirCluster, pSrcFile->DirEntry, EntryBuffer);
        err = FN_zfs_push_entry_with_context(pIoman, pSrcFile->dirEntry, &fetchContext, entryBuffer);
        if (ZFS_isERR(err)) {
            FN_zfs_unlock_dir(pIoman);
            FN_zfs_close(pSrcFile);
            FN_zfs_cleanup_entry_fetch(pIoman, &fetchContext);
            return err;
        }
        FN_zfs_cleanup_entry_fetch(pIoman, &fetchContext);
        FN_zfs_unlock_dir(pIoman);
        FN_zfs_close(pSrcFile);

        FN_zfs_flush_cache(pIoman);

        return ERR_OK;
    }

    return (ZFS_ERR_FILE_DIR_NOT_FOUND | ZFS_MOVE);
}

int zfs_iseof(pzfs_file_t pFile)
{
    if (pFile == NULL) {
        return FALSE;
    }
    if (pFile->filePointer >= pFile->filesize) {
        return TRUE;
    }
    else {
        return FALSE;
    }
}

int zfs_bytesleft(pzfs_file_t pFile)
{
    if (pFile == NULL) {
        return ZFS_ERR_NULL_POINTER | ZFS_BYTESLEFT;
    }
    if (!(pFile->mode & ZFS_MODE_READ)) {
        return ZFS_ERR_FILE_NOT_OPENED_IN_READ_MODE | ZFS_BYTESLEFT;
    }

    if (pFile->filePointer >= pFile->filesize) {
        return 0;
    }
    return pFile->filesize - pFile->filePointer;
}

uint32_t zfs_get_sequential_clusters(pzfs_io_manager_t pIoman, uint32_t StartCluster, uint32_t Limit, int*pError)
{
    uint32_t CurrentCluster;
    uint32_t NextCluster = StartCluster;
    uint32_t i = 0;
    USE_GLOBAL_BLOCK

    *pError = ERR_OK;

    do {
        CurrentCluster = NextCluster;
        NextCluster = FN_zfs_get_entry(pIoman, CurrentCluster, pError);
        if (*pError) {
            return 0;
        }
        if (NextCluster == (CurrentCluster + 1)) {
            i++;
        }
        else {
            break;
        }

        if (Limit) {
            if (i == Limit) {
                break;
            }
        }
    } while (NextCluster == (CurrentCluster + 1));

    return i;
}

int zfs_read_clusters(pzfs_file_t pFile, uint32_t Count, uint8_t *buffer)
{
    uint32_t ulSectors;
    uint32_t SequentialClusters = 0;
    uint32_t nItemLBA;
    int slRetVal;
    int    Error;
    USE_GLOBAL_BLOCK

    while (Count != 0) {
        if ((Count - 1) > 0) {
            SequentialClusters = FN_zfs_get_sequential_clusters(pFile->pIoman, pFile->addrCurrentCluster, (Count - 1), &Error);
            if (ZFS_isERR(Error)) {
                return Error;
            }
        }
        ulSectors = (SequentialClusters + 1) * ZFS_SECTORS_PER_CLUSTER;
        nItemLBA = FN_zfs_cluster_to_lba(pFile->pIoman, pFile->addrCurrentCluster);

        slRetVal = FN_zfs_read_block(pFile->pIoman, nItemLBA, ulSectors, buffer);
        if (slRetVal < 0) {
            return slRetVal;
        }
        
        Count -= (SequentialClusters + 1);
        pFile->addrCurrentCluster = FN_zfs_traverse(pFile->pIoman, pFile->addrCurrentCluster, (SequentialClusters + 1), &Error);
        if (ZFS_isERR(Error)) {
            return Error;
        }
        pFile->currentCluster += (SequentialClusters + 1);
        buffer += ulSectors * BDEV_BLOCK_SIZE;
        SequentialClusters = 0;
    }

    return ERR_OK;
}


int zfs_extend_file(pzfs_file_t pFile, uint32_t Size)
{
    zfs_io_manager_t* pIoman = pFile->pIoman;
    uint32_t nBytesPerCluster = BDEV_BLOCK_SIZE * ZFS_SECTORS_PER_CLUSTER;
    uint32_t nTotalClustersNeeded = (Size + nBytesPerCluster-1) / nBytesPerCluster;
    uint32_t nClusterToExtend; 
    uint32_t CurrentCluster, NextCluster;
    uint32_t i;
    zfs_dir_entry_t    OriginalEntry;
    int    Error;
    USE_GLOBAL_BLOCK

    if ((pFile->mode & ZFS_MODE_WRITE) != ZFS_MODE_WRITE) {
        return (ZFS_ERR_FILE_NOT_OPENED_IN_WRITE_MODE | ZFS_EXTENDFILE);
    }

    if (pFile->filesize == 0 && pFile->objectCluster == 0) {    // No Allocated clusters.
        // Create a Cluster chain!
        pFile->addrCurrentCluster = FN_zfs_create_cluster_chain(pFile->pIoman, &Error);

        if (ZFS_isERR(Error)) {
            return Error;
        }

        Error = FN_zfs_get_dir_entry(pIoman, pFile->dirEntry, pFile->dirCluster, &OriginalEntry);
        
        if (!Error) {
            OriginalEntry.objectCluster = pFile->addrCurrentCluster;
            Error = FN_zfs_put_dir_entry(pIoman, pFile->dirEntry, pFile->dirCluster, &OriginalEntry);
        }

        if (ZFS_isERR(Error)) {
            return Error;
        }

        pFile->objectCluster = pFile->addrCurrentCluster;
        pFile->iChainLength = 1;
        pFile->currentCluster = 0;
        pFile->iEndOfChain = pFile->addrCurrentCluster;
    }
    
    if (pFile->iChainLength == 0) {    // First extension requiring the chain length, 
        pFile->iChainLength = FN_zfs_get_chain_length(pIoman, pFile->objectCluster, &pFile->iEndOfChain, &Error);
        if (ZFS_isERR(Error)) {
            return Error;
        }
    }

    nClusterToExtend = (nTotalClustersNeeded - pFile->iChainLength);

    if (nTotalClustersNeeded > pFile->iChainLength) {

        NextCluster = pFile->addrCurrentCluster;
        FN_zfs_lock(pIoman);
        // HT This "<=" issue is now solved by asing for 1 extra byte
        // Thus not always asking for 1 extra cluster
        for (i = 0; i < nClusterToExtend; i++) {
            CurrentCluster = FN_zfs_find_end_of_chain(pIoman, NextCluster, &Error);
            if (ZFS_isERR(Error)) {
                FN_zfs_unlock(pIoman);
                FN_zfs_decrease_free_clusters(pIoman, i);
                return Error;
            }
            NextCluster = FN_zfs_find_free_cluster(pIoman, &Error);
            if (!Error && !NextCluster) {
                Error = ZFS_ERR_ZFS_NO_FREE_CLUSTERS | ZFS_EXTENDFILE;
            }
            if (ZFS_isERR(Error)) {
                FN_zfs_unlock(pIoman);
                FN_zfs_decrease_free_clusters(pIoman, i);
                return Error;
            }
            
            Error = FN_zfs_put_entry(pIoman, CurrentCluster, NextCluster);
            if (ZFS_isERR(Error)) {
                FN_zfs_unlock(pIoman);
                FN_zfs_decrease_free_clusters(pIoman, i);
                return Error;
            }
            Error = FN_zfs_put_entry(pIoman, NextCluster, 0xFFFFFFFF);
            if (ZFS_isERR(Error)) {
                FN_zfs_unlock(pIoman);
                FN_zfs_decrease_free_clusters(pIoman, i);
                return Error;
            }
        }
        
        pFile->iEndOfChain = FN_zfs_find_end_of_chain(pIoman, NextCluster, &Error);
        if (ZFS_isERR(Error)) {
            FN_zfs_unlock(pIoman);
            FN_zfs_decrease_free_clusters(pIoman, i);
            return Error;
        }
        FN_zfs_unlock(pIoman);
        pFile->iChainLength += i;
        Error = FN_zfs_decrease_free_clusters(pIoman, i);    // Keep Tab of Numbers for fast FreeSize()
        if (ZFS_isERR(Error)) {
            return Error;
        }
        Error = FN_zfs_flush_cache(pIoman);
        if (Error) {
            return Error;
        }
    }

    return ERR_OK;
}

int zfs_write_clusters(pzfs_file_t pFile, uint32_t Count, uint8_t *buffer)
{
    uint32_t ulSectors;
    uint32_t SequentialClusters = 0;
    uint32_t nItemLBA;
    int slRetVal;
    int    Error;
    USE_GLOBAL_BLOCK

    while (Count != 0) {
        if ((Count - 1) > 0) {
            SequentialClusters = FN_zfs_get_sequential_clusters(pFile->pIoman, pFile->addrCurrentCluster, (Count - 1), &Error);
            if (ZFS_isERR(Error)) {
                return Error;
            }
        }
        ulSectors = (SequentialClusters + 1) * ZFS_SECTORS_PER_CLUSTER;
        nItemLBA = FN_zfs_cluster_to_lba(pFile->pIoman, pFile->addrCurrentCluster);

        slRetVal = FN_zfs_write_block(pFile->pIoman, nItemLBA, ulSectors, buffer);

        if (slRetVal < 0) {
            return slRetVal;
        }
        
        Count -= (SequentialClusters + 1);
        pFile->addrCurrentCluster = FN_zfs_traverse(pFile->pIoman, pFile->addrCurrentCluster, (SequentialClusters + 1), &Error);
        if (ZFS_isERR(Error)) {
            return Error;
        }
        pFile->currentCluster += (SequentialClusters + 1);
        buffer += ulSectors * BDEV_BLOCK_SIZE;
        SequentialClusters = 0;
    }

    return 0;
}

int zfs_read(pzfs_file_t pFile, uint8_t* buffer, uint32_t size, uint32_t* pReaded)
{
    uint32_t nBytesToRead;
    zfs_io_manager_t* pIoman;
    zfs_buffer_t* pBuffer;
    uint32_t nRelBlockPos;
    uint32_t nItemLBA;
    int    ret = 0;
    uint16_t sSectors;
    uint32_t nRelClusterPos;
    uint32_t nBytesPerCluster;
    uint32_t nClusterDiff;
    uint32_t uRemain;
    int    err;
    USE_GLOBAL_BLOCK

    if (pFile == NULL || pReaded == NULL) {
        return (ZFS_ERR_NULL_POINTER | ZFS_READ);
    }
    
    *pReaded = 0;

    err = FN_zfs_checkvalid(pFile);
    if (err != ERR_OK) {
        return err;
    }

    if (!(pFile->mode & ZFS_MODE_READ)) {
        return (ZFS_ERR_FILE_NOT_OPENED_IN_READ_MODE | ZFS_READ);
    }

    pIoman = pFile->pIoman;

    if (pFile->filePointer >= pFile->filesize) {
        return ERR_OK;
    }

    if ((pFile->filePointer + size) > pFile->filesize) {
        size = pFile->filesize - pFile->filePointer;
    }
    
    nClusterDiff = FN_zfs_get_cluster_chain_number(pFile->filePointer, 1) - pFile->currentCluster;
    if (nClusterDiff) {
        if (pFile->currentCluster < FN_zfs_get_cluster_chain_number(pFile->filePointer, 1)) {
            pFile->addrCurrentCluster = FN_zfs_traverse(pIoman, pFile->addrCurrentCluster, nClusterDiff, &err);
            if (ZFS_isERR(err)) {
                return err;
            }
            pFile->currentCluster += nClusterDiff;
        }
    }

    nRelBlockPos = FN_zfs_get_minor_block_entry(pFile->filePointer, 1); // Get the position within a block.
    
    nItemLBA = FN_zfs_cluster_to_lba(pIoman, pFile->addrCurrentCluster);
    nItemLBA = nItemLBA + FN_zfs_get_major_block_number(pFile->filePointer, 1) + FN_zfs_get_minor_block_number(pFile->filePointer, 1);

    if ((nRelBlockPos + size) < BDEV_BLOCK_SIZE) {    // Bytes to read are within a block and less than a block size.
        pBuffer = FN_zfs_get_buffer(pIoman, nItemLBA, ZFS_MODE_READ);
        if (!pBuffer) {
            return (ZFS_ERR_DEVICE_DRIVER_FAILED | ZFS_READ);
        }
        MEMCPY(buffer, (pBuffer->pBuffer + nRelBlockPos), size);
        FN_zfs_release_buffer(pIoman, pBuffer);

        pFile->filePointer += size;
        *pReaded = size;
        return err;        // Return the number of bytes read.
    }
    else {
        //---------- Read (MEMCPY) to a Sector Boundary
        if (nRelBlockPos != 0) {    // Not on a sector boundary, at this point the LBA is known.
            nBytesToRead = BDEV_BLOCK_SIZE - nRelBlockPos;
            pBuffer = FN_zfs_get_buffer(pIoman, nItemLBA, ZFS_MODE_READ);
            if (!pBuffer) {
                return (ZFS_ERR_DEVICE_DRIVER_FAILED | ZFS_READ);
            }
            // Here we copy to the sector boudary.
            MEMCPY(buffer, (pBuffer->pBuffer + nRelBlockPos), nBytesToRead);
            FN_zfs_release_buffer(pIoman, pBuffer);

            size -= nBytesToRead;
            *pReaded += nBytesToRead;
            pFile->filePointer += nBytesToRead;
            buffer += nBytesToRead;
        }

        //---------- Read to a Cluster Boundary
        
        nRelClusterPos = FN_zfs_get_cluster_position(pFile->filePointer, 1);
        nBytesPerCluster = (ZFS_SECTORS_PER_CLUSTER * BDEV_BLOCK_SIZE);
        if (nRelClusterPos != 0 && size >= nBytesPerCluster) { // Need to get to cluster boundary
            nClusterDiff = FN_zfs_get_cluster_chain_number(pFile->filePointer, 1) - pFile->currentCluster;
            if (nClusterDiff) {
                if (pFile->currentCluster < FN_zfs_get_cluster_chain_number(pFile->filePointer, 1)) {
                    pFile->addrCurrentCluster = FN_zfs_traverse(pIoman, pFile->addrCurrentCluster, nClusterDiff, &err);
                    if (ZFS_isERR(err)) {
                        return err;    // Returning an error, ensuring we are signed (error flag).
                    }
                    pFile->currentCluster += nClusterDiff;
                }
            }
        
            nItemLBA = FN_zfs_cluster_to_lba(pIoman, pFile->addrCurrentCluster);
            nItemLBA = nItemLBA + FN_zfs_get_major_block_number(pFile->filePointer, 1) + FN_zfs_get_minor_block_number(pFile->filePointer, 1);

            sSectors = (uint16_t) (ZFS_SECTORS_PER_CLUSTER - (nRelClusterPos / BDEV_BLOCK_SIZE));
            
            ret = FN_zfs_read_block(pIoman, nItemLBA, (uint32_t) sSectors, buffer);
            if (ret < 0) {
                return ret;
            }
            
            nBytesToRead = sSectors * BDEV_BLOCK_SIZE;
            size -= nBytesToRead;
            buffer += nBytesToRead;
            *pReaded += nBytesToRead;
            pFile->filePointer += nBytesToRead;

        }

        //---------- Read Clusters
        if (size >= nBytesPerCluster) {
            //----- Thanks to Christopher Clark of DigiPen Institute of Technology in Redmond, US adding this traversal check.
            nClusterDiff = FN_zfs_get_cluster_chain_number(pFile->filePointer, 1) - pFile->currentCluster;
            if (nClusterDiff) {
                if (pFile->currentCluster < FN_zfs_get_cluster_chain_number(pFile->filePointer, 1)) {
                    pFile->addrCurrentCluster = FN_zfs_traverse(pIoman, pFile->addrCurrentCluster, nClusterDiff, &err);
                    if (ZFS_isERR(err)) {
                        return err;
                    }
                    pFile->currentCluster += nClusterDiff;
                }
            }
            //----- End of Contributor fix.

            ret = FN_zfs_read_clusters(pFile, (size / nBytesPerCluster), buffer);
            if (ret < 0) {
                return ret;
            }
            nBytesToRead = (nBytesPerCluster * (size / nBytesPerCluster));

            pFile->filePointer += nBytesToRead;

            size -= nBytesToRead;
            buffer += nBytesToRead;
            *pReaded += nBytesToRead;
        }

        //---------- Read Remaining Blocks
        while (size >= BDEV_BLOCK_SIZE) {
            sSectors = (uint16_t) (size / BDEV_BLOCK_SIZE);
            //FF_PARTITION *pPart                = pIoman->pPartition;
            //uOffset = 
            uRemain = ZFS_SECTORS_PER_CLUSTER - (/*uOffset*/ ((pFile->filePointer / BDEV_BLOCK_SIZE) % ZFS_SECTORS_PER_CLUSTER));
            if (sSectors > (uint16_t) uRemain) {
                sSectors = (uint16_t) uRemain;
            }
            
            nClusterDiff = FN_zfs_get_cluster_chain_number(pFile->filePointer, 1) - pFile->currentCluster;
            if (nClusterDiff) {
                if (pFile->currentCluster < FN_zfs_get_cluster_chain_number(pFile->filePointer, 1)) {
                    pFile->addrCurrentCluster = FN_zfs_traverse(pIoman, pFile->addrCurrentCluster, nClusterDiff, &err);
                    if (ZFS_isERR(err)) {
                        return err;
                    }
                    pFile->currentCluster += nClusterDiff;
                }
            }
            
            nItemLBA = FN_zfs_cluster_to_lba(pIoman, pFile->addrCurrentCluster);
            nItemLBA = nItemLBA + FN_zfs_get_major_block_number(pFile->filePointer, 1) + FN_zfs_get_minor_block_number(pFile->filePointer, 1);
            ret = FN_zfs_read_block(pIoman, nItemLBA, (uint32_t) sSectors, buffer);

            if (ret < 0) {
                return ret;
            }
            
            nBytesToRead = sSectors * BDEV_BLOCK_SIZE;
            pFile->filePointer += nBytesToRead;
            size -= nBytesToRead;
            buffer += nBytesToRead;
            *pReaded += nBytesToRead;
        }

        //---------- Read (MEMCPY) Remaining Bytes
        if (size > 0) {
            nClusterDiff = FN_zfs_get_cluster_chain_number(pFile->filePointer, 1) - pFile->currentCluster;
            if (nClusterDiff) {
                if (pFile->currentCluster < FN_zfs_get_cluster_chain_number(pFile->filePointer, 1)) {
                    pFile->addrCurrentCluster = FN_zfs_traverse(pIoman, pFile->addrCurrentCluster, nClusterDiff, &err);
                    if (ZFS_isERR(err)) {
                        return err;
                    }
                    pFile->currentCluster += nClusterDiff;
                }
            }
            
            nItemLBA = FN_zfs_cluster_to_lba(pIoman, pFile->addrCurrentCluster);
            nItemLBA = nItemLBA + FN_zfs_get_major_block_number(pFile->filePointer, 1) + FN_zfs_get_minor_block_number(pFile->filePointer, 1);
            pBuffer = FN_zfs_get_buffer(pIoman, nItemLBA, ZFS_MODE_READ);
            if (!pBuffer) {
                return (ZFS_ERR_DEVICE_DRIVER_FAILED | ZFS_READ);
            }
            MEMCPY(buffer, pBuffer->pBuffer, size);
            FN_zfs_release_buffer(pIoman, pBuffer);

            nBytesToRead = size;
            pFile->filePointer += nBytesToRead;
            size -= nBytesToRead;
            buffer += nBytesToRead;
            *pReaded += nBytesToRead;
        }
    }

    return err;
}

int zfs_getc(pzfs_file_t pFile)
{
    uint32_t fileLBA;
    zfs_buffer_t* pBuffer;
    uint8_t retChar;
    uint32_t relMinorBlockPos;
    uint32_t clusterNum;
    uint32_t nClusterDiff;
    int err;
    USE_GLOBAL_BLOCK    
    
    if (pFile == NULL) {
        return (ZFS_ERR_NULL_POINTER | ZFS_GETC);    // Ensure this is a signed error.
    }

    if (!(pFile->mode & ZFS_MODE_READ)) {
        return (ZFS_ERR_FILE_NOT_OPENED_IN_READ_MODE | ZFS_GETC);
    }

    if (pFile->filePointer >= pFile->filesize) {
        return -1; // EOF!
    }

    relMinorBlockPos = FN_zfs_get_minor_block_entry(pFile->filePointer, 1);
    clusterNum = FN_zfs_get_cluster_chain_number(pFile->filePointer, 1);

    nClusterDiff = FN_zfs_get_cluster_chain_number(pFile->filePointer, 1) - pFile->currentCluster;
    if (nClusterDiff) {
        if (pFile->currentCluster < FN_zfs_get_cluster_chain_number(pFile->filePointer, 1)) {
            pFile->addrCurrentCluster = FN_zfs_traverse(pFile->pIoman, pFile->addrCurrentCluster, nClusterDiff, &err);
            if (ZFS_isERR(err)) {
                return err;
            }
            pFile->currentCluster += nClusterDiff;
        }
    }

    fileLBA = FN_zfs_cluster_to_lba(pFile->pIoman, pFile->addrCurrentCluster)    + FN_zfs_get_major_block_number(pFile->filePointer, (uint16_t) 1);
    fileLBA = fileLBA + FN_zfs_get_minor_block_number(pFile->filePointer, 1);

    pBuffer = FN_zfs_get_buffer(pFile->pIoman, fileLBA, ZFS_MODE_READ);
    if (!pBuffer) {
        return (ZFS_ERR_DEVICE_DRIVER_FAILED | ZFS_GETC);
    }
    retChar = pBuffer->pBuffer[relMinorBlockPos];
    FN_zfs_release_buffer(pFile->pIoman, pBuffer);

    pFile->filePointer += 1;

    return (int)retChar;
}

int zfs_getline(pzfs_file_t pFile, char* szLine, uint32_t maxLen)
{
    int c = 0;
    uint32_t i;
    USE_GLOBAL_BLOCK

    if (pFile == NULL || szLine == NULL) {
        return (ZFS_ERR_NULL_POINTER | ZFS_GETLINE);
    }

    for (i = 0; i < (maxLen - 1) && (c = FN_zfs_getc(pFile)) >= 0 && c != '\n'; ++i) {
        if (c == '\r') {
            i--;
        }
        else {
            szLine[i] = (char)c;
        }
    }

    szLine[i] = '\0';    // Always do this before sending the err, we don't know what the user will do with this buffer if they don't see the error.
    if (ZFS_isERR(c)) {
        return c;        // Return 'c' as an error code.
    }

    return i;
}

int zfs_write(pzfs_file_t pFile, uint8_t* buffer, uint32_t size, uint32_t* pWritten)
{
    uint32_t nBytesToWrite;
    zfs_io_manager_t* pIoman;
    zfs_buffer_t* pBuffer;
    uint32_t nRelBlockPos;
    uint32_t nItemLBA;
    int    slRetVal = 0;
    uint16_t sSectors;
    uint32_t nRelClusterPos;
    uint32_t nBytesPerCluster, nClusterDiff, nClusters;
    uint32_t uRemain;
    int err;
    USE_GLOBAL_BLOCK

    if (pFile == NULL || pWritten == NULL) {
        return (ZFS_ERR_NULL_POINTER | ZFS_WRITE);
    }

    err = FN_zfs_checkvalid(pFile);
    if (err != ERR_OK) {
        return err;
    }

    if (!(pFile->mode & ZFS_MODE_WRITE)) {
        return (ZFS_ERR_FILE_NOT_OPENED_IN_WRITE_MODE | ZFS_WRITE);
    }

    // Make sure a write is after the append point.
    if ((pFile->mode & ZFS_MODE_APPEND)) {
        if (pFile->filePointer < pFile->filesize) {
            FN_zfs_seek(pFile, 0, ZFS_SEEK_END);
        }
    }

    pIoman = pFile->pIoman;

    nBytesPerCluster = (ZFS_SECTORS_PER_CLUSTER * BDEV_BLOCK_SIZE);

    // Extend File for atleast size!
    // Handle file-space allocation

    // HT: + 1 byte because the code assumes there is always a next cluster
    err = FN_zfs_extend_file(pFile, pFile->filePointer + size + 1);

    if (ZFS_isERR(err)) {
        return err;    
    }

    nRelBlockPos = FN_zfs_get_minor_block_entry(pFile->filePointer, 1); // Get the position within a block.
    
    nClusterDiff = FN_zfs_get_cluster_chain_number(pFile->filePointer, 1) - pFile->currentCluster;
    if (nClusterDiff) {
        if (pFile->currentCluster != FN_zfs_get_cluster_chain_number(pFile->filePointer, 1)) {
            pFile->addrCurrentCluster = FN_zfs_traverse(pIoman, pFile->addrCurrentCluster, nClusterDiff, &err);
            if (ZFS_isERR(err)) {
                return err;
            }
            pFile->currentCluster += nClusterDiff;
        }
    }
    
    nItemLBA = FN_zfs_cluster_to_lba(pIoman, pFile->addrCurrentCluster);
    nItemLBA = nItemLBA + FN_zfs_get_major_block_number(pFile->filePointer, 1) + FN_zfs_get_minor_block_number(pFile->filePointer, 1);

    if ((nRelBlockPos + size) < BDEV_BLOCK_SIZE) {    // Bytes to write are within a block and less than a block size.
        if (!nRelBlockPos && pFile->filePointer >= pFile->filesize) {
            pBuffer = FN_zfs_get_buffer(pIoman, nItemLBA, ZFS_MODE_WR_ONLY);
        }
        else {
            pBuffer = FN_zfs_get_buffer(pIoman, nItemLBA, ZFS_MODE_WRITE);
        }
        if (!pBuffer) {
            return (ZFS_ERR_DEVICE_DRIVER_FAILED | ZFS_WRITE);
        }
        MEMCPY((pBuffer->pBuffer + nRelBlockPos), buffer, size);
        FN_zfs_release_buffer(pIoman, pBuffer);

        pFile->filePointer += size;
        *pWritten = size;
        //return size;        // Return the number of bytes read.

    }
    else {
        //---------- Write (MEMCPY) to a Sector Boundary
        if (nRelBlockPos != 0) {    // Not on a sector boundary, at this point the LBA is known.
            nBytesToWrite = BDEV_BLOCK_SIZE - nRelBlockPos;
            pBuffer = FN_zfs_get_buffer(pIoman, nItemLBA, ZFS_MODE_WRITE);
            if (!pBuffer) {
                return (ZFS_ERR_DEVICE_DRIVER_FAILED | ZFS_WRITE);
            }
            // Here we copy to the sector boudary.
            MEMCPY((pBuffer->pBuffer + nRelBlockPos), buffer, nBytesToWrite);
            FN_zfs_release_buffer(pIoman, pBuffer);

            size -= nBytesToWrite;
            *pWritten += nBytesToWrite;
            pFile->filePointer += nBytesToWrite;
            buffer += nBytesToWrite;
        }

        //---------- Write to a Cluster Boundary
        
        nRelClusterPos = FN_zfs_get_cluster_position(pFile->filePointer, 1);
        if (nRelClusterPos != 0 && size >= nBytesPerCluster) { // Need to get to cluster boundary
            
            nClusterDiff = FN_zfs_get_cluster_chain_number(pFile->filePointer, 1) - pFile->currentCluster;
            if (nClusterDiff) {
                if (pFile->currentCluster < FN_zfs_get_cluster_chain_number(pFile->filePointer, 1)) {
                    pFile->addrCurrentCluster = FN_zfs_traverse(pIoman, pFile->addrCurrentCluster, nClusterDiff, &err);
                    if (ZFS_isERR(err)) {
                        return err;
                    }
                    pFile->currentCluster += nClusterDiff;
                }
            }
        
            nItemLBA = FN_zfs_cluster_to_lba(pIoman, pFile->addrCurrentCluster);
            nItemLBA = nItemLBA + FN_zfs_get_major_block_number(pFile->filePointer, 1) + FN_zfs_get_minor_block_number(pFile->filePointer, 1);

            sSectors = (uint16_t)(ZFS_SECTORS_PER_CLUSTER - (nRelClusterPos / BDEV_BLOCK_SIZE));

            slRetVal = FN_zfs_write_block(pFile->pIoman, nItemLBA, sSectors, buffer);
            if (slRetVal < 0) {
                return slRetVal;
            }
            
            nBytesToWrite = sSectors * BDEV_BLOCK_SIZE;
            size -= nBytesToWrite;
            buffer += nBytesToWrite;
            *pWritten += nBytesToWrite;
            pFile->filePointer += nBytesToWrite;

        }

        //---------- Write Clusters
        if (size >= nBytesPerCluster) {
            //----- Thanks to Christopher Clark of DigiPen Institute of Technology in Redmond, US adding this traversal check.
            nClusterDiff = FN_zfs_get_cluster_chain_number(pFile->filePointer, 1) - pFile->currentCluster;
            if (nClusterDiff) {
                if (pFile->currentCluster < FN_zfs_get_cluster_chain_number(pFile->filePointer, 1)) {
                    pFile->addrCurrentCluster = FN_zfs_traverse(pIoman, pFile->addrCurrentCluster, nClusterDiff, &err);
                    if (ZFS_isERR(err)) {
                        return err;
                    }
                    pFile->currentCluster += nClusterDiff;
                }
            }
            //----- End of Contributor fix.

            nClusters = (size / nBytesPerCluster);
            
            slRetVal = FN_zfs_write_clusters(pFile, nClusters, buffer);
            if (slRetVal < 0) {
                return slRetVal;
            }
            
            nBytesToWrite = (nBytesPerCluster *  nClusters);
            
            pFile->filePointer += nBytesToWrite;

            size -= nBytesToWrite;
            buffer += nBytesToWrite;
            *pWritten += nBytesToWrite;
        }

        //---------- Write Remaining Blocks
        while (size >= BDEV_BLOCK_SIZE) {
            sSectors = (uint16_t) (size / BDEV_BLOCK_SIZE);
            {
                //FF_PARTITION *pPart                = pIoman->pPartition;
                //uOffset = 
                uRemain = ZFS_SECTORS_PER_CLUSTER - (/*uOffset*/ ((pFile->filePointer / BDEV_BLOCK_SIZE) % ZFS_SECTORS_PER_CLUSTER));
                if (sSectors > (uint16_t) uRemain) {
                    sSectors = (uint16_t) uRemain;
                }
            }
            
            nClusterDiff = FN_zfs_get_cluster_chain_number(pFile->filePointer, 1) - pFile->currentCluster;
            if (nClusterDiff) {
                if (pFile->currentCluster < FN_zfs_get_cluster_chain_number(pFile->filePointer, 1)) {
                    pFile->addrCurrentCluster = FN_zfs_traverse(pIoman, pFile->addrCurrentCluster, nClusterDiff, &err);
                    if (ZFS_isERR(err)) {
                        return err;
                    }
                    pFile->currentCluster += nClusterDiff;
                }
            }            
            
            nItemLBA = FN_zfs_cluster_to_lba(pIoman, pFile->addrCurrentCluster);
            nItemLBA = nItemLBA + FN_zfs_get_major_block_number(pFile->filePointer, 1) + FN_zfs_get_minor_block_number(pFile->filePointer, 1);
            
            slRetVal = FN_zfs_write_block(pFile->pIoman, nItemLBA, sSectors, buffer);
            if (slRetVal < 0) {
                return slRetVal;
            }
            
            nBytesToWrite = sSectors * BDEV_BLOCK_SIZE;
            pFile->filePointer += nBytesToWrite;
            size -= nBytesToWrite;
            buffer += nBytesToWrite;
            *pWritten += nBytesToWrite;
        }

        //---------- Write (MEMCPY) Remaining Bytes
        if (size > 0) {            
            nClusterDiff = FN_zfs_get_cluster_chain_number(pFile->filePointer, 1) - pFile->currentCluster;
            if (nClusterDiff) {
                if (pFile->currentCluster < FN_zfs_get_cluster_chain_number(pFile->filePointer, 1)) {
                    pFile->addrCurrentCluster = FN_zfs_traverse(pIoman, pFile->addrCurrentCluster, nClusterDiff, &err);
                    if (ZFS_isERR(err)) {
                        return err;
                    }
                    pFile->currentCluster += nClusterDiff;
                }
            }
            
            nItemLBA = FN_zfs_cluster_to_lba(pIoman, pFile->addrCurrentCluster);
            nItemLBA = nItemLBA + FN_zfs_get_major_block_number(pFile->filePointer, 1) + FN_zfs_get_minor_block_number(pFile->filePointer, 1);
            pBuffer = FN_zfs_get_buffer(pIoman, nItemLBA, ZFS_MODE_WRITE);
            if (!pBuffer) {
                return (ZFS_ERR_DEVICE_DRIVER_FAILED | ZFS_WRITE);
            }
            MEMCPY(pBuffer->pBuffer, buffer, size);
            FN_zfs_release_buffer(pIoman, pBuffer);

            nBytesToWrite = size;
            pFile->filePointer += nBytesToWrite;
            size -= nBytesToWrite;
            buffer += nBytesToWrite;
            *pWritten += nBytesToWrite;

        }
    }

    if (pFile->filePointer > pFile->filesize) {
        pFile->filesize = pFile->filePointer;
    }

    return err;
}

int zfs_putc(pzfs_file_t pFile, uint8_t pa_cValue)
{
    zfs_buffer_t* pBuffer;
    uint32_t iItemLBA;
    uint32_t iRelPos;
    uint32_t nClusterDiff;
    int    err;
    USE_GLOBAL_BLOCK
    
    if (pFile == NULL) {    // Ensure we don't have a Null file pointer on a Public interface.
        return (ZFS_ERR_NULL_POINTER | ZFS_PUTC);
    }

    if (!(pFile->mode & ZFS_MODE_WRITE)) {
        return (ZFS_ERR_FILE_NOT_OPENED_IN_WRITE_MODE | ZFS_PUTC);
    }

    // Make sure a write is after the append point.
    if ((pFile->mode & ZFS_MODE_APPEND)) {
        if (pFile->filePointer < pFile->filesize) {
            err = FN_zfs_seek(pFile, 0, ZFS_SEEK_END);
            if (ZFS_isERR(err)) {
                return err;
            }
        }
    }

    iRelPos = FN_zfs_get_minor_block_entry(pFile->filePointer, 1);
    
    // Handle File Space Allocation.
    err = FN_zfs_extend_file(pFile, pFile->filePointer + 1);
    if (ZFS_isERR(err)) {
        return err;
    }
    
    nClusterDiff = FN_zfs_get_cluster_chain_number(pFile->filePointer, 1) - pFile->currentCluster;
    if (nClusterDiff) {
        if (pFile->currentCluster < FN_zfs_get_cluster_chain_number(pFile->filePointer, 1)) {
            pFile->addrCurrentCluster = FN_zfs_traverse(pFile->pIoman, pFile->addrCurrentCluster, nClusterDiff, &err);
            if (ZFS_isERR(err)) {
                return err;
            }
            pFile->currentCluster += nClusterDiff;
        }
    }

    iItemLBA = FN_zfs_cluster_to_lba(pFile->pIoman, pFile->addrCurrentCluster) + FN_zfs_get_major_block_number(pFile->filePointer, 1);
    iItemLBA = iItemLBA + FN_zfs_get_minor_block_number(pFile->filePointer, 1);
    
    pBuffer = FN_zfs_get_buffer(pFile->pIoman, iItemLBA, ZFS_MODE_WRITE);
    if (!pBuffer) {
        return (ZFS_ERR_DEVICE_DRIVER_FAILED | ZFS_PUTC);
    }
    pBuffer->pBuffer[iRelPos] = pa_cValue;
    FN_zfs_release_buffer(pFile->pIoman, pBuffer);

    pFile->filePointer += 1;
    if (pFile->filesize < (pFile->filePointer)) {
        pFile->filesize += 1;
    }
    return ERR_OK;
}

uint32_t zfs_tell(pzfs_file_t pFile)
{
    return pFile ? pFile->filePointer : 0;
}

int zfs_seek(pzfs_file_t pFile, int offset, char origin)
{    
    int    err;
    USE_GLOBAL_BLOCK

    if (pFile == NULL) {
        return ZFS_ERR_NULL_POINTER | ZFS_SEEK;
    }

    err = FN_zfs_flush_cache(pFile->pIoman);
    if (ZFS_isERR(err)) {
        return err;
    }

    if (origin == ZFS_SEEK_SET) {
        if (offset >= 0 && (uint32_t)offset <= pFile->filesize) {
            pFile->filePointer = offset;
            pFile->currentCluster = FN_zfs_get_cluster_chain_number(pFile->filePointer, 1);
            pFile->addrCurrentCluster = FN_zfs_traverse(pFile->pIoman, pFile->objectCluster, pFile->currentCluster, &err);
            if (ZFS_isERR(err)) {
                return err;
            }
        }
        else {
            return ZFS_SEEK | ZFS_ERR_FILE_INCORRECT_OFFSET;
        }
    }
    else if (origin == ZFS_SEEK_CUR) {
        if ((offset + pFile->filePointer) <= pFile->filesize && (offset + (int) pFile->filePointer) >= 0) {
            pFile->filePointer = offset + pFile->filePointer;
            pFile->currentCluster = FN_zfs_get_cluster_chain_number(pFile->filePointer, 1);
            pFile->addrCurrentCluster = FN_zfs_traverse(pFile->pIoman, pFile->objectCluster, pFile->currentCluster, &err);
            if (ZFS_isERR(err)) {
                return err;
            }
        }
        else {
            return ZFS_SEEK | ZFS_ERR_FILE_INCORRECT_OFFSET;
        }
    }
    else if (origin == ZFS_SEEK_END) {
        if ((offset + (int)pFile->filesize) >= 0 && (offset + (int)pFile->filesize) <= (int)pFile->filesize) {
            pFile->filePointer = offset + pFile->filesize;
            pFile->currentCluster = FN_zfs_get_cluster_chain_number(pFile->filePointer, 1);
            pFile->addrCurrentCluster = FN_zfs_traverse(pFile->pIoman, pFile->objectCluster, pFile->currentCluster, &err);
            if (ZFS_isERR(err)) {
                return err;
            }
        }
        else {
            return ZFS_SEEK | ZFS_ERR_FILE_INCORRECT_OFFSET;
        }
    }
    return ERR_OK;
}

int zfs_checkvalid(pzfs_file_t pFile)
{
    pzfs_file_t pFileChain;

    if (pFile == NULL || pFile->pIoman == NULL) {
        return (ZFS_ERR_NULL_POINTER | ZFS_CHECKVALID);
    }

    pFileChain = (pzfs_file_t)pFile->pIoman->firstFile;
    while (pFileChain) {
        if (pFileChain == pFile) {
            return ERR_OK;
        }
        pFileChain = pFileChain->pNext;
    }
    return (ZFS_ERR_FILE_BAD_HANDLE | ZFS_CHECKVALID);
}

int zfs_set_end_of_file(pzfs_file_t pFile)
{
    int err;
    uint32_t truncLen, num = 0, neededClusters;
    uint32_t zfsEntry, currentCluster;
    USE_GLOBAL_BLOCK

    err = FN_zfs_checkvalid(pFile);
    if (err) {
        return err;
    }

    if (pFile->validFlags & ZFS_VALID_FLAG_DELETED) { //if (pFile->FileDeleted)
        return ZFS_ERR_FILE_NOT_FOUND | ZFS_SETENDOFFILE;
    }
    if (!(pFile->mode & ZFS_MODE_WRITE)) {
        return ZFS_ERR_FILE_NOT_OPENED_IN_WRITE_MODE | ZFS_SETENDOFFILE;
    }

    truncLen = pFile->filesize - pFile->filePointer;
    if (truncLen == 0) {
        return ERR_OK;
    }

    neededClusters = truncLen / (ZFS_SECTORS_PER_CLUSTER * BDRV_SECTOR_SIZE);
    if ((neededClusters % (ZFS_SECTORS_PER_CLUSTER * BDRV_SECTOR_SIZE)) != 0 || (neededClusters == 0)) {
        ++neededClusters;
    }
    zfsEntry = currentCluster = pFile->objectCluster;
    while (!FN_zfs_is_end_of_chain(zfsEntry)) {
        zfsEntry = FN_zfs_get_entry(pFile->pIoman, currentCluster, &err);
        if (err) {
            return err;
        }

        if (FN_zfs_is_end_of_chain(zfsEntry)) {
            break;
        }
        else if (++num >= neededClusters) {
            break;
        }
        currentCluster = zfsEntry;
    }    

    if (num == neededClusters) {
        err = FN_zfs_unlink_cluster_chain(pFile->pIoman, zfsEntry);
        if (err == ERR_OK) {
            err = FN_zfs_put_entry(pFile->pIoman, currentCluster, 0xFFFFFFFF);
        }
    }

    if (err == ERR_OK) {
        pFile->filesize = pFile->filePointer;
        err = FN_zfs_flush_cache(pFile->pIoman); // Ensure all modfied blocks are flushed to disk!
    }

    return err;
}

int zfs_set_time(pzfs_io_manager_t pIoman, const char* path, uint32_t unixTime, uint32_t aWhat, uint8_t special)
{
    zfs_dir_entry_t    origEntry;
    int    err;
    uint32_t dirCluster;
    uint32_t fileCluster;
    uint32_t i;
    char filename[ZFS_MAX_FILENAME + 1];
    USE_GLOBAL_BLOCK

    i = (uint16_t)STRLEN(path);

    while (i != 0) {
        if (path[i] == '\\' || path[i] == '/') {
            break;
        }
        i--;
    }
    FN_STRCPY_S(filename, ZFS_MAX_FILENAME + 1, (path + i + 1));

    if (i == 0) {
        i = 1;
    }
    
    dirCluster = FN_zfs_find_dir(pIoman, path, (uint16_t) i, special, &err);
    if (err)
        return err;

    if (!dirCluster) {
        return ZFS_ERR_FILE_NOT_FOUND | ZFS_SETTIME;
    }
    fileCluster = FN_zfs_find_entry_in_dir(pIoman, dirCluster, filename, 0, &origEntry, &err);
    if (err)
        return err;

    if (fileCluster == 0) {
        return ZFS_ERR_FILE_NOT_FOUND | ZFS_SETTIME;
    }

    if (aWhat & ETimeCreate) {
        origEntry.createTime = unixTime;
    }
    if (aWhat & ETimeMod) {
        origEntry.modifiedTime = unixTime;
    }
    if (aWhat & ETimeAccess) {
        origEntry.accessedTime = unixTime;
    }

    err = FN_zfs_put_dir_entry(pIoman, origEntry.currentItem-1, dirCluster, &origEntry);

    if (err == ERR_OK) {
        err = FN_zfs_flush_cache(pIoman); // Ensure all modfied blocks are flushed to disk!
    }
    return err;
}

int zfs_get_time(pzfs_io_manager_t pIoman, const char* path, uint32_t aWhat, uint32_t* pUnixTime, uint8_t special)
{
    zfs_dir_entry_t    origEntry;
    int    err;
    uint32_t dirCluster;
    uint32_t fileCluster;
    uint32_t i;
    char filename[ZFS_MAX_FILENAME + 1];
    USE_GLOBAL_BLOCK

    *pUnixTime = 0;    

    i = (uint16_t)STRLEN(path);

    while (i != 0) {
        if (path[i] == '\\' || path[i] == '/') {
            break;
        }
        i--;
    }
    FN_STRCPY_S(filename, ZFS_MAX_FILENAME + 1, (path + i + 1));

    if (i == 0) {
        i = 1;
    }

    dirCluster = FN_zfs_find_dir(pIoman, path, (uint16_t) i, special, &err);
    if (err != ERR_OK) {
        return err;
    }

    if (!dirCluster) {
        return ZFS_ERR_FILE_NOT_FOUND | ZFS_SETTIME;
    }
    fileCluster = FN_zfs_find_entry_in_dir(pIoman, dirCluster, filename, 0, &origEntry, &err);
    if (err != ERR_OK) {
        return err;
    }

    if (fileCluster == 0) {
        return ZFS_ERR_FILE_NOT_FOUND | ZFS_SETTIME;
    }

    if (aWhat & ETimeCreate) {
        *pUnixTime = origEntry.createTime;
    }
    if (aWhat & ETimeMod) {
        *pUnixTime = origEntry.modifiedTime;
    }
    if (aWhat & ETimeAccess) {
        *pUnixTime = origEntry.accessedTime;
    }

    return err;
}

int zfs_close(pzfs_file_t pFile)
{
    zfs_file_t* pFileChain;
    zfs_dir_entry_t    originalEntry;
    int    err;
    USE_GLOBAL_BLOCK

    if (pFile == NULL) {
        return (ZFS_ERR_NULL_POINTER | ZFS_CLOSE);
    }

    /*
     * HT thinks:
     * It is important to check that:
     * user doesn't supply invalid handle
     * or a handle invalid because of "media removed"
     */
    err = FN_zfs_checkvalid(pFile);
    if (err != ERR_OK) {
        return err; // ZFS_ERR_FILE_BAD_HANDLE or ZFS_ERR_NULL_POINTER
    }

    /*
     * So here we have a normal valid file handle
     */

    
    // UpDate Dirent if File-size has changed?
    if (!(pFile->validFlags & ZFS_VALID_FLAG_DELETED) && (pFile->mode & (ZFS_MODE_WRITE |ZFS_MODE_APPEND))) {
        // Update the Dirent!
        err = FN_zfs_get_dir_entry(pFile->pIoman, pFile->dirEntry, pFile->dirCluster, &originalEntry);
        // Error might be non-zero, but don't forget to remove handle from list
        // and to free the pFile pointer
        if (!err && pFile->filesize != originalEntry.filesize) {
            originalEntry.filesize = pFile->filesize;
            err = FN_zfs_put_dir_entry(pFile->pIoman, pFile->dirEntry, pFile->dirCluster, &originalEntry);
        }
    }
    if (!ZFS_GETERROR (err)) {
        err = FN_zfs_flush_cache(pFile->pIoman);        // Ensure all modfied blocks are flushed to disk!
    }

    // Handle Linked list!
    FN_ZFS_LOCK_MUTEX(&pFile->pIoman->mutex);
    pFileChain = (pzfs_file_t ) pFile->pIoman->firstFile;
    if (pFileChain == pFile) {
        pFile->pIoman->firstFile = pFile->pNext;
    }
    else {
        while (pFileChain) {
            if (pFileChain->pNext == pFile) {
                pFileChain->pNext = pFile->pNext;
                break;
            }
            pFileChain = pFileChain->pNext;    // Forgot this one
        }
    }
    FN_ZFS_UNLOCK_MUTEX(&pFile->pIoman->mutex);

    // If file written, flush to disk
    SYS_DEALLOCATOR(pFile);
    // Simply free the pointer!
    return err;
}
