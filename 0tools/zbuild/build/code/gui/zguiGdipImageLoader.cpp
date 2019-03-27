#include "zgui.h"

Gdiplus::Bitmap* LockerGdiImageLoader::load(const void* rawData, const int dataSize)
{
	HGLOBAL hBuffer;
	Gdiplus::Bitmap* pBitmap = 0;

    hBuffer = ::GlobalAlloc(GMEM_MOVEABLE, dataSize);
    if (hBuffer == 0) {
        return pBitmap;
    }

    void* pBuffer = ::GlobalLock(hBuffer);
    if (pBuffer != 0) {
	    CopyMemory(pBuffer, rawData, dataSize);
        ::GlobalUnlock(hBuffer);
	    IStream* pStream = NULL;
	    if (::CreateStreamOnHGlobal(hBuffer, FALSE/*TRUE*/, &pStream) == S_OK) {
            pBitmap = Gdiplus::Bitmap::FromStream(pStream);
            if (pBitmap->GetLastStatus() != Gdiplus::Ok) {
                delete pBitmap;
                pBitmap = 0;
            }

		    pStream->Release();
	    }
    }

    ::GlobalFree(hBuffer);

	return pBitmap;
}
