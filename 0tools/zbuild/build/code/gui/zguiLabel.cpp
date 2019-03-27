#include "zgui.h"

// #include <atlconv.h>

#ifdef ZGUI_USE_LABEL

namespace zgui
{
    Color _MakeRGB(int a, Color cl)
    {
        return Color(a, cl.GetR(), cl.GetG(), cl.GetB());
    }

    Color _MakeRGB(int r, int g, int b)
    {
        return Color(255, r, g, b);
    }

	CLabelUI::CLabelUI() : m_uTextStyle(DT_VCENTER), m_dwTextColor(0), 
	m_dwDisabledTextColor(0),
    m_iFont(-1),
    m_bShowHtml(false),

    m_EnableEffect(false),
    m_gdiplusToken(0),
    m_TextRenderingHintAntiAlias(TextRenderingHintSystemDefault),
    m_TransShadow(60),
    m_TransText(168),
    m_TransShadow1(60),
    m_TransText1(168),
    m_hAlign(DT_LEFT),
    m_vAlign(DT_CENTER),
    m_dwTextColor1(-1),
    m_dwTextShadowColorA(0xff000000),
    m_dwTextShadowColorB(-1),
    m_GradientAngle(0),
    m_EnabledStroke(false),
    m_TransStroke(255),
    m_dwStrokeColor(0),
    m_EnabledShadow(false),
    m_GradientLength(0)
	{
        m_ShadowOffset.X		= 0.0f;
        m_ShadowOffset.Y		= 0.0f;
        m_ShadowOffset.Width	= 0.0f;
        m_ShadowOffset.Height	= 0.0f;

		::ZeroMemory(&m_rcTextPadding, sizeof(m_rcTextPadding));
	}

	LPCTSTR CLabelUI::GetClass() const
	{
		return _T("LabelUI");
	}

	LPVOID CLabelUI::GetInterface(LPCTSTR pstrName)
	{
		if( lstrcmp(pstrName, DUI_CTR_LABEL) == 0 ) return static_cast<CLabelUI*>(this);
		return CControlUI::GetInterface(pstrName);
	}

	void CLabelUI::SetTextStyle(UINT uStyle)
	{
		m_uTextStyle = uStyle;
		Invalidate();
	}

	UINT CLabelUI::GetTextStyle() const
	{
		return m_uTextStyle;
	}

	void CLabelUI::SetTextColor(DWORD dwTextColor)
	{
		m_dwTextColor = dwTextColor;
		Invalidate();
	}

	uint32_t CLabelUI::GetTextColor() const
	{
		return m_dwTextColor;
	}

	void CLabelUI::SetDisabledTextColor(DWORD dwTextColor)
	{
		m_dwDisabledTextColor = dwTextColor;
		Invalidate();
	}

	DWORD CLabelUI::GetDisabledTextColor() const
	{
		return m_dwDisabledTextColor;
	}

	void CLabelUI::SetFont(int index)
	{
		m_iFont = index;
		Invalidate();
	}

	int CLabelUI::GetFont() const
	{
		return m_iFont;
	}

	RECT CLabelUI::GetTextPadding() const
	{
		return m_rcTextPadding;
	}

	void CLabelUI::SetTextPadding(RECT rc)
	{
		m_rcTextPadding = rc;
		Invalidate();
	}

	bool CLabelUI::IsShowHtml()
	{
		return m_bShowHtml;
	}

	void CLabelUI::SetShowHtml(bool bShowHtml)
	{
        if (m_bShowHtml == bShowHtml) {
            return;
        }

		m_bShowHtml = bShowHtml;
		Invalidate();
	}

	SIZE CLabelUI::EstimateSize(SIZE szAvailable)
	{
        if (_cxyFixed.cy == 0) {
            return CSize(_cxyFixed.cx, _pManager->GetFontInfo(GetFont())->tm.tmHeight + 4);
        }

		return CControlUI::EstimateSize(szAvailable);
	}

	void CLabelUI::DoEvent(TEventUI& event)
	{
		if (event.Type == UIEVENT_SETFOCUS) {
			m_bFocused = true;
			return;
		}
		if (event.Type == UIEVENT_KILLFOCUS) {
			m_bFocused = false;
			return;
		}
		if (event.Type == UIEVENT_MOUSEENTER) {
			// return;
		}
		if (event.Type == UIEVENT_MOUSELEAVE) {
			// return;
		}
		CControlUI::DoEvent(event);
	}

	void CLabelUI::SetAttribute(const String& pstrName, const String& pstrValue)
	{
		if (pstrName == "align") {
			if (pstrValue.contains("left")) {
				m_uTextStyle &= ~(DT_CENTER | DT_RIGHT);
				m_uTextStyle |= DT_LEFT;
                m_hAlign	= DT_LEFT;
			}
			if (pstrValue.contains("center")) {
				m_uTextStyle &= ~(DT_LEFT | DT_RIGHT);
				m_uTextStyle |= DT_CENTER;
                m_hAlign	= DT_CENTER;
			}
			if (pstrValue.contains("right")) {
				m_uTextStyle &= ~(DT_LEFT | DT_CENTER);
				m_uTextStyle |= DT_RIGHT;
                m_hAlign	= DT_RIGHT;
			}
			if (pstrValue.contains("top")) {
				m_uTextStyle &= ~(DT_BOTTOM | DT_VCENTER);
				m_uTextStyle |= DT_TOP;
                m_vAlign	= DT_TOP;
			}
			if (pstrValue.contains("vcenter")) {
				m_uTextStyle &= ~(DT_TOP | DT_BOTTOM);			
				m_uTextStyle |= DT_VCENTER;
                m_vAlign	= DT_CENTER;
			}
			if (pstrValue.contains("bottom")) {
				m_uTextStyle &= ~(DT_TOP | DT_VCENTER);
				m_uTextStyle |= DT_BOTTOM;
                m_vAlign	= DT_RIGHT;
			}
		}
		else if (pstrName == "endellipsis") {
            if (pstrValue == "true") {
                m_uTextStyle |= DT_END_ELLIPSIS;
            }
            else {
                m_uTextStyle &= ~DT_END_ELLIPSIS;
            }
		}
        else if (pstrName == "font") {
            SetFont(pstrValue.getIntValue());
        }
		else if (pstrName == "textcolor") {
			SetTextColor((uint32_t)pstrValue.getHexValue32());
		}
		else if (pstrName == "disabledtextcolor") {
			SetDisabledTextColor((uint32_t)pstrValue.getHexValue32());
		}
		else if (pstrName == "textpadding") {
            RECT rcTextPadding = { 0 };
            if (Helper::splitString(pstrValue, ",", String::empty, (int&)rcTextPadding.left, (int&)rcTextPadding.top, (int&)rcTextPadding.right, (int&)rcTextPadding.bottom)) {
                SetTextPadding(rcTextPadding);
            }
		}
        else if (pstrName == "showhtml") {
            SetShowHtml(pstrValue == "true");
        }
        else if (pstrName == "enabledeffect") SetEnabledEffect(pstrValue == "true");
        else if (pstrName == "rhaa") SetTextRenderingHintAntiAlias(pstrValue.getIntValue());
        else if (pstrName == "transshadow") SetTransShadow(pstrValue.getIntValue());
        else if (pstrName == "transtext") SetTransText(pstrValue.getIntValue());
        else if (pstrName == "transshadow1") SetTransShadow1(pstrValue.getIntValue());
        else if (pstrName == "transtext1") SetTransText1(pstrValue.getIntValue());
        else if (pstrName == "gradientangle") SetGradientAngle(pstrValue.getIntValue());
        else if (pstrName == "enabledstroke") SetEnabledStroke(pstrValue == "true");
        else if (pstrName == "enabledshadow") SetEnabledShadow(pstrValue == "true");
        else if (pstrName == "transstroke") SetTransStroke(pstrValue.getIntValue());
        else if (pstrName == "gradientlength") SetGradientLength(pstrValue.getIntValue());
        else if (pstrName == "shadowoffset") {
            int offsetx, offsety;
            if (Helper::splitString(pstrValue, ",", String::empty, offsetx, offsety)) {
                SetShadowOffset(offsetx,offsety);
            }
        }
        else if (pstrName == "textcolor1") {
            SetTextColor1((uint32_t)pstrValue.getHexValue32());
        }
        else if (pstrName == "textshadowcolora") {
            SetTextShadowColorA((uint32_t)pstrValue.getHexValue32());
        }
        else if (pstrName == "textshadowcolorb") {
            SetTextShadowColorB((uint32_t)pstrValue.getHexValue32());
        }
        else if (pstrName == "strokecolor") {
            SetStrokeColor((uint32_t)pstrValue.getHexValue32());
        }
        else {
            CControlUI::SetAttribute(pstrName, pstrValue);
        }
	}

	void CLabelUI::PaintText(HDC hDC)
	{
        if (m_dwTextColor == 0) {
            m_dwTextColor = _pManager->GetDefaultFontColor();
        }
        if (m_dwDisabledTextColor == 0) {
            m_dwDisabledTextColor = _pManager->GetDefaultDisabledColor();
        }

//         if (_text.isEmpty()) {
//             return;
//         }
// 
// 		int nLinks = 0;
		RECT rc = _rcItem;
		rc.left += m_rcTextPadding.left;
		rc.right -= m_rcTextPadding.right;
		rc.top += m_rcTextPadding.top;
		rc.bottom -= m_rcTextPadding.bottom;
// 		if (IsEnabled()) {
//             if (m_bShowHtml) {
// 				CRenderEngine::DrawHtmlText(hDC, _pManager, rc, _text, m_dwTextColor, NULL, NULL, nLinks, DT_SINGLELINE | m_uTextStyle);
//             }
//             else {
// 				CRenderEngine::DrawText(hDC, _pManager, rc, _text, m_dwTextColor, m_iFont, DT_SINGLELINE | m_uTextStyle);
//             }
        if(!GetEnabledEffect()) {
            if (_text.isEmpty()) {
                return;
            }
            int nLinks = 0;
            if (IsEnabled()) {
                if (m_bShowHtml) {
                    CRenderEngine::DrawHtmlText(hDC, _pManager, rc, _text, m_dwTextColor, NULL, NULL, nLinks, DT_SINGLELINE | m_uTextStyle);
                }
                else {
                    CRenderEngine::DrawText(hDC, _pManager, rc, _text, m_dwTextColor, m_iFont, DT_SINGLELINE | m_uTextStyle);
                }
            }
            else {
                if (m_bShowHtml) {
                    CRenderEngine::DrawHtmlText(hDC, _pManager, rc, _text, m_dwDisabledTextColor, NULL, NULL, nLinks, DT_SINGLELINE | m_uTextStyle);
                }
                else {
                    CRenderEngine::DrawText(hDC, _pManager, rc, _text, m_dwDisabledTextColor, m_iFont, DT_SINGLELINE | m_uTextStyle);
                }
            }
		}
        else
        {
            Font nFont(hDC, _pManager->GetFont(GetFont()));

            Graphics nGraphics(hDC);
            nGraphics.SetTextRenderingHint(m_TextRenderingHintAntiAlias);

            StringFormat format;
            format.SetAlignment((StringAlignment)m_hAlign);
            format.SetLineAlignment((StringAlignment)m_vAlign);

            RectF nRc((float)rc.left,(float)rc.top,(float)rc.right-rc.left,(float)rc.bottom-rc.top);
            RectF nShadowRc = nRc;
            nShadowRc.X += m_ShadowOffset.X;
            nShadowRc.Y += m_ShadowOffset.Y;

            int nGradientLength	= GetGradientLength();

            if(nGradientLength == 0)
                nGradientLength = (rc.bottom-rc.top);

            LinearGradientBrush nLineGrBrushA(Point(GetGradientAngle(), 0),Point(0,nGradientLength),_MakeRGB(GetTransShadow(),GetTextShadowColorA()),_MakeRGB(GetTransShadow1(),GetTextShadowColorB() == -1?GetTextShadowColorA():GetTextShadowColorB()));
            LinearGradientBrush nLineGrBrushB(Point(GetGradientAngle(), 0),Point(0,nGradientLength),_MakeRGB(GetTransText(),GetTextColor()),_MakeRGB(GetTransText1(),GetTextColor1() == -1?GetTextColor():GetTextColor1()));

            if (GetEnabledStroke() && GetStrokeColor() > 0) {
                LinearGradientBrush nLineGrBrushStroke(Point(GetGradientAngle(),0),Point(0,rc.bottom-rc.top+2),_MakeRGB(GetTransStroke(),GetStrokeColor()),_MakeRGB(GetTransStroke(),GetStrokeColor()));

#ifdef _UNICODE
                nRc.Offset(-1,0);
                nGraphics.DrawString(m_TextValue.toWideCharPointer(), m_TextValue.length(),&nFont,nRc,&format,&nLineGrBrushStroke);
                nRc.Offset(2,0);
                nGraphics.DrawString(m_TextValue.toWideCharPointer(),m_TextValue.length(),&nFont,nRc,&format,&nLineGrBrushStroke);
                nRc.Offset(-1,-1);
                nGraphics.DrawString(m_TextValue.toWideCharPointer(),m_TextValue.length(),&nFont,nRc,&format,&nLineGrBrushStroke);
                nRc.Offset(0,2);
                nGraphics.DrawString(m_TextValue.toWideCharPointer(),m_TextValue.length(),&nFont,nRc,&format,&nLineGrBrushStroke);
                nRc.Offset(0,-1);
#else
                USES_CONVERSION;
                wstring mTextValue = A2W(m_TextValue.GetData());

                nRc.Offset(-1,0);
                nGraphics.DrawString(mTextValue.c_str(),mTextValue.length(),&nFont,nRc,&format,&nLineGrBrushStroke);
                nRc.Offset(2,0);
                nGraphics.DrawString(mTextValue.c_str(),mTextValue.length(),&nFont,nRc,&format,&nLineGrBrushStroke);
                nRc.Offset(-1,-1);
                nGraphics.DrawString(mTextValue.c_str(),mTextValue.length(),&nFont,nRc,&format,&nLineGrBrushStroke);
                nRc.Offset(0,2);
                nGraphics.DrawString(mTextValue.c_str(),mTextValue.length(),&nFont,nRc,&format,&nLineGrBrushStroke);
                nRc.Offset(0,-1);
#endif

            }
#ifdef _UNICODE
            if(GetEnabledShadow() && (GetTextShadowColorA() > 0 || GetTextShadowColorB() > 0))
                nGraphics.DrawString(m_TextValue.toWideCharPointer(), m_TextValue.length(),&nFont,nShadowRc,&format,&nLineGrBrushA);

            nGraphics.DrawString(m_TextValue.toWideCharPointer(), m_TextValue.length(),&nFont,nRc,&format,&nLineGrBrushB);
#else
            USES_CONVERSION;
            wstring mTextValue = A2W(m_TextValue.GetData());

            if(GetEnabledShadow() && (GetTextShadowColorA() > 0 || GetTextShadowColorB() > 0))
                nGraphics.DrawString(mTextValue.c_str(),mTextValue.length(),&nFont,nShadowRc,&format,&nLineGrBrushA);

            nGraphics.DrawString(mTextValue.c_str(),mTextValue.length(),&nFont,nRc,&format,&nLineGrBrushB);
#endif

        }
	}

	void CLabelUI::SetTransShadow( int _TransShadow )
	{
		m_TransShadow = _TransShadow;
	}

	int CLabelUI::GetTransShadow()
	{
		return m_TransShadow;
	}

	void CLabelUI::SetTextRenderingHintAntiAlias( int _TextRenderingHintAntiAlias )
	{
        if (_TextRenderingHintAntiAlias < 0 || _TextRenderingHintAntiAlias > 5) {
			_TextRenderingHintAntiAlias = 0;
        }
		m_TextRenderingHintAntiAlias = (TextRenderingHint)_TextRenderingHintAntiAlias;
	}

	int CLabelUI::GetTextRenderingHintAntiAlias()
	{
		return m_TextRenderingHintAntiAlias;
	}

	void CLabelUI::SetShadowOffset( int _offset,int _angle )
	{
		if(_angle > 180 || _angle < -180)
			return;

		RECT rc = _rcItem;

		if(_angle >= 0 && _angle <= 180)
			rc.top -= _offset;
		else if(_angle > -180 && _angle < 0)
			rc.top += _offset;

		if(_angle > -90 && _angle <= 90)
			rc.left -= _offset;
		else if( _angle > 90 || _angle < -90)
			rc.left += _offset;

		m_ShadowOffset.X = (float)rc.top;
		m_ShadowOffset.Y = (float)rc.left;
	}

	RectF CLabelUI::GetShadowOffset()
	{
		return m_ShadowOffset;
	}

	void CLabelUI::SetText(const String& pstrText )
	{
		if(!GetEnabledEffect())
			return CControlUI::SetText(pstrText);

		m_TextValue = pstrText;
	}

	String CLabelUI::GetText() const
	{
		if(!m_EnableEffect)
			return CControlUI::GetText();
		return m_TextValue;
	}

	void CLabelUI::SetEnabledEffect( bool _EnabledEffect )
	{
		m_EnableEffect = _EnabledEffect;
	}

	bool CLabelUI::GetEnabledEffect()
	{
		return m_EnableEffect;
	}

	void CLabelUI::SetTextColor1( DWORD _TextColor1 )
	{
		m_dwTextColor1	= _TextColor1;
	}

	DWORD CLabelUI::GetTextColor1()
	{
		return m_dwTextColor1;
	}

	void CLabelUI::SetTextShadowColorA( DWORD _TextShadowColorA )
	{
		m_dwTextShadowColorA	= _TextShadowColorA;
	}

	DWORD CLabelUI::GetTextShadowColorA()
	{
		return m_dwTextShadowColorA;
	}

	void CLabelUI::SetTextShadowColorB( DWORD _TextShadowColorB )
	{
		m_dwTextShadowColorB	= _TextShadowColorB;
	}

	DWORD CLabelUI::GetTextShadowColorB()
	{
		return m_dwTextShadowColorB;
	}

	void CLabelUI::SetTransText( int _TransText )
	{
		m_TransText = _TransText;
	}

	int CLabelUI::GetTransText()
	{
		return m_TransText;
	}

	void CLabelUI::SetTransShadow1( int _TransShadow )
	{
		m_TransShadow1	= _TransShadow;
	}

	int CLabelUI::GetTransShadow1()
	{
		return m_TransShadow1;
	}

	void CLabelUI::SetTransText1( int _TransText )
	{
		m_TransText1	= _TransText;
	}

	int CLabelUI::GetTransText1()
	{
		return m_TransText1;
	}

	void CLabelUI::SetGradientAngle( int _SetGradientAngle )
	{
		m_GradientAngle	= _SetGradientAngle;
	}

	int CLabelUI::GetGradientAngle()
	{
		return m_GradientAngle;
	}

	void CLabelUI::SetEnabledStroke( bool _EnabledStroke )
	{
		m_EnabledStroke = _EnabledStroke;
	}

	bool CLabelUI::GetEnabledStroke()
	{
		return m_EnabledStroke;
	}

	void CLabelUI::SetTransStroke( int _TransStroke )
	{
		m_TransStroke = _TransStroke;
	}

	int CLabelUI::GetTransStroke()
	{
		return m_TransStroke;
	}

	void CLabelUI::SetStrokeColor( DWORD _StrokeColor )
	{
		m_dwStrokeColor = _StrokeColor;
	}

	DWORD CLabelUI::GetStrokeColor()
	{
		return m_dwStrokeColor;
	}

	void CLabelUI::SetEnabledShadow( bool _EnabledShadowe )
	{
		m_EnabledShadow = _EnabledShadowe;
	}

	bool CLabelUI::GetEnabledShadow()
	{
		return m_EnabledShadow;
	}

	void CLabelUI::SetGradientLength( int _GradientLength )
	{
		m_GradientLength	= _GradientLength;
	}

	int CLabelUI::GetGradientLength()
	{
		return m_GradientLength;
	}
}

#endif // ZGUI_USE_LABEL
