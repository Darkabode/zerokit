#include "zgui.h"

#ifdef ZGUI_USE_LABEL

#ifdef ZGUI_USE_BUTTON

namespace zgui {

const String Button::CLASS_NAME = ZGUI_BUTTON;

Button::Button() :
_buttonState(0),
_dwHotTextColor(0),
_dwPushedTextColor(0),
_dwFocusedTextColor(0),
_dwHotBkColor(0),
_dwPushedBkColor(0),
_dwDisabledBkColor(0),
_selected(false),
_toggledOnClicking(false),
_textSized(false)
{
	_uTextStyle = DT_SINGLELINE | DT_VCENTER | DT_CENTER;
}

Button::~Button()
{
	if (!_groupName.isEmpty() && _pManager != 0) {
		_pManager->RemoveOptionGroup(_groupName, this);
	}
}

const String& Button::getClass() const
{
	return CLASS_NAME;
}

LPVOID Button::getInterface(const String& name)
{
	if (name == ZGUI_BUTTON) {
        return static_cast<Button*>(this);
    }
	return Label::getInterface(name);
}

UINT Button::GetControlFlags() const
{
	return (IsKeyboardEnabled() ? UIFLAG_TABSTOP : 0) | (IsEnabled() ? UIFLAG_SETCURSOR : 0);
}

void Button::DoEvent(TEventUI& event)
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

	if (event.Type == UIEVENT_SETFOCUS) {
		Invalidate();
	}
	else if (event.Type == UIEVENT_KILLFOCUS) {
		Invalidate();
	}
	else if (event.Type == UIEVENT_KEYDOWN) {
		if (IsKeyboardEnabled()) {
			if (event.chKey == VK_SPACE || event.chKey == VK_RETURN) {
				Activate();
				return;
			}
		}
	}
	else if (event.Type == UIEVENT_BUTTONDOWN || event.Type == UIEVENT_DBLCLICK) {
		if (fn_PtInRect(&_rcItem, event.ptMouse) && IsEnabled()) {
			_buttonState |= UISTATE_PUSHED | UISTATE_CAPTURED;
			Invalidate();
		}
		return;
	}
	else if (event.Type == UIEVENT_MOUSEMOVE) {
		if ((_buttonState & UISTATE_CAPTURED) != 0) {
            if (fn_PtInRect(&_rcItem, event.ptMouse)) {
                _buttonState |= UISTATE_PUSHED;
            }
            else {
                _buttonState &= ~UISTATE_PUSHED;
            }
			Invalidate();
		}
		return;
	}
	else if (event.Type == UIEVENT_BUTTONUP) {
		if ((_buttonState & UISTATE_CAPTURED) != 0) {
            if (fn_PtInRect(&_rcItem, event.ptMouse)) {
                Activate();
            }
			_buttonState &= ~(UISTATE_PUSHED | UISTATE_CAPTURED);
			Invalidate();
		}
		return;
	}
	else if (event.Type == UIEVENT_CONTEXTMENU) {
		if (IsContextMenuUsed()) {
			_pManager->SendNotify(this, ZGUI_MSGTYPE_MENU, event.wParam, event.lParam);
		}
		return;
	}
	else if (event.Type == UIEVENT_MOUSEENTER) {
		if (IsEnabled()) {
			_buttonState |= UISTATE_HOT;
			Invalidate();
		}
		// return;
	}
	else if (event.Type == UIEVENT_MOUSELEAVE) {
		if( IsEnabled() ) {
			_buttonState &= ~UISTATE_HOT;
			Invalidate();
		}
		// return;
	}
	else if (event.Type == UIEVENT_SETCURSOR) {
		fn_SetCursor(fn_LoadCursorW(NULL, MAKEINTRESOURCE(IDC_HAND)));
		return;
	}

	Label::DoEvent(event);
}

void Button::SetManager(PaintManager* pManager, Control* pParent, bool bInit)
{
	Control::SetManager(pManager, pParent, bInit);
	if (bInit && !_groupName.isEmpty()) {
		if (_pManager != 0) {
			_pManager->AddOptionGroup(_groupName, this);
		}
	}
}

bool Button::Activate()
{
	if (!Control::Activate()) {
		return false;
	}
	
	if (!_groupName.isEmpty()) {
		setSelected(true);
	}
	else if (_toggledOnClicking) {
		setSelected(!_selected);
	}

	if (_pManager != 0) {
		_pManager->SendNotify(this, ZGUI_MSGTYPE_CLICK);
	}

	return true;
}

void Button::SetEnabled(bool bEnable)
{
	Control::SetEnabled(bEnable);
	if (!IsEnabled()) {
		_buttonState = 0;
		if (_selected) {
			_buttonState = UISTATE_SELECTED;
		}
	}
}

void Button::SetHotBkColor(DWORD dwColor)
{
	_dwHotBkColor = dwColor;
}

DWORD Button::GetHotBkColor() const
{
	return _dwHotBkColor;
}

void Button::SetPushedBkColor(DWORD dwColor)
{
	_dwPushedBkColor = dwColor;
}

DWORD Button::GetPushedBkColor() const
{
	return _dwPushedBkColor;
}

void Button::setDisabledBkColor(DWORD dwColor)
{
	_dwDisabledBkColor = dwColor;
}

DWORD Button::getDisabledBkColor() const
{
	return _dwDisabledBkColor;
}

void Button::SetHotTextColor(DWORD dwColor)
{
	_dwHotTextColor = dwColor;
}

DWORD Button::GetHotTextColor() const
{
	return _dwHotTextColor;
}

void Button::SetPushedTextColor(DWORD dwColor)
{
	_dwPushedTextColor = dwColor;
}

DWORD Button::GetPushedTextColor() const
{
	return _dwPushedTextColor;
}

void Button::SetFocusedTextColor(DWORD dwColor)
{
	_dwFocusedTextColor = dwColor;
}

DWORD Button::GetFocusedTextColor() const
{
	return _dwFocusedTextColor;
}

const String& Button::GetNormalImage()
{
	return _normalImageName;
}

void Button::SetNormalImage(const String& pStrImage)
{
	_normalImageName = pStrImage;
	Invalidate();
}

const String& Button::GetHotImage()
{
	return _hotImageName;
}

void Button::SetHotImage(const String& pStrImage)
{
	_hotImageName = pStrImage;
	Invalidate();
}

const String& Button::GetPushedImage()
{
	return _pushedImageName;
}

void Button::SetPushedImage(const String& pStrImage)
{
	_pushedImageName = pStrImage;
	Invalidate();
}

const String& Button::GetFocusedImage()
{
	return _focusedImageName;
}

void Button::SetFocusedImage(const String& pStrImage)
{
	_focusedImageName = pStrImage;
	Invalidate();
}

const String& Button::GetDisabledImage()
{
	return _disabledImageName;
}

void Button::SetDisabledImage(const String& pStrImage)
{
	_disabledImageName = pStrImage;
	Invalidate();
}

const String& Button::GetForeImage()
{
	return m_sForeImage;
}

void Button::SetForeImage(const String& pStrImage)
{
	m_sForeImage = pStrImage;
	Invalidate();
}

const String& Button::GetHotForeImage()
{
	return _sHotForeImage;
}

void Button::SetHotForeImage(const String& pStrImage)
{
	_sHotForeImage = pStrImage;
	Invalidate();
}

const String& Button::GetPushedForeImage()
{
	return _sPushedForeImage;
}

void Button::SetPushedForeImage(const String& pStrImage)
{
	_sPushedForeImage = pStrImage;
	Invalidate();
}

const String& Button::GetGroup() const
{
	return _groupName;
}

void Button::SetGroup(const String& pStrGroupName)
{
	if (pStrGroupName.isEmpty()) {
		if (_groupName.isEmpty()) {
			return;
		}
		_groupName = String::empty;
	}
	else {
		if (_groupName == pStrGroupName) {
			return;
		}
		if (!_groupName.isEmpty() && _pManager) {
			_pManager->RemoveOptionGroup(_groupName, this);
		}
		_groupName = pStrGroupName;
	}

	if (!_groupName.isEmpty()) {
		if (_pManager != 0) {
			_pManager->AddOptionGroup(_groupName, this);
		}
	}
	else {
		if (_pManager != 0) {
			_pManager->RemoveOptionGroup(_groupName, this);
		}
	}

	setSelected(_selected);
}

SIZE Button::EstimateSize(SIZE szAvailable)
{
	if (_textSized) {
		RECT rcText = {0, 0, MAX(szAvailable.cx, _cxyFixed.cx), 9999};
		rcText.left += _rcTextPadding.left;
		rcText.right -= _rcTextPadding.right;
		if (m_bShowHtml) {
			int nLinks = 0;
			RenderEngine::drawHtmlText(_pManager->GetPaintDC(), _pManager, rcText, _text, _textColor, NULL, NULL, nLinks, DT_CALCRECT | _uTextStyle);
		}
		else {
			RenderEngine::DrawText(_pManager->GetPaintDC(), _pManager, rcText, _text, _textColor, m_iFont, DT_CALCRECT | _uTextStyle);
		}
		SIZE cXY = {rcText.right - rcText.left + _rcTextPadding.left + _rcTextPadding.right, rcText.bottom - rcText.top + _rcTextPadding.top + _rcTextPadding.bottom};

		if (_cxyFixed.cy != 0) {
			cXY.cy = _cxyFixed.cy;
		}

		return cXY;
	}
	if (_cxyFixed.cy == 0) {
		return Size(_cxyFixed.cx, _pManager->GetFontInfo(GetFont())->tm.tmHeight + 8);
	}
	return Control::EstimateSize(szAvailable);
}

void Button::setAttribute(const String& name, const String& value)
{
    if (name == "normalimage") {
        SetNormalImage(value);
    }
    else if (name == "hotimage") {
        SetHotImage(value);
    }
    else if (name == "pushedimage") {
        SetPushedImage(value);
    }
    else if (name == "focusedimage") {
        SetFocusedImage(value);
    }
    else if (name == "disabledimage") {
        SetDisabledImage(value);
    }
	else if (name == "foreimage") {
		SetForeImage(value);
	}
    else if (name == "hotforeimage") {
        SetHotForeImage(value);
    }
	else if (name == "pushedforeimage") {
		SetPushedForeImage(value);
	}
    else if (name == "hotbkcolor") {
        SetHotBkColor((uint32_t)value.getHexValue32());
    }
	else if (name == "pushedbkcolor") {
		SetPushedBkColor((uint32_t)value.getHexValue32());
	}
	else if (name == "disabledbkcolor") {
		setDisabledBkColor((uint32_t)value.getHexValue32());
	}
    else if (name == "hottextcolor") {
        SetHotTextColor((uint32_t)value.getHexValue32());
    }
	else if (name == "pushedtextcolor") {
		SetPushedTextColor((uint32_t)value.getHexValue32());
	}
	else if (name == "focusedtextcolor") {
		SetFocusedTextColor((uint32_t)value.getHexValue32());
	}
	else if (name == "group") {
		SetGroup(value);
	}
	else if (name == "toggledonclicking") {
		setToggledOnClicking(value == "true");
	}
	else if (name == "selected") {
		setSelected(value == "true");
	}
	else if (name == "textsized") {
		_textSized = (value == "true");
	}
    else {
        Label::setAttribute(name, value);
    }
}

bool Button::isSelected() const
{
	return _selected;
}

void Button::setSelected(bool selected)
{
	if (_selected == selected) {
		return;
	}
	_selected = selected;
	if (_selected) {
		_buttonState |= UISTATE_SELECTED;
	}
	else {
		_buttonState &= ~UISTATE_SELECTED;
	}
	if (_pManager != 0) {
		if (!_groupName.isEmpty()) {
			if (_selected) {
				Array<void*>* aOptionGroup = _pManager->GetOptionGroup(_groupName);
				for (int i = aOptionGroup->size(); --i >= 0; ) {
					Button* pControl = static_cast<Button*>(aOptionGroup->getUnchecked(i));
					if (pControl != this) {
						pControl->setSelected(false);
					}
				}
				_pManager->SendNotify(this, ZGUI_MSGTYPE_SELECTCHANGED);
			}
		}
		else {
			_pManager->SendNotify(this, ZGUI_MSGTYPE_SELECTCHANGED);
		}
	}

	Invalidate();
}

void Button::setToggledOnClicking(bool toggled)
{
	_toggledOnClicking = toggled;
}

void Button::PaintText(HDC hDC)
{
    if (IsFocused()) {
        _buttonState |= UISTATE_FOCUSED;
    }
    else {
        _buttonState &= ~ UISTATE_FOCUSED;
    }
    if (!IsEnabled()) {
        _buttonState |= UISTATE_DISABLED;
    }
    else {
        _buttonState &= ~ UISTATE_DISABLED;
    }

    if (_textColor == 0) {
        _textColor = _pManager->GetDefaultFontColor();
    }
    if (_disabledTextColor == 0) {
        _disabledTextColor = _pManager->GetDefaultDisabledColor();
    }

    if (_text.isEmpty()) {
        return;
    }
	int nLinks = 0;
	RECT rc = _rcItem;
	rc.left += _rcTextPadding.left;
	rc.right -= _rcTextPadding.right;
	rc.top += _rcTextPadding.top;
	rc.bottom -= _rcTextPadding.bottom;

	DWORD clrColor = (IsEnabled() ? _textColor : _disabledTextColor);

	if (GetPushedTextColor() != 0 && (_groupName.isEmpty() && !_toggledOnClicking && (_buttonState & UISTATE_PUSHED) != 0) || (_buttonState & UISTATE_SELECTED) != 0) {
		clrColor = GetPushedTextColor();
	}
	else if (GetHotTextColor() != 0 && (_buttonState & UISTATE_HOT) != 0) {
		clrColor = GetHotTextColor();
	}
	else if (GetFocusedTextColor() != 0 && (_buttonState & UISTATE_FOCUSED) != 0) {
		clrColor = GetFocusedTextColor();
	}

	if (m_bShowHtml) {
		RenderEngine::drawHtmlText(hDC, _pManager, rc, _text, clrColor, NULL, NULL, nLinks, _uTextStyle);
	}
	else {
		RenderEngine::DrawText(hDC, _pManager, rc, _text, clrColor, m_iFont, _uTextStyle);
	}
}

void Button::PaintStatusImage(HDC hDC)
{
    if (IsFocused()) {
        _buttonState |= UISTATE_FOCUSED;
    }
    else {
        _buttonState &= ~UISTATE_FOCUSED;
    }
	
    if (!IsEnabled()) {
        _buttonState |= UISTATE_DISABLED;
    }
    else {
        _buttonState &= ~UISTATE_DISABLED;
    }

	if ((_buttonState & UISTATE_DISABLED) != 0) {
		if (!_disabledImageName.isEmpty()) {
            if (!DrawImage(hDC, _disabledImageName)) {
                _disabledImageName = String::empty;
            }
            else {
                goto Label_ForeImage;
            }
		}
		else if (_dwPushedBkColor != 0) {
			RenderEngine::DrawColor(hDC, _rcPaint, GetAdjustColor(_dwDisabledBkColor));
		}
	}
	else if ((_groupName.isEmpty() && !_toggledOnClicking && (_buttonState & UISTATE_PUSHED) != 0) || (_buttonState & UISTATE_SELECTED) != 0) {
		if (!_pushedImageName.isEmpty() ) {
            if (!DrawImage(hDC, _pushedImageName)) {
                _pushedImageName = String::empty;
            }
		}
		else if (_dwPushedBkColor != 0) {
			RenderEngine::DrawColor(hDC, _rcPaint, GetAdjustColor(_dwPushedBkColor));
		}

		if (!_sPushedForeImage.isEmpty()) {
			if (!DrawImage(hDC, _sPushedForeImage)) {
				_sPushedForeImage = String::empty;
			}
			return;
		}
		else {
			goto Label_ForeImage;
		}
	}
	else if ((_buttonState & UISTATE_HOT) != 0) {
		if (!_hotImageName.isEmpty()) {
            if (!DrawImage(hDC, _hotImageName)) {
                _hotImageName = String::empty;
            }
		}
        else if (_dwHotBkColor != 0) {
            RenderEngine::DrawColor(hDC, _rcPaint, GetAdjustColor(_dwHotBkColor));
        }
		
		if (!_sHotForeImage.isEmpty()) {
			if (!DrawImage(hDC, _sHotForeImage)) {
				_sHotForeImage = String::empty;
			}
			return;
		}
		else {
			goto Label_ForeImage;
		}
	}
	else if( (_buttonState & UISTATE_FOCUSED) != 0 ) {
		if( !_focusedImageName.isEmpty() ) {
            if (!DrawImage(hDC, _focusedImageName)) {
                _focusedImageName = String::empty;
            }
            else {
                goto Label_ForeImage;
            }
		}
	}

	if( !_normalImageName.isEmpty() ) {
        if (!DrawImage(hDC, _normalImageName)) {
            _normalImageName = String::empty;
        }
        else {
            goto Label_ForeImage;
        }
	}

//         if (!m_sForeImage.isEmpty()) {
//             goto Label_ForeImage;
//         }
// 
//         return;

Label_ForeImage:
    if (!m_sForeImage.isEmpty()) {
        if (!DrawImage(hDC, m_sForeImage)) {
            m_sForeImage = String::empty;
        }
    }
}

}

#endif // ZGUI_USE_BUTTON

#endif // ZGUI_USE_LABEL