

unsigned int lzma_arch_get_entry_offset(plzma_arch_header_t pArchHeader, const char* entryName, unsigned int* pSize, unsigned int* pFlags)
{
    int offset = sizeof(lzma_arch_header_t);
    const char* pEn = (const char*)pArchHeader + sizeof(lzma_arch_header_t);
    
    for ( ; pEn < ((const char*)pArchHeader + pArchHeader->totalSize); ) {
        size_t enLen = fn_lstrlenA(pEn);
        
        if (fn_lstrcmpA(pEn, entryName) == 0) {
            if (pSize != NULL) {
                *pSize = *(unsigned int*)(pEn + enLen + 1);
            }

            if (pFlags != NULL) {
                *pFlags = *(unsigned int*)(pEn + enLen + 1 + sizeof(unsigned int));
            }
            return offset;
        }
        
        pEn = pEn + enLen + 1 + 2 * sizeof(unsigned int) + *(unsigned int*)(pEn + enLen + 1);
        offset = pEn - (const char*)pArchHeader;
    }

    return (unsigned int)-1;
}
