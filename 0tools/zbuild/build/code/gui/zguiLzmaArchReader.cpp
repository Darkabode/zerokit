#include "zgui.h"

#include "..\..\..\..\shared\lzma_arch.c"

#include "..\..\..\..\..\shared_code\lzma.h"

#define USE_LZMA_DECOMPRESSOR 1
#include "..\..\..\..\..\shared_code\lzma.c"

int LZMADecompress(pvoid_t inStream, uint32_t inSize, pvoid_t* outStream, uint32_t* poutSize)
{
    uint32_t outSize, origOutSize;
    int ret = 1;
    ELzmaStatus st;

    do {
        outSize = inSize;
        do {
            if (*outStream != NULL) {
                SYS_DEALLOCATOR(*outStream);
            }
            outSize *= 2;
            origOutSize = outSize;
            *outStream = SYS_ALLOCATOR(outSize);
            ret = lzma_decode((uint8_t*)*outStream, &outSize, (uint8_t*)inStream, inSize, &st);
        } while (ret == ERR_OK && outSize == origOutSize);

        *poutSize = outSize;
    } while (0);

    return ret;
}

namespace zgui
{

LockerLzmaArchReader::LockerLzmaArchReader()
{
    
}

LockerLzmaArchReader::~LockerLzmaArchReader()
{

}

bool LockerLzmaArchReader::init(plzma_arch_header_t pLzmaArchHeader, uint32_t size)
{
    _pLzmaArchHeader = pLzmaArchHeader;

    return (_pLzmaArchHeader->signature == 0x79977997 && size == _pLzmaArchHeader->totalSize);
}

uint32_t LockerLzmaArchReader::getEntryOffset(const char* name)
{
    return lzma_arch_get_entry_offset(_pLzmaArchHeader, name, 0, 0);
}

bool LockerLzmaArchReader::getEntry(const char* name, MemoryBlock& entryBlock)
{
    unsigned int entrySize;
    unsigned int entryFlags;
    unsigned int entryOffset = lzma_arch_get_entry_offset(_pLzmaArchHeader, name, &entrySize, &entryFlags);
    void* outBuffer;
    uint32_t outSize;
    
    if (entryOffset == (unsigned int)-1) {
        return false;
    }

    char* pEntryData = (char*)_pLzmaArchHeader + entryOffset + lstrlenA((char*)_pLzmaArchHeader + entryOffset) + 1 + 2 * sizeof(unsigned int);
    if (entryFlags & LAF_ENTRY_COMPRESSED) {
        int err = LZMADecompress(pEntryData, entrySize, (pvoid_t*)&outBuffer, &outSize);

        if (err != 0) {
            return false;
        }

        entryBlock.setSize(outSize);
        memcpy(entryBlock.getData(), outBuffer, outSize);
        SYS_DEALLOCATOR(outBuffer);
    }
    else {
        entryBlock.setSize(entrySize);
        memcpy(entryBlock.getData(), pEntryData, entrySize);
    }

    return true;
}

}