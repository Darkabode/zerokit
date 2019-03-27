#include "zgui.h"

#ifdef ZGUI_USE_TEXT

namespace zgui {

const String Text::CLASS_NAME = ZGUI_TEXT;

Text::Text() :
_nLinks(0),
_nHoverLink(-1)
{
	_uTextStyle = DT_WORDBREAK;
	_rcTextPadding.left = 2;
	_rcTextPadding.right = 2;
	__stosb((uint8_t*)_rcLinks, 0, sizeof(_rcLinks));
}

Text::~Text()
{
}

const String& Text::getClass() const
{
	return CLASS_NAME;
}

LPVOID Text::getInterface(const String& name)
{
    if (name == ZGUI_TEXT) {
        return static_cast<Text*>(this);
    }
	return Label::getInterface(name);
}

void Text::setTextCallback(ITextCallback* pTextCallback)
{
	SetUserData(Language::getInstance()->scanTextAndTranslate(GetUserData()));
	_pTextCallback = pTextCallback;
}

UINT Text::GetControlFlags() const
{
    if (IsEnabled() && _nLinks > 0) {
        return UIFLAG_SETCURSOR;
    }
    else {
        return 0;
    }
}

String* Text::GetLinkContent(int iIndex)
{
    if (iIndex >= 0 && iIndex < _nLinks) {
        return &_sLinks[iIndex];
    }
	return 0;
}

void Text::DoEvent(TEventUI& event)
{
	if (!IsMouseEnabled() && event.Type > UIEVENT__MOUSEBEGIN && event.Type < UIEVENT__MOUSEEND) {
		if (_pParent != 0) {
			_pParent->DoEvent(event);
		}
		else {
			Label::DoEvent(event);
		}
		return;
	}

	if (event.Type == UIEVENT_SETCURSOR) {
		for (int i = 0; i < _nLinks; ++i) {
			if (fn_PtInRect(&_rcLinks[i], event.ptMouse)) {
				fn_SetCursor(fn_LoadCursorW(NULL, MAKEINTRESOURCE(IDC_HAND)));
				return;
			}
		}
	}
	if (event.Type == UIEVENT_BUTTONDOWN || event.Type == UIEVENT_DBLCLICK && IsEnabled()) {
		for (int i = 0; i < _nLinks; ++i) {
			if (fn_PtInRect(&_rcLinks[i], event.ptMouse)) {
				Invalidate();
				return;
			}
		}
	}
	if (event.Type == UIEVENT_BUTTONUP && IsEnabled()) {
		for( int i = 0; i < _nLinks; ++i) {
			if (fn_PtInRect(&_rcLinks[i], event.ptMouse)) {
				_pManager->SendNotify(this, "link", i);
				return;
			}
		}
	}
	if (event.Type == UIEVENT_CONTEXTMENU) {
		return;
	}
	// When you move over a link
	if (_nLinks > 0 && event.Type == UIEVENT_MOUSEMOVE && IsEnabled()) {
		int nHoverLink = -1;
		for (int i = 0; i < _nLinks; ++i) {
			if (fn_PtInRect(&_rcLinks[i], event.ptMouse)) {
				nHoverLink = i;
				break;
			}
		}

		if (_nHoverLink != nHoverLink) {
			_nHoverLink = nHoverLink;
			Invalidate();
			return;
		}      
	}
	if (event.Type == UIEVENT_MOUSELEAVE) {
		if (_nLinks > 0 && IsEnabled()) {
			if (_nHoverLink != -1) {
				_nHoverLink = -1;
				Invalidate();
				return;
			}
		}
	}

	Label::DoEvent(event);
}

SIZE Text::EstimateSize(SIZE szAvailable)
{
	RECT rcText = {0, 0, MAX(szAvailable.cx, _cxyFixed.cx), 9999};
	rcText.left += _rcTextPadding.left;
	rcText.right -= _rcTextPadding.right;
	zgui::String text;
	if (_pTextCallback != 0) {
		text = _pTextCallback->getText(this);
	}
	else {
		text = _text;
	}
	if (m_bShowHtml) {
	    int nLinks = 0;
	    RenderEngine::drawHtmlText(_pManager->GetPaintDC(), _pManager, rcText, text, _textColor, NULL, NULL, nLinks, DT_CALCRECT | _uTextStyle);
	}
	else {
		RenderEngine::DrawText(_pManager->GetPaintDC(), _pManager, rcText, text, _textColor, m_iFont, DT_CALCRECT | _uTextStyle);
	}
	SIZE cXY = {rcText.right - rcText.left + _rcTextPadding.left + _rcTextPadding.right, rcText.bottom - rcText.top + _rcTextPadding.top + _rcTextPadding.bottom};

    if (_cxyFixed.cy != 0) {
        cXY.cy = _cxyFixed.cy;
    }

	return cXY;
}
// 
// void Text::setAttribute(const String& name, const String& value)
// {
//     if (name == "linkstyle") {
//         SetNormalImage(value);
//     }
//     else {
//         Label::setAttribute(name, value);
//     }
// }
// 

void Text::PaintText(HDC hDC)
{
	zgui::String text;
	if (_pTextCallback != 0) {
		text = _pTextCallback->getText(this);
	}
	else {
		text = _text;
	}

	if (text.isEmpty()) {
		_nLinks = 0;
		return;
	}

    if (_textColor == 0) {
        _textColor = _pManager->GetDefaultFontColor();
    }
    if (_disabledTextColor == 0) {
        _disabledTextColor = _pManager->GetDefaultDisabledColor();
    }

	_nLinks = lengthof(_rcLinks);
	RECT rc = _rcItem;
	rc.left += _rcTextPadding.left;
	rc.right -= _rcTextPadding.right;
	rc.top += _rcTextPadding.top;
	rc.bottom -= _rcTextPadding.bottom;
    uint32_t color = (IsEnabled() ? _textColor : _disabledTextColor); 
    if (m_bShowHtml) {
        RenderEngine::drawHtmlText(hDC, _pManager, rc, text, color, _rcLinks, _sLinks, _nLinks, _uTextStyle);
    }
    else {
        RenderEngine::DrawText(hDC, _pManager, rc, text, color, m_iFont, _uTextStyle);
    }
}

}

#endif // ZGUI_USE_TEXT