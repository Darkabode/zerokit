#include "..\lzma_arch.c"

#include "..\lzma.h"

#define USE_LZMA_DECOMPRESSOR 1
#include "..\lzma.c"

namespace zgui
{

    int LZMADecompress(pvoid_t inStream, uint32_t inSize, pvoid_t* outStream, uint32_t* poutSize)
    {
        uint32_t outSize, origOutSize;
        int ret = 1;
        ELzmaStatus st;

        outSize = inSize;
        do {
            if (*outStream != NULL) {
                fn_memfree(*outStream);
            }
            outSize *= 2;
            origOutSize = outSize;
            *outStream = fn_memalloc(outSize);
            ret = lzma_decode((uint8_t*)*outStream, &outSize, (uint8_t*)inStream, inSize, &st);
        } while (ret == ERR_OK && outSize == origOutSize);

        *poutSize = outSize;

        return ret;
    }


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

uint32_t LockerLzmaArchReader::getEntryOffset(const String& name)
{
    return lzma_arch_get_entry_offset(_pLzmaArchHeader, name.toUTF8().getAddress(), 0, 0);
}

bool LockerLzmaArchReader::getEntry(const String& name, MemoryBlock& entryBlock)
{
    unsigned int entrySize;
    unsigned int entryFlags;
	unsigned int entryOffset = lzma_arch_get_entry_offset(_pLzmaArchHeader, name.toUTF8().getAddress(), &entrySize, &entryFlags);
    void* outBuffer = NULL;
    uint32_t outSize;
    
    if (entryOffset == (unsigned int)-1) {
        return false;
    }

    char* pEntryData = (char*)_pLzmaArchHeader + entryOffset + fn_lstrlenA((char*)_pLzmaArchHeader + entryOffset) + 1 + 2 * sizeof(unsigned int);
    if (entryFlags & LAF_ENTRY_COMPRESSED) {
        int err = LZMADecompress(pEntryData, entrySize, (pvoid_t*)&outBuffer, &outSize);

        if (err != 0) {
            return false;
        }

        entryBlock.setSize(outSize);
        __movsb((uint8_t*)entryBlock.getData(), (const uint8_t*)outBuffer, outSize);
        fn_memfree(outBuffer);
    }
    else {
        entryBlock.setSize(entrySize);
        __movsb((uint8_t*)entryBlock.getData(), (const uint8_t*)pEntryData, entrySize);
    }

    return true;
}

}