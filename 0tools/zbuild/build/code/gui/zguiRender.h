#ifndef __UIRENDER_H__
#define __UIRENDER_H__

#pragma once

namespace zgui {

class CRenderClip
{
public:
    ~CRenderClip();
    RECT rcItem;
    HDC hDC;
    HRGN hRgn;
    HRGN hOldRgn;

    static void GenerateClip(HDC hDC, RECT rc, CRenderClip& clip);
    static void GenerateRoundClip(HDC hDC, RECT rc, RECT rcItem, int width, int height, CRenderClip& clip);
    static void UseOldClipBegin(HDC hDC, CRenderClip& clip);
    static void UseOldClipEnd(HDC hDC, CRenderClip& clip);
};

/////////////////////////////////////////////////////////////////////////////////////
//

class CRenderEngine
{
public:
    static DWORD AdjustColor(DWORD dwColor, short H, short S, short L);
    static TImageInfo* LoadImage(const String& bitmap, DWORD mask = 0);
    static void FreeImage(const TImageInfo* bitmap);
    static void DrawImage(HDC hDC, Gdiplus::Bitmap* pBitmap, const RECT& rc, const RECT& rcPaint, const RECT& rcBmpPart, const RECT& rcCorners, bool alphaChannel, BYTE uFade = 255, bool hole = false, bool xtiled = false, bool ytiled = false);
    static bool DrawImageString(HDC hDC, CPaintManagerUI* pManager, const RECT& rcItem, const RECT& rcPaint, const String& pStrImage, const String& pStrModify = String::empty);
    static void DrawColor(HDC hDC, const RECT& rc, DWORD color);
    static void DrawGradient(HDC hDC, const RECT& rc, DWORD dwFirst, DWORD dwSecond, bool bVertical, int nSteps);

    static void DrawLine(HDC hDC, const RECT& rc, int nSize, DWORD dwPenColor,int nStyle = PS_SOLID);
    static void DrawRect(HDC hDC, const RECT& rc, int nSize, DWORD dwPenColor);
    static void DrawRoundRect(HDC hDC, const RECT& rc, int width, int height, int nSize, DWORD dwPenColor);
    static void DrawText(HDC hDC, CPaintManagerUI* pManager, RECT& rc, const String& pstrText, DWORD dwTextColor, int iFont, UINT uStyle);
    static void DrawHtmlText(HDC hDC, CPaintManagerUI* pManager, RECT& rc, const String& pstrText, DWORD dwTextColor, RECT* pLinks, String* sLinks, int& nLinkRects, UINT uStyle);
    static HBITMAP GenerateBitmap(CPaintManagerUI* pManager, CControlUI* pControl, RECT rc);
};

} // namespace zgui

#endif // __UIRENDER_H__
