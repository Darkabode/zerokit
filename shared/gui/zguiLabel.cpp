#include "zgui.h"

#ifdef ZGUI_USE_LABEL

namespace zgui {

const String Label::CLASS_NAME = ZGUI_LABEL;

Gdiplus::Color _MakeRGB(int a, Gdiplus::Color cl)
{
    return Gdiplus::Color(a, cl.GetR(), cl.GetG(), cl.GetB());
}

Gdiplus::Color _MakeRGB(int r, int g, int b)
{
    return Gdiplus::Color(255, r, g, b);
}

Label::Label() :
_uTextStyle(DT_VCENTER),
_textColor(0), 
_disabledTextColor(0),
m_iFont(-1),
m_bShowHtml(false),
_enableEffect(false),
m_TextRenderingHintAntiAlias(Gdiplus::TextRenderingHintSystemDefault),
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
    m_ShadowOffset.X= 0.0f;
    m_ShadowOffset.Y = 0.0f;
    m_ShadowOffset.Width = 0.0f;
    m_ShadowOffset.Height = 0.0f;

	__stosb((uint8_t*)&_rcTextPadding, 0, sizeof(_rcTextPadding));
}

const String& Label::getClass() const
{
	return CLASS_NAME;
}

LPVOID Label::getInterface(const String& name)
{
	if (name == ZGUI_LABEL) {
        return static_cast<Label*>(this);
    }
	return Control::getInterface(name);
}

void Label::SetTextStyle(UINT uStyle)
{
	_uTextStyle = uStyle;
	Invalidate();
}

UINT Label::GetTextStyle() const
{
	return _uTextStyle;
}

void Label::SetTextColor(DWORD dwTextColor)
{
	_textColor = dwTextColor;
	Invalidate();
}

uint32_t Label::GetTextColor() const
{
	return _textColor;
}

void Label::SetDisabledTextColor(DWORD dwTextColor)
{
	_disabledTextColor = dwTextColor;
	Invalidate();
}

DWORD Label::GetDisabledTextColor() const
{
	return _disabledTextColor;
}

void Label::SetFont(int index)
{
	m_iFont = index;
	Invalidate();
}

int Label::GetFont() const
{
	return m_iFont;
}

RECT Label::GetTextPadding() const
{
	return _rcTextPadding;
}

void Label::SetTextPadding(RECT rc)
{
	_rcTextPadding = rc;
	Invalidate();
}

bool Label::IsShowHtml()
{
	return m_bShowHtml;
}

void Label::SetShowHtml(bool bShowHtml)
{
    if (m_bShowHtml == bShowHtml) {
        return;
    }

	m_bShowHtml = bShowHtml;
	Invalidate();
}

SIZE Label::EstimateSize(SIZE szAvailable)
{
    if (_cxyFixed.cy == 0) {
        return Size(_cxyFixed.cx, _pManager->GetFontInfo(GetFont())->tm.tmHeight + 4);
    }

	return Control::EstimateSize(szAvailable);
}

void Label::DoEvent(TEventUI& event)
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
	Control::DoEvent(event);
}

void Label::setAttribute(const String& name, const String& value)
{
	if (name == "align") {
		if (value.contains("left")) {
			_uTextStyle &= ~(DT_CENTER | DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
			_uTextStyle |= DT_LEFT;
		}
		if (value.contains("vleft")) {
			_uTextStyle &= ~(DT_CENTER | DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
			_uTextStyle |= (DT_LEFT | DT_VCENTER | DT_SINGLELINE);
		}
		if (value.contains("center")) {
			_uTextStyle &= ~(DT_LEFT | DT_RIGHT | DT_SINGLELINE);
			_uTextStyle |= DT_CENTER;
		}
		if (value.contains("right")) {
			_uTextStyle &= ~(DT_LEFT | DT_CENTER | DT_VCENTER | DT_SINGLELINE);
			_uTextStyle |= DT_RIGHT;
		}
		if (value.contains("top")) {
			_uTextStyle &= ~(DT_BOTTOM | DT_VCENTER | DT_VCENTER);
			_uTextStyle |= (DT_TOP | DT_SINGLELINE);
		}
		if (value.contains("vcenter")) {
			_uTextStyle &= ~(DT_TOP | DT_BOTTOM );			
			_uTextStyle |= (DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		}
		if (value.contains("bottom")) {
			_uTextStyle &= ~(DT_TOP | DT_VCENTER | DT_VCENTER | DT_SINGLELINE);
			_uTextStyle |= (DT_BOTTOM);
		}
	}
	else if (name == "endellipsis") {
        if (value == "true") {
            _uTextStyle |= DT_END_ELLIPSIS;
        }
        else {
            _uTextStyle &= ~DT_END_ELLIPSIS;
        }
	}
    else if (name == "font") {
        SetFont(value.getIntValue());
    }
	else if (name == "textcolor") {
		SetTextColor((uint32_t)value.getHexValue32());
	}
	else if (name == "disabledtextcolor") {
		SetDisabledTextColor((uint32_t)value.getHexValue32());
	}
	else if (name == "textpadding") {
        RECT rcTextPadding;
        if (Helper::splitString(value, ",", String::empty, (int&)rcTextPadding.left, (int&)rcTextPadding.top, (int&)rcTextPadding.right, (int&)rcTextPadding.bottom)) {
            SetTextPadding(rcTextPadding);
        }
	}
    else if (name == "showhtml") {
        SetShowHtml(value == "true");
    }
	else if (name == "enabledeffect") {
		SetEnabledEffect(value == "true");
	}
	else if (name == "rhaa") {
		SetTextRenderingHintAntiAlias(value.getIntValue());
	}
	else if (name == "transshadow") {
		SetTransShadow(value.getIntValue());
	}
	else if (name == "transtext") {
		SetTransText(value.getIntValue());
	}
	else if (name == "transshadow1") {
		SetTransShadow1(value.getIntValue());
	}
	else if (name == "transtext1") {
		SetTransText1(value.getIntValue());
	}
	else if (name == "gradientangle") {
		SetGradientAngle(value.getIntValue());
	}
	else if (name == "enabledstroke") {
		SetEnabledStroke(value == "true");
	}
	else if (name == "enabledshadow") {
		SetEnabledShadow(value == "true");
	}
	else if (name == "transstroke") {
		SetTransStroke(value.getIntValue());
	}
	else if (name == "gradientlength") {
		SetGradientLength(value.getIntValue());
	}
    else if (name == "shadowoffset") {
        int offsetx, offsety;
        if (Helper::splitString(value, ",", String::empty, offsetx, offsety)) {
            SetShadowOffset(offsetx,offsety);
        }
    }
    else if (name == "textcolor1") {
        SetTextColor1((uint32_t)value.getHexValue32());
    }
    else if (name == "textshadowcolora") {
        SetTextShadowColorA((uint32_t)value.getHexValue32());
    }
    else if (name == "textshadowcolorb") {
        SetTextShadowColorB((uint32_t)value.getHexValue32());
    }
    else if (name == "strokecolor") {
        SetStrokeColor((uint32_t)value.getHexValue32());
    }
    else {
        Control::setAttribute(name, value);
    }
}

void Label::PaintText(HDC hDC)
{
    if (_textColor == 0) {
        _textColor = _pManager->GetDefaultFontColor();
    }
    if (_disabledTextColor == 0) {
        _disabledTextColor = _pManager->GetDefaultDisabledColor();
    }

	RECT rc = _rcItem;
	rc.left += _rcTextPadding.left;
	rc.right -= _rcTextPadding.right;
	rc.top += _rcTextPadding.top;
	rc.bottom -= _rcTextPadding.bottom;
    if (!GetEnabledEffect()) {
        if (_text.isEmpty()) {
            return;
        }
        int nLinks = 0;
        uint32_t color = (IsEnabled() ? _textColor : _disabledTextColor);
        if (m_bShowHtml) {
            RenderEngine::drawHtmlText(hDC, _pManager, rc, _text, color, 0, 0, nLinks, DT_SINGLELINE | _uTextStyle);
        }
        else {
            RenderEngine::DrawText(hDC, _pManager, rc, _text, color, m_iFont, DT_SINGLELINE | _uTextStyle);
        }
	}
    else {
        Gdiplus::Font nFont(hDC, _pManager->GetFont(GetFont()));

        Gdiplus::Graphics nGraphics(hDC);
        nGraphics.SetTextRenderingHint(m_TextRenderingHintAntiAlias);

        Gdiplus::StringFormat format;
        format.SetAlignment((Gdiplus::StringAlignment)m_hAlign);
        format.SetLineAlignment((Gdiplus::StringAlignment)m_vAlign);

        Gdiplus::RectF nRc((float)rc.left,(float)rc.top,(float)rc.right-rc.left,(float)rc.bottom-rc.top);
        Gdiplus::RectF nShadowRc = nRc;
        nShadowRc.X += m_ShadowOffset.X;
        nShadowRc.Y += m_ShadowOffset.Y;

        int nGradientLength	= GetGradientLength();

		if (nGradientLength == 0) {
            nGradientLength = (rc.bottom-rc.top);
		}

        Gdiplus::LinearGradientBrush nLineGrBrushA(Gdiplus::Point(GetGradientAngle(), 0),Gdiplus::Point(0,nGradientLength),_MakeRGB(GetTransShadow(),GetTextShadowColorA()),_MakeRGB(GetTransShadow1(),GetTextShadowColorB() == -1?GetTextShadowColorA():GetTextShadowColorB()));
        Gdiplus::LinearGradientBrush nLineGrBrushB(Gdiplus::Point(GetGradientAngle(), 0),Gdiplus::Point(0,nGradientLength),_MakeRGB(GetTransText(),GetTextColor()),_MakeRGB(GetTransText1(),GetTextColor1() == -1?GetTextColor():GetTextColor1()));

        if (GetEnabledStroke() && GetStrokeColor() > 0) {
            Gdiplus::LinearGradientBrush nLineGrBrushStroke(Gdiplus::Point(GetGradientAngle(),0), Gdiplus::Point(0,rc.bottom-rc.top+2),_MakeRGB(GetTransStroke(),GetStrokeColor()),_MakeRGB(GetTransStroke(),GetStrokeColor()));

            nRc.Offset(-1,0);
            nGraphics.DrawString(m_TextValue.toWideCharPointer(), m_TextValue.length(),&nFont,nRc,&format,&nLineGrBrushStroke);
            nRc.Offset(2,0);
            nGraphics.DrawString(m_TextValue.toWideCharPointer(),m_TextValue.length(),&nFont,nRc,&format,&nLineGrBrushStroke);
            nRc.Offset(-1,-1);
            nGraphics.DrawString(m_TextValue.toWideCharPointer(),m_TextValue.length(),&nFont,nRc,&format,&nLineGrBrushStroke);
            nRc.Offset(0,2);
            nGraphics.DrawString(m_TextValue.toWideCharPointer(),m_TextValue.length(),&nFont,nRc,&format,&nLineGrBrushStroke);
            nRc.Offset(0,-1);
        }

		if (GetEnabledShadow() && (GetTextShadowColorA() > 0 || GetTextShadowColorB() > 0)) {
            nGraphics.DrawString(m_TextValue.toWideCharPointer(), m_TextValue.length(),&nFont,nShadowRc,&format,&nLineGrBrushA);
		}
        nGraphics.DrawString(m_TextValue.toWideCharPointer(), m_TextValue.length(),&nFont,nRc,&format,&nLineGrBrushB);
    }
}

void Label::SetTransShadow( int _TransShadow )
{
	m_TransShadow = _TransShadow;
}

int Label::GetTransShadow()
{
	return m_TransShadow;
}

void Label::SetTextRenderingHintAntiAlias( int _TextRenderingHintAntiAlias )
{
    if (_TextRenderingHintAntiAlias < 0 || _TextRenderingHintAntiAlias > 5) {
		_TextRenderingHintAntiAlias = 0;
    }
	m_TextRenderingHintAntiAlias = (Gdiplus::TextRenderingHint)_TextRenderingHintAntiAlias;
}

int Label::GetTextRenderingHintAntiAlias()
{
	return m_TextRenderingHintAntiAlias;
}

void Label::SetShadowOffset( int _offset,int _angle )
{
	if(_angle > 180 || _angle < -180)
		return;

	RECT rc = _rcItem;

	if (_angle >= 0 && _angle <= 180) {
		rc.top -= _offset;
	}
	else if(_angle > -180 && _angle < 0) {
		rc.top += _offset;
	}

	if (_angle > -90 && _angle <= 90) {
		rc.left -= _offset;
	}
	else if (_angle > 90 || _angle < -90) {
		rc.left += _offset;
	}

	m_ShadowOffset.X = (float)rc.top;
	m_ShadowOffset.Y = (float)rc.left;
}

Gdiplus::RectF Label::GetShadowOffset()
{
	return m_ShadowOffset;
}

void Label::setText(const String& pstrText, const bool needTranslate)
{
	if (!GetEnabledEffect()) {
		return Control::setText(pstrText, needTranslate);
	}

	m_TextValue = pstrText;
}

String Label::getText() const
{
	if (!_enableEffect) {
		return Control::getText();
	}
	return m_TextValue;
}

void Label::SetEnabledEffect( bool _EnabledEffect )
{
	_enableEffect = _EnabledEffect;
}

bool Label::GetEnabledEffect()
{
	return _enableEffect;
}

void Label::SetTextColor1( DWORD _TextColor1 )
{
	m_dwTextColor1 = _TextColor1;
}

DWORD Label::GetTextColor1()
{
	return m_dwTextColor1;
}

void Label::SetTextShadowColorA(DWORD _TextShadowColorA)
{
	m_dwTextShadowColorA = _TextShadowColorA;
}

DWORD Label::GetTextShadowColorA()
{
	return m_dwTextShadowColorA;
}

void Label::SetTextShadowColorB(DWORD _TextShadowColorB)
{
	m_dwTextShadowColorB = _TextShadowColorB;
}

DWORD Label::GetTextShadowColorB()
{
	return m_dwTextShadowColorB;
}

void Label::SetTransText(int _TransText)
{
	m_TransText = _TransText;
}

int Label::GetTransText()
{
	return m_TransText;
}

void Label::SetTransShadow1(int _TransShadow)
{
	m_TransShadow1 = _TransShadow;
}

int Label::GetTransShadow1()
{
	return m_TransShadow1;
}

void Label::SetTransText1( int _TransText )
{
	m_TransText1 = _TransText;
}

int Label::GetTransText1()
{
	return m_TransText1;
}

void Label::SetGradientAngle( int _SetGradientAngle )
{
	m_GradientAngle	= _SetGradientAngle;
}

int Label::GetGradientAngle()
{
	return m_GradientAngle;
}

void Label::SetEnabledStroke( bool _EnabledStroke )
{
	m_EnabledStroke = _EnabledStroke;
}

bool Label::GetEnabledStroke()
{
	return m_EnabledStroke;
}

void Label::SetTransStroke( int _TransStroke )
{
	m_TransStroke = _TransStroke;
}

int Label::GetTransStroke()
{
	return m_TransStroke;
}

void Label::SetStrokeColor( DWORD _StrokeColor )
{
	m_dwStrokeColor = _StrokeColor;
}

DWORD Label::GetStrokeColor()
{
	return m_dwStrokeColor;
}

void Label::SetEnabledShadow( bool _EnabledShadowe )
{
	m_EnabledShadow = _EnabledShadowe;
}

bool Label::GetEnabledShadow()
{
	return m_EnabledShadow;
}

void Label::SetGradientLength( int _GradientLength )
{
	m_GradientLength = _GradientLength;
}

int Label::GetGradientLength()
{
	return m_GradientLength;
}

}

#endif // ZGUI_USE_LABEL
