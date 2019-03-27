/**************************************************************************\
*
* Copyright (c) 1998-2001, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   Gdiplus.h
*
* Abstract:
*
*   GDI+ public header file
*
\**************************************************************************/

#ifndef _GDIPLUS_H
#define _GDIPLUS_H

struct IDirectDrawSurface7;

typedef signed   short   INT16;
typedef unsigned short  UINT16;


// Define the Current GDIPlus Version
#ifndef GDIPVER
#define GDIPVER 0x0100
#endif

#include <pshpack8.h>   // set structure packing to 8

namespace Gdiplus
{
    namespace DllExports
    {
        #include "GdiplusMem.h"
    };

    #include "GdiplusEnums.h"
    #include "GdiplusTypes.h"
    #include "GdiplusInit.h"
    #include "GdiplusPixelFormats.h"
    #include "GdiplusColor.h"
    #include "GdiplusMetaHeader.h"
    #include "GdiplusImaging.h"
    #include "GdiplusColorMatrix.h"
#if (GDIPVER >= 0x0110)    
    #include "GdiplusEffects.h"
#endif
    #include "GdiplusGpStubs.h"

    namespace DllExports
    {
        #include "GdiplusFlat.h"
    };
#ifdef fn_GdipAlloc
#define GdipAlloc_Hash 0x5512D7B3
    typedef void* (*FnGdipAlloc)(size_t size);
#endif

#ifdef fn_GdipFree
#define GdipFree_Hash 0x15DA9AA1
    typedef GpStatus (*FnGdipFree)(void* ptr);
#endif
    FnGdipAlloc fnGdipAlloc;
    FnGdipFree fnGdipFree;


#include "GdiplusBase.h"
#include "GdiplusHeaders.h"
	
#ifdef fn_GdiplusStartup
#define GdiplusStartup_Hash 0x9A996251
typedef GpStatus (*FnGdiplusStartup)(ULONG_PTR* token, const GdiplusStartupInput* input, GdiplusStartupOutput *output);
#endif

#ifdef fn_GdiplusShutdown
#define GdiplusShutdown_Hash 0x4B33638F
typedef void (*FnGdiplusShutdown)(ULONG_PTR token);
#endif

#ifdef fn_GdipDeleteBrush
#define GdipDeleteBrush_Hash 0x47835491
typedef GpStatus (*FnGdipDeleteBrush)(GpBrush *brush);
#endif

#ifdef fn_GdipCloneBrush
#define GdipCloneBrush_Hash 0x38493F4B
typedef GpStatus (*FnGdipCloneBrush)(GpBrush *brush, GpBrush **cloneBrush);
#endif

#ifdef fn_GdipCreateFontFromLogfontA
#define GdipCreateFontFromLogfontA_Hash 0x18A3AB56
typedef GpStatus (*FnGdipCreateFontFromLogfontA)(HDC hdc, GDIPCONST LOGFONTA *logfont, GpFont **font);
#endif

#ifdef fn_GdipSetStringFormatLineAlign
#define GdipSetStringFormatLineAlign_Hash 0xB2EB7953
typedef GpStatus (*FnGdipSetStringFormatLineAlign)(GpStringFormat *format, StringAlignment align);
#endif

#ifdef fn_GdipSetTextRenderingHint
#define GdipSetTextRenderingHint_Hash 0x88F9CE01
typedef GpStatus (*FnGdipSetTextRenderingHint)(GpGraphics *graphics, TextRenderingHint mode);
#endif

#ifdef fn_GdipDeleteFont
#define GdipDeleteFont_Hash 0x19C13E3F
typedef GpStatus (*FnGdipDeleteFont)(GpFont* font);
#endif

#ifdef fn_GdipDeleteGraphics
#define GdipDeleteGraphics_Hash 0xB01B26BB
typedef GpStatus (*FnGdipDeleteGraphics)(GpGraphics *graphics);
#endif

#ifdef fn_GdipSetStringFormatAlign
#define GdipSetStringFormatAlign_Hash 0x1CDAE68F
typedef GpStatus (*FnGdipSetStringFormatAlign)(GpStringFormat *format, StringAlignment align);
#endif

#ifdef fn_GdipDrawString
#define GdipDrawString_Hash 0x9A58F14C
typedef GpStatus (*FnGdipDrawString)(GpGraphics *graphics, GDIPCONST WCHAR *string, INT length, GDIPCONST GpFont *font, GDIPCONST RectF *layoutRect, GDIPCONST GpStringFormat *stringFormat, GDIPCONST GpBrush *brush);
#endif

#ifdef fn_GdipCreateFromHDC
#define GdipCreateFromHDC_Hash 0x8555CBDE
typedef GpStatus (*FnGdipCreateFromHDC)(HDC hdc, GpGraphics **graphics);
#endif

#ifdef fn_GdipCreateLineBrushI
#define GdipCreateLineBrushI_Hash 0x12849D50
typedef GpStatus (*FnGdipCreateLineBrushI)(GDIPCONST GpPoint* point1, GDIPCONST GpPoint* point2, ARGB color1, ARGB color2, GpWrapMode wrapMode, GpLineGradient **lineGradient);
#endif

#ifdef fn_GdipCreateStringFormat
#define GdipCreateStringFormat_Hash 0xDFE17A40
typedef GpStatus (*FnGdipCreateStringFormat)(INT formatAttributes, LANGID language, GpStringFormat **format);
#endif

#ifdef fn_GdipDeleteStringFormat
#define GdipDeleteStringFormat_Hash 0x17D47B61
typedef GpStatus (*FnGdipDeleteStringFormat)(GpStringFormat *format);
#endif

#ifdef fn_GdipCreateFontFromDC
#define GdipCreateFontFromDC_Hash 0x0E04934A
typedef GpStatus (*FnGdipCreateFontFromDC)(HDC hdc, GpFont **font);
#endif


// gdiplus.dll
FnGdiplusStartup fnGdiplusStartup;
FnGdiplusShutdown fnGdiplusShutdown;
FnGdipDeleteBrush fnGdipDeleteBrush;
FnGdipCloneBrush fnGdipCloneBrush;
FnGdipCreateFontFromLogfontA fnGdipCreateFontFromLogfontA;
FnGdipSetStringFormatLineAlign fnGdipSetStringFormatLineAlign;
FnGdipSetTextRenderingHint fnGdipSetTextRenderingHint;
FnGdipDeleteFont fnGdipDeleteFont;
FnGdipDeleteGraphics fnGdipDeleteGraphics;
FnGdipSetStringFormatAlign fnGdipSetStringFormatAlign;
FnGdipDrawString fnGdipDrawString;
FnGdipCreateFromHDC fnGdipCreateFromHDC;
FnGdipCreateLineBrushI fnGdipCreateLineBrushI;
FnGdipCreateStringFormat fnGdipCreateStringFormat;
FnGdipDeleteStringFormat fnGdipDeleteStringFormat;
FnGdipCreateFontFromDC fnGdipCreateFontFromDC;


    #include "GdiplusImageAttributes.h"
    #include "GdiplusMatrix.h"
    #include "GdiplusBrush.h"
    #include "GdiplusPen.h"
    #include "GdiplusStringFormat.h"
    #include "GdiplusPath.h"
    #include "GdiplusLineCaps.h"
    #include "GdiplusGraphics.h"
    #include "GdiplusMetafile.h"
    #include "GdiplusCachedBitmap.h"
    #include "GdiplusRegion.h"
    #include "GdiplusFontCollection.h"
    #include "GdiplusFontFamily.h"
    #include "GdiplusFont.h"
    #include "GdiplusBitmap.h"
    #include "GdiplusImageCodec.h"
}; // namespace Gdiplus

#include <poppack.h>    // pop structure packing back to previous state

#endif // !_GDIPLUS_HPP

