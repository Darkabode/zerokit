#include "zgui.h"

namespace zgui {

RenderClip::~RenderClip()
{
    zgui_assert(fn_GetObjectType(hDC)==OBJ_DC || fn_GetObjectType(hDC)==OBJ_MEMDC);
    zgui_assert(fn_GetObjectType(hRgn)==OBJ_REGION);
    zgui_assert(fn_GetObjectType(hOldRgn)==OBJ_REGION);
    fn_SelectClipRgn(hDC, hOldRgn);
    fn_DeleteObject(hOldRgn);
    fn_DeleteObject(hRgn);
}

void RenderClip::GenerateClip(HDC hDC, RECT rc, RenderClip& clip)
{
    RECT rcClip;
    __stosb((uint8_t*)&rcClip, 0, sizeof(rcClip));
    fn_GetClipBox(hDC, &rcClip);
    clip.hOldRgn = fn_CreateRectRgnIndirect(&rcClip);
    clip.hRgn = fn_CreateRectRgnIndirect(&rc);
    fn_ExtSelectClipRgn(hDC, clip.hRgn, RGN_AND);
    clip.hDC = hDC;
    clip.rcItem = rc;
}

void RenderClip::GenerateRoundClip(HDC hDC, RECT rc, RECT rcItem, int width, int height, RenderClip& clip)
{
    RECT rcClip;
    __stosb((uint8_t*)&rcClip, 0, sizeof(rcClip));
    fn_GetClipBox(hDC, &rcClip);
    clip.hOldRgn = fn_CreateRectRgnIndirect(&rcClip);
    clip.hRgn = fn_CreateRectRgnIndirect(&rc);
    HRGN hRgnItem = fn_CreateRoundRectRgn(rcItem.left, rcItem.top, rcItem.right + 1, rcItem.bottom + 1, width, height);
    fn_CombineRgn(clip.hRgn, clip.hRgn, hRgnItem, RGN_AND);
    fn_ExtSelectClipRgn(hDC, clip.hRgn, RGN_AND);
    clip.hDC = hDC;
    clip.rcItem = rc;
    fn_DeleteObject(hRgnItem);
}

void RenderClip::UseOldClipBegin(HDC hDC, RenderClip& clip)
{
    fn_SelectClipRgn(hDC, clip.hOldRgn);
}

void RenderClip::UseOldClipEnd(HDC hDC, RenderClip& clip)
{
    fn_SelectClipRgn(hDC, clip.hRgn);
}


static const float OneThird = 1.0f / 3;

static void RGBtoHSL(DWORD ARGB, float* H, float* S, float* L) {
    const float
        R = (float)GetRValue(ARGB),
        G = (float)GetGValue(ARGB),
        B = (float)GetBValue(ARGB),
        nR = (R<0?0:(R>255?255:R))/255,
        nG = (G<0?0:(G>255?255:G))/255,
        nB = (B<0?0:(B>255?255:B))/255,
        m = min(min(nR,nG),nB),
        M = max(max(nR,nG),nB);
    *L = (m + M)/2;
    if (M==m) *H = *S = 0;
    else {
        const float
            f = (nR==m)?(nG-nB):((nG==m)?(nB-nR):(nR-nG)),
            i = (nR==m)?3.0f:((nG==m)?5.0f:1.0f);
        *H = (i-f/(M-m));
        if (*H>=6) *H-=6;
        *H*=60;
        *S = (2*(*L)<=1)?((M-m)/(M+m)):((M-m)/(2-M-m));
    }
}

static void HSLtoRGB(DWORD* ARGB, float H, float S, float L) {
    const float
        q = 2*L<1?L*(1+S):(L+S-L*S),
        p = 2*L-q,
        h = H/360,
        tr = h + OneThird,
        tg = h,
        tb = h - OneThird,
        ntr = tr<0?tr+1:(tr>1?tr-1:tr),
        ntg = tg<0?tg+1:(tg>1?tg-1:tg),
        ntb = tb<0?tb+1:(tb>1?tb-1:tb),
        R = 255*(6*ntr<1?p+(q-p)*6*ntr:(2*ntr<1?q:(3*ntr<2?p+(q-p)*6*(2.0f*OneThird-ntr):p))),
        G = 255*(6*ntg<1?p+(q-p)*6*ntg:(2*ntg<1?q:(3*ntg<2?p+(q-p)*6*(2.0f*OneThird-ntg):p))),
        B = 255*(6*ntb<1?p+(q-p)*6*ntb:(2*ntb<1?q:(3*ntb<2?p+(q-p)*6*(2.0f*OneThird-ntb):p)));
    *ARGB &= 0xFF000000;
    *ARGB |= RGB( (BYTE)(R<0?0:(R>255?255:R)), (BYTE)(G<0?0:(G>255?255:G)), (BYTE)(B<0?0:(B>255?255:B)) );
}

DWORD RenderEngine::AdjustColor(DWORD dwColor, short H, short S, short L)
{
    if( H == 180 && S == 100 && L == 100 ) return dwColor;
    float fH, fS, fL;
    float S1 = S / 100.0f;
    float L1 = L / 100.0f;
    RGBtoHSL(dwColor, &fH, &fS, &fL);
    fH += (H - 180);
    fH = fH > 0 ? fH : fH + 360; 
    fS *= S1;
    fL *= L1;
    HSLtoRGB(&dwColor, fH, fS, fL);
    return dwColor;
}

TImageInfo* RenderEngine::loadImage(const String& bitmap)
{
    uint32_t width, height;

    HBITMAP hBitmap = Resources::getInstance()->getImage(bitmap.toUTF8(), &width, &height);

    if (hBitmap == 0) {
        return 0;
    }

    TImageInfo* data = new TImageInfo;
    data->hBitmap = hBitmap;
    data->width = width;
    data->height = height;
    return data;
}

TImageInfo* RenderEngine::loadImage(const String& bitmap, const RECT& rc)
{
	uint32_t width, height;

	HBITMAP hBitmap = Resources::getInstance()->getImage(bitmap.toUTF8(), &width, &height);

	if (hBitmap == 0) {
		return 0;
	}

	width = rc.right - rc.left;
	height = rc.bottom - rc.top;

	HDC hSrcDC = fn_CreateCompatibleDC(fn_GetDC(0));
	HBITMAP hSrcOldBitmap = (HBITMAP)fn_SelectObject(hSrcDC, hBitmap);
	HBITMAP hNewBitmap = fn_CreateCompatibleBitmap(fn_GetDC(0), width, height);
	HDC hDestDC = fn_CreateCompatibleDC(fn_GetDC(0));
	HBITMAP hDestOldBitmap = (HBITMAP)fn_SelectObject(hDestDC, hNewBitmap);
	fn_SetStretchBltMode(hDestDC, HALFTONE);

	BLENDFUNCTION bf = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };

	fn_AlphaBlend(hDestDC, 0, 0, width, height, hSrcDC, rc.left, rc.top, width, height, bf);

	fn_SelectObject(hSrcDC, hSrcOldBitmap);
	fn_DeleteDC(hSrcDC);
	fn_SelectObject(hDestDC, hDestOldBitmap);
	fn_DeleteDC(hDestDC);
	
	TImageInfo* data = new TImageInfo;
	data->hBitmap = hNewBitmap;
	data->width = width;
	data->height = height;
	return data;
}


void RenderEngine::FreeImage(const TImageInfo* bitmap)
{
	if (bitmap->hBitmap) {
        fn_DeleteObject(bitmap->hBitmap);
	}
	delete bitmap;
}

void RenderEngine::DrawImage(HDC hDC, HBITMAP hBitmap, const RECT& rc, const RECT& rcPaint, const RECT& rcBmpPart, const RECT& rcCorners, BYTE uFade, bool hole, bool xtiled, bool ytiled)
{
    if (hBitmap == NULL) {
        return;
    }

    HDC hCloneDC = fn_CreateCompatibleDC(hDC);
    HBITMAP hOldBitmap = (HBITMAP)fn_SelectObject(hCloneDC, hBitmap);
    fn_SetStretchBltMode(hDC, HALFTONE);

    RECT rcTemp;
    RECT rcDest;
    __stosb((uint8_t*)&rcTemp, 0, sizeof(rcTemp));
    __stosb((uint8_t*)&rcDest, 0, sizeof(rcDest));

    BLENDFUNCTION bf = {AC_SRC_OVER, 0, uFade, AC_SRC_ALPHA};

    if (!hole) {
        rcDest.left = rc.left + rcCorners.left;
        rcDest.top = rc.top + rcCorners.top;
        rcDest.right = rc.right - rc.left - rcCorners.left - rcCorners.right;
        rcDest.bottom = rc.bottom - rc.top - rcCorners.top - rcCorners.bottom;
        rcDest.right += rcDest.left;
        rcDest.bottom += rcDest.top;
        if (fn_IntersectRect(&rcTemp, &rcPaint, &rcDest)) {
            if (!xtiled && !ytiled) {
                rcDest.right -= rcDest.left;
                rcDest.bottom -= rcDest.top;
                fn_AlphaBlend(hDC, rcDest.left, rcDest.top, rcDest.right, rcDest.bottom, hCloneDC, rcBmpPart.left + rcCorners.left, rcBmpPart.top + rcCorners.top, rcBmpPart.right - rcBmpPart.left - rcCorners.left - rcCorners.right, rcBmpPart.bottom - rcBmpPart.top - rcCorners.top - rcCorners.bottom, bf);
            }
            else if (xtiled && ytiled) {
                LONG lWidth = rcBmpPart.right - rcBmpPart.left - rcCorners.left - rcCorners.right;
                LONG lHeight = rcBmpPart.bottom - rcBmpPart.top - rcCorners.top - rcCorners.bottom;
                int iTimesX = (rcDest.right - rcDest.left + lWidth - 1) / lWidth;
                int iTimesY = (rcDest.bottom - rcDest.top + lHeight - 1) / lHeight;
                for (int j = 0; j < iTimesY; ++j) {
                    LONG lDestTop = rcDest.top + lHeight * j;
                    LONG lDestBottom = rcDest.top + lHeight * (j + 1);
                    LONG lDrawHeight = lHeight;
                    if (lDestBottom > rcDest.bottom) {
                        lDrawHeight -= lDestBottom - rcDest.bottom;
                        lDestBottom = rcDest.bottom;
                    }
                    for (int i = 0; i < iTimesX; ++i) {
                        LONG lDestLeft = rcDest.left + lWidth * i;
                        LONG lDestRight = rcDest.left + lWidth * (i + 1);
                        LONG lDrawWidth = lWidth;
                        if( lDestRight > rcDest.right ) {
                            lDrawWidth -= lDestRight - rcDest.right;
                            lDestRight = rcDest.right;
                        }
                        fn_AlphaBlend(hDC, rcDest.left + lWidth * i, rcDest.top + lHeight * j, lDestRight - lDestLeft, lDestBottom - lDestTop, hCloneDC, rcBmpPart.left + rcCorners.left, rcBmpPart.top + rcCorners.top, lDrawWidth, lDrawHeight, bf);
                    }
                }
            }
            else if (xtiled) {
				LONG lWidth = rcBmpPart.right - rcBmpPart.left - rcCorners.left - rcCorners.right;
				LONG lDestLeft;
				LONG lDestRight;
					
				int iTimes;
				iTimes = (rcDest.right - rcDest.left + lWidth - 1) / lWidth;
				for (int i = 0; i < iTimes; ++i) {
					lDestLeft = rcDest.left + lWidth * i;
					lDestRight = rcDest.left + lWidth * (i + 1);
					LONG lDrawWidth = lWidth;
					if (lDestRight > rcDest.right) {
						lDrawWidth -= lDestRight - rcDest.right;
						lDestRight = rcDest.right;
					}
					fn_AlphaBlend(hDC, lDestLeft, rcDest.top, lDestRight - lDestLeft, rcDest.bottom - rcDest.top, hCloneDC, rcBmpPart.left + rcCorners.left, rcBmpPart.top + rcCorners.top, lDrawWidth, rcBmpPart.bottom - rcBmpPart.top - rcCorners.top - rcCorners.bottom, bf);
				}
            }
            else { // ytiled
                LONG lHeight = rcBmpPart.bottom - rcBmpPart.top - rcCorners.top - rcCorners.bottom;
                int iTimes = (rcDest.bottom - rcDest.top + lHeight - 1) / lHeight;
                for (int i = 0; i < iTimes; ++i) {
                    LONG lDestTop = rcDest.top + lHeight * i;
                    LONG lDestBottom = rcDest.top + lHeight * (i + 1);
                    LONG lDrawHeight = lHeight;
                    if (lDestBottom > rcDest.bottom) {
                        lDrawHeight -= lDestBottom - rcDest.bottom;
                        lDestBottom = rcDest.bottom;
                    }
                    fn_AlphaBlend(hDC, rcDest.left, rcDest.top + lHeight * i, rcDest.right - rcDest.left, lDestBottom - lDestTop, hCloneDC, rcBmpPart.left + rcCorners.left, rcBmpPart.top + rcCorners.top, rcBmpPart.right - rcBmpPart.left - rcCorners.left - rcCorners.right, lDrawHeight, bf);                    
                }
            }
        }
    }

    // left-top
    if( rcCorners.left > 0 && rcCorners.top > 0 ) {
        rcDest.left = rc.left;
        rcDest.top = rc.top;
        rcDest.right = rcCorners.left;
        rcDest.bottom = rcCorners.top;
        rcDest.right += rcDest.left;
        rcDest.bottom += rcDest.top;
        if (fn_IntersectRect(&rcTemp, &rcPaint, &rcDest)) {
            rcDest.right -= rcDest.left;
            rcDest.bottom -= rcDest.top;
            fn_AlphaBlend(hDC, rcDest.left, rcDest.top, rcDest.right, rcDest.bottom, hCloneDC, rcBmpPart.left, rcBmpPart.top, rcCorners.left, rcCorners.top, bf);
        }
    }
    // top
    if( rcCorners.top > 0 ) {
        rcDest.left = rc.left + rcCorners.left;
        rcDest.top = rc.top;
        rcDest.right = rc.right - rc.left - rcCorners.left - rcCorners.right;
        rcDest.bottom = rcCorners.top;
        rcDest.right += rcDest.left;
        rcDest.bottom += rcDest.top;
        if (fn_IntersectRect(&rcTemp, &rcPaint, &rcDest)) {
            rcDest.right -= rcDest.left;
            rcDest.bottom -= rcDest.top;
            fn_AlphaBlend(hDC, rcDest.left, rcDest.top, rcDest.right, rcDest.bottom, hCloneDC, rcBmpPart.left + rcCorners.left, rcBmpPart.top, rcBmpPart.right - rcBmpPart.left - rcCorners.left - rcCorners.right, rcCorners.top, bf);
        }
    }
    // right-top
    if( rcCorners.right > 0 && rcCorners.top > 0 ) {
        rcDest.left = rc.right - rcCorners.right;
        rcDest.top = rc.top;
        rcDest.right = rcCorners.right;
        rcDest.bottom = rcCorners.top;
        rcDest.right += rcDest.left;
        rcDest.bottom += rcDest.top;
        if (fn_IntersectRect(&rcTemp, &rcPaint, &rcDest)) {
            rcDest.right -= rcDest.left;
            rcDest.bottom -= rcDest.top;
            fn_AlphaBlend(hDC, rcDest.left, rcDest.top, rcDest.right, rcDest.bottom, hCloneDC, rcBmpPart.right - rcCorners.right, rcBmpPart.top, rcCorners.right, rcCorners.top, bf);
        }
    }
    // left
    if( rcCorners.left > 0 ) {
        rcDest.left = rc.left;
        rcDest.top = rc.top + rcCorners.top;
        rcDest.right = rcCorners.left;
        rcDest.bottom = rc.bottom - rc.top - rcCorners.top - rcCorners.bottom;
        rcDest.right += rcDest.left;
        rcDest.bottom += rcDest.top;
        if (fn_IntersectRect(&rcTemp, &rcPaint, &rcDest)) {
            rcDest.right -= rcDest.left;
            rcDest.bottom -= rcDest.top;
            fn_AlphaBlend(hDC, rcDest.left, rcDest.top, rcDest.right, rcDest.bottom, hCloneDC, \
                rcBmpPart.left, rcBmpPart.top + rcCorners.top, rcCorners.left, rcBmpPart.bottom - \
                rcBmpPart.top - rcCorners.top - rcCorners.bottom, bf);
        }
    }
    // right
    if( rcCorners.right > 0 ) {
        rcDest.left = rc.right - rcCorners.right;
        rcDest.top = rc.top + rcCorners.top;
        rcDest.right = rcCorners.right;
        rcDest.bottom = rc.bottom - rc.top - rcCorners.top - rcCorners.bottom;
        rcDest.right += rcDest.left;
        rcDest.bottom += rcDest.top;
        if (fn_IntersectRect(&rcTemp, &rcPaint, &rcDest)) {
            rcDest.right -= rcDest.left;
            rcDest.bottom -= rcDest.top;
            fn_AlphaBlend(hDC, rcDest.left, rcDest.top, rcDest.right, rcDest.bottom, hCloneDC, rcBmpPart.right - rcCorners.right, rcBmpPart.top + rcCorners.top, rcCorners.right, rcBmpPart.bottom - rcBmpPart.top - rcCorners.top - rcCorners.bottom, bf);
        }
    }
    // left-bottom
    if( rcCorners.left > 0 && rcCorners.bottom > 0 ) {
        rcDest.left = rc.left;
        rcDest.top = rc.bottom - rcCorners.bottom;
        rcDest.right = rcCorners.left;
        rcDest.bottom = rcCorners.bottom;
        rcDest.right += rcDest.left;
        rcDest.bottom += rcDest.top;
        if (fn_IntersectRect(&rcTemp, &rcPaint, &rcDest)) {
            rcDest.right -= rcDest.left;
            rcDest.bottom -= rcDest.top;
            fn_AlphaBlend(hDC, rcDest.left, rcDest.top, rcDest.right, rcDest.bottom, hCloneDC, rcBmpPart.left, rcBmpPart.bottom - rcCorners.bottom, rcCorners.left, rcCorners.bottom, bf);
        }
    }
    // bottom
    if( rcCorners.bottom > 0 ) {
        rcDest.left = rc.left + rcCorners.left;
        rcDest.top = rc.bottom - rcCorners.bottom;
        rcDest.right = rc.right - rc.left - rcCorners.left - rcCorners.right;
        rcDest.bottom = rcCorners.bottom;
        rcDest.right += rcDest.left;
        rcDest.bottom += rcDest.top;
        if (fn_IntersectRect(&rcTemp, &rcPaint, &rcDest)) {
            rcDest.right -= rcDest.left;
            rcDest.bottom -= rcDest.top;
            fn_AlphaBlend(hDC, rcDest.left, rcDest.top, rcDest.right, rcDest.bottom, hCloneDC, rcBmpPart.left + rcCorners.left, rcBmpPart.bottom - rcCorners.bottom, rcBmpPart.right - rcBmpPart.left - rcCorners.left - rcCorners.right, rcCorners.bottom, bf);
        }
    }
    // right-bottom
    if( rcCorners.right > 0 && rcCorners.bottom > 0 ) {
        rcDest.left = rc.right - rcCorners.right;
        rcDest.top = rc.bottom - rcCorners.bottom;
        rcDest.right = rcCorners.right;
        rcDest.bottom = rcCorners.bottom;
        rcDest.right += rcDest.left;
        rcDest.bottom += rcDest.top;
        if (fn_IntersectRect(&rcTemp, &rcPaint, &rcDest)) {
            rcDest.right -= rcDest.left;
            rcDest.bottom -= rcDest.top;
            fn_AlphaBlend(hDC, rcDest.left, rcDest.top, rcDest.right, rcDest.bottom, hCloneDC, rcBmpPart.right - rcCorners.right, rcBmpPart.bottom - rcCorners.bottom, rcCorners.right, rcCorners.bottom, bf);
        }
    }

    fn_SelectObject(hCloneDC, hOldBitmap);
    fn_DeleteDC(hCloneDC);
}


bool drawImage(HDC hDC, PaintManager* pManager, const RECT& rc, const RECT& rcPaint, const String& sImageName, RECT rcItem, RECT rcBmpPart, RECT rcCorner, BYTE bFade, bool bHole, bool bTiledX, bool bTiledY)
{
	if (sImageName.isEmpty()) {
		return false;
	}
	const TImageInfo* data = 0;
	data = pManager->getImageEx(sImageName);
    if (!data) {
        return false;
    }

	if (rcBmpPart.left == 0 && rcBmpPart.right == 0 && rcBmpPart.top == 0 && rcBmpPart.bottom == 0) {
		rcBmpPart.right = data->width;
		rcBmpPart.bottom = data->height;
	}
	if (rcBmpPart.right > data->width) {
		rcBmpPart.right = data->width;
	}
	if (rcBmpPart.bottom > data->height) {
		rcBmpPart.bottom = data->height;
	}

	RECT rcTemp;
    if (!fn_IntersectRect(&rcTemp, &rcItem, &rc)) {
        return true;
    }
    if (!fn_IntersectRect(&rcTemp, &rcItem, &rcPaint)) {
        return true;
    }

	RenderEngine::DrawImage(hDC, data->hBitmap, rcItem, rcPaint, rcBmpPart, rcCorner, bFade, bHole, bTiledX, bTiledY);

	return true;
}

bool RenderEngine::drawImageString(HDC hDC, PaintManager* pManager, const RECT& rc, const RECT& rcPaint, const String& imageName, const String& modifyImageName)
{
    if ((pManager == 0) || (hDC == 0)) {
        return false;
    }

    // 1. image.png
    // 2. file='image.png' dest='0,0,0,0' source='0,0,0,0' corner='0,0,0,0' fade='255' hole='false' xtiled='false' ytiled='false'

    String currImageName = imageName;
    RECT rcItem;
    RECT rcBmpPart;
    RECT rcCorner;
    BYTE bFade = 0xFF;
    bool bHole = false;
    bool bTiledX = false;
    bool bTiledY = false;
	int image_count = 0;

    StringArray attrs;

    __movsb((uint8_t*)&rcItem, (const uint8_t*)&rc, sizeof(rcItem));
    __stosb((uint8_t*)&rcBmpPart, 0, sizeof(rcBmpPart));
    __stosb((uint8_t*)&rcCorner, 0, sizeof(rcCorner));

    for (int i = 0; i < 2; ++i, image_count = 0) {
        attrs.clear();
        if (i == 0) {
            if (imageName.isEmpty()) {
                continue;
            }
            attrs.addTokens(imageName, " \t\r\n", "'");
        }
        else {
            if (modifyImageName.isEmpty()) {
                continue;
            }
			attrs.addTokens(modifyImageName, " \t\r\n", "'");
        }

        for (int j = 0; j < attrs.size(); ++j) {
            StringArray keyVal;

            keyVal.addTokens(attrs[j], "=", "'");

            if (keyVal.size() != 2) {
                continue;
            }

            if (keyVal[0] == "file") {
                if (image_count > 0) {
                    zgui::drawImage(hDC, pManager, rc, rcPaint, currImageName, rcItem, rcBmpPart, rcCorner, bFade, bHole, bTiledX, bTiledY);
                }

                currImageName = keyVal[1].substring(1, keyVal[1].length() - 1);
                ++image_count;
            }
            else if (keyVal[0] == "dest") {
                int val1, val2, val3, val4;
                String val = keyVal[1].substring(1, keyVal[1].length() - 1);
                if (!Helper::splitString(val, ",", String::empty, val1, val2, val3, val4)) {
                    continue;
                }

                rcItem.left = rc.left + val1;
                rcItem.top = rc.top + val2;
                rcItem.right = rc.left + val3;
                if (rcItem.right > rc.right) {
                    rcItem.right = rc.right;
                }

                rcItem.bottom = rc.top + val4;
                if (rcItem.bottom > rc.bottom) {
                    rcItem.bottom = rc.bottom;
                }
            }
            else if (keyVal[0] == "source") {
                String val = keyVal[1].substring(1, keyVal[1].length() - 1);
                if (!Helper::splitString(val, ",", String::empty, (int&)rcBmpPart.left, (int&)rcBmpPart.top, (int&)rcBmpPart.right, (int&)rcBmpPart.bottom)) {
                    continue;
                }
            }
            else if (keyVal[0] == "corner") {
                String val = keyVal[1].substring(1, keyVal[1].length() - 1);
                if (!Helper::splitString(val, ",", String::empty, (int&)rcCorner.left, (int&)rcCorner.top, (int&)rcCorner.right, (int&)rcCorner.bottom)) {
                    continue;
                }
            }
            else if (keyVal[0] == "fade") {
                bFade = (BYTE)keyVal[1].getIntValue();
            }
            else if (keyVal[0] == "hole") {
                bHole = (keyVal[1] == "true");
            }
            else if (keyVal[0] == "xtiled") {
				String val = keyVal[1].substring(1, keyVal[1].length() - 1);
				bTiledX = ((val == "true"));
            }
            else if (keyVal[0] == "ytiled") {
				String val = keyVal[1].substring(1, keyVal[1].length() - 1);
                bTiledY = ((val == "true"));
            }
        }
    }

	zgui::drawImage(hDC, pManager, rc, rcPaint, currImageName, rcItem, rcBmpPart, rcCorner, bFade, bHole, bTiledX, bTiledY);

    return true;
}

void RenderEngine::DrawColor(HDC hDC, const RECT& rc, DWORD color)
{
    if (color <= 0x00FFFFFF) {
        return;
    }
    if (color >= 0xFF000000) {
        fn_SetBkColor(hDC, RGB(GetBValue(color), GetGValue(color), GetRValue(color)));
        fn_ExtTextOutW(hDC, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
    }
    else {
        // Create a new 32bpp bitmap with room for an alpha channel
        BITMAPINFO bmi;
        __stosb((uint8_t*)&bmi, 0, sizeof(bmi));
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = 1;
        bmi.bmiHeader.biHeight = 1;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;
        bmi.bmiHeader.biSizeImage = 1 * 1 * sizeof(DWORD);
        LPDWORD pDest = NULL;
        HBITMAP hBitmap = fn_CreateDIBSection(hDC, &bmi, DIB_RGB_COLORS, (LPVOID*) &pDest, NULL, 0);
        if( !hBitmap ) return;

        *pDest = color;

        RECT rcBmpPart = {0, 0, 1, 1};
        RECT rcCorners;
		__stosb((uint8_t*)&rcCorners, 0, sizeof(rcCorners));
        DrawImage(hDC, hBitmap, rc, rc, rcBmpPart, rcCorners, 255);
        fn_DeleteObject(hBitmap);

    }
}

void RenderEngine::DrawGradient(HDC hDC, const RECT& rc, DWORD dwFirst, DWORD dwSecond, bool bVertical, int nSteps)
{
    BYTE bAlpha = (BYTE)(((dwFirst >> 24) + (dwSecond >> 24)) >> 1);
    if( bAlpha == 0 ) return;
    int cx = rc.right - rc.left;
    int cy = rc.bottom - rc.top;
    RECT rcPaint = rc;
    HDC hPaintDC = hDC;
    HBITMAP hPaintBitmap = NULL;
    HBITMAP hOldPaintBitmap = NULL;
    if( bAlpha < 255 ) {
        rcPaint.left = rcPaint.top = 0;
        rcPaint.right = cx;
        rcPaint.bottom = cy;
        hPaintDC = fn_CreateCompatibleDC(hDC);
        hPaintBitmap = fn_CreateCompatibleBitmap(hDC, cx, cy);
        zgui_assert(hPaintDC);
        zgui_assert(hPaintBitmap);
        hOldPaintBitmap = (HBITMAP) fn_SelectObject(hPaintDC, hPaintBitmap);
    }
    TRIVERTEX triv[2] = 
    {
        { rcPaint.left, rcPaint.top, GetBValue(dwFirst) << 8, GetGValue(dwFirst) << 8, GetRValue(dwFirst) << 8, 0xFF00 },
        { rcPaint.right, rcPaint.bottom, GetBValue(dwSecond) << 8, GetGValue(dwSecond) << 8, GetRValue(dwSecond) << 8, 0xFF00 }
    };
    GRADIENT_RECT grc = { 0, 1 };
    fn_GradientFill(hPaintDC, triv, 2, &grc, 1, bVertical ? GRADIENT_FILL_RECT_V : GRADIENT_FILL_RECT_H);

    if( bAlpha < 255 ) {
        BLENDFUNCTION bf = { AC_SRC_OVER, 0, bAlpha, AC_SRC_ALPHA };
        fn_AlphaBlend(hDC, rc.left, rc.top, cx, cy, hPaintDC, 0, 0, cx, cy, bf);
        fn_SelectObject(hPaintDC, hOldPaintBitmap);
        fn_DeleteObject(hPaintBitmap);
        fn_DeleteDC(hPaintDC);
    }
}

void RenderEngine::DrawLine(HDC hDC, const RECT& rc, int nSize, DWORD dwPenColor,int nStyle /*= PS_SOLID*/ )
{
    LOGPEN lg;
    lg.lopnColor = RGB(GetBValue(dwPenColor), GetGValue(dwPenColor), GetRValue(dwPenColor));
	lg.lopnStyle = nStyle;
    lg.lopnWidth.x = nSize;
    HPEN hPen = fn_CreatePenIndirect(&lg);
    HPEN hOldPen = (HPEN)fn_SelectObject(hDC, hPen);
    fn_MoveToEx(hDC, rc.left, rc.top, 0);
    fn_LineTo(hDC, rc.right, rc.bottom);
    fn_SelectObject(hDC, hOldPen);
    fn_DeleteObject(hPen);
     //zgui_assert(fn_GetObjectType(hDC)==OBJ_DC || fn_GetObjectType(hDC)==OBJ_MEMDC);
     //HPEN hPen = fn_CreatePen(PS_SOLID, nSize, RGB(GetBValue(dwPenColor), GetGValue(dwPenColor), GetRValue(dwPenColor)));
     //HPEN hOldPen = (HPEN)fn_SelectObject(hDC, hPen);
     //fn_MoveToEx(hDC, rc.left, rc.top, 0);
     //fn_LineTo(hDC, rc.right, rc.bottom);
     //fn_SelectObject(hDC, hOldPen);
     //fn_DeleteObject(hPen);
}

void RenderEngine::DrawRect(HDC hDC, const RECT& rc, int nSize, DWORD dwPenColor)
{
    zgui_assert(fn_GetObjectType(hDC)==OBJ_DC || fn_GetObjectType(hDC)==OBJ_MEMDC);
    HPEN hPen = fn_CreatePen(PS_SOLID | PS_INSIDEFRAME, nSize, RGB(GetBValue(dwPenColor), GetGValue(dwPenColor), GetRValue(dwPenColor)));
    HPEN hOldPen = (HPEN)fn_SelectObject(hDC, hPen);
    fn_SelectObject(hDC, fn_GetStockObject(HOLLOW_BRUSH));
    fn_Rectangle(hDC, rc.left, rc.top, rc.right, rc.bottom);
    fn_SelectObject(hDC, hOldPen);
    fn_DeleteObject(hPen);
}

void RenderEngine::DrawRoundRect(HDC hDC, const RECT& rc, int nSize, int width, int height, DWORD dwPenColor)
{
    zgui_assert(fn_GetObjectType(hDC)==OBJ_DC || fn_GetObjectType(hDC)==OBJ_MEMDC);
    HPEN hPen = fn_CreatePen(PS_SOLID | PS_INSIDEFRAME, nSize, RGB(GetBValue(dwPenColor), GetGValue(dwPenColor), GetRValue(dwPenColor)));
    HPEN hOldPen = (HPEN)fn_SelectObject(hDC, hPen);
    fn_SelectObject(hDC, fn_GetStockObject(HOLLOW_BRUSH));
    fn_RoundRect(hDC, rc.left, rc.top, rc.right, rc.bottom, width, height);
    fn_SelectObject(hDC, hOldPen);
    fn_DeleteObject(hPen);
}

void RenderEngine::DrawText(HDC hDC, PaintManager* pManager, RECT& rc, const String& pstrText, DWORD dwTextColor, int iFont, UINT uStyle)
{
    zgui_assert(fn_GetObjectType(hDC)==OBJ_DC || fn_GetObjectType(hDC)==OBJ_MEMDC);
    if (pstrText.isEmpty() || pManager == NULL ) {
        return;
    }

    fn_SetBkMode(hDC, TRANSPARENT);
    fn_SetTextColor(hDC, RGB(GetBValue(dwTextColor), GetGValue(dwTextColor), GetRValue(dwTextColor)));
    HFONT hOldFont = (HFONT)fn_SelectObject(hDC, pManager->GetFont(iFont));
    fn_DrawTextW(hDC, pstrText.toWideCharPointer(), -1, &rc, uStyle | DT_NOPREFIX);
    fn_SelectObject(hDC, hOldFont);
}

void RenderEngine::drawHtmlText(HDC hDC, PaintManager* pManager, RECT& rc, const String& text, DWORD dwTextColor, RECT* prcLinks, String* sLinks, int& nLinkRects, UINT uStyle)
{
    // The string formatter supports a kind of "mini-html" that consists of various short tags:
    //
    //   Bold:             <b>text</b>
    //   Color:            <c #xxxxxx>text</c>  where x = RGB in hex
    //   Font:             <f x>text</f>        where x = font id
    //   Italic:           <i>text</i>
    //   Image:            <i x y z>            where x = image name and y = imagelist num and z(optional) = imagelist id
    //   Link:             <a x>text</a>        where x(optional) = link content, normal like app:notepad or http:www.xxx.com
    //   NewLine           <n>                  
    //   Paragraph:        <p x>text</p>        where x = extra pixels indent in p
    //   Raw Text:         <r>text</r>
    //   Selected:         <s>text</s>
    //   Underline:        <u>text</u>
    //   X Indent:         <x i>                where i = hor indent in pixels
    //   Y Indent:         <y i>                where i = ver indent in pixels 

    zgui_assert(fn_GetObjectType(hDC) == OBJ_DC || fn_GetObjectType(hDC) == OBJ_MEMDC);
    if (text.isEmpty() || pManager == 0) {
        return;
    }
    if (fn_IsRectEmpty(&rc)) {
        return;
    }

    CharPointer_UTF16 itrText = text.getCharPointer();
    bool bDraw = ((uStyle & DT_CALCRECT) == 0);

	Array<void*> fontArray;
	Array<void*> colorArray;
	Array<void*> pIndentArray;

    RECT rcClip;
    __stosb((uint8_t*)&rcClip, 0, sizeof(rcClip));
    fn_GetClipBox(hDC, &rcClip);
    HRGN hOldRgn = fn_CreateRectRgnIndirect(&rcClip);
    HRGN hRgn = fn_CreateRectRgnIndirect(&rc);
    if (bDraw) {
        fn_ExtSelectClipRgn(hDC, hRgn, RGN_AND);
    }

    TEXTMETRIC* pTm = &pManager->GetDefaultFontInfo()->tm;
    HFONT hOldFont = (HFONT) fn_SelectObject(hDC, pManager->GetDefaultFontInfo()->hFont);
    fn_SetBkMode(hDC, TRANSPARENT);
    fn_SetTextColor(hDC, RGB(GetBValue(dwTextColor), GetGValue(dwTextColor), GetRValue(dwTextColor)));
    DWORD dwBkColor = pManager->GetDefaultSelectedBkColor();
    fn_SetBkColor(hDC, RGB(GetBValue(dwBkColor), GetGValue(dwBkColor), GetRValue(dwBkColor)));

    // If the drawstyle include a alignment, we'll need to first determine the text-size so
    // we can draw it at the correct position...
	if (((uStyle & DT_CENTER) != 0 || (uStyle & DT_RIGHT) != 0 || (uStyle & DT_VCENTER) != 0 || (uStyle & DT_BOTTOM) != 0) && (uStyle & DT_CALCRECT) == 0) {
		RECT rcText = {0, 0, 9999, 100};
		int nLinks = 0;
		drawHtmlText(hDC, pManager, rcText, text/*itrText*/, dwTextColor, NULL, NULL, nLinks, uStyle | DT_CALCRECT);
		if ((uStyle & DT_SINGLELINE) != 0) {
			if ((uStyle & DT_CENTER) != 0) {
				rc.left = rc.left + ((rc.right - rc.left) / 2) - ((rcText.right - rcText.left) / 2);
				rc.right = rc.left + (rcText.right - rcText.left);
			}
			if ((uStyle & DT_RIGHT) != 0) {
				rc.left = rc.right - (rcText.right - rcText.left);
			}
		}
		if ((uStyle & DT_VCENTER) != 0) {
			rc.top = rc.top + ((rc.bottom - rc.top) / 2) - ((rcText.bottom - rcText.top) / 2);
			rc.bottom = rc.top + (rcText.bottom - rcText.top);
		}
		if ((uStyle & DT_BOTTOM) != 0) {
			rc.top = rc.bottom - (rcText.bottom - rcText.top);
		}
	}

    bool bHoverLink = false;
    String sHoverLink;
    POINT ptMouse = pManager->GetMousePos();
    for (int i = 0; !bHoverLink && i < nLinkRects; ++i) {
        if (fn_PtInRect(prcLinks + i, ptMouse)) {
            sHoverLink = *(String*)(sLinks + i);
            bHoverLink = true;
        }
    }

    POINT pt = {rc.left, rc.top};
    int iLinkIndex = 0;
    int cyLine = pTm->tmHeight + pTm->tmExternalLeading + (int)pIndentArray.getLast();
    int cyMinHeight = 0;
    int cxMaxWidth = 0;
    POINT ptLinkStart;
    bool bLineEnd = false;
    bool bInRaw = false;
    bool bInLink = false;
    bool bInSelected = false;
    int iLineLinkIndex = 0;
	Array<void*> lineFontArray;
	Array<void*> lineColorArray;
	Array<void*> linePIndentArray;
    CharPointer_UTF16 pstrLineBegin = itrText;
    bool bLineInRaw = false;
    bool bLineInLink = false;
    bool bLineInSelected = false;
    int cyLineHeight = 0;
    bool bLineDraw = false;

    __stosb((uint8_t*)&ptLinkStart, 0, sizeof(ptLinkStart));
    
    while (*itrText != L'\0') {
        if (pt.x >= rc.right || *itrText == L'\n' || bLineEnd) {
            if (*itrText == L'\n') {
                ++itrText;
            }
            if (bLineEnd) {
                bLineEnd = false;
            }
            if (!bLineDraw) {
                if (bInLink && iLinkIndex < nLinkRects) {
                    fn_SetRect(&prcLinks[iLinkIndex++], ptLinkStart.x, ptLinkStart.y, MIN(pt.x, rc.right), pt.y + cyLine);
                    String *pStr1 = (String*)(sLinks + iLinkIndex - 1);
                    String *pStr2 = (String*)(sLinks + iLinkIndex);
                    *pStr2 = *pStr1;
                }
                for (int i = iLineLinkIndex; i < iLinkIndex; ++i) {
                    prcLinks[i].bottom = pt.y + cyLine;
                }
                if (bDraw) {
                    bInLink = bLineInLink;
                    iLinkIndex = iLineLinkIndex;
                }
            }
            else {
                if (bInLink && iLinkIndex < nLinkRects) {
                    ++iLinkIndex;
                }
                bLineInLink = bInLink;
                iLineLinkIndex = iLinkIndex;
            }
            if ((uStyle & DT_SINGLELINE) != 0 && (!bDraw || bLineDraw)) {
                break;
            }
            if (bDraw) {
                bLineDraw = !bLineDraw; // !
            }
            pt.x = rc.left;
            if (!bLineDraw ) {
                pt.y += cyLine;
            }
            if (pt.y > rc.bottom && bDraw) {
                break;
            }
            ptLinkStart = pt;
            cyLine = pTm->tmHeight + pTm->tmExternalLeading + (int)pIndentArray.getLast();
            if (pt.x >= rc.right) {
                break;
            }
        }
        else if (!bInRaw && (*itrText == L'<' || *itrText == L'{') && (itrText[1] >= L'a' && itrText[1] <= L'z')
            && (itrText[2] == L' ' || itrText[2] == L'>' || itrText[2] == L'}')) {
                itrText++;
                CharPointer_UTF16 pstrNextStart = String::empty.getCharPointer();
                switch (*itrText) {
                    case L'a':  // Link
                        {
                            while (*(++itrText) > L'\0' && *itrText <= L' ');
                            if (iLinkIndex < nLinkRects && !bLineDraw) {
                                String *pStr = (String*)(sLinks + iLinkIndex);
                                *pStr = String::empty;
                                while (*itrText != L'\0' && *itrText != L'>' && *itrText != L'}') {
                                    CharPointer_UTF16 pstrTemp = itrText + 1;
                                    while (itrText < pstrTemp) {
                                        *pStr += *itrText++;
                                    }
                                }
                            }

                            DWORD clrColor = pManager->GetDefaultLinkFontColor();
                            if (bHoverLink && iLinkIndex < nLinkRects ) {
                                String *pStr = (String*)(sLinks + iLinkIndex);
                                if (sHoverLink == *pStr) {
                                    clrColor = pManager->GetDefaultLinkHoverFontColor();
                                }
                            }
                            //else if( prcLinks == NULL ) {
                            //    if( fn_PtInRect(&rc, ptMouse) )
                            //        clrColor = pManager->GetDefaultLinkHoverFontColor();
                            //}
                            colorArray.add((LPVOID)clrColor);
                            fn_SetTextColor(hDC,  RGB(GetBValue(clrColor), GetGValue(clrColor), GetRValue(clrColor)));
                            TFontInfo* pFontInfo = pManager->GetDefaultFontInfo();
                            if (fontArray.size() > 0) {
                                pFontInfo = (TFontInfo*)fontArray.getLast();
                            }
                            if (pFontInfo->bUnderline == false) {
                                HFONT hFont = pManager->GetFont(pFontInfo->sFontName, pFontInfo->iSize, pFontInfo->bBold, true, pFontInfo->bItalic);
                                if (hFont == 0) {
                                    hFont = pManager->AddFont(pFontInfo->sFontName, pFontInfo->iSize, pFontInfo->bBold, true, pFontInfo->bItalic);
                                }
                                pFontInfo = pManager->GetFontInfo(hFont);
                                fontArray.add(pFontInfo);
                                pTm = &pFontInfo->tm;
                                fn_SelectObject(hDC, pFontInfo->hFont);
                                cyLine = MAX(cyLine, pTm->tmHeight + pTm->tmExternalLeading + (int)pIndentArray.getLast());
                            }
                            ptLinkStart = pt;
                            bInLink = true;
                        }
                        break;
                    case L'b':  // Bold
                        {
                            itrText++;
                            TFontInfo* pFontInfo = pManager->GetDefaultFontInfo();
                            if (fontArray.size() > 0) {
                                pFontInfo = (TFontInfo*)fontArray.getLast();
                            }
                            if (pFontInfo->bBold == false) {
                                HFONT hFont = pManager->GetFont(pFontInfo->sFontName, pFontInfo->iSize, true, pFontInfo->bUnderline, pFontInfo->bItalic);
                                if( hFont == NULL ) hFont = pManager->AddFont(pFontInfo->sFontName, pFontInfo->iSize, true, pFontInfo->bUnderline, pFontInfo->bItalic);
                                pFontInfo = pManager->GetFontInfo(hFont);
                                fontArray.add(pFontInfo);
                                pTm = &pFontInfo->tm;
                                fn_SelectObject(hDC, pFontInfo->hFont);
                                cyLine = MAX(cyLine, pTm->tmHeight + pTm->tmExternalLeading + (int)pIndentArray.getLast());
                            }
                        }
                        break;
                    case L'c':  // Color
                        {
                            while (*(++itrText) > L'\0' && *itrText <= L' ');
                            DWORD clrColor = HexConverter<int>::stringToHex(itrText);
                            colorArray.add((LPVOID)clrColor);
                            fn_SetTextColor(hDC, RGB(GetBValue(clrColor), GetGValue(clrColor), GetRValue(clrColor)));
                        }
                        break;
                    case L'f':  // Font
                        {
                            while (*(++itrText) > L'\0' && *itrText <= L' ');
                            CharPointer_UTF16 pstrTemp = itrText;
                            int iFont = (int) HexConverter<int>::stringToHex(itrText);
                            //if( isdigit(*pstrText) ) { // debug版本会引起异常
                            if (pstrTemp != itrText) {
                                TFontInfo* pFontInfo = pManager->GetFontInfo(iFont);
                                fontArray.add(pFontInfo);
                                pTm = &pFontInfo->tm;
                                fn_SelectObject(hDC, pFontInfo->hFont);
                            }
                            else {
                                String sFontName;
                                int iFontSize = 10;
                                String sFontAttr;
                                bool bBold = false;
                                bool bUnderline = false;
                                bool bItalic = false;
                                while (*itrText != L'\0' && *itrText != L'>' && *itrText != L'}' && *itrText != L' ') {
                                    pstrTemp = itrText + 1;
                                    while (itrText < pstrTemp) {
                                        sFontName += *itrText++;
                                    }
                                }
                                for ( ; *itrText > L'\0' && *itrText <= L' '; ++itrText);
                                if (isdigit(*itrText)) {
                                    iFontSize = itrText.getIntValue32();
                                }
                                while (*itrText > L'\0' && *itrText <= L' ') {
                                    ++itrText;
                                }
                                while (*itrText != L'\0' && *itrText != L'>' && *itrText != L'}') {
                                    pstrTemp = itrText + 1;
                                    while( itrText < pstrTemp) {
                                        sFontAttr += *itrText++;
                                    }
                                }
                                sFontAttr = sFontAttr.toLowerCase();
                                if (sFontAttr.contains("bold")) {
                                    bBold = true;
                                }
                                if (sFontAttr.contains("underline")) {
                                    bUnderline = true;
                                }
                                if (sFontAttr.contains("italic")) {
                                    bItalic = true;
                                }

                                HFONT hFont = pManager->GetFont(sFontName, iFontSize, bBold, bUnderline, bItalic);
                                if (hFont == NULL) {
                                    hFont = pManager->AddFont(sFontName, iFontSize, bBold, bUnderline, bItalic);
                                }
                                TFontInfo* pFontInfo = pManager->GetFontInfo(hFont);
                                fontArray.add(pFontInfo);
                                pTm = &pFontInfo->tm;
                                fn_SelectObject(hDC, pFontInfo->hFont);
                            }
                            cyLine = MAX(cyLine, pTm->tmHeight + pTm->tmExternalLeading + (int)pIndentArray.getLast());
                        }
                        break;
                    case L'i':  // Italic or Image
                        {    
                            pstrNextStart = itrText - 1;
                            itrText++;
					        String sImageString = itrText;
                            int iWidth = 0;
                            int iHeight = 0;
                            for ( ; *itrText > L'\0' && *itrText <= L' '; ++itrText);
                            const TImageInfo* pImageInfo = 0;
                            String sName;
                            while (*itrText != L'\0' && *itrText != L'>' && *itrText != L'}' && *itrText != L' ') {
                                LPCTSTR pstrTemp = itrText + 1;
                                while( itrText < pstrTemp) {
                                    sName += *itrText++;
                                }
                            }
                            if (sName.isEmpty()) { // Italic
                                pstrNextStart = String::empty.getCharPointer();
                                TFontInfo* pFontInfo = pManager->GetDefaultFontInfo();
								if (fontArray.size() > 0) {
									pFontInfo = (TFontInfo*)fontArray.getLast();
								}
                                if (pFontInfo->bItalic == false) {
                                    HFONT hFont = pManager->GetFont(pFontInfo->sFontName, pFontInfo->iSize, pFontInfo->bBold, pFontInfo->bUnderline, true);
									if (hFont == 0) {
										hFont = pManager->AddFont(pFontInfo->sFontName, pFontInfo->iSize, pFontInfo->bBold, pFontInfo->bUnderline, true);
									}
                                    pFontInfo = pManager->GetFontInfo(hFont);
                                    fontArray.add(pFontInfo);
                                    pTm = &pFontInfo->tm;
                                    fn_SelectObject(hDC, pFontInfo->hFont);
                                    cyLine = MAX(cyLine, pTm->tmHeight + pTm->tmExternalLeading + (int)pIndentArray.getLast());
                                }
                            }
                            else {
                                while (*itrText > L'\0' && *itrText <= L' ') {
                                    ++itrText;
                                }
                                int iImageListNum = itrText.getIntValue32();
                                if (iImageListNum <= 0) {
                                    iImageListNum = 1;
                                }
                                while (*itrText > L'\0' && *itrText <= L' ') {
                                    ++itrText;
                                }
						        int iImageListIndex = itrText.getIntValue32();
                                if (iImageListIndex < 0 || iImageListIndex >= iImageListNum) {
                                    iImageListIndex = 0;
                                }

						        if (sImageString.contains("file=\'")) {
							        String sImageName;
							        CharPointer_UTF16 pStrImage = sImageString.getCharPointer();
							        String sItem;
							        String sValue;
							        while (*pStrImage != L'\0') {
                                        sItem = String::empty;
                                        sValue = String::empty;
                                        while (*pStrImage > L'\0' && *pStrImage <= L' ') {
                                            ++pStrImage;
                                        }
								        while (*pStrImage != L'\0' && *pStrImage != L'=' && *pStrImage > L' ') {
									        CharPointer_UTF16 pstrTemp = pStrImage + 1;
									        while (pStrImage < pstrTemp) {
										        sItem += *pStrImage++;
									        }
								        }
                                        while (*pStrImage > L'\0' && *pStrImage <= L' ') {
                                            ++pStrImage;
                                        }
                                        if (*pStrImage++ != L'=') {
                                            break;
                                        }
                                        while (*pStrImage > L'\0' && *pStrImage <= L' ') {
                                            ++pStrImage;
                                        }
                                        if (*pStrImage++ != L'\'') {
                                            break;
                                        }
								        while (*pStrImage != L'\0' && *pStrImage != L'\'') {
									        CharPointer_UTF16 pstrTemp = pStrImage + 1;
									        while (pStrImage < pstrTemp) {
										        sValue += *pStrImage++;
									        }
								        }
                                        if (*pStrImage++ != L'\'') {
                                            break;
                                        }
								        if (!sValue.isEmpty()) {
									        if (sItem == "file") {
										        sImageName = sValue;
									        }
								        }
                                        if (*pStrImage++ != L' ') {
                                            break;
                                        }
							        }

							        pImageInfo = pManager->getImageEx(sImageName);
						        }
                                else {
							        pImageInfo = pManager->getImageEx(sName);
                                }

						        if (pImageInfo) {
							        iWidth = pImageInfo->width;
							        iHeight = pImageInfo->height;
							        if( iImageListNum > 1 ) iWidth /= iImageListNum;

                                    if( pt.x + iWidth > rc.right && pt.x > rc.left && (uStyle & DT_SINGLELINE) == 0 ) {
                                        bLineEnd = true;
                                    }
                                    else {
                                        pstrNextStart = NULL;
                                        if( bDraw && bLineDraw ) {
                                            Rect rcImage(pt.x, pt.y + cyLineHeight - iHeight, pt.x + iWidth, pt.y + cyLineHeight);
                                            if( iHeight < cyLineHeight ) { 
                                                rcImage.bottom -= (cyLineHeight - iHeight) / 2;
                                                rcImage.top = rcImage.bottom -  iHeight;
                                            }
                                            Rect rcBmpPart(0, 0, iWidth, iHeight);
                                            rcBmpPart.left = iWidth * iImageListIndex;
                                            rcBmpPart.right = iWidth * (iImageListIndex + 1);
                                            Rect rcCorner(0, 0, 0, 0);
                                            DrawImage(hDC, pImageInfo->hBitmap, rcImage, rcImage, rcBmpPart, rcCorner, 255);
                                        }

                                        cyLine = MAX(iHeight, cyLine);
                                        pt.x += iWidth;
                                        cyMinHeight = pt.y + iHeight;
                                        cxMaxWidth = MAX(cxMaxWidth, pt.x);
                                    }
                                }
                                else pstrNextStart = NULL;
                            }
                        }
                        break;
                    case L'n':  // Newline
                        {
                            ++itrText;
							if ((uStyle & DT_SINGLELINE) != 0) {
								break;
							}
                            bLineEnd = true;
                        }
                        break;
                    case L'p':  // Paragraph
                        {
                            itrText++;
                            if( pt.x > rc.left ) bLineEnd = true;
                            while( *itrText > L'\0' && *itrText <= L' ') {
                                ++itrText;
                            }
                            int cyLineExtra = itrText.getIntValue32();
                            pIndentArray.add((LPVOID)cyLineExtra);
                            cyLine = MAX(cyLine, pTm->tmHeight + pTm->tmExternalLeading + cyLineExtra);
                        }
                        break;
                    case L'r':  // Raw Text
                        {
                            itrText++;
                            bInRaw = true;
                        }
                        break;
                    case L's':  // Selected text background color
                        {
                            itrText++;
                            bInSelected = !bInSelected;
                            if( bDraw && bLineDraw ) {
                                if (bInSelected) {
                                    fn_SetBkMode(hDC, OPAQUE);
                                }
                                else {
                                    fn_SetBkMode(hDC, TRANSPARENT);
                                }
                            }
                        }
                        break;
                    case L'u':  // Underline text
                        {
                            itrText++;
                            TFontInfo* pFontInfo = pManager->GetDefaultFontInfo();
							if (fontArray.size() > 0) {
								pFontInfo = (TFontInfo*)fontArray.getLast();
							}
                            if (pFontInfo->bUnderline == false ) {
                                HFONT hFont = pManager->GetFont(pFontInfo->sFontName, pFontInfo->iSize, pFontInfo->bBold, true, pFontInfo->bItalic);
                                if( hFont == NULL ) hFont = pManager->AddFont(pFontInfo->sFontName, pFontInfo->iSize, pFontInfo->bBold, true, pFontInfo->bItalic);
                                pFontInfo = pManager->GetFontInfo(hFont);
                                fontArray.add(pFontInfo);
                                pTm = &pFontInfo->tm;
                                fn_SelectObject(hDC, pFontInfo->hFont);
                                cyLine = MAX(cyLine, pTm->tmHeight + pTm->tmExternalLeading + (int)pIndentArray.getLast());
                            }
                        }
                        break;
                    case L'x':  // X Indent
                        {
                            itrText++;
                            while (*itrText > L'\0' && *itrText <= L' ') {
                                ++itrText;
                            }
                            int iWidth = itrText.getIntValue32();
                            pt.x += iWidth;
                            cxMaxWidth = MAX(cxMaxWidth, pt.x);
                        }
                        break;
                    case L'y':  // Y Indent
                        {
                            itrText++;
                            while (*itrText > L'\0' && *itrText <= L' ') {
                                ++itrText;
                            }
                            cyLine = itrText.getIntValue32();
                        }
                        break;
                }
                if (pstrNextStart != String::empty.getCharPointer()) {
                    itrText = pstrNextStart;
                }
                else {
                    while (*itrText != L'\0' && *itrText != L'>' && *itrText != L'}') {
                        ++itrText;
                    }
                    ++itrText;
                }
        }
        else if (!bInRaw && ( *itrText == L'<' || *itrText == L'{') && itrText[1] == L'/') {
            itrText++;
            itrText++;
            switch (*itrText) {
                case L'c':
                    {
                        itrText++;
                        colorArray.removeLast();
                        DWORD clrColor = dwTextColor;
                        if (colorArray.size() > 0) {
                            clrColor = (int)colorArray.getLast();
                        }
                        fn_SetTextColor(hDC, RGB(GetBValue(clrColor), GetGValue(clrColor), GetRValue(clrColor)));
                    }
                    break;
                case L'p':
                    itrText++;
                    if (pt.x > rc.left) {
                        bLineEnd = true;
                    }
                    pIndentArray.removeLast();
                    cyLine = MAX(cyLine, pTm->tmHeight + pTm->tmExternalLeading + (int)pIndentArray.getLast());
                    break;
                case L's':
                    {
                        itrText++;
                        bInSelected = !bInSelected;
                        if (bDraw && bLineDraw) {
                            if (bInSelected) {
                                fn_SetBkMode(hDC, OPAQUE);
                            }
                            else {
                                fn_SetBkMode(hDC, TRANSPARENT);
                            }
                        }
                    }
                    break;
                case L'a':
                    {
                        if (iLinkIndex < nLinkRects) {
                            if (!bLineDraw) {
                                fn_SetRect(&prcLinks[iLinkIndex], ptLinkStart.x, ptLinkStart.y, MIN(pt.x, rc.right), pt.y + pTm->tmHeight + pTm->tmExternalLeading);
                            }
                            iLinkIndex++;
                        }
                        colorArray.removeLast();
                        DWORD clrColor = dwTextColor;
                        if (colorArray.size() > 0) {
                            clrColor = (int)colorArray.getLast();
                        }
                        fn_SetTextColor(hDC, RGB(GetBValue(clrColor), GetGValue(clrColor), GetRValue(clrColor)));
                        bInLink = false;
                    }
                case L'b':
                case L'f':
                case L'i':
                case L'u':
                    {
                        itrText++;
                        fontArray.removeLast();
                        TFontInfo* pFontInfo = (TFontInfo*)fontArray.getLast();
                        if (pFontInfo == 0) {
                            pFontInfo = pManager->GetDefaultFontInfo();
                        }
                        if (pTm->tmItalic && pFontInfo->bItalic == false) {
                            ABC abc;
                            fn_GetCharABCWidthsW(hDC, L' ', L' ', &abc);
                            pt.x += abc.abcC / 2; // 简单修正一下斜体混排的问题, 正确做法应该是http://support.microsoft.com/kb/244798/en-us
                        }
                        pTm = &pFontInfo->tm;
                        fn_SelectObject(hDC, pFontInfo->hFont);
                        cyLine = MAX(cyLine, pTm->tmHeight + pTm->tmExternalLeading + (int)pIndentArray.getLast());
                    }
                    break;
            }
            while (*itrText != L'\0' && *itrText != L'>' && *itrText != L'}') {
                ++itrText;
            }
            ++itrText;
        }
        else if (!bInRaw &&  *itrText == L'<' && itrText[2] == L'>' && (itrText[1] == L'{'  || itrText[1] == L'}')) {
            SIZE szSpace;
            __stosb((uint8_t*)&szSpace, 0, sizeof(szSpace));
            fn_GetTextExtentPoint32W(hDC, itrText + 1, 1, &szSpace);
            if (bDraw && bLineDraw) {
                fn_TextOutW(hDC, pt.x, pt.y + cyLineHeight - pTm->tmHeight - pTm->tmExternalLeading, itrText + 1, 1);
            }
            pt.x += szSpace.cx;
            cxMaxWidth = MAX(cxMaxWidth, pt.x);
            itrText += 3;
        }
        else if (!bInRaw &&  *itrText == L'{' && itrText[2] == L'}' && (itrText[1] == L'<'  || itrText[1] == L'>')) {
            SIZE szSpace;
            __stosb((uint8_t*)&szSpace, 0, sizeof(szSpace));
            fn_GetTextExtentPoint32W(hDC, itrText + 1, 1, &szSpace);
            if( bDraw && bLineDraw ) fn_TextOutW(hDC, pt.x,  pt.y + cyLineHeight - pTm->tmHeight - pTm->tmExternalLeading, itrText + 1, 1);
            pt.x += szSpace.cx;
            cxMaxWidth = MAX(cxMaxWidth, pt.x);
            itrText += 3;
        }
        else if (!bInRaw &&  *itrText == L' ') {
            SIZE szSpace;
            __stosb((uint8_t*)&szSpace, 0, sizeof(szSpace));
            fn_GetTextExtentPoint32W(hDC, L" ", 1, &szSpace);
            // Still need to paint the space because the font might have
            // underline formatting.
            if (bDraw && bLineDraw) {
                fn_TextOutW(hDC, pt.x,  pt.y + cyLineHeight - pTm->tmHeight - pTm->tmExternalLeading, L" ", 1);
            }
            pt.x += szSpace.cx;
            cxMaxWidth = MAX(cxMaxWidth, pt.x);
            itrText++;
        }
        else {
            POINT ptPos = pt;
            int cchChars = 0;
            int cchSize = 0;
            int cchLastGoodWord = 0;
            int cchLastGoodSize = 0;
            CharPointer_UTF16 p = itrText;
            CharPointer_UTF16 pstrNext = itrText;
            SIZE szText;
            __stosb((uint8_t*)&szText, 0, sizeof(szText));
            if (!bInRaw && *p == L'<' || *p == L'{') {
                p++, cchChars++, cchSize++;
            }
            while (*p != L'\0' && *p != L'\n') {
                // This part makes sure that we're word-wrapping if needed or providing support
                // for DT_END_ELLIPSIS. Unfortunately the GetTextExtentPoint32() call is pretty
                // slow when repeated so often.
                // TODO: Rewrite and use GetTextExtentExPoint() instead!
                if (bInRaw) {
                    if ((*p == L'<' || *p == L'{') && p[1] == L'/' && p[2] == L'r' && (p[3] == L'>' || p[3] == L'}')) {
                        p += 4;
                        bInRaw = false;
                        break;
                    }
                }
                else {
                    if (*p == L'<' || *p == L'{') {
                        break;
                    }
                }
                pstrNext = p + 1;
                cchChars++;
                cchSize += (int)(pstrNext - p);
                szText.cx = cchChars * pTm->tmMaxCharWidth;
                if (pt.x + szText.cx >= rc.right) {
                    fn_GetTextExtentPoint32W(hDC, itrText, cchSize, &szText);
                }
                if (pt.x + szText.cx > rc.right) {
                    if (pt.x + szText.cx > rc.right && pt.x != rc.left) {
                        cchChars--;
                        cchSize -= (int)(pstrNext - p);
                    }
                    if ((uStyle & DT_WORDBREAK) != 0 && cchLastGoodWord > 0) {
                        cchChars = cchLastGoodWord;
                        cchSize = cchLastGoodSize;                 
                    }
                    if ((uStyle & DT_END_ELLIPSIS) != 0 && cchChars > 0) {
                        cchChars -= 1;
                        LPCTSTR pstrPrev = p - 1;
                        if( cchChars > 0 ) {
                            cchChars -= 1;
                            pstrPrev = pstrPrev - 1;
                            cchSize -= (int)(p - pstrPrev);
                        }
                        else 
                            cchSize -= (int)(p - pstrPrev);
                        pt.x = rc.right;
                    }
                    bLineEnd = true;
                    cxMaxWidth = MAX(cxMaxWidth, pt.x);
                    break;
                }
                if (!((p[0] >= L'a' && p[0] <= L'z') || ( p[0] >= L'A' && p[0] <= L'Z'))) {
                    cchLastGoodWord = cchChars;
                    cchLastGoodSize = cchSize;
                }
                if (*p == L' ') {
                    cchLastGoodWord = cchChars;
                    cchLastGoodSize = cchSize;
                }
                ++p;
            }
            
			fn_GetTextExtentPoint32W(hDC, itrText, cchSize, &szText);
            if (bDraw && bLineDraw) {
				if ((uStyle & DT_SINGLELINE) == 0 && (uStyle & DT_CENTER) != 0) {
					ptPos.x += (rc.right - rc.left - szText.cx) / 2;
				}
				else if ((uStyle & DT_SINGLELINE) == 0 && (uStyle & DT_RIGHT) != 0) {
					ptPos.x += (rc.right - rc.left - szText.cx);
				}
				fn_TextOutW(hDC, ptPos.x, ptPos.y + cyLineHeight - pTm->tmHeight - pTm->tmExternalLeading, itrText, cchSize);
                if (pt.x >= rc.right && (uStyle & DT_END_ELLIPSIS) != 0)  {
                    fn_TextOutW(hDC, ptPos.x + szText.cx, ptPos.y, L"...", 3);
                }
            }
            pt.x += szText.cx;
            cxMaxWidth = MAX(cxMaxWidth, pt.x);
            itrText += cchSize;
        }

        if ( pt.x >= rc.right || *itrText == L'\n' || *itrText == L'\0') {
            bLineEnd = true;
        }
        if (bDraw && bLineEnd) {
            if (!bLineDraw) {
                fontArray.resize(lineFontArray.size());
				__movsb((uint8_t*)fontArray.getRawDataPointer(), (const uint8_t*)lineFontArray.getRawDataPointer(), lineFontArray.size() * sizeof(LPVOID));
                colorArray.resize(lineColorArray.size());
				__movsb((uint8_t*)colorArray.getRawDataPointer(), (const uint8_t*)lineColorArray.getRawDataPointer(), lineColorArray.size() * sizeof(LPVOID));
                pIndentArray.resize(linePIndentArray.size());
				__movsb((uint8_t*)pIndentArray.getRawDataPointer(), (const uint8_t*)linePIndentArray.getRawDataPointer(), linePIndentArray.size() * sizeof(LPVOID));

                cyLineHeight = cyLine;
                itrText = pstrLineBegin;
                bInRaw = bLineInRaw;
                bInSelected = bLineInSelected;

                DWORD clrColor = dwTextColor;
				if (colorArray.size() > 0) {
					clrColor = (int)colorArray.getLast();
				}
                fn_SetTextColor(hDC, RGB(GetBValue(clrColor), GetGValue(clrColor), GetRValue(clrColor)));
                TFontInfo* pFontInfo = (TFontInfo*)fontArray.getLast();
				if (pFontInfo == 0) {
					pFontInfo = pManager->GetDefaultFontInfo();
				}
                pTm = &pFontInfo->tm;
                fn_SelectObject(hDC, pFontInfo->hFont);
                if (bInSelected) {
                    fn_SetBkMode(hDC, OPAQUE);
                }
            }
            else {
                lineFontArray.resize(fontArray.size());
				__movsb((uint8_t*)lineFontArray.getRawDataPointer(), (const uint8_t*)fontArray.getRawDataPointer(), fontArray.size() * sizeof(LPVOID));
				lineColorArray.resize(colorArray.size());
				__movsb((uint8_t*)lineColorArray.getRawDataPointer(), (const uint8_t*)colorArray.getRawDataPointer(), colorArray.size() * sizeof(LPVOID));
				linePIndentArray.resize(pIndentArray.size());
				__movsb((uint8_t*)linePIndentArray.getRawDataPointer(), (const uint8_t*)pIndentArray.getRawDataPointer(), pIndentArray.size() * sizeof(LPVOID));
                pstrLineBegin = itrText;
                bLineInSelected = bInSelected;
                bLineInRaw = bInRaw;
            }
        }

        zgui_assert(iLinkIndex<=nLinkRects);
    }

    nLinkRects = iLinkIndex;

    // Return size of text when requested
    if( (uStyle & DT_CALCRECT) != 0 ) {
        rc.bottom = MAX(cyMinHeight, pt.y + cyLine);
        rc.right = MIN(rc.right, cxMaxWidth);
    }

    if( bDraw ) {
        fn_SelectClipRgn(hDC, hOldRgn);
    }
    fn_DeleteObject(hOldRgn);
    fn_DeleteObject(hRgn);

    fn_SelectObject(hDC, hOldFont);
}

HBITMAP RenderEngine::GenerateBitmap(PaintManager* pManager, Control* pControl, RECT rc)
{
    int cx = rc.right - rc.left;
    int cy = rc.bottom - rc.top;

    HDC hPaintDC = fn_CreateCompatibleDC(pManager->GetPaintDC());
    HBITMAP hPaintBitmap = fn_CreateCompatibleBitmap(pManager->GetPaintDC(), rc.right, rc.bottom);
    zgui_assert(hPaintDC);
    zgui_assert(hPaintBitmap);
    HBITMAP hOldPaintBitmap = (HBITMAP)fn_SelectObject(hPaintDC, hPaintBitmap);
    pControl->DoPaint(hPaintDC, rc);

    BITMAPINFO bmi;
    __stosb((uint8_t*)&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = cx;
    bmi.bmiHeader.biHeight = cy;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biSizeImage = cx * cy * sizeof(DWORD);
    LPDWORD pDest = NULL;
    HDC hCloneDC = fn_CreateCompatibleDC(pManager->GetPaintDC());
    HBITMAP hBitmap = fn_CreateDIBSection(pManager->GetPaintDC(), &bmi, DIB_RGB_COLORS, (LPVOID*) &pDest, NULL, 0);
    zgui_assert(hCloneDC);
    zgui_assert(hBitmap);
    if (hBitmap != NULL) {
        HBITMAP hOldBitmap = (HBITMAP) fn_SelectObject(hCloneDC, hBitmap);
        fn_BitBlt(hCloneDC, 0, 0, cx, cy, hPaintDC, rc.left, rc.top, SRCCOPY);
        fn_SelectObject(hCloneDC, hOldBitmap);
        fn_DeleteDC(hCloneDC);  
        fn_GdiFlush();
    }

    // Cleanup
    fn_SelectObject(hPaintDC, hOldPaintBitmap);
    fn_DeleteObject(hPaintBitmap);
    fn_DeleteDC(hPaintDC);

    return hBitmap;
}

SIZE RenderEngine::GetTextSize(HDC hDC, PaintManager* pManager, const String& pstrText, int iFont, UINT uStyle)
{
	SIZE size;
	__stosb((uint8_t*)&size, 0, sizeof(size));
	zgui_assert(fn_GetObjectType(hDC)==OBJ_DC || fn_GetObjectType(hDC)==OBJ_MEMDC);
	if (pManager == 0) {
		return size;
	}
	fn_SetBkMode(hDC, TRANSPARENT);
	HFONT hOldFont = (HFONT)fn_SelectObject(hDC, pManager->GetFont(iFont));
	fn_GetTextExtentPoint32W(hDC, pstrText.toWideCharPointer(), pstrText.length() , &size);
	fn_SelectObject(hDC, hOldFont);
	return size;
}

} // namespace zgui
