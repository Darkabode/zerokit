#ifndef __ZGUI_PNGLOAD_H_
#define __ZGUI_PNGLOAD_H_

namespace zgui
{

HBITMAP load_png(uint8_t* buffer, uint32_t size, uint32_t* pWidth, uint32_t* pHeight, BOOL premultiply);

}

#endif // __ZGUI_PNGLOAD_H_
