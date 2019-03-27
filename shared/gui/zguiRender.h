#ifndef __ZGUI_RENDER_H_
#define __ZGUI_RENDER_H_

#pragma once

namespace zgui {

class RenderClip
{
public:
    ~RenderClip();

    static void GenerateClip(HDC hDC, RECT rc, RenderClip& clip);
    static void GenerateRoundClip(HDC hDC, RECT rc, RECT rcItem, int width, int height, RenderClip& clip);
    static void UseOldClipBegin(HDC hDC, RenderClip& clip);
    static void UseOldClipEnd(HDC hDC, RenderClip& clip);

	RECT rcItem;
	HDC hDC;
	HRGN hRgn;
	HRGN hOldRgn;
};


class RenderEngine
{
public:
    static DWORD AdjustColor(DWORD dwColor, short H, short S, short L);
    static TImageInfo* loadImage(const String& bitmap);
	static TImageInfo* loadImage(const String& bitmap, const RECT& rc);
    static void FreeImage(const TImageInfo* bitmap);
    static void DrawImage(HDC hDC, HBITMAP hBitmap, const RECT& rc, const RECT& rcPaint, const RECT& rcBmpPart, const RECT& rcCorners, BYTE uFade = 255, bool hole = false, bool xtiled = false, bool ytiled = false);
    static bool drawImageString(HDC hDC, PaintManager* pManager, const RECT& rcItem, const RECT& rcPaint, const String& pStrImage, const String& pStrModify = String::empty);
    static void DrawColor(HDC hDC, const RECT& rc, DWORD color);
    static void DrawGradient(HDC hDC, const RECT& rc, DWORD dwFirst, DWORD dwSecond, bool bVertical, int nSteps);

    static void DrawLine(HDC hDC, const RECT& rc, int nSize, DWORD dwPenColor,int nStyle = PS_SOLID);
    static void DrawRect(HDC hDC, const RECT& rc, int nSize, DWORD dwPenColor);
    static void DrawRoundRect(HDC hDC, const RECT& rc, int width, int height, int nSize, DWORD dwPenColor);
    static void DrawText(HDC hDC, PaintManager* pManager, RECT& rc, const String& pstrText, DWORD dwTextColor, int iFont, UINT uStyle);
    static void drawHtmlText(HDC hDC, PaintManager* pManager, RECT& rc, const String& pstrText, DWORD dwTextColor, RECT* pLinks, String* sLinks, int& nLinkRects, UINT uStyle);
    static HBITMAP GenerateBitmap(PaintManager* pManager, Control* pControl, RECT rc);
	static SIZE GetTextSize(HDC hDC, PaintManager* pManager, const String& pstrText, int iFont, UINT uStyle);
};

} // namespace zgui

#endif // __ZGUI_RENDER_H_
