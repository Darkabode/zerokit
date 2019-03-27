#include "zgui.h"

namespace zgui
{

zgui_ImplementSingleton (Resources)

Resources::Resources()
{
    cvec_init(_sources);
}

Resources::~Resources()
{
    cvec_destroy(_sources);
}

void Resources::addSource(uint8_t* pBuffer, uint32_t sz)
{
    Source* pSrc = new Source;

    pSrc->pBuffer = pBuffer;
    pSrc->size = sz;

    pSrc->lzmaArchReader.init((plzma_arch_header_t)pBuffer, sz);

    cvec_push(Source*, _sources, pSrc);
}

bool Resources::contains(const char* name)
{
    for (size_t i = 0, count = cvec_size(_sources); i < count; ++i) {
        Source* pSrc = cvec_A(_sources, i);
        if (pSrc->lzmaArchReader.getEntryOffset(name) != (uint32_t)-1) {
            return true;
        }
    }

    return false;
}

Gdiplus::Bitmap* Resources::getImage(const char* name)
{
    Gdiplus::Bitmap* pImage = 0;
    bool found = false;
    
    do {
        MemoryBlock block;

        for (size_t i = 0, count = cvec_size(_sources); i < count; ++i) {
            Source* pSrc = cvec_A(_sources, i);

            if (pSrc->lzmaArchReader.getEntry(name, block)) {
                found = true;
                break;
            }
        }

        if (!found) {
            break;
        }        

        pImage = LockerGdiImageLoader::load(block.getData(), block.getSize());
    } while (0);

    return pImage;
}

bool Resources::getBinary(const char* name, MemoryBlock& block)
{
    for (size_t i = 0, count = cvec_size(_sources); i < count; ++i) {
        Source* pSrc = cvec_A(_sources, i);
        if (pSrc->lzmaArchReader.getEntry(name, block)) {
            return true;
        }
    }

    return false;
}

}