#include "zgui.h"

#ifdef ZGUI_USE_LABEL

#ifdef ZGUI_USE_EDIT

namespace zgui {

class CEditWnd : public Window
{
public:
	CEditWnd();

	void Init(Edit* pOwner);
	RECT CalPos();

	const String& GetWindowClassName() const;
	LPCTSTR GetSuperClassName() const;
	void OnFinalMessage(HWND hWnd);

	LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnKillFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnEditChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

protected:
	Edit* m_pOwner;
	HBRUSH m_hBkBrush;
	bool m_bInit;
private:
    static const String CLASS_NAME;
};


const String CEditWnd::CLASS_NAME = "EditWnd";

CEditWnd::CEditWnd() :
m_pOwner(0),
m_hBkBrush(0),
m_bInit(false)
{
}

void CEditWnd::Init(Edit* pOwner)
{
	m_pOwner = pOwner;
	RECT rcPos = CalPos();
	UINT uStyle = WS_CHILD | ES_AUTOHSCROLL;
    if (m_pOwner->IsPasswordMode()) {
        uStyle |= ES_PASSWORD;
    }
	Create(m_pOwner->getManager()->GetPaintWindow(), NULL, uStyle, 0, rcPos);
	HFONT hFont = 0;
	int iFontIndex=m_pOwner->GetFont();
    if (iFontIndex!=-1) {
		hFont=m_pOwner->getManager()->GetFont(iFontIndex);
    }
    if (hFont == 0) {
		hFont=m_pOwner->getManager()->GetDefaultFontInfo()->hFont;
    }

    fn_SendMessageW(_hWnd, WM_SETFONT, (WPARAM)(HFONT)hFont, (LPARAM)TRUE);

	fn_SendMessageW(_hWnd, EM_LIMITTEXT, (WPARAM)m_pOwner->GetMaxChar(), 0L);
    if (m_pOwner->IsPasswordMode()) {
        fn_SendMessageW(_hWnd, EM_SETPASSWORDCHAR, (WPARAM)(UINT)m_pOwner->GetPasswordChar(), 0L);
    }
    
	fn_SetWindowTextW(_hWnd, m_pOwner->getText().toWideCharPointer());
    fn_SendMessageW(_hWnd, EM_SETMODIFY, (WPARAM)(UINT)FALSE, 0L);
    Window::sendMessage(EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELPARAM(0, 0));
	fn_EnableWindow(_hWnd, m_pOwner->IsEnabled() == true);
    fn_SendMessageW(_hWnd, EM_SETREADONLY, (WPARAM)(BOOL)(m_pOwner->IsReadOnly() == true), 0L);
	//Styls
	LONG styleValue = fn_GetWindowLongW(_hWnd, GWL_STYLE);
	styleValue |= pOwner->GetWindowStyls();
	fn_SetWindowLongW(getHWND(), GWL_STYLE, styleValue);
	fn_ShowWindow(_hWnd, SW_SHOWNOACTIVATE);
	fn_SetFocus(_hWnd);
	m_bInit = true;    
}

RECT CEditWnd::CalPos()
{
    zgui::Rect rcPos = m_pOwner->GetPos();
	RECT rcInset = m_pOwner->GetTextPadding();
	rcPos.left += rcInset.left;
	rcPos.top += rcInset.top;
	rcPos.right -= rcInset.right;
	rcPos.bottom -= rcInset.bottom;
	LONG lEditHeight = m_pOwner->getManager()->GetFontInfo(m_pOwner->GetFont())->tm.tmHeight;
	if (lEditHeight < rcPos.getHeight()) {
		rcPos.top += (rcPos.getHeight() - lEditHeight) / 2;
		rcPos.bottom = rcPos.top + lEditHeight;
	}
	return rcPos;
}

const String& CEditWnd::GetWindowClassName() const
{
	return CLASS_NAME;
}

LPCTSTR CEditWnd::GetSuperClassName() const
{
	return WC_EDIT;
}

void CEditWnd::OnFinalMessage(HWND /*hWnd*/)
{
	m_pOwner->Invalidate();
	// Clear reference and die
    if (m_hBkBrush != 0) {
        fn_DeleteObject(m_hBkBrush);
    }
	m_pOwner->m_pWindow = 0;
	delete this;
}

LRESULT CEditWnd::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT lRes = 0;
	BOOL bHandled = TRUE;
    if (uMsg == WM_KILLFOCUS) {
        lRes = OnKillFocus(uMsg, wParam, lParam, bHandled);
    }
	else if (uMsg == OCM_COMMAND) {
        if (GET_WM_COMMAND_CMD(wParam, lParam) == EN_CHANGE) {
            lRes = OnEditChanged(uMsg, wParam, lParam, bHandled);
        }
		else if (GET_WM_COMMAND_CMD(wParam, lParam) == EN_UPDATE) {
			RECT rcClient;
			fn_GetClientRect(_hWnd, &rcClient);
			fn_InvalidateRect(_hWnd, &rcClient, FALSE);
		}
	}
	else if (uMsg == WM_KEYDOWN && wchar_t(wParam) == VK_RETURN) {
		m_pOwner->getManager()->SendNotify(m_pOwner, ZGUI_MSGTYPE_RETURN);
	}
	else if (uMsg == OCM__BASE + WM_CTLCOLOREDIT  || uMsg == OCM__BASE + WM_CTLCOLORSTATIC ) {
        if (m_pOwner->GetNativeEditBkColor() == 0xFFFFFFFF) {
            return 0;
        }
		fn_SetBkMode((HDC)wParam, TRANSPARENT);
		DWORD dwTextColor = m_pOwner->GetTextColor();
		fn_SetTextColor((HDC)wParam, RGB(GetBValue(dwTextColor),GetGValue(dwTextColor),GetRValue(dwTextColor)));
		if (m_hBkBrush == 0) {
			DWORD clrColor = m_pOwner->GetNativeEditBkColor();
			m_hBkBrush = fn_CreateSolidBrush(RGB(GetBValue(clrColor), GetGValue(clrColor), GetRValue(clrColor)));
		}
		return (LRESULT)m_hBkBrush;
	}
    else {
        bHandled = FALSE;
    }
    if (!bHandled) {
        return Window::HandleMessage(uMsg, wParam, lParam);
    }
	return lRes;
}

LRESULT CEditWnd::OnKillFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	LRESULT lRes = fn_DefWindowProcW(_hWnd, uMsg, wParam, lParam);
    Window::postMessage(WM_CLOSE);
	return lRes;
}

LRESULT CEditWnd::OnEditChanged(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    if (!m_bInit) {
        return 0;
    }
    if (m_pOwner == 0) {
        return 0;
    }
	// Copy text back
	int cchLen = fn_GetWindowTextLengthW(_hWnd) + 1;
	LPTSTR pstr = static_cast<LPTSTR>(fn_memalloc(cchLen * sizeof(TCHAR)));
    if (pstr == NULL) {
        return 0;
    }
	fn_GetWindowTextW(_hWnd, pstr, cchLen);
	m_pOwner->_text = pstr;
	m_pOwner->getManager()->SendNotify(m_pOwner, ZGUI_MSGTYPE_TEXTCHANGED);

    fn_memfree(pstr);
	return 0;
}

const String Edit::CLASS_NAME = ZGUI_EDIT;

Edit::Edit() :
m_pWindow(0),
m_uMaxChar(255),
m_bReadOnly(false), 
m_bPasswordMode(false),
m_cPasswordChar(L'*'),
m_uButtonState(0), 
m_dwEditbkColor(0xFFFFFFFF),
m_iWindowStyls(0)
{
	SetTextPadding(Rect(4, 3, 4, 3));
	SetBkColor(0xFFFFFFFF);
}

const String& Edit::getClass() const
{
	return CLASS_NAME;
}

LPVOID Edit::getInterface(const String& name)
{
	if (name == ZGUI_EDIT) {
        return static_cast<Edit*>(this);
    }
	return Label::getInterface(name);
}

UINT Edit::GetControlFlags() const
{
    if (!IsEnabled()) {
        return Control::GetControlFlags();
    }

	return UIFLAG_SETCURSOR | UIFLAG_TABSTOP;
}

void Edit::DoEvent(TEventUI& event)
{
	if (!IsMouseEnabled() && event.Type > UIEVENT__MOUSEBEGIN && event.Type < UIEVENT__MOUSEEND) {
        if (_pParent != NULL) {
            _pParent->DoEvent(event);
        }
        else {
            Label::DoEvent(event);
        }
		return;
	}

	if (event.Type == UIEVENT_SETCURSOR && IsEnabled()) {
		fn_SetCursor(fn_LoadCursorW(NULL, MAKEINTRESOURCE(IDC_IBEAM)));
		return;
	}
	if (event.Type == UIEVENT_WINDOWSIZE) {
		if( m_pWindow != NULL ) _pManager->SetFocusNeeded(this);
	}
	if (event.Type == UIEVENT_SCROLLWHEEL) {
		if( m_pWindow != NULL ) return;
	}
	if (event.Type == UIEVENT_SETFOCUS && IsEnabled()) {
        if (m_pWindow) {
            return;
        }
		m_pWindow = new CEditWnd();
		zgui_assert(m_pWindow);
		m_pWindow->Init(this);
		Invalidate();
	}
	if (event.Type == UIEVENT_KILLFOCUS && IsEnabled()) {
		Invalidate();
	}
	if( event.Type == UIEVENT_BUTTONDOWN || event.Type == UIEVENT_DBLCLICK || event.Type == UIEVENT_RBUTTONDOWN) {
		if (IsEnabled()) {
			getManager()->ReleaseCapture();
			if (IsFocused() && m_pWindow == 0) {
				m_pWindow = new CEditWnd();
				zgui_assert(m_pWindow);
				m_pWindow->Init(this);

				if (fn_PtInRect(&_rcItem, event.ptMouse)) {
					int nSize = fn_GetWindowTextLengthW(*m_pWindow);
                    if (nSize == 0) {
						nSize = 1;
                    }

                    fn_SendMessageW(*m_pWindow, EM_SETSEL, 0, nSize);

				}
			}
			else if (m_pWindow != 0) {
				int nSize = fn_GetWindowTextLengthW(*m_pWindow);
                if (nSize == 0) {
					nSize = 1;
                }

                fn_SendMessageW(*m_pWindow, EM_SETSEL, 0, nSize);
			}
		}
		return;
	}
	if (event.Type == UIEVENT_MOUSEMOVE) {
		return;
	}
	if (event.Type == UIEVENT_BUTTONUP) {
		return;
	}
	if (event.Type == UIEVENT_CONTEXTMENU) {
		return;
	}
	if (event.Type == UIEVENT_MOUSEENTER) {
		if (IsEnabled()) {
			m_uButtonState |= UISTATE_HOT;
			Invalidate();
		}
		return;
	}
	if (event.Type == UIEVENT_MOUSELEAVE) {
		if (IsEnabled()) {
			m_uButtonState &= ~UISTATE_HOT;
			Invalidate();
		}
		return;
	}
	Label::DoEvent(event);
}

void Edit::SetEnabled(bool bEnable)
{
	Control::SetEnabled(bEnable);
	if (!IsEnabled()) {
		m_uButtonState = 0;
	}
}

void Edit::setText(const String& text)
{
    if (text.length() > m_uMaxChar) {
        _text = text.substring(0, m_uMaxChar);
    }
    else {
        _text = text;
    }
	
    if (m_pWindow != 0) {
        fn_SetWindowTextW(*m_pWindow, _text.toWideCharPointer());
    }
	Invalidate();
}

void Edit::SetMaxChar(UINT uMax)
{
	m_uMaxChar = uMax;
    if (m_pWindow != 0) {
        fn_SendMessageW(*m_pWindow, EM_LIMITTEXT, (WPARAM)m_uMaxChar, 0L);
    }
}

UINT Edit::GetMaxChar()
{
	return m_uMaxChar;
}

void Edit::SetReadOnly(bool bReadOnly)
{
    if (m_bReadOnly == bReadOnly) {
        return;
    }

	m_bReadOnly = bReadOnly;
    if (m_pWindow != 0) {
        fn_SendMessageW(*m_pWindow, EM_SETREADONLY, (WPARAM)m_bReadOnly, 0L);
    }
	Invalidate();
}

bool Edit::IsReadOnly() const
{
	return m_bReadOnly;
}

void Edit::SetNumberOnly(bool bNumberOnly)
{
	if (bNumberOnly) {
		m_iWindowStyls |= ES_NUMBER;
	}
	else {
		m_iWindowStyls |= ~ES_NUMBER;
	}
}

bool Edit::IsNumberOnly() const
{
	return m_iWindowStyls&ES_NUMBER ? true:false;
}

int Edit::GetWindowStyls() const 
{
	return m_iWindowStyls;
}

void Edit::SetPasswordMode(bool bPasswordMode)
{
    if (m_bPasswordMode == bPasswordMode) {
        return;
    }
	m_bPasswordMode = bPasswordMode;
	Invalidate();
}

bool Edit::IsPasswordMode() const
{
	return m_bPasswordMode;
}

void Edit::SetPasswordChar(TCHAR cPasswordChar)
{
    if (m_cPasswordChar == cPasswordChar) {
        return;
    }
	m_cPasswordChar = cPasswordChar;
    if (m_pWindow != 0) {
        fn_SendMessageW(*m_pWindow, EM_SETPASSWORDCHAR, (WPARAM)(UINT)m_cPasswordChar, 0L);
    }
	Invalidate();
}

TCHAR Edit::GetPasswordChar() const
{
	return m_cPasswordChar;
}

const String& Edit::GetNormalImage()
{
	return _sNormalImage;
}

void Edit::SetNormalImage(const String& pStrImage)
{
	_sNormalImage = pStrImage;
	Invalidate();
}

const String& Edit::GetHotImage()
{
	return m_sHotImage;
}

void Edit::SetHotImage(const String& pStrImage)
{
	m_sHotImage = pStrImage;
	Invalidate();
}

const String& Edit::GetFocusedImage()
{
	return m_sFocusedImage;
}

void Edit::SetFocusedImage(const String& pStrImage)
{
	m_sFocusedImage = pStrImage;
	Invalidate();
}

const String& Edit::GetDisabledImage()
{
	return m_sDisabledImage;
}

void Edit::SetDisabledImage(const String& pStrImage)
{
	m_sDisabledImage = pStrImage;
	Invalidate();
}

void Edit::SetNativeEditBkColor(DWORD dwBkColor)
{
	m_dwEditbkColor = dwBkColor;
}

DWORD Edit::GetNativeEditBkColor() const
{
	return m_dwEditbkColor;
}

void Edit::SetSel(long nStartChar, long nEndChar)
{
    if (m_pWindow != 0) {
        fn_SendMessageW(*m_pWindow, EM_SETSEL, nStartChar, nEndChar);
    }
}

void Edit::SetSelAll()
{
	SetSel(0,-1);
}

void Edit::SetReplaceSel(LPCTSTR lpszReplace)
{
    if (m_pWindow != 0) {
        fn_SendMessageW(*m_pWindow, EM_REPLACESEL, 0L, (LPARAM)(LPCTSTR)lpszReplace);
    }
}

void Edit::SetPos(RECT rc)
{
	Control::SetPos(rc);
	if (m_pWindow != 0) {
		RECT rcPos = m_pWindow->CalPos();
		fn_SetWindowPos(m_pWindow->getHWND(), NULL, rcPos.left, rcPos.top, rcPos.right - rcPos.left, rcPos.bottom - rcPos.top, SWP_NOZORDER | SWP_NOACTIVATE);        
	}
}

void Edit::SetVisible(bool bVisible)
{
	Control::SetVisible(bVisible);
    if (!IsVisible() && m_pWindow != 0) {
        _pManager->SetFocus(0);
    }
}

void Edit::SetInternVisible(bool bVisible)
{
    if (!IsVisible() && m_pWindow != 0) {
        _pManager->SetFocus(0);
    }
}

SIZE Edit::EstimateSize(SIZE szAvailable)
{
    if (_cxyFixed.cy == 0) {
        return Size(_cxyFixed.cx, _pManager->GetFontInfo(GetFont())->tm.tmHeight + 6);
    }
	return Control::EstimateSize(szAvailable);
}

void Edit::setAttribute(const String& pstrName, const String& pstrValue)
{
    if (pstrName == "readonly") {
        SetReadOnly(pstrValue == "true");
    }
    else if (pstrName == "numberonly") {
        SetNumberOnly(pstrValue == "true");
    }
    else if (pstrName == "password") {
        SetPasswordMode(pstrValue == "true");
    }
    else if (pstrName == "maxchar") {
        SetMaxChar(pstrValue.getIntValue());
    }
    else if (pstrName == "normalimage") {
        SetNormalImage(pstrValue);
    }
    else if (pstrName == "hotimage") {
        SetHotImage(pstrValue);
    }
    else if (pstrName == "focusedimage") {
        SetFocusedImage(pstrValue);
    }
    else if (pstrName == "disabledimage") {
        SetDisabledImage(pstrValue);
    }
	else if (pstrName == "nativebkcolor") {
		SetNativeEditBkColor((uint32_t)pstrValue.getHexValue32());
	}
    else {
        Label::setAttribute(pstrName, pstrValue);
    }
}

void Edit::PaintStatusImage(HDC hDC)
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
		if (!m_sDisabledImage.isEmpty()) {
            if (!DrawImage(hDC, m_sDisabledImage)) {
                m_sDisabledImage = String::empty;
            }
            else {
                return;
            }
		}
	}
	else if ((m_uButtonState & UISTATE_FOCUSED) != 0) {
		if (!m_sFocusedImage.isEmpty()) {
            if (!DrawImage(hDC, m_sFocusedImage)) {
                m_sFocusedImage = String::empty;
            }
            else {
                return;
            }
		}
	}
	else if ((m_uButtonState & UISTATE_HOT) != 0) {
		if (!m_sHotImage.isEmpty()) {
            if (!DrawImage(hDC, m_sHotImage)) {
                m_sHotImage = String::empty;
            }
            else {
                return;
            }
		}
	}

	if( !_sNormalImage.isEmpty() ) {
        if (!DrawImage(hDC, _sNormalImage)) {
            _sNormalImage = String::empty;
        }
        else {
            return;
        }
	}
}

void Edit::PaintText(HDC hDC)
{
    if (_textColor == 0) {
        _textColor = _pManager->GetDefaultFontColor();
    }
    if (_disabledTextColor == 0) {
        _disabledTextColor = _pManager->GetDefaultDisabledColor();
    }

    if (_text.isEmpty()) {
        return;
    }

	String sText = _text;
	if (m_bPasswordMode) {
        sText = String::empty;
		int len = _text.length();
		while (--len >= 0) {
			sText += m_cPasswordChar;
		}
	}

	RECT rc = _rcItem;
	rc.left += _rcTextPadding.left;
	rc.right -= _rcTextPadding.right;
	rc.top += _rcTextPadding.top;
	rc.bottom -= _rcTextPadding.bottom;
	if (IsEnabled()) {
		RenderEngine::DrawText(hDC, _pManager, rc, sText, _textColor, m_iFont, DT_SINGLELINE |_uTextStyle);
	}
	else {
		RenderEngine::DrawText(hDC, _pManager, rc, sText, _disabledTextColor, m_iFont, DT_SINGLELINE | _uTextStyle);

	}
}

}

#endif // ZGUI_USE_EDIT

#endif // ZGUI_USE_LABEL