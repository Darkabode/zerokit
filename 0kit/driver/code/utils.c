#if _AMD64_

extern void UtilsAtomicAdd16(UINT32* atomic, UINT32 value);
extern void UtilsAtomicSub16(UINT32* atomic, UINT32 value);

#else

void UtilsAtomicAdd16(UINT16* atomic, UINT16 value)
{
    __asm {
        mov ecx, atomic
        mov dx, value
        lock add word ptr [ecx], dx
    }
}

void UtilsAtomicSub16(UINT32* atomic, UINT16 value)
{
    __asm {
        mov ecx, atomic
        mov dx, value
        lock sub word ptr [ecx], dx
    }
}

// void UtilsAtomicAdd32(UINT32* atomic, UINT32 value)
// {
//     __asm {
//         mov ecx, atomic
//             mov edx, value
//             lock add dword ptr [ecx], edx
//     }
// }
// 
// void UtilsAtomicSub(UINT32* atomic, UINT32 value)
// {
//     __asm {
//         mov ecx, atomic
//         mov edx, value
//         lock sub dword ptr [ecx], edx
//     }
// }

#endif

UINT16 UtilsAtomicGet(const UINT16* atomic)
{
    return (*(volatile UINT16*)atomic);
} 



// UINT64 UtilsAnsiToUInt64(char* str)
// {
//     UINT64 val = 0;
//     UINT64 multiplier = 1;
//     char* head = str;
//     char* itr;
//     int isSigned = (str[0] == '-');
// 
//     if (isSigned)
//         head++; 
// 
//     for (itr = head; *itr >= '0' && *itr <= '9'; ++itr);
// 
//     for ( ; --itr >= head; ) {
//         val += multiplier * (*itr - '0');
//         multiplier *= 10;
//     }
// 
//     return val;
// }

UINT32 UtilsGetSystem()
{
    DEFINE_GLOBAL_BLOCK(pGlobalData);
    IMPL_GLOBAL_BLOCK(pGlobalData);
    return pGlobalData->sysSpec.osInfo & 0xFF;
}

// KIRQL UtilsRaiseIRQL()
// {
//     KIRQL curr;
//     KIRQL prev;
// 
//     curr = pGlobalData->fnKeGetCurrentIrql();
//     prev = curr;
//     if (curr < DISPATCH_LEVEL) {
//         KeRaiseIrql(DISPATCH_LEVEL, &prev);
//     }
//     return prev;
// }
// 
// VOID UtilsLowerIRQL(KIRQL prev)
// {
//     KeLowerIrql(prev);
// }


#if _NON_STD_DRIVER == 0
WP_GLOBALS UtilsDisableWP_MDL(void* ptr, ULONG size)
{
    WP_GLOBALS wpGlobals;
    DEFINE_GLOBAL_BLOCK(pGlobalData);
    IMPL_GLOBAL_BLOCK(pGlobalData);

    wpGlobals.pMDL = pGlobalData->fnIoAllocateMdl(ptr, size, FALSE, FALSE, NULL);
    pGlobalData->fnMmBuildMdlForNonPagedPool(wpGlobals.pMDL);
     wpGlobals.pMDL->MdlFlags = wpGlobals.pMDL->MdlFlags | MDL_MAPPED_TO_SYSTEM_VA;

    wpGlobals.ptr = pGlobalData->fnMmMapLockedPages(wpGlobals.pMDL, KernelMode);
    return wpGlobals;
}

void UtilsEnableWP_MDL(PMDL pMDL, void* ptr)
{
    DEFINE_GLOBAL_BLOCK(pGlobalData);
    IMPL_GLOBAL_BLOCK(pGlobalData);

    if (pMDL != NULL) {
        pGlobalData->fnMmUnmapLockedPages(ptr, pMDL);
        pGlobalData->fnIoFreeMdl(pMDL);
    }
}

#endif
// 
// #define NX_FLAG 0x8000000000000000ui64
// 
// #if _AMD64_
// 
// extern UINT64 GetPML4Base();
// 
// // Disables NX bit for page pointed by ptr on x64 platform
// BOOLEAN UtilsDisablePageNXBit(void* ptr) // 64-bit not PAE!
// {
//     DWORD_PTR* pPML4,* pPDP,* pPD,* pPT, PML4e, PDPe, PDe, PTe;
//     PHYSICAL_ADDRESS phys;
//     DEFINE_GLOBAL_DATA(pGlobalData);
//     IMPL_GLOBAL_DATA(pGlobalData);
// 
// // 
// //     KDPRINT(("\nNXFree goes.. param = 0x%p\n", ptr));
// // 
//      pPML4 = (DWORD_PTR*) GetPML4Base();
//      if (pPML4)
//      {
// //         KDPRINT((" pPML4 = 0x%p\n", pPML4));
//  
//          phys.QuadPart = (LONGLONG)((UINT64)pPML4 + (UINT64)((((UINT64)ptr >> 39) & 0x01FF) * sizeof(DWORD_PTR)));
//  
// //         KDPRINT((" PML4 index = 0x%p, PML4 phys addr = 0x%p\n", VM_GET_PML4OFFSET(ptr), phys.QuadPart));
//  
//          pPML4 = (DWORD_PTR*)pGlobalData->fnMmMapIoSpace(phys, sizeof(DWORD_PTR), MmNonCached);
//          if (pPML4 == NULL) {
// //             KDPRINT(("*** ERROR: PML4e phys map fails"));
//              return 0;
//          }
//          PML4e = *pPML4;
//          pGlobalData->fnMmUnmapIoSpace((void*)pPML4, sizeof(ptrdiff_t));
//          if (PML4e) {
// //             KDPRINT((" PML4e = 0x%p\n", PML4e));
//  
//              phys.QuadPart = (LONGLONG)((ptrdiff_t)(PML4e /*& VM_MAP_BASE_POINTER*/) + (ptrdiff_t)((((UINT64)ptr >> 30) & 0x01FF) * sizeof(DWORD_PTR)));
//  
// //             KDPRINT((" PDP index = 0x%p, PDP phys addr = 0x%p\n", VM_GET_PDPOFFSET(ptr), phys.QuadPart));
//  
//              pPDP = pGlobalData->fnMmMapIoSpace(phys, sizeof(DWORD_PTR), MmNonCached);
//              if (pPDP == NULL) {    
// //                 KDPRINT(("*** ERROR: PDPe phys map fails"));
//                  return FALSE;
//              }
//              PDPe = *pPDP;
//              pGlobalData->fnMmUnmapIoSpace(pPDP, sizeof(DWORD_PTR));
//              if (PDPe) {
// //                 KDPRINT((" PDPe = 0x%p\n", PDPe));
//  
//                  phys.QuadPart = (LONGLONG)((ptrdiff_t)(PDPe /*& VM_MAP_BASE_POINTER*/) + (ptrdiff_t)(/*VM_GET_PDOFFSET*/(((UINT64)ptr >> 21) & 0x01FF) * sizeof(DWORD_PTR)));
//  
// //                 KDPRINT((" PD intex = 0x%p, PD phys addr = 0x%p\n", VM_GET_PDOFFSET(ptr), phys.QuadPart));
//  
//                  pPD = (DWORD_PTR*)pGlobalData->fnMmMapIoSpace(phys, sizeof(DWORD_PTR), MmNonCached);
//                  if (pPD == NULL) {
// //                     KDPRINT(("*** ERROR: PDe phys map fails"));
//                      return FALSE;
//                  }
//                  PDe = *pPD;
//  
//  
// //                 KDPRINT((" PDe = 0x%p\n", PDe));
//                  //
//                  //    checking for 4k/2m pages
//                  //
//                  if (PDe & 0x80 /*PDE_PS*/) {
//                      // 2M pages
// //                     KDPRINT(("2 megabyte page detected\n"));
//                      if (PDe & NX_FLAG) {
//                          *pPD = PDe & (~NX_FLAG);
// //                         KDPRINT(("New PTE = 0x%p\n", *pPD));
//                      }
//                      else
//                      {
// //                         KDPRINT(("page is already executable\n"));
//                      }
//  
//                  }
//                  else
//                  {
//                      // 4K pages
//                      if (PDe != (DWORD_PTR)NULL) {
//                          phys.QuadPart = (LONGLONG)((ptrdiff_t)(PDe /*& VM_MAP_BASE_POINTER*/) + (ptrdiff_t)(/*VM_GET_PTOFFSET*/(((UINT64)ptr >> 12) & 0x01FF) * sizeof(DWORD_PTR)));
//  
// //                         KDPRINT((" PT index = 0x%p, PT phys addr = 0x%p\n", VM_GET_PTOFFSET(ptr), phys.QuadPart));
//  
//                          pPT = (DWORD_PTR *)pGlobalData->fnMmMapIoSpace(phys, sizeof(DWORD_PTR), MmNonCached);
//                          if (pPT == NULL) {
// //                             KDPRINT(("*** ERROR: PTe phys map fails"));
//                              return FALSE;
//                          }
//                          PTe = *pPT;
//  
//                          if (PTe != (DWORD_PTR)NULL) {
// //                             KDPRINT((" PTe = 0x%p\n", PTe));
//  
//                              //
//                              // clear NX bite
//                              //
//                              if (PTe & NX_FLAG) {
//                                  UtilsDisableWP();
//                                  *pPT = PTe & (~NX_FLAG);
//                                 UtilsEnableWP();
//                                  //WriteFlagRestore();
// //                                 KDPRINT(("New PTE = 0x%p\n", *pPT));
//                              }
//                          }
//                          else {
// //                             KDPRINT(("*** ERROR: PTe is empty"));
//                              return FALSE;
//                          }
//                          pGlobalData->fnMmUnmapIoSpace(pPT, sizeof(DWORD_PTR));
//                      }
//                      else {
// //                         KDPRINT(("*** ERROR: PDe is empty"));
//                          return FALSE;
//                      }
//                      //end 4k
//                  }
// //                 KDPRINT(("unmap pPD...\n"));
//                  pGlobalData->fnMmUnmapIoSpace(pPD, sizeof(DWORD_PTR));
//              }
//              else {
// //                 KDPRINT(("*** ERROR: PDPe is empty"));
//                  return FALSE;
//              }
//  
//          }
//          else {
// //             KDPRINT(("*** ERROR: PML4e is empty"));
//              return FALSE;
//          }
//      }
//      else {
// //         KDPRINT(("*** ERROR: pPML4 is empty"));
//          return FALSE;
//      }
// 
//     return TRUE;
// }
// 
// #else
// 
// #define PDE_TABLE_SIZE 4096
// #define PTE_TABLE_SIZE 4096
// 
// typedef struct _PTE_PAE
// {
//     /* [0-11] - Flags */
//     ULONG Present           :1; // [V]   - Valid - point to physical memory
//     ULONG Writable          :1; // [W|R] - W when set 
//     ULONG Owner             :1; // [K|U] - Kernel mode, User mode
//     ULONG WriteThrough      :1; // [T]   - when set
//     ULONG CacheDisable      :1; // [N]   - when set
//     ULONG Accessed          :1; // [A]   - when set Page has been read
//     ULONG Dirty             :1; // [D]   - when set Page has been written to
//     ULONG LargePage         :1; // [L]   - when set PDE maps 4MB
//     ULONG Global            :1; // [G]   - global when set
//     ULONG ForUse1           :1; // [C]   - copy on write, when set
//     ULONG ForUse2           :1;
//     ULONG ForUse3           :1;
// 
//     /* [12-31] */
//     ULONG PageBaseAddress   :20;
// 
//     /* [35-32] */
//     ULONG BaseAddress       :4;
// 
//     /* [36-63] */
//     ULONG Reserved          :28;
// 
// } PTE_PAE, *PPTE_PAE;
// 
// 
// // typedef struct _PTE
// // {
// //     ULONG Present           :1; // [V]   - Valid - point to physical memory
// //     ULONG Writable          :1; // [W|R] - W when set 
// //     ULONG Owner             :1; // [K|U] - Kernel mode, User mode
// //     ULONG WriteThrough      :1; // [T]   - when set
// //     ULONG CacheDisable      :1; // [N]   - when set
// //     ULONG Accessed          :1; // [A]   - when set Page has been read
// //     ULONG Dirty             :1; // [D]   - when set Page has been written to
// //     ULONG LargePage         :1; // [L]   - when set PDE maps 4MB
// //     ULONG Global            :1; // [G]   - global when set
// //     ULONG ForUse1           :1; // [C]   - copy on write, when set
// //     ULONG ForUse2           :1;
// //     ULONG ForUse3           :1;
// //     ULONG PageFrameNumber   :20;
// // } PTE, *PPTE;
// 
// // typedef PTE PDE;
// // typedef PDE* PPDE;
// 
// typedef PTE_PAE PDE_PAE;
// typedef PDE_PAE* PPDE_PAE;
// 
// ///!!!///
// BOOLEAN UtilsDisablePageNXBit(void* ptr, UINT32 size)
// {
//     BOOLEAN isPAE;
//     void* pBaseVAPPDE;
//     void* pBaseVAPDE;
//     PHYSICAL_ADDRESS physical;
//     BOOLEAN ret = FALSE;
//     PEPROCESS pep;
//     PUINT8 currAddr;
//     PUINT8 endAddr = (PUINT8)ptr + size + 2048;
//     DEFINE_GLOBAL_DATA(pGlobalData);
//     IMPL_GLOBAL_DATA(pGlobalData);
// 
//     pep = pGlobalData->fnIoGetCurrentProcess();
// 
//     isPAE = pGlobalData->sysSpec.cpuid & 0x00000040;
// 
//     if (isPAE) {
//         for (currAddr = (PUINT8)ptr; currAddr < endAddr; currAddr += 2048) {
//             PPDE_PAE ppde;
// 
//             physical.LowPart = *(ULONG*)((PUCHAR)pep + 0x18);
//             physical.HighPart = 0;
// 
//             pBaseVAPPDE = pGlobalData->fnMmMapIoSpace(physical, 32, MmNonCached);
// 
//             ppde =  (PPDE_PAE)((ULONG)pBaseVAPPDE + (((UINT32)currAddr >> 30) << 3));
//             if (ppde->Present) {
//                 //             if (ppde->LargePage) {
//                 // 
//                 //             }
//                 //             else {
//                 PPDE_PAE pde;
// 
//                 physical.LowPart = (ppde->PageBaseAddress << 12);
//                 physical.HighPart = 0;
// 
//                 pBaseVAPDE = pGlobalData->fnMmMapIoSpace(physical, PDE_TABLE_SIZE, MmNonCached);
// 
//                 pde =  (PPDE_PAE)((ULONG)pBaseVAPDE + ((((UINT32)currAddr >> 21) & 0x1FF) << 3));
//                 if (pde->Present) {
//                     if (pde->LargePage) { // 2M pages
//                         if (*(UINT64*)pde & NX_FLAG) {
//                             pGlobalData->fnUtilsDisableWP();
//                             *(UINT64*)pde = *(UINT64*)pde & (~NX_FLAG);
//                             pGlobalData->fnUtilsEnableWP();
//                         }
//                         ret = TRUE;
//                     }
//                     else {
//                         PPTE_PAE pte;
//                         void* PTE_Virtual;
// 
//                         physical.LowPart = (pde->PageBaseAddress << 12);
//                         physical.HighPart = 0;
// 
//                         PTE_Virtual = pGlobalData->fnMmMapIoSpace(physical, PTE_TABLE_SIZE, MmNonCached);
//                         pte = (PPTE_PAE)((ULONG)PTE_Virtual + ((((UINT32)currAddr >> 12) & 0x1FF) << 3) );
// 
//                         if (pte->Present) {
//                             pGlobalData->fnUtilsDisableWP();
//                             *(UINT64*)pte = *(UINT64*)pte & (~NX_FLAG);
//                             pGlobalData->fnUtilsEnableWP();
// 
//                             ret = TRUE;
//                         }
//                         pGlobalData->fnMmUnmapIoSpace(PTE_Virtual, PTE_TABLE_SIZE);
//                     }
//                 }
//                 pGlobalData->fnMmUnmapIoSpace(pBaseVAPDE, PDE_TABLE_SIZE);
//                 //}
//             }
//             pGlobalData->fnMmUnmapIoSpace(pBaseVAPPDE, 32);
//         }
//     }
// 
//     return ret;
// }
// 
// #endif
