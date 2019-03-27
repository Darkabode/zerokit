#include "zgui.h"

#ifdef ZGUI_USE_LABEL

#ifdef ZGUI_USE_BUTTON

namespace zgui
{
	CButtonUI::CButtonUI() :
    m_uButtonState(0),
    m_dwHotTextColor(0),
    m_dwPushedTextColor(0),
    m_dwFocusedTextColor(0),
    m_dwHotBkColor(0)
	{
		m_uTextStyle = DT_SINGLELINE | DT_VCENTER | DT_CENTER;
	}

	LPCTSTR CButtonUI::GetClass() const
	{
		return _T("ButtonUI");
	}

	LPVOID CButtonUI::GetInterface(LPCTSTR pstrName)
	{
        if (lstrcmp(pstrName, DUI_CTR_BUTTON) == 0 ) {
            return static_cast<CButtonUI*>(this);
        }
		return CLabelUI::GetInterface(pstrName);
	}

	UINT CButtonUI::GetControlFlags() const
	{
		return (IsKeyboardEnabled() ? UIFLAG_TABSTOP : 0) | (IsEnabled() ? UIFLAG_SETCURSOR : 0);
	}

	void CButtonUI::DoEvent(TEventUI& event)
	{
		if( !IsMouseEnabled() && event.Type > UIEVENT__MOUSEBEGIN && event.Type < UIEVENT__MOUSEEND ) {
			if( _pParent != NULL ) _pParent->DoEvent(event);
			else CLabelUI::DoEvent(event);
			return;
		}

		if( event.Type == UIEVENT_SETFOCUS ) 
		{
			Invalidate();
		}
		if( event.Type == UIEVENT_KILLFOCUS ) 
		{
			Invalidate();
		}
		if( event.Type == UIEVENT_KEYDOWN )
		{
			if (IsKeyboardEnabled()) {
				if( event.chKey == VK_SPACE || event.chKey == VK_RETURN ) {
					Activate();
					return;
				}
			}
		}
		if( event.Type == UIEVENT_BUTTONDOWN || event.Type == UIEVENT_DBLCLICK )
		{
			if( ::PtInRect(&_rcItem, event.ptMouse) && IsEnabled() ) {
				m_uButtonState |= UISTATE_PUSHED | UISTATE_CAPTURED;
				Invalidate();
			}
			return;
		}
		if( event.Type == UIEVENT_MOUSEMOVE )
		{
			if( (m_uButtonState & UISTATE_CAPTURED) != 0 ) {
				if( ::PtInRect(&_rcItem, event.ptMouse) ) m_uButtonState |= UISTATE_PUSHED;
				else m_uButtonState &= ~UISTATE_PUSHED;
				Invalidate();
			}
			return;
		}
		if( event.Type == UIEVENT_BUTTONUP )
		{
			if( (m_uButtonState & UISTATE_CAPTURED) != 0 ) {
				if( ::PtInRect(&_rcItem, event.ptMouse) ) Activate();
				m_uButtonState &= ~(UISTATE_PUSHED | UISTATE_CAPTURED);
				Invalidate();
			}
			return;
		}
		if( event.Type == UIEVENT_CONTEXTMENU )
		{
			if( IsContextMenuUsed() ) {
				_pManager->SendNotify(this, DUI_MSGTYPE_MENU, event.wParam, event.lParam);
			}
			return;
		}
		if( event.Type == UIEVENT_MOUSEENTER )
		{
			if( IsEnabled() ) {
				m_uButtonState |= UISTATE_HOT;
				Invalidate();
			}
			// return;
		}
		if( event.Type == UIEVENT_MOUSELEAVE )
		{
			if( IsEnabled() ) {
				m_uButtonState &= ~UISTATE_HOT;
				Invalidate();
			}
			// return;
		}
		if( event.Type == UIEVENT_SETCURSOR ) {
			::SetCursor(::LoadCursor(NULL, MAKEINTRESOURCE(IDC_HAND)));
			return;
		}
		CLabelUI::DoEvent(event);
	}

	bool CButtonUI::Activate()
	{
		if( !CControlUI::Activate() ) return false;
		if( _pManager != NULL ) _pManager->SendNotify(this, DUI_MSGTYPE_CLICK);
		return true;
	}

	void CButtonUI::SetEnabled(bool bEnable)
	{
		CControlUI::SetEnabled(bEnable);
		if( !IsEnabled() ) {
			m_uButtonState = 0;
		}
	}

    void CButtonUI::SetHotBkColor(DWORD dwColor)
	{
		m_dwHotBkColor = dwColor;
	}

	DWORD CButtonUI::GetHotBkColor() const
	{
		return m_dwHotBkColor;
	}

	void CButtonUI::SetHotTextColor(DWORD dwColor)
	{
		m_dwHotTextColor = dwColor;
	}

	DWORD CButtonUI::GetHotTextColor() const
	{
		return m_dwHotTextColor;
	}

	void CButtonUI::SetPushedTextColor(DWORD dwColor)
	{
		m_dwPushedTextColor = dwColor;
	}

	DWORD CButtonUI::GetPushedTextColor() const
	{
		return m_dwPushedTextColor;
	}

	void CButtonUI::SetFocusedTextColor(DWORD dwColor)
	{
		m_dwFocusedTextColor = dwColor;
	}

	DWORD CButtonUI::GetFocusedTextColor() const
	{
		return m_dwFocusedTextColor;
	}

	const String& CButtonUI::GetNormalImage()
	{
		return _normalImageName;
	}

	void CButtonUI::SetNormalImage(const String& pStrImage)
	{
		_normalImageName = pStrImage;
		Invalidate();
	}

	const String& CButtonUI::GetHotImage()
	{
		return _hotImageName;
	}

	void CButtonUI::SetHotImage(const String& pStrImage)
	{
		_hotImageName = pStrImage;
		Invalidate();
	}

	const String& CButtonUI::GetPushedImage()
	{
		return _pushedImageName;
	}

	void CButtonUI::SetPushedImage(const String& pStrImage)
	{
		_pushedImageName = pStrImage;
		Invalidate();
	}

	const String& CButtonUI::GetFocusedImage()
	{
		return _focusedImageName;
	}

	void CButtonUI::SetFocusedImage(const String& pStrImage)
	{
		_focusedImageName = pStrImage;
		Invalidate();
	}

	const String& CButtonUI::GetDisabledImage()
	{
		return _disabledImageName;
	}

	void CButtonUI::SetDisabledImage(const String& pStrImage)
	{
		_disabledImageName = pStrImage;
		Invalidate();
	}

    const String& CButtonUI::GetForeImage()
	{
		return m_sForeImage;
	}

	void CButtonUI::SetForeImage(const String& pStrImage)
	{
		m_sForeImage = pStrImage;
		Invalidate();
	}

	const String& CButtonUI::GetHotForeImage()
	{
		return m_sHotForeImage;
	}

	void CButtonUI::SetHotForeImage(const String& pStrImage)
	{
		m_sHotForeImage = pStrImage;
		Invalidate();
	}

	SIZE CButtonUI::EstimateSize(SIZE szAvailable)
	{
		if( _cxyFixed.cy == 0 ) return CSize(_cxyFixed.cx, _pManager->GetFontInfo(GetFont())->tm.tmHeight + 8);
		return CControlUI::EstimateSize(szAvailable);
	}

	void CButtonUI::SetAttribute(const String& pstrName, const String& pstrValue)
	{
        if (pstrName == "normalimage") {
            SetNormalImage(pstrValue);
        }
        else if (pstrName == "hotimage") {
            SetHotImage(pstrValue);
        }
        else if (pstrName == "pushedimage") {
            SetPushedImage(pstrValue);
        }
        else if (pstrName == "focusedimage") {
            SetFocusedImage(pstrValue);
        }
        else if (pstrName == "disabledimage") {
            SetDisabledImage(pstrValue);
        }
		else if (pstrName == "foreimage") {
			SetForeImage(pstrValue);
		}
        else if (pstrName == "hotforeimage") {
            SetHotForeImage(pstrValue);
        }
        else if (pstrName == "hotbkcolor") {
            SetHotBkColor((uint32_t)pstrValue.getHexValue32());
        }
        else if (pstrName == "hottextcolor") {
            SetHotTextColor((uint32_t)pstrValue.getHexValue32());
        }
		else if (pstrName == "pushedtextcolor") {
			SetPushedTextColor((uint32_t)pstrValue.getHexValue32());
		}
		else if (pstrName == "focusedtextcolor") {
			SetFocusedTextColor((uint32_t)pstrValue.getHexValue32());
		}
        else {
            CLabelUI::SetAttribute(pstrName, pstrValue);
        }
	}

	void CButtonUI::PaintText(HDC hDC)
	{
        if (IsFocused()) {
            m_uButtonState |= UISTATE_FOCUSED;
        }
        else {
            m_uButtonState &= ~ UISTATE_FOCUSED;
        }
        if (!IsEnabled()) {
            m_uButtonState |= UISTATE_DISABLED;
        }
        else {
            m_uButtonState &= ~ UISTATE_DISABLED;
        }

        if (m_dwTextColor == 0) {
            m_dwTextColor = _pManager->GetDefaultFontColor();
        }
        if (m_dwDisabledTextColor == 0) {
            m_dwDisabledTextColor = _pManager->GetDefaultDisabledColor();
        }

        if (_text.isEmpty()) {
            return;
        }
		int nLinks = 0;
		RECT rc = _rcItem;
		rc.left += m_rcTextPadding.left;
		rc.right -= m_rcTextPadding.right;
		rc.top += m_rcTextPadding.top;
		rc.bottom -= m_rcTextPadding.bottom;

		DWORD clrColor = IsEnabled()?m_dwTextColor:m_dwDisabledTextColor;

		if( ((m_uButtonState & UISTATE_PUSHED) != 0) && (GetPushedTextColor() != 0) )
			clrColor = GetPushedTextColor();
		else if( ((m_uButtonState & UISTATE_HOT) != 0) && (GetHotTextColor() != 0) )
			clrColor = GetHotTextColor();
		else if( ((m_uButtonState & UISTATE_FOCUSED) != 0) && (GetFocusedTextColor() != 0) )
			clrColor = GetFocusedTextColor();

		if( m_bShowHtml )
			CRenderEngine::DrawHtmlText(hDC, _pManager, rc, _text, clrColor, \
			NULL, NULL, nLinks, m_uTextStyle);
		else
			CRenderEngine::DrawText(hDC, _pManager, rc, _text, clrColor, \
			m_iFont, m_uTextStyle);
	}

	void CButtonUI::PaintStatusImage(HDC hDC)
	{
        if (IsFocused()) {
            m_uButtonState |= UISTATE_FOCUSED;
        }
        else {
            m_uButtonState &= ~ UISTATE_FOCUSED;
        }
		
        if (!IsEnabled()) {
            m_uButtonState |= UISTATE_DISABLED;
        }
        else {
            m_uButtonState &= ~ UISTATE_DISABLED;
        }

		if ((m_uButtonState & UISTATE_DISABLED) != 0) {
			if (!_disabledImageName.isEmpty()) {
                if (!DrawImage(hDC, _disabledImageName)) {
                    _disabledImageName = String::empty;
                }
                else {
                    goto Label_ForeImage;
                }
			}
		}
		else if ((m_uButtonState & UISTATE_PUSHED) != 0) {
			if (!_pushedImageName.isEmpty() ) {
                if (!DrawImage(hDC, _pushedImageName)) {
                    _pushedImageName = String::empty;
                }
                if (!m_sPushedForeImage.isEmpty()) {
                    if (!DrawImage(hDC, m_sPushedForeImage)) {
                        m_sPushedForeImage = String::empty;
                    }
                    return;
                }
                else {
                    goto Label_ForeImage;
                }
			}
		}
		else if ((m_uButtonState & UISTATE_HOT) != 0) {
			if (!_hotImageName.isEmpty()) {
                if (!DrawImage(hDC, _hotImageName)) {
                    _hotImageName = String::empty;
                }
                if (!m_sHotForeImage.isEmpty()) {
                    if (!DrawImage(hDC, m_sHotForeImage)) {
                        m_sHotForeImage = String::empty;
                    }
                    return;
                }
                else {
                    goto Label_ForeImage;
                }
			}
            else if (m_dwHotBkColor != 0) {
                CRenderEngine::DrawColor(hDC, _rcPaint, GetAdjustColor(m_dwHotBkColor));
                return;
            }
		}
		else if( (m_uButtonState & UISTATE_FOCUSED) != 0 ) {
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

        if (!m_sForeImage.isEmpty()) {
            goto Label_ForeImage;
        }

        return;

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