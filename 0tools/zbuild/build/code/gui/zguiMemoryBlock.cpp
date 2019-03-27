#include "zgui.h"

namespace zgui
{
    
MemoryBlock::MemoryBlock() throw() :
_data(0),
_size(0)
{
}

MemoryBlock::MemoryBlock (const size_t initialSize, const bool initialiseToZero)
{
    if (initialSize > 0) {
        _size = initialSize;
        _data = (char*)memalloc(initialSize);
        if (initialiseToZero) {
            memset((unsigned char*)_data, 0, initialSize);
        }
    }
    else {
        _size = 0;
    }
}

MemoryBlock::MemoryBlock (const MemoryBlock& other) :
_size(other._size)
{
    if (_size > 0) {
        _data = (char*)memalloc(_size);
        memcpy(_data, other._data, _size);
    }
}

MemoryBlock::MemoryBlock (const void* const dataToInitialiseFrom, const size_t sizeInBytes) : _size(max((size_t)0, sizeInBytes))
{
    if (_size > 0) {
        _data = (char*)memalloc(_size);

        if (dataToInitialiseFrom != 0) {
            memcpy(_data, dataToInitialiseFrom, _size);
        }
    }
}

MemoryBlock::~MemoryBlock() throw()
{
    if (_data != 0) {
        memfree(_data);
    }
}

bool MemoryBlock::operator== (const MemoryBlock& other) const throw()
{
    return matches (other._data, other._size);
}

bool MemoryBlock::operator!= (const MemoryBlock& other) const throw()
{
    return ! operator== (other);
}

bool MemoryBlock::matches (const void* dataToCompare, size_t dataSize) const throw()
{
    return _size == dataSize && memcmp (_data, dataToCompare, _size) == 0;
}

//==============================================================================
// this will resize the block to this size
void MemoryBlock::setSize(const size_t newSize, const bool initialiseToZero)
{
    if (_size != newSize) {
        if (newSize <= 0) {
            memfree(_data);
            _size = 0;
        }
        else {
            if (_data != 0) {
                _data = (char*)memrealloc(_data, newSize);

                if (initialiseToZero && (newSize > _size))
                    memset((unsigned char*)(_data + _size), 0, newSize - _size);
            }
            else {
                _data = (char*)memalloc(newSize);
                if (initialiseToZero) {
                    memset((unsigned char*)_data, 0, newSize);
                }
            }

            _size = newSize;
        }
    }
}

void MemoryBlock::ensureSize (const size_t minimumSize, const bool initialiseToZero)
{
    if (_size < minimumSize)
        setSize (minimumSize, initialiseToZero);
}

//==============================================================================
void MemoryBlock::fillWith (const uint8_t value) throw()
{
    memset (_data, (int) value, _size);
}

void MemoryBlock::append (const void* const srcData, const size_t numBytes)
{
    if (numBytes > 0) {
        const size_t oldSize = _size;
        setSize (_size + numBytes);
        memcpy (_data + oldSize, srcData, numBytes);
    }
}

void MemoryBlock::replaceWith (const void* const srcData, const size_t numBytes)
{
    if (numBytes > 0) {
        setSize (numBytes);
        memcpy (_data, srcData, numBytes);
    }
}

void MemoryBlock::insert (const void* const srcData, const size_t numBytes, size_t insertPosition)
{
    if (numBytes > 0) {
        insertPosition = min (_size, insertPosition);
        const size_t trailingDataSize = _size - insertPosition;
        setSize (_size + numBytes, false);

        if (trailingDataSize > 0)
            memmove (_data + insertPosition + numBytes, _data + insertPosition, trailingDataSize);

        memcpy (_data + insertPosition, srcData, numBytes);
    }
}

// void MemoryBlock::removeSection (const size_t startByte, const size_t numBytesToRemove)
// {
//     if (startByte + numBytesToRemove >= size) {
//         setSize (startByte);
//     }
//     else if (numBytesToRemove > 0) {
//         memmove (data + startByte, data + startByte + numBytesToRemove, size - (startByte + numBytesToRemove));
// 
//         setSize (size - numBytesToRemove);
//     }
// }

void MemoryBlock::copyFrom (const void* const src, int offset, size_t num) throw()
{
    const char* d = static_cast<const char*> (src);

    if (offset < 0)
    {
        d -= offset;
        num -= offset;
        offset = 0;
    }

    if (offset + num > _size)
        num = _size - offset;

    if (num > 0)
        memcpy (_data + offset, d, num);
}

void MemoryBlock::copyTo (void* const dst, int offset, size_t num) const throw()
{
    char* d = static_cast<char*> (dst);

    if (offset < 0) {
        memset((unsigned char*)d, 0, (size_t) -offset);
        d -= offset;

        num += offset;
        offset = 0;
    }

    if (offset + num > _size)
    {
        const size_t newNum = _size - offset;
        memset((unsigned char*)(d + newNum), 0, num - newNum);
        num = newNum;
    }

    if (num > 0) {
        memcpy (d, _data + offset, num);
    }
}
// 
// bool MemoryBlock::loadFromFile(CDuiString& filePath) throw()
// {
//     bool ret = false;
//     DWORD dwSize;
// 
//     do {
//         HANDLE hFile = ::CreateFile(filePath.GetData(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
//         if (hFile == INVALID_HANDLE_VALUE ) {
//             break;
//         }
//         dwSize = ::GetFileSize(hFile, NULL);
//         
//         if (dwSize == 0) {
//             break;
//         }
// 
//         DWORD dwRead = 0;
//         setSize(dwSize);
//         ::ReadFile(hFile, _data, dwSize, &dwRead, NULL );
//         ::CloseHandle(hFile);
// 
//         if (dwRead != dwSize) {
//             setSize(0);
//             break;
//         }
// 
//         ret = true;
//     } while (0);
// 
//     return ret;
// }
// 

}