#include "zgui.h"

#ifdef ZGUI_USE_SCROLLBAR

namespace zgui
{
	CScrollBarUI::CScrollBarUI() : m_bHorizontal(false), m_nRange(100), m_nScrollPos(0), m_nLineSize(8), 
		m_pOwner(NULL), m_nLastScrollPos(0), m_nLastScrollOffset(0), m_nScrollRepeatDelay(0), m_uButton1State(0), \
		m_uButton2State(0), m_uThumbState(0), m_bShowButton1(true), m_bShowButton2(true)
	{
		_cxyFixed.cx = DEFAULT_SCROLLBAR_SIZE;
		ptLastMouse.x = ptLastMouse.y = 0;
		::ZeroMemory(&m_rcThumb, sizeof(m_rcThumb));
		::ZeroMemory(&m_rcButton1, sizeof(m_rcButton1));
		::ZeroMemory(&m_rcButton2, sizeof(m_rcButton2));
	}

	LPCTSTR CScrollBarUI::GetClass() const
	{
		return _T("ScrollBarUI");
	}

	LPVOID CScrollBarUI::GetInterface(LPCTSTR pstrName)
	{
		if( lstrcmp(pstrName, DUI_CTR_SCROLLBAR) == 0 ) return static_cast<CScrollBarUI*>(this);
		return CControlUI::GetInterface(pstrName);
	}

	CContainerUI* CScrollBarUI::GetOwner() const
	{
		return m_pOwner;
	}

	void CScrollBarUI::SetOwner(CContainerUI* pOwner)
	{
		m_pOwner = pOwner;
	}

	void CScrollBarUI::SetVisible(bool bVisible)
	{
        if (m_bVisible == bVisible) {
            return;
        }

		m_bVisible = bVisible;
        if (m_bFocused) {
            m_bFocused = false;
        }
	}

	void CScrollBarUI::SetEnabled(bool bEnable)
	{
		CControlUI::SetEnabled(bEnable);
		if( !IsEnabled() ) {
			m_uButton1State = 0;
			m_uButton2State = 0;
			m_uThumbState = 0;
		}
	}

	void CScrollBarUI::SetFocus()
	{
		if( m_pOwner != NULL ) m_pOwner->SetFocus();
		else CControlUI::SetFocus();
	}

	bool CScrollBarUI::IsHorizontal()
	{
		return m_bHorizontal;
	}

	void CScrollBarUI::SetHorizontal(bool bHorizontal)
	{
		if( m_bHorizontal == bHorizontal ) return;

		m_bHorizontal = bHorizontal;
		if( m_bHorizontal ) {
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

		if( m_pOwner != NULL ) m_pOwner->NeedUpdate(); else NeedParentUpdate();
	}

	int CScrollBarUI::GetScrollRange() const
	{
		return m_nRange;
	}

	void CScrollBarUI::SetScrollRange(int nRange)
	{
		if( m_nRange == nRange ) return;

		m_nRange = nRange;
		if( m_nRange < 0 ) m_nRange = 0;
		if( m_nScrollPos > m_nRange ) m_nScrollPos = m_nRange;
		SetPos(_rcItem);
	}

	int CScrollBarUI::GetScrollPos() const
	{
		return m_nScrollPos;
	}

	void CScrollBarUI::SetScrollPos(int nPos)
	{
		if( m_nScrollPos == nPos ) return;

		m_nScrollPos = nPos;
		if( m_nScrollPos < 0 ) m_nScrollPos = 0;
		if( m_nScrollPos > m_nRange ) m_nScrollPos = m_nRange;
		SetPos(_rcItem);
	}

	int CScrollBarUI::GetLineSize() const
	{
		return m_nLineSize;
	}

	void CScrollBarUI::SetLineSize(int nSize)
	{
		m_nLineSize = nSize;
	}

	bool CScrollBarUI::GetShowButton1()
	{
		return m_bShowButton1;
	}

	void CScrollBarUI::SetShowButton1(bool bShow)
	{
		m_bShowButton1 = bShow;
		SetPos(_rcItem);
	}

	const String& CScrollBarUI::GetButton1NormalImage()
	{
		return _button1NormalImageName;
	}

	void CScrollBarUI::SetButton1NormalImage(const String& pStrImage)
	{
		_button1NormalImageName = pStrImage;
		Invalidate();
	}

	const String& CScrollBarUI::GetButton1HotImage()
	{
		return m_sButton1HotImage;
	}

	void CScrollBarUI::SetButton1HotImage(const String& pStrImage)
	{
		m_sButton1HotImage = pStrImage;
		Invalidate();
	}

	const String& CScrollBarUI::GetButton1PushedImage()
	{
		return m_sButton1PushedImage;
	}

	void CScrollBarUI::SetButton1PushedImage(const String& pStrImage)
	{
		m_sButton1PushedImage = pStrImage;
		Invalidate();
	}

	const String& CScrollBarUI::GetButton1DisabledImage()
	{
		return m_sButton1DisabledImage;
	}

	void CScrollBarUI::SetButton1DisabledImage(const String& pStrImage)
	{
		m_sButton1DisabledImage = pStrImage;
		Invalidate();
	}

	bool CScrollBarUI::GetShowButton2()
	{
		return m_bShowButton2;
	}

	void CScrollBarUI::SetShowButton2(bool bShow)
	{
		m_bShowButton2 = bShow;
		SetPos(_rcItem);
	}

	const String& CScrollBarUI::GetButton2NormalImage()
	{
		return m_sButton2NormalImage;
	}

	void CScrollBarUI::SetButton2NormalImage(const String& pStrImage)
	{
		m_sButton2NormalImage = pStrImage;
		Invalidate();
	}

	const String& CScrollBarUI::GetButton2HotImage()
	{
		return m_sButton2HotImage;
	}

	void CScrollBarUI::SetButton2HotImage(const String& pStrImage)
	{
		m_sButton2HotImage = pStrImage;
		Invalidate();
	}

	const String& CScrollBarUI::GetButton2PushedImage()
	{
		return m_sButton2PushedImage;
	}

	void CScrollBarUI::SetButton2PushedImage(const String& pStrImage)
	{
		m_sButton2PushedImage = pStrImage;
		Invalidate();
	}

	const String& CScrollBarUI::GetButton2DisabledImage()
	{
		return m_sButton2DisabledImage;
	}

	void CScrollBarUI::SetButton2DisabledImage(const String& pStrImage)
	{
		m_sButton2DisabledImage = pStrImage;
		Invalidate();
	}

	const String& CScrollBarUI::GetThumbNormalImage()
	{
		return m_sThumbNormalImage;
	}

	void CScrollBarUI::SetThumbNormalImage(const String& pStrImage)
	{
		m_sThumbNormalImage = pStrImage;
		Invalidate();
	}

	const String& CScrollBarUI::GetThumbHotImage()
	{
		return m_sThumbHotImage;
	}

	void CScrollBarUI::SetThumbHotImage(const String& pStrImage)
	{
		m_sThumbHotImage = pStrImage;
		Invalidate();
	}

	const String& CScrollBarUI::GetThumbPushedImage()
	{
		return m_sThumbPushedImage;
	}

	void CScrollBarUI::SetThumbPushedImage(const String& pStrImage)
	{
		m_sThumbPushedImage = pStrImage;
		Invalidate();
	}

	const String& CScrollBarUI::GetThumbDisabledImage()
	{
		return m_sThumbDisabledImage;
	}

	void CScrollBarUI::SetThumbDisabledImage(const String& pStrImage)
	{
		m_sThumbDisabledImage = pStrImage;
		Invalidate();
	}

	const String& CScrollBarUI::GetRailNormalImage()
	{
		return m_sRailNormalImage;
	}

	void CScrollBarUI::SetRailNormalImage(const String& pStrImage)
	{
		m_sRailNormalImage = pStrImage;
		Invalidate();
	}

	const String& CScrollBarUI::GetRailHotImage()
	{
		return m_sRailHotImage;
	}

	void CScrollBarUI::SetRailHotImage(const String& pStrImage)
	{
		m_sRailHotImage = pStrImage;
		Invalidate();
	}

	const String& CScrollBarUI::GetRailPushedImage()
	{
		return m_sRailPushedImage;
	}

	void CScrollBarUI::SetRailPushedImage(const String& pStrImage)
	{
		m_sRailPushedImage = pStrImage;
		Invalidate();
	}

	const String& CScrollBarUI::GetRailDisabledImage()
	{
		return m_sRailDisabledImage;
	}

	void CScrollBarUI::SetRailDisabledImage(const String& pStrImage)
	{
		m_sRailDisabledImage = pStrImage;
		Invalidate();
	}

	const String& CScrollBarUI::GetBkNormalImage()
	{
		return m_sBkNormalImage;
	}

	void CScrollBarUI::SetBkNormalImage(const String& pStrImage)
	{
		m_sBkNormalImage = pStrImage;
		Invalidate();
	}

	const String& CScrollBarUI::GetBkHotImage()
	{
		return m_sBkHotImage;
	}

	void CScrollBarUI::SetBkHotImage(const String& pStrImage)
	{
		m_sBkHotImage = pStrImage;
		Invalidate();
	}

	const String& CScrollBarUI::GetBkPushedImage()
	{
		return m_sBkPushedImage;
	}

	void CScrollBarUI::SetBkPushedImage(const String& pStrImage)
	{
		m_sBkPushedImage = pStrImage;
		Invalidate();
	}

	const String& CScrollBarUI::GetBkDisabledImage()
	{
		return m_sBkDisabledImage;
	}

	void CScrollBarUI::SetBkDisabledImage(const String& pStrImage)
	{
		m_sBkDisabledImage = pStrImage;
		Invalidate();
	}

	void CScrollBarUI::SetPos(RECT rc)
	{
		CControlUI::SetPos(rc);
		rc = _rcItem;

		if( m_bHorizontal ) {
			int cx = rc.right - rc.left;
			if( m_bShowButton1 ) cx -= _cxyFixed.cy;
			if( m_bShowButton2 ) cx -= _cxyFixed.cy;
			if( cx > _cxyFixed.cy ) {
				m_rcButton1.left = rc.left;
				m_rcButton1.top = rc.top;
				if( m_bShowButton1 ) {
					m_rcButton1.right = rc.left + _cxyFixed.cy;
					m_rcButton1.bottom = rc.top + _cxyFixed.cy;
				}
				else {
					m_rcButton1.right = m_rcButton1.left;
					m_rcButton1.bottom = m_rcButton1.top;
				}

				m_rcButton2.top = rc.top;
				m_rcButton2.right = rc.right;
				if( m_bShowButton2 ) {
					m_rcButton2.left = rc.right - _cxyFixed.cy;
					m_rcButton2.bottom = rc.top + _cxyFixed.cy;
				}
				else {
					m_rcButton2.left = m_rcButton2.right;
					m_rcButton2.bottom = m_rcButton2.top;
				}

				m_rcThumb.top = rc.top;
				m_rcThumb.bottom = rc.top + _cxyFixed.cy;
				if( m_nRange > 0 ) {
					int cxThumb = cx * (rc.right - rc.left) / (m_nRange + rc.right - rc.left);
					if( cxThumb < _cxyFixed.cy ) cxThumb = _cxyFixed.cy;

					m_rcThumb.left = m_nScrollPos * (cx - cxThumb) / m_nRange + m_rcButton1.right;
					m_rcThumb.right = m_rcThumb.left + cxThumb;
					if( m_rcThumb.right > m_rcButton2.left ) {
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

				::ZeroMemory(&m_rcThumb, sizeof(m_rcThumb));
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

				::ZeroMemory(&m_rcThumb, sizeof(m_rcThumb));
			}
		}
	}

	void CScrollBarUI::DoEvent(TEventUI& event)
	{
		if( !IsMouseEnabled() && event.Type > UIEVENT__MOUSEBEGIN && event.Type < UIEVENT__MOUSEEND ) {
			if( m_pOwner != NULL ) m_pOwner->DoEvent(event);
			else CControlUI::DoEvent(event);
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

			if( ::PtInRect(&m_rcButton1, event.ptMouse) ) {
				m_uButton1State |= UISTATE_PUSHED;
				if( !m_bHorizontal ) {
					if( m_pOwner != NULL ) m_pOwner->LineUp(); 
					else SetScrollPos(m_nScrollPos - m_nLineSize);
				}
				else {
					if( m_pOwner != NULL ) m_pOwner->LineLeft(); 
					else SetScrollPos(m_nScrollPos - m_nLineSize);
				}
			}
			else if( ::PtInRect(&m_rcButton2, event.ptMouse) ) {
				m_uButton2State |= UISTATE_PUSHED;
				if( !m_bHorizontal ) {
					if( m_pOwner != NULL ) m_pOwner->LineDown(); 
					else SetScrollPos(m_nScrollPos + m_nLineSize);
				}
				else {
					if( m_pOwner != NULL ) m_pOwner->LineRight(); 
					else SetScrollPos(m_nScrollPos + m_nLineSize);
				}
			}
			else if( ::PtInRect(&m_rcThumb, event.ptMouse) ) {
				m_uThumbState |= UISTATE_CAPTURED | UISTATE_PUSHED;
				ptLastMouse = event.ptMouse;
				m_nLastScrollPos = m_nScrollPos;
			}
			else {
				if( !m_bHorizontal ) {
					if( event.ptMouse.y < m_rcThumb.top ) {
						if( m_pOwner != NULL ) m_pOwner->PageUp(); 
						else SetScrollPos(m_nScrollPos + _rcItem.top - _rcItem.bottom);
					}
					else if ( event.ptMouse.y > m_rcThumb.bottom ){
						if( m_pOwner != NULL ) m_pOwner->PageDown(); 
						else SetScrollPos(m_nScrollPos - _rcItem.top + _rcItem.bottom);                    
					}
				}
				else {
					if( event.ptMouse.x < m_rcThumb.left ) {
						if( m_pOwner != NULL ) m_pOwner->PageLeft(); 
						else SetScrollPos(m_nScrollPos + _rcItem.left - _rcItem.right);
					}
					else if ( event.ptMouse.x > m_rcThumb.right ){
						if( m_pOwner != NULL ) m_pOwner->PageRight(); 
						else SetScrollPos(m_nScrollPos - _rcItem.left + _rcItem.right);                    
					}
				}
			}
			if( _pManager != NULL && m_pOwner == NULL ) _pManager->SendNotify(this, DUI_MSGTYPE_SCROLL);
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
				if( !m_bHorizontal ) {
					m_nLastScrollOffset = (event.ptMouse.y - ptLastMouse.y) * m_nRange / \
						(_rcItem.bottom - _rcItem.top - m_rcThumb.bottom + m_rcThumb.top - 2 * _cxyFixed.cx);
				}
				else {
					m_nLastScrollOffset = (event.ptMouse.x - ptLastMouse.x) * m_nRange / \
						(_rcItem.right - _rcItem.left - m_rcThumb.right + m_rcThumb.left - 2 * _cxyFixed.cy);
				}
			}
			else {
				if( (m_uThumbState & UISTATE_HOT) != 0 ) {
					if( !::PtInRect(&m_rcThumb, event.ptMouse) ) {
						m_uThumbState &= ~UISTATE_HOT;
						Invalidate();
					}
				}
				else {
					if( !IsEnabled() ) return;
					if( ::PtInRect(&m_rcThumb, event.ptMouse) ) {
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
			if( (m_uThumbState & UISTATE_CAPTURED) != 0 ) {
				if( !m_bHorizontal ) {
					if( m_pOwner != NULL ) m_pOwner->SetScrollPos(CSize(m_pOwner->GetScrollPos().cx, \
						m_nLastScrollPos + m_nLastScrollOffset)); 
					else SetScrollPos(m_nLastScrollPos + m_nLastScrollOffset);
				}
				else {
					if( m_pOwner != NULL ) m_pOwner->SetScrollPos(CSize(m_nLastScrollPos + m_nLastScrollOffset, \
						m_pOwner->GetScrollPos().cy)); 
					else SetScrollPos(m_nLastScrollPos + m_nLastScrollOffset);
				}
				Invalidate();
			}
			else if( (m_uButton1State & UISTATE_PUSHED) != 0 ) {
				if( m_nScrollRepeatDelay <= 5 ) return;
				if( !m_bHorizontal ) {
					if( m_pOwner != NULL ) m_pOwner->LineUp(); 
					else SetScrollPos(m_nScrollPos - m_nLineSize);
				}
				else {
					if( m_pOwner != NULL ) m_pOwner->LineLeft(); 
					else SetScrollPos(m_nScrollPos - m_nLineSize);
				}
			}
			else if( (m_uButton2State & UISTATE_PUSHED) != 0 ) {
				if( m_nScrollRepeatDelay <= 5 ) return;
				if( !m_bHorizontal ) {
					if( m_pOwner != NULL ) m_pOwner->LineDown(); 
					else SetScrollPos(m_nScrollPos + m_nLineSize);
				}
				else {
					if( m_pOwner != NULL ) m_pOwner->LineRight(); 
					else SetScrollPos(m_nScrollPos + m_nLineSize);
				}
			}
			else {
				if( m_nScrollRepeatDelay <= 5 ) return;
				POINT pt = { 0 };
				::GetCursorPos(&pt);
				::ScreenToClient(_pManager->GetPaintWindow(), &pt);
				if( !m_bHorizontal ) {
					if( pt.y < m_rcThumb.top ) {
						if( m_pOwner != NULL ) m_pOwner->PageUp(); 
						else SetScrollPos(m_nScrollPos + _rcItem.top - _rcItem.bottom);
					}
					else if ( pt.y > m_rcThumb.bottom ){
						if( m_pOwner != NULL ) m_pOwner->PageDown(); 
						else SetScrollPos(m_nScrollPos - _rcItem.top + _rcItem.bottom);                    
					}
				}
				else {
					if( pt.x < m_rcThumb.left ) {
						if( m_pOwner != NULL ) m_pOwner->PageLeft(); 
						else SetScrollPos(m_nScrollPos + _rcItem.left - _rcItem.right);
					}
					else if ( pt.x > m_rcThumb.right ){
						if( m_pOwner != NULL ) m_pOwner->PageRight(); 
						else SetScrollPos(m_nScrollPos - _rcItem.left + _rcItem.right);                    
					}
				}
			}
			if( _pManager != NULL && m_pOwner == NULL ) _pManager->SendNotify(this, DUI_MSGTYPE_SCROLL);
			return;
		}
		if( event.Type == UIEVENT_MOUSEENTER )
		{
			if( IsEnabled() ) {
				m_uButton1State |= UISTATE_HOT;
				m_uButton2State |= UISTATE_HOT;
				if( ::PtInRect(&m_rcThumb, event.ptMouse) ) m_uThumbState |= UISTATE_HOT;
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

		if( m_pOwner != NULL ) m_pOwner->DoEvent(event); else CControlUI::DoEvent(event);
	}

	void CScrollBarUI::SetAttribute(const String& pstrName, const String& pstrValue)
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
            CControlUI::SetAttribute(pstrName, pstrValue);
        }
	}

	void CScrollBarUI::DoPaint(HDC hDC, const RECT& rcPaint)
	{
        if (!::IntersectRect(&_rcPaint, &rcPaint, &_rcItem)) {
            return;
        }
		PaintBk(hDC);
		PaintButton1(hDC);
		PaintButton2(hDC);
		PaintThumb(hDC);
		PaintRail(hDC);
	}

	void CScrollBarUI::PaintBk(HDC hDC)
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

	void CScrollBarUI::PaintButton1(HDC hDC)
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
		CRenderEngine::DrawRect(hDC, m_rcButton1, nBorderSize, dwBorderColor);
	}

	void CScrollBarUI::PaintButton2(HDC hDC)
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
		CRenderEngine::DrawRect(hDC, m_rcButton2, nBorderSize, dwBorderColor);
	}

	void CScrollBarUI::PaintThumb(HDC hDC)
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
		CRenderEngine::DrawRect(hDC, m_rcThumb, nBorderSize, dwBorderColor);
	}

	void CScrollBarUI::PaintRail(HDC hDC)
	{
		if( m_rcThumb.left == 0 && m_rcThumb.top == 0 && m_rcThumb.right == 0 && m_rcThumb.bottom == 0 ) return;
		if( !IsEnabled() ) m_uThumbState |= UISTATE_DISABLED;
		else m_uThumbState &= ~ UISTATE_DISABLED;

        _imageModifyName = String::empty;
		if (!m_bHorizontal) {
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