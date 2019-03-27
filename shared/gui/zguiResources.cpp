#include "zgui.h"

namespace zgui
{

ZGUI_IMPLEMENT_SINGLETON(Resources);

Resources::Resources()
{
}

Resources::~Resources()
{
    for (int i = _sources.size(); --i >= 0; ) {
		delete _sources.getUnchecked(i);
    }
}

void Resources::addSource(uint8_t* pBuffer, uint32_t sz)
{
    Source* pSrc = new Source;

    pSrc->pBuffer = pBuffer;
    pSrc->size = sz;

    pSrc->lzmaArchReader.init((plzma_arch_header_t)pBuffer, sz);

	_sources.add(pSrc);
}

bool Resources::contains(const String& name)
{
    for (size_t i = 0, count = _sources.size(); i < count; ++i) {
        Source* pSrc = _sources.getUnchecked(i);
        if (pSrc->lzmaArchReader.getEntryOffset(name) != (uint32_t)-1) {
            return true;
        }
    }

    return false;
}

HBITMAP Resources::getImage(const String& name, uint32_t* pWidth, uint32_t* pHeight)
{
    HBITMAP hBitmap = 0;
    bool found = false;
    
    do {
        MemoryBlock block;

        for (size_t i = 0, count = _sources.size(); i < count; ++i) {
            Source* pSrc = _sources.getUnchecked(i);

            if (pSrc->lzmaArchReader.getEntry(name, block)) {
                found = true;
                break;
            }
        }

        if (!found) {
            break;
        }        

        hBitmap = load_png((uint8_t*)block.getData(), block.getSize(), pWidth, pHeight, TRUE);
    } while (0);

    return hBitmap;
}

bool Resources::getBinary(const String& name, MemoryBlock& block)
{
    for (size_t i = 0, count = _sources.size(); i < count; ++i) {
        Source* pSrc = _sources.getUnchecked(i);
        if (pSrc->lzmaArchReader.getEntry(name, block)) {
            return true;
        }
    }

    return false;
}

String Resources::getString(const String& name)
{
	MemoryBlock block;
	if (getBinary(name, block)) {
		return String::fromUTF8((const char*)block.getData(), block.getSize());
	}

	return String::empty;
}

}