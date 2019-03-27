#include "zgui.h"

#ifdef ZGUI_USE_SCROLLBAR

namespace zgui {

const String ScrollBar::CLASS_NAME = ZGUI_SCROLLBAR;

ScrollBar::ScrollBar() :
_bHorizontal(false),
m_nRange(100),
m_nScrollPos(0),
m_nLineSize(8), 
_pOwner(NULL),
m_nLastScrollPos(0),
m_nLastScrollOffset(0),
m_nScrollRepeatDelay(0),
m_uButton1State(0),
m_uButton2State(0),
m_uThumbState(0),
m_bShowButton1(true),
m_bShowButton2(true)
{
	_cxyFixed.cx = DEFAULT_SCROLLBAR_SIZE;
	ptLastMouse.x = ptLastMouse.y = 0;
	__stosb((uint8_t*)&m_rcThumb, 0, sizeof(m_rcThumb));
	__stosb((uint8_t*)&m_rcButton1, 0, sizeof(m_rcButton1));
	__stosb((uint8_t*)&m_rcButton2, 0, sizeof(m_rcButton2));
}

const String& ScrollBar::getClass() const
{
	return CLASS_NAME;
}

LPVOID ScrollBar::getInterface(const String& name)
{
	if (name == ZGUI_SCROLLBAR) {
        return static_cast<ScrollBar*>(this);
    }
	return Control::getInterface(name);
}

Container* ScrollBar::GetOwner() const
{
	return _pOwner;
}

void ScrollBar::SetOwner(Container* pOwner)
{
	_pOwner = pOwner;
}

void ScrollBar::SetVisible(bool bVisible, const bool needUpdate)
{
    if (m_bVisible == bVisible) {
        return;
    }

	m_bVisible = bVisible;
    if (m_bFocused) {
        m_bFocused = false;
    }
}

void ScrollBar::SetEnabled(bool bEnable)
{
	Control::SetEnabled(bEnable);
	if( !IsEnabled() ) {
		m_uButton1State = 0;
		m_uButton2State = 0;
		m_uThumbState = 0;
	}
}

void ScrollBar::SetFocus()
{
	if( _pOwner != NULL ) _pOwner->SetFocus();
	else Control::SetFocus();
}

bool ScrollBar::IsHorizontal()
{
	return _bHorizontal;
}

void ScrollBar::SetHorizontal(bool bHorizontal)
{
	if( _bHorizontal == bHorizontal ) return;

	_bHorizontal = bHorizontal;
	if( _bHorizontal ) {
		if( _cxyFixed.cy == 0 ) {
			_cxyFixed.cx = 0;
			_cxyFixed.cy = DEFAULT_SCROLLBAR_SIZE;
		}
	}
	else {
		if( _cxyFixed.cx == 0 ) {
			_cxyFixed.cx = DEFAULT_SCROLLBAR_SIZE;
			_cxyFixed.cy = 0;
		}
	}

	if( _pOwner != NULL ) _pOwner->NeedUpdate(); else NeedParentUpdate();
}

int ScrollBar::GetScrollRange() const
{
	return m_nRange;
}

void ScrollBar::SetScrollRange(int nRange)
{
    if (m_nRange == nRange) {
        return;
    }

	m_nRange = nRange;
    if (m_nRange < 0) {
        m_nRange = 0;
    }
    if (m_nScrollPos > m_nRange) {
        m_nScrollPos = m_nRange;
    }
	SetPos(_rcItem);
}

int ScrollBar::GetScrollPos() const
{
	return m_nScrollPos;
}

void ScrollBar::SetScrollPos(int nPos)
{
    if (m_nScrollPos == nPos) {
        return;
    }

	m_nScrollPos = nPos;
    if (m_nScrollPos < 0 ) {
        m_nScrollPos = 0;
    }
    if (m_nScrollPos > m_nRange) {
        m_nScrollPos = m_nRange;
    }
	SetPos(_rcItem);
}

int ScrollBar::GetLineSize() const
{
	return m_nLineSize;
}

void ScrollBar::SetLineSize(int nSize)
{
	m_nLineSize = nSize;
}

bool ScrollBar::GetShowButton1()
{
	return m_bShowButton1;
}

void ScrollBar::SetShowButton1(bool bShow)
{
	m_bShowButton1 = bShow;
	SetPos(_rcItem);
}

const String& ScrollBar::GetButton1NormalImage()
{
	return _button1NormalImageName;
}

void ScrollBar::SetButton1NormalImage(const String& pStrImage)
{
	_button1NormalImageName = pStrImage;
	Invalidate();
}

const String& ScrollBar::GetButton1HotImage()
{
	return m_sButton1HotImage;
}

void ScrollBar::SetButton1HotImage(const String& pStrImage)
{
	m_sButton1HotImage = pStrImage;
	Invalidate();
}

const String& ScrollBar::GetButton1PushedImage()
{
	return m_sButton1PushedImage;
}

void ScrollBar::SetButton1PushedImage(const String& pStrImage)
{
	m_sButton1PushedImage = pStrImage;
	Invalidate();
}

const String& ScrollBar::GetButton1DisabledImage()
{
	return m_sButton1DisabledImage;
}

void ScrollBar::SetButton1DisabledImage(const String& pStrImage)
{
	m_sButton1DisabledImage = pStrImage;
	Invalidate();
}

bool ScrollBar::GetShowButton2()
{
	return m_bShowButton2;
}

void ScrollBar::SetShowButton2(bool bShow)
{
	m_bShowButton2 = bShow;
	SetPos(_rcItem);
}

const String& ScrollBar::GetButton2NormalImage()
{
	return m_sButton2NormalImage;
}

void ScrollBar::SetButton2NormalImage(const String& pStrImage)
{
	m_sButton2NormalImage = pStrImage;
	Invalidate();
}

const String& ScrollBar::GetButton2HotImage()
{
	return m_sButton2HotImage;
}

void ScrollBar::SetButton2HotImage(const String& pStrImage)
{
	m_sButton2HotImage = pStrImage;
	Invalidate();
}

const String& ScrollBar::GetButton2PushedImage()
{
	return m_sButton2PushedImage;
}

void ScrollBar::SetButton2PushedImage(const String& pStrImage)
{
	m_sButton2PushedImage = pStrImage;
	Invalidate();
}

const String& ScrollBar::GetButton2DisabledImage()
{
	return m_sButton2DisabledImage;
}

void ScrollBar::SetButton2DisabledImage(const String& pStrImage)
{
	m_sButton2DisabledImage = pStrImage;
	Invalidate();
}

const String& ScrollBar::GetThumbNormalImage()
{
	return m_sThumbNormalImage;
}

void ScrollBar::SetThumbNormalImage(const String& pStrImage)
{
	m_sThumbNormalImage = pStrImage;
	Invalidate();
}

const String& ScrollBar::GetThumbHotImage()
{
	return m_sThumbHotImage;
}

void ScrollBar::SetThumbHotImage(const String& pStrImage)
{
	m_sThumbHotImage = pStrImage;
	Invalidate();
}

const String& ScrollBar::GetThumbPushedImage()
{
	return m_sThumbPushedImage;
}

void ScrollBar::SetThumbPushedImage(const String& pStrImage)
{
	m_sThumbPushedImage = pStrImage;
	Invalidate();
}

const String& ScrollBar::GetThumbDisabledImage()
{
	return m_sThumbDisabledImage;
}

void ScrollBar::SetThumbDisabledImage(const String& pStrImage)
{
	m_sThumbDisabledImage = pStrImage;
	Invalidate();
}

const String& ScrollBar::GetRailNormalImage()
{
	return m_sRailNormalImage;
}

void ScrollBar::SetRailNormalImage(const String& pStrImage)
{
	m_sRailNormalImage = pStrImage;
	Invalidate();
}

const String& ScrollBar::GetRailHotImage()
{
	return m_sRailHotImage;
}

void ScrollBar::SetRailHotImage(const String& pStrImage)
{
	m_sRailHotImage = pStrImage;
	Invalidate();
}

const String& ScrollBar::GetRailPushedImage()
{
	return m_sRailPushedImage;
}

void ScrollBar::SetRailPushedImage(const String& pStrImage)
{
	m_sRailPushedImage = pStrImage;
	Invalidate();
}

const String& ScrollBar::GetRailDisabledImage()
{
	return m_sRailDisabledImage;
}

void ScrollBar::SetRailDisabledImage(const String& pStrImage)
{
	m_sRailDisabledImage = pStrImage;
	Invalidate();
}

const String& ScrollBar::GetBkNormalImage()
{
	return m_sBkNormalImage;
}

void ScrollBar::SetBkNormalImage(const String& pStrImage)
{
	m_sBkNormalImage = pStrImage;
	Invalidate();
}

const String& ScrollBar::GetBkHotImage()
{
	return m_sBkHotImage;
}

void ScrollBar::SetBkHotImage(const String& pStrImage)
{
	m_sBkHotImage = pStrImage;
	Invalidate();
}

const String& ScrollBar::GetBkPushedImage()
{
	return m_sBkPushedImage;
}

void ScrollBar::SetBkPushedImage(const String& pStrImage)
{
	m_sBkPushedImage = pStrImage;
	Invalidate();
}

const String& ScrollBar::GetBkDisabledImage()
{
	return m_sBkDisabledImage;
}

void ScrollBar::SetBkDisabledImage(const String& pStrImage)
{
	m_sBkDisabledImage = pStrImage;
	Invalidate();
}

void ScrollBar::SetPos(RECT rc)
{
	Control::SetPos(rc);
	rc = _rcItem;

	if (_bHorizontal) {
		int cx = rc.right - rc.left;
        if (m_bShowButton1) {
            cx -= _cxyFixed.cy;
        }
        if (m_bShowButton2) {
            cx -= _cxyFixed.cy;
        }
		if (cx > _cxyFixed.cy) {
			m_rcButton1.left = rc.left;
			m_rcButton1.top = rc.top;
			if (m_bShowButton1) {
				m_rcButton1.right = rc.left + _cxyFixed.cy;
				m_rcButton1.bottom = rc.top + _cxyFixed.cy;
			}
			else {
				m_rcButton1.right = m_rcButton1.left;
				m_rcButton1.bottom = m_rcButton1.top;
			}

			m_rcButton2.top = rc.top;
			m_rcButton2.right = rc.right;
			if (m_bShowButton2) {
				m_rcButton2.left = rc.right - _cxyFixed.cy;
				m_rcButton2.bottom = rc.top + _cxyFixed.cy;
			}
			else {
				m_rcButton2.left = m_rcButton2.right;
				m_rcButton2.bottom = m_rcButton2.top;
			}

			m_rcThumb.top = rc.top;
			m_rcThumb.bottom = rc.top + _cxyFixed.cy;
			if (m_nRange > 0) {
				int cxThumb = cx * (rc.right - rc.left) / (m_nRange + rc.right - rc.left);
                if (cxThumb < _cxyFixed.cy) {
                    cxThumb = _cxyFixed.cy;
                }

				m_rcThumb.left = m_nScrollPos * (cx - cxThumb) / m_nRange + m_rcButton1.right;
				m_rcThumb.right = m_rcThumb.left + cxThumb;
				if (m_rcThumb.right > m_rcButton2.left) {
					m_rcThumb.left = m_rcButton2.left - cxThumb;
					m_rcThumb.right = m_rcButton2.left;
				}
			}
			else {
				m_rcThumb.left = m_rcButton1.right;
				m_rcThumb.right = m_rcButton2.left;
			}
		}
		else {
			int cxButton = (rc.right - rc.left) / 2;
			if( cxButton > _cxyFixed.cy ) cxButton = _cxyFixed.cy;
			m_rcButton1.left = rc.left;
			m_rcButton1.top = rc.top;
			if( m_bShowButton1 ) {
				m_rcButton1.right = rc.left + cxButton;
				m_rcButton1.bottom = rc.top + _cxyFixed.cy;
			}
			else {
				m_rcButton1.right = m_rcButton1.left;
				m_rcButton1.bottom = m_rcButton1.top;
			}

			m_rcButton2.top = rc.top;
			m_rcButton2.right = rc.right;
			if( m_bShowButton2 ) {
				m_rcButton2.left = rc.right - cxButton;
				m_rcButton2.bottom = rc.top + _cxyFixed.cy;
			}
			else {
				m_rcButton2.left = m_rcButton2.right;
				m_rcButton2.bottom = m_rcButton2.top;
			}

			__stosb((uint8_t*)&m_rcThumb, 0, sizeof(m_rcThumb));
		}
	}
	else {
		int cy = rc.bottom - rc.top;
		if( m_bShowButton1 ) cy -= _cxyFixed.cx;
		if( m_bShowButton2 ) cy -= _cxyFixed.cx;
		if( cy > _cxyFixed.cx ) {
			m_rcButton1.left = rc.left;
			m_rcButton1.top = rc.top;
			if( m_bShowButton1 ) {
				m_rcButton1.right = rc.left + _cxyFixed.cx;
				m_rcButton1.bottom = rc.top + _cxyFixed.cx;
			}
			else {
				m_rcButton1.right = m_rcButton1.left;
				m_rcButton1.bottom = m_rcButton1.top;
			}

			m_rcButton2.left = rc.left;
			m_rcButton2.bottom = rc.bottom;
			if( m_bShowButton2 ) {
				m_rcButton2.top = rc.bottom - _cxyFixed.cx;
				m_rcButton2.right = rc.left + _cxyFixed.cx;
			}
			else {
				m_rcButton2.top = m_rcButton2.bottom;
				m_rcButton2.right = m_rcButton2.left;
			}

			m_rcThumb.left = rc.left;
			m_rcThumb.right = rc.left + _cxyFixed.cx;
			if( m_nRange > 0 ) {
				int cyThumb = cy * (rc.bottom - rc.top) / (m_nRange + rc.bottom - rc.top);
				if( cyThumb < _cxyFixed.cx ) cyThumb = _cxyFixed.cx;

				m_rcThumb.top = m_nScrollPos * (cy - cyThumb) / m_nRange + m_rcButton1.bottom;
				m_rcThumb.bottom = m_rcThumb.top + cyThumb;
				if( m_rcThumb.bottom > m_rcButton2.top ) {
					m_rcThumb.top = m_rcButton2.top - cyThumb;
					m_rcThumb.bottom = m_rcButton2.top;
				}
			}
			else {
				m_rcThumb.top = m_rcButton1.bottom;
				m_rcThumb.bottom = m_rcButton2.top;
			}
		}
		else {
			int cyButton = (rc.bottom - rc.top) / 2;
			if( cyButton > _cxyFixed.cx ) cyButton = _cxyFixed.cx;
			m_rcButton1.left = rc.left;
			m_rcButton1.top = rc.top;
			if( m_bShowButton1 ) {
				m_rcButton1.right = rc.left + _cxyFixed.cx;
				m_rcButton1.bottom = rc.top + cyButton;
			}
			else {
				m_rcButton1.right = m_rcButton1.left;
				m_rcButton1.bottom = m_rcButton1.top;
			}

			m_rcButton2.left = rc.left;
			m_rcButton2.bottom = rc.bottom;
			if( m_bShowButton2 ) {
				m_rcButton2.top = rc.bottom - cyButton;
				m_rcButton2.right = rc.left + _cxyFixed.cx;
			}
			else {
				m_rcButton2.top = m_rcButton2.bottom;
				m_rcButton2.right = m_rcButton2.left;
			}

			__stosb((uint8_t*)&m_rcThumb, 0, sizeof(m_rcThumb));
		}
	}
}

void ScrollBar::DoEvent(TEventUI& event)
{
	if( !IsMouseEnabled() && event.Type > UIEVENT__MOUSEBEGIN && event.Type < UIEVENT__MOUSEEND ) {
		if( _pOwner != NULL ) _pOwner->DoEvent(event);
		else Control::DoEvent(event);
		return;
	}

	if( event.Type == UIEVENT_SETFOCUS ) 
	{
		return;
	}
	if( event.Type == UIEVENT_KILLFOCUS ) 
	{
		return;
	}
	if( event.Type == UIEVENT_BUTTONDOWN || event.Type == UIEVENT_DBLCLICK )
	{
		if( !IsEnabled() ) return;

		m_nLastScrollOffset = 0;
		m_nScrollRepeatDelay = 0;
		_pManager->SetTimer(this, DEFAULT_TIMERID, 50U);

		if (fn_PtInRect(&m_rcButton1, event.ptMouse)) {
			m_uButton1State |= UISTATE_PUSHED;
			if( !_bHorizontal ) {
				if( _pOwner != NULL ) _pOwner->lineUp(); 
				else SetScrollPos(m_nScrollPos - m_nLineSize);
			}
			else {
				if( _pOwner != NULL ) _pOwner->lineLeft(); 
				else SetScrollPos(m_nScrollPos - m_nLineSize);
			}
		}
		else if (fn_PtInRect(&m_rcButton2, event.ptMouse)) {
			m_uButton2State |= UISTATE_PUSHED;
			if( !_bHorizontal ) {
				if( _pOwner != NULL ) _pOwner->lineDown(); 
				else SetScrollPos(m_nScrollPos + m_nLineSize);
			}
			else {
				if( _pOwner != NULL ) _pOwner->lineRight(); 
				else SetScrollPos(m_nScrollPos + m_nLineSize);
			}
		}
		else if (fn_PtInRect(&m_rcThumb, event.ptMouse)) {
			m_uThumbState |= UISTATE_CAPTURED | UISTATE_PUSHED;
			ptLastMouse = event.ptMouse;
			m_nLastScrollPos = m_nScrollPos;
		}
		else {
			if( !_bHorizontal ) {
				if( event.ptMouse.y < m_rcThumb.top ) {
					if( _pOwner != NULL ) _pOwner->pageUp(); 
					else SetScrollPos(m_nScrollPos + _rcItem.top - _rcItem.bottom);
				}
				else if ( event.ptMouse.y > m_rcThumb.bottom ){
					if( _pOwner != NULL ) _pOwner->pageDown(); 
					else SetScrollPos(m_nScrollPos - _rcItem.top + _rcItem.bottom);                    
				}
			}
			else {
				if( event.ptMouse.x < m_rcThumb.left ) {
					if( _pOwner != NULL ) _pOwner->pageLeft(); 
					else SetScrollPos(m_nScrollPos + _rcItem.left - _rcItem.right);
				}
				else if ( event.ptMouse.x > m_rcThumb.right ){
					if( _pOwner != NULL ) _pOwner->pageRight(); 
					else SetScrollPos(m_nScrollPos - _rcItem.left + _rcItem.right);                    
				}
			}
		}
        if (_pManager != NULL && _pOwner == NULL) {
            _pManager->SendNotify(this, ZGUI_MSGTYPE_SCROLL);
        }
		return;
	}
	if( event.Type == UIEVENT_BUTTONUP )
	{
		m_nScrollRepeatDelay = 0;
		m_nLastScrollOffset = 0;
		_pManager->KillTimer(this, DEFAULT_TIMERID);

		if( (m_uThumbState & UISTATE_CAPTURED) != 0 ) {
			m_uThumbState &= ~( UISTATE_CAPTURED | UISTATE_PUSHED );
			Invalidate();
		}
		else if( (m_uButton1State & UISTATE_PUSHED) != 0 ) {
			m_uButton1State &= ~UISTATE_PUSHED;
			Invalidate();
		}
		else if( (m_uButton2State & UISTATE_PUSHED) != 0 ) {
			m_uButton2State &= ~UISTATE_PUSHED;
			Invalidate();
		}
		return;
	}
	if( event.Type == UIEVENT_MOUSEMOVE )
	{
		if( (m_uThumbState & UISTATE_CAPTURED) != 0 ) {
			if( !_bHorizontal ) {
				int vRange = _rcItem.bottom - _rcItem.top - m_rcThumb.bottom + m_rcThumb.top - 2 * _cxyFixed.cx;

				if (vRange != 0) {
					m_nLastScrollOffset = (event.ptMouse.y - ptLastMouse.y) * m_nRange / vRange;
				}
			}
			else {
				int hRange = _rcItem.right - _rcItem.left - m_rcThumb.right + m_rcThumb.left - 2 * _cxyFixed.cy;

				if (hRange != 0) {
					m_nLastScrollOffset = (event.ptMouse.x - ptLastMouse.x) * m_nRange / hRange;
				}
			}
		}
		else {
			if( (m_uThumbState & UISTATE_HOT) != 0 ) {
				if (!fn_PtInRect(&m_rcThumb, event.ptMouse)) {
					m_uThumbState &= ~UISTATE_HOT;
					Invalidate();
				}
			}
			else {
				if( !IsEnabled() ) return;
				if (fn_PtInRect(&m_rcThumb, event.ptMouse)) {
					m_uThumbState |= UISTATE_HOT;
					Invalidate();
				}
			}
		}
		return;
	}
	if( event.Type == UIEVENT_CONTEXTMENU )
	{
		return;
	}
	if( event.Type == UIEVENT_TIMER && event.wParam == DEFAULT_TIMERID )
	{
		++m_nScrollRepeatDelay;
		if ((m_uThumbState & UISTATE_CAPTURED) != 0) {
			if (!_bHorizontal) {
				if (_pOwner != 0) {
					_pOwner->setScrollPos(Size(_pOwner->getScrollPos().cx, m_nLastScrollPos + m_nLastScrollOffset));
				}
				else {
					SetScrollPos(m_nLastScrollPos + m_nLastScrollOffset);
				}
			}
			else {
				if (_pOwner != 0) {
					_pOwner->setScrollPos(Size(m_nLastScrollPos + m_nLastScrollOffset, _pOwner->getScrollPos().cy));
				}
				else {
					SetScrollPos(m_nLastScrollPos + m_nLastScrollOffset);
				}
			}
			Invalidate();
		}
		else if ((m_uButton1State & UISTATE_PUSHED) != 0) {
			if (m_nScrollRepeatDelay <= 5) {
				return;
			}
			if (!_bHorizontal) {
				if (_pOwner != 0) {
					_pOwner->lineUp();
				}
				else {
					SetScrollPos(m_nScrollPos - m_nLineSize);
				}
			}
			else {
				if (_pOwner != 0) {
					_pOwner->lineLeft();
				}
				else {
					SetScrollPos(m_nScrollPos - m_nLineSize);
				}
			}
		}
		else if ((m_uButton2State & UISTATE_PUSHED) != 0) {
			if (m_nScrollRepeatDelay <= 5) {
				return;
			}
			if (!_bHorizontal) {
				if (_pOwner != NULL) {
					_pOwner->lineDown();
				}
				else {
					SetScrollPos(m_nScrollPos + m_nLineSize);
				}
			}
			else {
				if (_pOwner != 0) {
					_pOwner->lineRight(); 
				}
				else {
					SetScrollPos(m_nScrollPos + m_nLineSize);
				}
			}
		}
		else {
			if (m_nScrollRepeatDelay <= 5) {
				return;
			}
			POINT pt;
            __stosb((uint8_t*)&pt, 0, sizeof(pt));
			fn_GetCursorPos(&pt);
			fn_ScreenToClient(_pManager->GetPaintWindow(), &pt);
			if( !_bHorizontal ) {
				if( pt.y < m_rcThumb.top ) {
					if( _pOwner != NULL ) _pOwner->pageUp(); 
					else SetScrollPos(m_nScrollPos + _rcItem.top - _rcItem.bottom);
				}
				else if ( pt.y > m_rcThumb.bottom ){
					if( _pOwner != NULL ) _pOwner->pageDown(); 
					else SetScrollPos(m_nScrollPos - _rcItem.top + _rcItem.bottom);                    
				}
			}
			else {
				if( pt.x < m_rcThumb.left ) {
					if( _pOwner != NULL ) _pOwner->pageLeft(); 
					else SetScrollPos(m_nScrollPos + _rcItem.left - _rcItem.right);
				}
				else if ( pt.x > m_rcThumb.right ){
					if( _pOwner != NULL ) _pOwner->pageRight(); 
					else SetScrollPos(m_nScrollPos - _rcItem.left + _rcItem.right);                    
				}
			}
		}
        if (_pManager != NULL && _pOwner == NULL) {
            _pManager->SendNotify(this, ZGUI_MSGTYPE_SCROLL);
        }
		return;
	}
	if( event.Type == UIEVENT_MOUSEENTER )
	{
		if( IsEnabled() ) {
			m_uButton1State |= UISTATE_HOT;
			m_uButton2State |= UISTATE_HOT;
            if (fn_PtInRect(&m_rcThumb, event.ptMouse)) {
                m_uThumbState |= UISTATE_HOT;
            }
			Invalidate();
		}
		return;
	}
	if( event.Type == UIEVENT_MOUSELEAVE )
	{
		if( IsEnabled() ) {
			m_uButton1State &= ~UISTATE_HOT;
			m_uButton2State &= ~UISTATE_HOT;
			m_uThumbState &= ~UISTATE_HOT;
			Invalidate();
		}
		return;
	}

	if( _pOwner != NULL ) _pOwner->DoEvent(event); else Control::DoEvent(event);
}

void ScrollBar::setAttribute(const String& pstrName, const String& pstrValue)
{
    if (pstrName == "button1normalimage") {
        SetButton1NormalImage(pstrValue);
    }
    else if (pstrName == "button1hotimage") {
        SetButton1HotImage(pstrValue);
    }
    else if (pstrName == "button1pushedimage") {
        SetButton1PushedImage(pstrValue);
    }
    else if (pstrName == "button1disabledimage") {
        SetButton1DisabledImage(pstrValue);
    }
    else if (pstrName == "button2normalimage") {
        SetButton2NormalImage(pstrValue);
    }
    else if (pstrName == "button2hotimage") {
        SetButton2HotImage(pstrValue);
    }
    else if (pstrName == "button2pushedimage") {
        SetButton2PushedImage(pstrValue);
    }
    else if (pstrName == "button2disabledimage") {
        SetButton2DisabledImage(pstrValue);
    }
    else if (pstrName == "thumbnormalimage") {
        SetThumbNormalImage(pstrValue);
    }
    else if (pstrName == "thumbhotimage") {
        SetThumbHotImage(pstrValue);
    }
    else if (pstrName == "thumbpushedimage") {
        SetThumbPushedImage(pstrValue);
    }
    else if (pstrName == "thumbdisabledimage") {
        SetThumbDisabledImage(pstrValue);
    }
    else if (pstrName == "railnormalimage") {
        SetRailNormalImage(pstrValue);
    }
    else if (pstrName == "railhotimage") {
        SetRailHotImage(pstrValue);
    }
    else if (pstrName == "railpushedimage") {
        SetRailPushedImage(pstrValue);
    }
    else if (pstrName == "raildisabledimage") {
        SetRailDisabledImage(pstrValue);
    }
    else if (pstrName == "bknormalimage") {
        SetBkNormalImage(pstrValue);
    }
    else if (pstrName == "bkhotimage") {
        SetBkHotImage(pstrValue);
    }
    else if (pstrName == "bkpushedimage") {
        SetBkPushedImage(pstrValue);
    }
    else if (pstrName == "bkdisabledimage") {
        SetBkDisabledImage(pstrValue);
    }
    else if (pstrName == "hor") {
        SetHorizontal(pstrValue == "true");
    }
    else if (pstrName == "linesize") {
        SetLineSize(pstrValue.getIntValue());
    }
    else if (pstrName == "range") {
        SetScrollRange(pstrValue.getIntValue());
    }
    else if (pstrName == "value") {
        SetScrollPos(pstrValue.getIntValue());
    }
    else if (pstrName == "showbutton1") {
        SetShowButton1(pstrValue == "true");
    }
    else if (pstrName == "showbutton2") {
        SetShowButton2(pstrValue == "true");
    }
    else {
        Control::setAttribute(pstrName, pstrValue);
    }
}

void ScrollBar::DoPaint(HDC hDC, const RECT& rcPaint)
{
    if (!fn_IntersectRect(&_rcPaint, &rcPaint, &_rcItem)) {
        return;
    }
	PaintBk(hDC);
	PaintButton1(hDC);
	PaintButton2(hDC);
	PaintThumb(hDC);
	PaintRail(hDC);
}

void ScrollBar::PaintBk(HDC hDC)
{
    if (!IsEnabled()) {
        m_uThumbState |= UISTATE_DISABLED;
    }
    else {
        m_uThumbState &= ~ UISTATE_DISABLED;
    }

	if ((m_uThumbState & UISTATE_DISABLED) != 0) {
		if (!m_sBkDisabledImage.isEmpty()) {
            if (!DrawImage(hDC, m_sBkDisabledImage)) {
                m_sBkDisabledImage = String::empty;
            }
            else {
                return;
            }
		}
	}
	else if ((m_uThumbState & UISTATE_PUSHED) != 0) {
		if (!m_sBkPushedImage.isEmpty()) {
            if (!DrawImage(hDC, m_sBkPushedImage)) {
                m_sBkPushedImage = String::empty;
            }
            else {
                return;
            }
		}
	}
	else if ((m_uThumbState & UISTATE_HOT) != 0) {
		if (!m_sBkHotImage.isEmpty()) {
            if (!DrawImage(hDC, m_sBkHotImage)) {
                m_sBkHotImage = String::empty;
            }
            else {
                return;
            }
		}
	}

	if (!m_sBkNormalImage.isEmpty()) {
        if (!DrawImage(hDC, m_sBkNormalImage)) {
            m_sBkNormalImage = String::empty;
        }
        else {
            return;
        }
	}
}

void ScrollBar::PaintButton1(HDC hDC)
{
	if( !m_bShowButton1 ) return;

	if( !IsEnabled() ) m_uButton1State |= UISTATE_DISABLED;
	else m_uButton1State &= ~ UISTATE_DISABLED;

    _imageModifyName = String::empty;
    _imageModifyName << "dest='" << m_rcButton1.left - _rcItem.left << "," << m_rcButton1.top - _rcItem.top << "," <<
        m_rcButton1.right - _rcItem.left << "," << m_rcButton1.bottom - _rcItem.top << "'";

	if ((m_uButton1State & UISTATE_DISABLED) != 0) {
		if (!m_sButton1DisabledImage.isEmpty()) {
            if (!DrawImage(hDC, m_sButton1DisabledImage, _imageModifyName)) {
                m_sButton1DisabledImage = String::empty;
            }
            else {
                return;
            }
		}
	}
	else if ((m_uButton1State & UISTATE_PUSHED) != 0) {
		if (!m_sButton1PushedImage.isEmpty()) {
            if (!DrawImage(hDC, m_sButton1PushedImage, _imageModifyName)) {
                m_sButton1PushedImage = String::empty;
            }
            else {
                return;
            }
		}
	}
	else if ((m_uButton1State & UISTATE_HOT) != 0) {
		if (!m_sButton1HotImage.isEmpty()) {
            if (!DrawImage(hDC, m_sButton1HotImage, _imageModifyName)) {
                m_sButton1HotImage = String::empty;
            }
            else {
                return;
            }
		}
	}

	if (!_button1NormalImageName.isEmpty()) {
        if (!DrawImage(hDC, _button1NormalImageName, _imageModifyName)) {
            _button1NormalImageName = String::empty;
        }
        else {
            return;
        }
	}

	DWORD dwBorderColor = 0xFF85E4FF;
	int nBorderSize = 2;
	RenderEngine::DrawRect(hDC, m_rcButton1, nBorderSize, dwBorderColor);
}

void ScrollBar::PaintButton2(HDC hDC)
{
	if( !m_bShowButton2 ) return;

	if( !IsEnabled() ) m_uButton2State |= UISTATE_DISABLED;
	else m_uButton2State &= ~ UISTATE_DISABLED;

    _imageModifyName = String::empty;
    _imageModifyName << "dest='" << m_rcButton2.left - _rcItem.left << "," << m_rcButton2.top - _rcItem.top << "," << m_rcButton2.right - _rcItem.left
        << "," << m_rcButton2.bottom - _rcItem.top << "'";

	if((m_uButton2State & UISTATE_DISABLED) != 0) {
		if (!m_sButton2DisabledImage.isEmpty()) {
            if (!DrawImage(hDC, m_sButton2DisabledImage, _imageModifyName)) {
                m_sButton2DisabledImage = String::empty;
            }
            else {
                return;
            }
		}
	}
	else if ((m_uButton2State & UISTATE_PUSHED) != 0) {
		if (!m_sButton2PushedImage.isEmpty()) {
            if (!DrawImage(hDC, m_sButton2PushedImage, _imageModifyName)) {
                m_sButton2PushedImage = String::empty;
            }
            else {
                return;
            }
		}
	}
	else if ((m_uButton2State & UISTATE_HOT) != 0) {
		if ( !m_sButton2HotImage.isEmpty()) {
            if (!DrawImage(hDC, m_sButton2HotImage, _imageModifyName)) {
                m_sButton2HotImage = String::empty;
            }
            else {
                return;
            }
		}
	}

	if (!m_sButton2NormalImage.isEmpty()) {
        if (!DrawImage(hDC, m_sButton2NormalImage, _imageModifyName)) {
            m_sButton2NormalImage = String::empty;
        }
        else {
            return;
        }
	}

	DWORD dwBorderColor = 0xFF85E4FF;
	int nBorderSize = 2;
	RenderEngine::DrawRect(hDC, m_rcButton2, nBorderSize, dwBorderColor);
}

void ScrollBar::PaintThumb(HDC hDC)
{
	if( m_rcThumb.left == 0 && m_rcThumb.top == 0 && m_rcThumb.right == 0 && m_rcThumb.bottom == 0 ) return;
	if( !IsEnabled() ) m_uThumbState |= UISTATE_DISABLED;
	else m_uThumbState &= ~ UISTATE_DISABLED;

    _imageModifyName = String::empty;
    _imageModifyName << "dest='" << m_rcThumb.left - _rcItem.left << "," << m_rcThumb.top - _rcItem.top << "," <<
        m_rcThumb.right - _rcItem.left << "," << m_rcThumb.bottom - _rcItem.top << "'";

	if ((m_uThumbState & UISTATE_DISABLED) != 0) {
		if (!m_sThumbDisabledImage.isEmpty()) {
            if (!DrawImage(hDC, m_sThumbDisabledImage, _imageModifyName)) {
                m_sThumbDisabledImage = String::empty;
            }
            else {
                return;
            }
		}
	}
	else if ((m_uThumbState & UISTATE_PUSHED) != 0) {
		if (!m_sThumbPushedImage.isEmpty()) {
            if (!DrawImage(hDC, m_sThumbPushedImage, _imageModifyName)) {
                m_sThumbPushedImage = String::empty;
            }
            else {
                return;
            }
		}
	}
	else if ((m_uThumbState & UISTATE_HOT) != 0) {
		if (!m_sThumbHotImage.isEmpty()) {
            if (!DrawImage(hDC, m_sThumbHotImage, _imageModifyName)) {
                m_sThumbHotImage = String::empty;
            }
            else {
                return;
            }
		}
	}

	if (!m_sThumbNormalImage.isEmpty()) {
        if (!DrawImage(hDC, m_sThumbNormalImage, _imageModifyName)) {
            m_sThumbNormalImage = String::empty;
        }
        else {
            return;
        }
	}

	DWORD dwBorderColor = 0xFF85E4FF;
	int nBorderSize = 2;
	RenderEngine::DrawRect(hDC, m_rcThumb, nBorderSize, dwBorderColor);
}

void ScrollBar::PaintRail(HDC hDC)
{
	if( m_rcThumb.left == 0 && m_rcThumb.top == 0 && m_rcThumb.right == 0 && m_rcThumb.bottom == 0 ) return;
	if( !IsEnabled() ) m_uThumbState |= UISTATE_DISABLED;
	else m_uThumbState &= ~ UISTATE_DISABLED;

    _imageModifyName = String::empty;
	if (!_bHorizontal) {
        _imageModifyName << "dest='" << m_rcThumb.left - _rcItem.left
            << "," << (m_rcThumb.top + m_rcThumb.bottom) / 2 - _rcItem.top - _cxyFixed.cx / 2
            << "," << m_rcThumb.right - _rcItem.left
            << "," << (m_rcThumb.top + m_rcThumb.bottom) / 2 - _rcItem.top + _cxyFixed.cx - _cxyFixed.cx / 2  << "'";
	}
	else {
        _imageModifyName << "dest='" << (m_rcThumb.left + m_rcThumb.right) / 2 - _rcItem.left - _cxyFixed.cy / 2
            << "," << m_rcThumb.top - _rcItem.top
            << "," << (m_rcThumb.left + m_rcThumb.right) / 2 - _rcItem.left + _cxyFixed.cy - _cxyFixed.cy / 2
            << "," << m_rcThumb.bottom - _rcItem.top << "'";
	}

	if ((m_uThumbState & UISTATE_DISABLED) != 0) {
		if (!m_sRailDisabledImage.isEmpty()) {
            if (!DrawImage(hDC, m_sRailDisabledImage, _imageModifyName)) {
                m_sRailDisabledImage = String::empty;
            }
            else {
                return;
            }
		}
	}
	else if ((m_uThumbState & UISTATE_PUSHED) != 0) {
		if (!m_sRailPushedImage.isEmpty()) {
            if (!DrawImage(hDC, m_sRailPushedImage, _imageModifyName)) {
                m_sRailPushedImage = String::empty;
            }
            else {
                return;
            }
		}
	}
	else if ((m_uThumbState & UISTATE_HOT) != 0) {
		if (!m_sRailHotImage.isEmpty()) {
            if (!DrawImage(hDC, m_sRailHotImage, _imageModifyName)) {
                m_sRailHotImage = String::empty;
            }
            else {
                return;
            }
		}
	}

	if (!m_sRailNormalImage.isEmpty()) {
        if (!DrawImage(hDC, m_sRailNormalImage, _imageModifyName)) {
            m_sRailNormalImage = String::empty;
        }
        else {
            return;
        }
	}
}

}

#endif // ZGUI_USE_SCROLLBAR