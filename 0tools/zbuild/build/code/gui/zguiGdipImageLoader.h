#ifndef __ZGUI_GDIPIMAGELOADER_H_
#define __ZGUI_GDIPIMAGELOADER_H_

struct LockerGdiImageLoader
{
    static Gdiplus::Bitmap* load(const void* rawData, const int dataSize/*, const int width, const int height*/);
};

#endif // __ZGUI_GDIPIMAGELOADER_H_
