#ifndef __ZGUI_RESOURCES_H_
#define __ZGUI_RESOURCES_H_

namespace zgui
{

class Resources
{
public:
    Resources();
    ~Resources();

    void addSource(uint8_t* pBuffer, uint32_t sz);

    bool contains(const char* name);
    Gdiplus::Bitmap* getImage(const char* name);
    bool getBinary(const char* name, MemoryBlock& block);

    zgui_DeclareSingleton(Resources)

private:
    struct Source
    {
        uint8_t* pBuffer;
        uint32_t size;
        LockerLzmaArchReader lzmaArchReader;
    };

    //Array<Source*> _sources;
    cvec_t(Source*) _sources;
};

}

#endif // __ZGUI_RESOURCES_H_
