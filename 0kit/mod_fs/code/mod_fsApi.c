void fs_shutdown_routine()
{
    USE_GLOBAL_BLOCK

    pGlobalBlock->pFsBlock->fnzfs_unmount(pGlobalBlock->pFsBlock->pZfsIo);
    pGlobalBlock->pFsBlock->fnzfs_close_device(pGlobalBlock->pFsBlock->pZfsIo);
    pGlobalBlock->pFsBlock->fnzfs_destroy_io_manager(pGlobalBlock->pFsBlock->pZfsIo);

    pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(pGlobalBlock->pFsBlock, LOADER_TAG);
}
