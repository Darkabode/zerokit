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

	bool contains(const String&);
	HBITMAP getImage(const String&, uint32_t* pWidth, uint32_t* pHeight);
	bool getBinary(const String&, MemoryBlock& block);
	String getString(const String& name);

	ZGUI_DECLARE_SINGLETON(Resources);

private:
    struct Source
    {
        Source()
        {
        }

        ~Source()
        {
        }

        uint8_t* pBuffer;
        uint32_t size;
        LockerLzmaArchReader lzmaArchReader;
    };

    Array<Source*> _sources;
};

}

#endif // __ZGUI_RESOURCES_H_
